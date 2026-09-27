#include "ProceduralAssemblies.hpp"
#include "DamageField.hpp"
#include "DamageGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>
#include <numbers>
#include <numeric>
#include <set>
#include <stdexcept>

namespace Playground::Assembly
{
namespace
{
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double k) {return {a.x*k,a.y*k,a.z*k};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3d a) {return std::sqrt(dot(a,a));}
Vec3d unit(Vec3d a)
{
    const auto magnitude=length(a);
    if (!std::isfinite(magnitude) || magnitude<1e-10)
        throw std::runtime_error("Assembly member has undefined length");
    return mul(a,1/magnitude);
}
struct Point2 {double u{},v{};};
struct SurfacePoint
{
    std::array<double,3> weights{};
    double squaredDistance{std::numeric_limits<double>::infinity()};
};
SurfacePoint nearestOnTriangle(Vec3d point,Vec3d a,Vec3d b,Vec3d c)
{
    const auto ab=sub(b,a),ac=sub(c,a),ap=sub(point,a);
    const auto aa=dot(ab,ab),abac=dot(ab,ac),cc=dot(ac,ac);
    const auto denominator=aa*cc-abac*abac;
    if (!(denominator>1e-18)) throw std::runtime_error("Damage UV source triangle collapsed");
    const auto u=(dot(ap,ab)*cc-dot(ap,ac)*abac)/denominator;
    const auto v=(dot(ap,ac)*aa-dot(ap,ab)*abac)/denominator;
    auto candidate=[&](std::array<double,3> weights)
    {
        const auto nearest=add(mul(a,weights[0]),
            add(mul(b,weights[1]),mul(c,weights[2])));
        const auto delta=sub(point,nearest);
        return SurfacePoint{weights,dot(delta,delta)};
    };
    if (u>=0 && v>=0 && u+v<=1) return candidate({1-u-v,u,v});
    SurfacePoint best;
    const Vec3d vertices[]={a,b,c};
    for (int edge=0;edge<3;++edge)
    {
        const auto next=(edge+1)%3;
        const auto direction=sub(vertices[next],vertices[edge]);
        const auto t=std::clamp(dot(sub(point,vertices[edge]),direction)/dot(direction,direction),
                                0.0,1.0);
        std::array<double,3> weights{};
        weights[edge]=1-t;weights[next]=t;
        auto found=candidate(weights);
        if (found.squaredDistance<best.squaredDistance) best=found;
    }
    return best;
}

class Builder
{
  public:
    Settings settings;
    Result result;
    Jerico::Guid currentGuid;
    float currentField{};

    Builder(const PA::ProceduralSpline &spline,const Settings &input):settings(input)
    {
        if (settings.lod<0 || settings.lod>2 || settings.maximumParts<1 ||
            settings.maximumParts>10000 || settings.maximumVertices<100 ||
            settings.maximumVertices>1000000)
            throw std::runtime_error("Assembly LOD or part/vertex budget is invalid");
        if (settings.damage.areaRules.size()>maximumDamageAreas ||
            settings.damage.pathRules.size()>maximumDamageAreas)
            throw std::runtime_error("Assembly exceeds damage area budget");
        std::set<std::string> pathIds,partIds;
        for (const auto &rule:settings.damage.pathRules)
        {
            if (rule.id.empty() || rule.semanticPartId.empty() ||
                !pathIds.insert(rule.id).second ||
                !partIds.insert(rule.semanticPartId).second ||
                !std::isfinite(rule.cutter.radius) || rule.cutter.radius<.001 ||
                rule.cutter.radius>100 || !std::isfinite(rule.cutter.depth) ||
                rule.cutter.depth<.001 || rule.cutter.depth>100)
                throw std::runtime_error("Assembly selected crack settings invalid");
            sampleDamagePathField(rule.cutter.projector.center,rule.cutter);
        }
        const double dimensions[]={settings.widthMeters,settings.heightMeters,settings.gaugeMeters,
            settings.sleeperSpacingMeters,settings.sleeperLengthMeters,settings.sleeperWidthMeters,
            settings.sleeperHeightMeters,settings.boardWidthMeters,settings.boardThicknessMeters,
            settings.beamWidthMeters,settings.beamHeightMeters,settings.supportSpacingMeters,
            settings.baySpacingMeters,settings.couplingSpacingMeters,settings.couplingWidthMeters,
            settings.couplingRadialThicknessMeters,settings.pipeRadiusMeters,settings.railWidthMeters,
            settings.railHeightMeters,settings.spanMeters,settings.towerHeightMeters,settings.sagMeters,
            settings.hangerSpacingMeters,settings.cableRadiusMeters};
        for (double value:dimensions)
            if (!std::isfinite(value) || value<=0 || value>10000)
                throw std::runtime_error("Assembly dimensions and spacing must be finite and positive");
        if (!std::isfinite(settings.boardGapMeters) || settings.boardGapMeters<0 ||
            !std::isfinite(settings.irregularityMeters) || settings.irregularityMeters<0 ||
            !std::isfinite(settings.couplingStartPaddingMeters) ||
            settings.couplingStartPaddingMeters<0 ||
            !std::isfinite(settings.couplingEndPaddingMeters) ||
            settings.couplingEndPaddingMeters<0 ||
            !std::isfinite(settings.couplingVariationMeters) ||
            settings.couplingVariationMeters<0 ||
            settings.couplingVariationMeters>=settings.couplingRadialThicknessMeters)
            throw std::runtime_error("Invalid assembly board gap or irregularity");
        result.path=Construction::samplePath(spline,settings.quality);
        if (settings.kind==Kind::BrokenRailway)
        {
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::Sleeper,4,4,0,0,State::Missing});
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::Rail,3,3,0,0,State::Broken});
        }
        if (settings.kind==Kind::BrokenRailing)
        {
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::Post,2,2,0,0,State::Missing});
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::TopRail,2,2,0,0,State::Broken});
        }
        if (settings.kind==Kind::DamagedPier)
        {
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::DeckPlank,5,5,0,0,State::Missing});
            settings.damage.rules.push_back({SelectorMode::IndexRange,Role::DeckPlank,7,7,0,0,State::Broken});
        }
    }
    std::uint32_t vertex(Vec3d position)
    {
        if (result.mesh.mesh.positions.size()>=settings.maximumVertices ||
            !std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z))
            throw std::runtime_error("Assembly exceeds vertex budget or contains non-finite position");
        const auto index=std::uint32_t(result.mesh.mesh.positions.size());
        result.mesh.mesh.positions.push_back(position);
        result.mesh.field.push_back(currentField);
        result.mesh.normals.push_back({});
        return index;
    }
    void triangle(std::uint32_t a,std::uint32_t b,std::uint32_t c,
                  std::array<float,2> uvA={0,0},std::array<float,2> uvB={0,0},
                  std::array<float,2> uvC={0,0})
    {
        const auto &mesh=result.mesh.mesh;
        const auto normal=cross(sub(mesh.positions[b],mesh.positions[a]),
                                sub(mesh.positions[c],mesh.positions[a]));
        if (length(normal)<1e-10)
            throw std::runtime_error("Assembly produced a degenerate triangle");
        result.mesh.mesh.indices.insert(result.mesh.mesh.indices.end(),{a,b,c});
        result.mesh.mesh.triangleRegions.push_back(currentGuid);
        result.mesh.cornerNormals.insert(result.mesh.cornerNormals.end(),3,unit(normal));
        result.cornerUV.insert(result.cornerUV.end(),{uvA,uvB,uvC});
    }
    template<class Draw> void part(Role role,std::uint32_t index,int side,double station,
                                   Vec3d position,Draw draw)
    {
        if (result.parts.size()>=settings.maximumParts)
            throw std::runtime_error("Assembly exceeds semantic part budget");
        Part item;
        item.id=stablePartId(settings.kind,role,index,side);
        item.guid=stablePartGuid(item.id);
        item.role=role;item.index=index;item.side=side;item.stationMeters=station;
        item.position=position;
        item.protectedPart=settings.damage.protectedIds.contains(item.id);
        item.state=resolveDamage(settings.damage,item);
        item.firstTriangle=result.mesh.mesh.indices.size()/3;
        const auto firstVertex=result.mesh.mesh.positions.size();
        currentGuid=item.guid;
        currentField=float(int(role)+1)+(item.state==State::Intact?0.0f:0.45f);
        if (item.state!=State::Missing)
        {
            draw(item);
            if (!item.protectedPart)
                for (const auto &rule:settings.damage.pathRules)
                    if (rule.semanticPartId==item.id && rule.cutter.enabled)
                        applyPathDamage(item,firstVertex,rule);
        }
        item.triangleCount=result.mesh.mesh.indices.size()/3-item.firstTriangle;
        result.parts.push_back(std::move(item));
    }
    void applyPathDamage(Part &item,std::size_t firstVertex,const PathDamageRule &rule)
    {
        auto &global=result.mesh.mesh;
        const auto firstIndex=item.firstTriangle*3;
        TriangleMesh source;
        source.positions.assign(global.positions.begin()+std::ptrdiff_t(firstVertex),
                                global.positions.end());
        source.indices.assign(global.indices.begin()+std::ptrdiff_t(firstIndex),
                              global.indices.end());
        source.triangleRegions.assign(
            global.triangleRegions.begin()+std::ptrdiff_t(item.firstTriangle),
            global.triangleRegions.end());
        for (auto &index:source.indices)
        {
            if (index<firstVertex || index-firstVertex>=source.positions.size())
                throw std::runtime_error("Assembly damage part crosses another mesh range");
            index-=std::uint32_t(firstVertex);
        }
        const std::vector<std::array<float,2>> sourceUV(
            result.cornerUV.begin()+std::ptrdiff_t(firstIndex),result.cornerUV.end());
        const auto operation=Jerico::stableGuidHash(stablePartGuid(rule.id));
        Jerico::Guid fresh;
        bool unique=false;
        for (std::uint64_t salt=0;salt<256;++salt)
        {
            fresh=Solid::region(settings.damage.seed,operation ^
                Jerico::stableGuidHash(item.guid) ^ 0xDA6A9ULL ^ salt);
            unique=fresh!=item.guid &&
                std::none_of(result.mesh.regions.begin(),result.mesh.regions.end(),
                    [&](const RegionInfo &region){return region.id==fresh;});
            if (unique) break;
        }
        if (!unique) throw std::runtime_error("Assembly damage fresh region ID collision");
        const auto cut=cutDamagePath(source,rule.cutter,fresh,240000);
        if (PA::hashTriangleMesh(cut)==PA::hashTriangleMesh(source)) return;
        if (firstVertex+cut.positions.size()>settings.maximumVertices)
            throw std::runtime_error("Assembly damage exceeds vertex budget");
        global.positions.resize(firstVertex);
        result.mesh.field.resize(firstVertex);
        result.mesh.normals.resize(firstVertex);
        global.indices.resize(firstIndex);
        global.triangleRegions.resize(item.firstTriangle);
        result.mesh.cornerNormals.resize(firstIndex);
        result.cornerUV.resize(firstIndex);
        for (const auto point:cut.positions) vertex(point);
        for (std::size_t face=0;face<cut.indices.size()/3;++face)
        {
            const auto a=cut.indices[3*face],b=cut.indices[3*face+1],c=cut.indices[3*face+2];
            const auto p=cut.positions[a],q=cut.positions[b],r=cut.positions[c];
            const auto faceNormal=unit(cross(sub(q,p),sub(r,p)));
            global.indices.insert(global.indices.end(),{
                std::uint32_t(firstVertex)+a,std::uint32_t(firstVertex)+b,
                std::uint32_t(firstVertex)+c});
            const auto region=cut.triangleRegions[face];
            global.triangleRegions.push_back(region);
            result.mesh.cornerNormals.insert(result.mesh.cornerNormals.end(),3,faceNormal);
            if (region==fresh)
            {
                for (const auto point:{p,q,r})
                {
                    const auto local=projectorCoordinates(rule.cutter.projector,point);
                    result.cornerUV.push_back({float(local.x),float(local.y)});
                }
                continue;
            }
            const auto midpoint=mul(add(p,add(q,r)),1.0/3);
            std::size_t closest=0;
            auto distance=std::numeric_limits<double>::infinity();
            for (std::size_t sourceFace=0;sourceFace<source.indices.size()/3;++sourceFace)
            {
                const auto offset=sourceFace*3;
                const auto found=nearestOnTriangle(midpoint,
                    source.positions[source.indices[offset]],
                    source.positions[source.indices[offset+1]],
                    source.positions[source.indices[offset+2]]);
                if (found.squaredDistance<distance)
                {distance=found.squaredDistance;closest=sourceFace;}
            }
            for (const auto point:{p,q,r})
            {
                const auto offset=closest*3;
                const auto weights=nearestOnTriangle(point,
                    source.positions[source.indices[offset]],
                    source.positions[source.indices[offset+1]],
                    source.positions[source.indices[offset+2]]).weights;
                std::array<float,2> uv{};
                for (int corner=0;corner<3;++corner)
                    for (int axis=0;axis<2;++axis)
                        uv[axis]+=float(weights[corner]*sourceUV[offset+corner][axis]);
                result.cornerUV.push_back(uv);
            }
        }
        item.freshInteriorRegions.push_back(fresh);
        result.mesh.regions.push_back({fresh,item.guid,SurfaceRole::DamageInterior,-1,-1});
    }
    template<class Draw> void part(Role role,std::uint32_t index,int side,double station,Draw draw)
    {
        part(role,index,side,station,
             Construction::frameAtDistance(result.path,station).position,draw);
    }
    void box(Vec3d center,Vec3d tangent,Vec3d normal,Vec3d binormal,
             double along,double high,double across)
    {
        if (!(along>1e-6 && high>1e-6 && across>1e-6))
            throw std::runtime_error("Assembly box dimensions collapsed");
        const auto base=std::uint32_t(result.mesh.mesh.positions.size());
        for (unsigned corner=0;corner<8;++corner)
        {
            const auto point=add(center,add(mul(tangent,along*(corner&1?0.5:-0.5)),
                add(mul(normal,high*(corner&2?0.5:-0.5)),
                    mul(binormal,across*(corner&4?0.5:-0.5)))));
            vertex(point);
        }
        constexpr unsigned faces[][4]={{0,4,6,2},{1,3,7,5},{0,1,5,4},
                                        {2,6,7,3},{0,2,3,1},{4,5,7,6}};
        for (const auto &face:faces)
        {
            const auto a=base+face[0],b=base+face[1],c=base+face[2],d=base+face[3];
            triangle(a,b,c,{0,0},{1,0},{1,1});
            triangle(a,c,d,{0,0},{1,1},{0,1});
        }
    }
    void plank(Vec3d center,Vec3d tangent,Vec3d normal,Vec3d binormal,
               double along,double high,double across)
    {
        if (settings.lod==2) {box(center,tangent,normal,binormal,along,high,across);return;}
        const auto bevel=std::min({high*.22,along*.08,across*.02,.012});
        if (bevel<=1e-6 || bevel*std::min({along,high,across})<=4e-10)
        {box(center,tangent,normal,binormal,along,high,across);return;}
        const auto base=std::uint32_t(result.mesh.mesh.positions.size());
        for (unsigned ring=0;ring<4;++ring)
        {
            const bool inner=ring==0 || ring==3;
            const double a=along*.5-(inner?bevel:0);
            const double b=across*.5-(inner?bevel:0);
            const double y=ring==0?high*.5:ring==1?high*.5-bevel:
                           ring==2?-high*.5+bevel:-high*.5;
            for (unsigned corner=0;corner<4;++corner)
                vertex(add(center,add(mul(tangent,corner==1 || corner==2?a:-a),
                    add(mul(normal,y),mul(binormal,corner>=2?b:-b)))));
        }
        auto quad=[&](unsigned a,unsigned b,unsigned c,unsigned d)
        {
            triangle(base+a,base+b,base+c,{0,0},{1,0},{1,1});
            triangle(base+a,base+c,base+d,{0,0},{1,1},{0,1});
        };
        quad(0,3,2,1);
        for (unsigned ring=0;ring<3;++ring)
            for (unsigned side=0;side<4;++side)
                quad(ring*4+side,ring*4+(side+1)%4,
                     (ring+1)*4+(side+1)%4,(ring+1)*4+side);
        quad(12,13,14,15);
    }
    void beam(Vec3d start,Vec3d end,double width,double height,Vec3d upHint,State state=State::Intact)
    {
        const auto axis=sub(end,start);
        const double span=length(axis);
        if (span<1e-5) throw std::runtime_error("Assembly beam length collapsed");
        const auto t=unit(axis);
        auto projected=sub(upHint,mul(t,dot(upHint,t)));
        if (length(projected)<1e-6)
        {
            const auto alternative=std::abs(t.y)<.9?Vec3d{0,1,0}:Vec3d{0,0,1};
            projected=sub(alternative,mul(t,dot(alternative,t)));
        }
        const auto n=unit(projected),b=unit(cross(t,n));
        if (state==State::Broken)
        {
            beam(start,add(start,mul(axis,.44)),width,height,upHint);
            beam(add(start,mul(axis,.56)),end,width,height,upHint);
            return;
        }
        Vec3d center=mul(add(start,end),.5);
        if (state==State::Loose || state==State::Detached)
            center=add(center,mul(n,-std::min(.12,span*.08)));
        if (state==State::Bent || state==State::Collapsed)
        {
            const auto middle=add(center,mul(n,-std::min(.15,span*.08)));
            beam(start,middle,width,height,upHint);
            beam(middle,end,width,height,upHint);
            return;
        }
        box(center,t,n,b,span,height,width);
    }
    void appendSweep(const Construction::ConstructionResult &swept)
    {
        const auto base=std::uint32_t(result.mesh.mesh.positions.size());
        if (swept.mesh.mesh.positions.size()>settings.maximumVertices-base)
            throw std::runtime_error("Assembly sweep exceeds vertex budget");
        for (auto position:swept.mesh.mesh.positions) vertex(position);
        for (auto index:swept.mesh.mesh.indices)
            result.mesh.mesh.indices.push_back(base+index);
        result.mesh.mesh.triangleRegions.insert(result.mesh.mesh.triangleRegions.end(),
            swept.mesh.mesh.indices.size()/3,currentGuid);
        result.mesh.cornerNormals.insert(result.mesh.cornerNormals.end(),
            swept.mesh.cornerNormals.begin(),swept.mesh.cornerNormals.end());
        result.cornerUV.insert(result.cornerUV.end(),swept.cornerUV.begin(),swept.cornerUV.end());
    }
    void tube(const std::vector<Vec3d> &positions,double radius,
              Construction::ProfileKind profile=Construction::ProfileKind::Circle,
              double width=0,double height=0,double wall=0,bool fracture=false)
    {
        if (positions.size()<2 || positions.size()>256 || radius<=0)
            throw std::runtime_error("Assembly sweep path or radius is invalid");
        PA::ProceduralSpline line;
        line.splineGuid=currentGuid;
        line.name="Assembly member";
        line.interpolation=PA::SplineInterpolation::Polyline;
        for (std::size_t i=0;i<positions.size();++i)
        {
            PA::SplineControlPoint point;
            point.pointGuid=Solid::region(2203,Jerico::stableGuidHash(currentGuid)^i);
            point.position=positions[i];
            line.points.push_back(point);
        }
        Construction::ConstructionSettings sweep;
        sweep.pathQuality=settings.quality;
        sweep.pathQuality.maximumStepMeters=std::max(sweep.pathQuality.maximumStepMeters,
            settings.lod==2?.75:settings.lod==1?.4:.2);
        sweep.profile=profile;
        sweep.radiusMeters=radius;
        sweep.widthMeters=width>0?width:radius*2;
        sweep.heightMeters=height>0?height:radius*2;
        sweep.wallThicknessMeters=wall>0?wall:
            std::min(radius*.3,std::min(sweep.widthMeters,sweep.heightMeters)*.2);
        sweep.profileChordErrorMeters=settings.lod==2?.008:settings.lod==1?.003:.001;
        sweep.profileMaximumAngleRadians=settings.lod==2?.6:settings.lod==1?.35:.2;
        sweep.breakEnabled=fracture;
        if (fracture)
        {
            double lengthMeters=0;
            for (std::size_t i=1;i<positions.size();++i)
                lengthMeters+=length(sub(positions[i],positions[i-1]));
            sweep.breakGapMeters=std::max(radius*3,
                std::clamp(lengthMeters*.08,.08,.7));
        }
        sweep.breakRoughness=.5;
        sweep.maximumVertices=settings.maximumVertices-result.mesh.mesh.positions.size();
        appendSweep(Construction::buildConstruction(line,sweep));
    }
    void strand(std::vector<Vec3d> points,double radius,Vec3d up,State state)
    {
        if (points.size()<8)
            throw std::runtime_error("Assembly strand requires at least eight samples");
        if (state==State::Bent)
            for (std::size_t i=1;i+1<points.size();++i)
                points[i]=add(points[i],mul(up,-.15*std::sin(
                    std::numbers::pi*double(i)/double(points.size()-1))));
        if (state==State::Detached)
            for (auto &point:points) point=add(point,mul(up,-.15));
        if (state==State::Broken)
            tube(points,radius,Construction::ProfileKind::Circle,0,0,0,true);
        else tube(points,radius);
    }
    void brokenBoard(const Construction::PathFrame &frame,double boardWidth,
                     double boardLength,double thickness,const Part &part)
    {
        auto cutCenter=boardLength*.5*(damageRandom01(settings.damage.seed,
            part.id,"cut-position")-.5);
        for (const auto &rule:settings.damage.areaRules)
            if (rule.state==State::Broken &&
                sampleDamageArea(rule.area,{part.position,part.id,name(part.role),
                    part.index,part.stationMeters})>.5)
            {
                cutCenter=std::clamp(dot(sub(rule.area.projector.center,frame.position),
                    frame.binormal),-boardLength*.35,boardLength*.35);
                break;
            }
        const bool fallenRight=damageRandom01(settings.damage.seed,
            part.id,"fallen-side")>=.5;
        const auto cut=[&](double u)
        {
            const auto x=(u/boardWidth+.5);
            return cutCenter+fractureProfileOffset(x,
                std::min(boardLength*.05,boardWidth*.22),boardWidth,
                settings.damage.seed,part.id);
        };
        const auto gap=boardLength*.04;
        auto prism=[&](bool left)
        {
            std::vector<Point2> polygon;
            const auto band=std::min(boardLength*.12,
                std::max(boardWidth*.5,thickness*2));
            const bool localTessellation=boardWidth*band>1e-8 && thickness>1e-5;
            const unsigned steps=localTessellation?16:8;
            if (left)
            {
                polygon.push_back({-boardWidth*.5,-boardLength*.5});
                polygon.push_back({boardWidth*.5,-boardLength*.5});
                for (int i=steps;i>=0;--i)
                {
                    const auto u=boardWidth*(double(i)/steps-.5);
                    polygon.push_back({u,cut(u)-gap*.5});
                }
            }
            else
            {
                for (unsigned i=0;i<=steps;++i)
                {
                    const auto u=boardWidth*(double(i)/steps-.5);
                    polygon.push_back({u,cut(u)+gap*.5});
                }
                polygon.push_back({boardWidth*.5,boardLength*.5});
                polygon.push_back({-boardWidth*.5,boardLength*.5});
            }
            auto points=polygon;
            const auto innerBase=std::uint32_t(points.size());
            if (localTessellation)
                for (unsigned i=0;i<=steps;++i)
                {
                    const auto u=boardWidth*(double(i)/steps-.5);
                    points.push_back({u,cut(u)+(left?-gap*.5-band:gap*.5+band)});
                }
            const auto base=std::uint32_t(result.mesh.mesh.positions.size());
            for (const auto &point:points)
            {
                auto center=add(frame.position,add(mul(frame.tangent,point.u),mul(frame.binormal,point.v)));
                if (left!=fallenRight)
                    center=add(center,add(mul(frame.normal,-thickness*1.2),
                        mul(frame.binormal,(left?-1:1)*thickness*.3)));
                center=add(center,mul(frame.normal,-thickness*.5));
                vertex(center);
                vertex(add(center,mul(frame.normal,thickness)));
            }
            auto orient=[](Point2 a,Point2 b,Point2 c)
            {return (b.u-a.u)*(c.v-a.v)-(b.v-a.v)*(c.u-a.u);};
            auto capFace=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c)
            {
                triangle(base+2*a,base+2*b,base+2*c);
                triangle(base+2*c+1,base+2*b+1,base+2*a+1);
            };
            auto capPolygon=[&](std::vector<std::uint32_t> remaining)
            {
                while (remaining.size()>2)
                {
                    bool clipped=false;
                    for (std::size_t i=0;i<remaining.size();++i)
                    {
                        const auto a=remaining[(i+remaining.size()-1)%remaining.size()],
                                   b=remaining[i],c=remaining[(i+1)%remaining.size()];
                        if (orient(points[a],points[b],points[c])<=1e-12) continue;
                        bool contains=false;
                        for (const auto p:remaining)
                            if (p!=a && p!=b && p!=c &&
                                orient(points[a],points[b],points[p])>=-1e-12 &&
                                orient(points[b],points[c],points[p])>=-1e-12 &&
                                orient(points[c],points[a],points[p])>=-1e-12)
                                {contains=true;break;}
                        if (contains) continue;
                        capFace(a,b,c);
                        remaining.erase(remaining.begin()+std::ptrdiff_t(i));
                        clipped=true;break;
                    }
                    if (!clipped) throw std::runtime_error("Broken board cut cannot be triangulated");
                }
            };
            if (!localTessellation)
            {
                std::vector<std::uint32_t> outline(polygon.size());
                std::iota(outline.begin(),outline.end(),0);
                capPolygon(std::move(outline));
            }
            else
            {
                std::vector<std::uint32_t> farOutline;
                if (left)
                {
                    farOutline={0,1};
                    for (int i=int(steps);i>=0;--i)
                        farOutline.push_back(innerBase+std::uint32_t(i));
                }
                else
                {
                    for (unsigned i=0;i<=steps;++i)
                        farOutline.push_back(innerBase+i);
                    farOutline.push_back(steps+1);
                    farOutline.push_back(steps+2);
                }
                capPolygon(std::move(farOutline));
                for (unsigned i=0;i<steps;++i)
                {
                    const auto cutAt=[&](unsigned sample)
                    {return left?2+steps-sample:sample;};
                    if (left)
                    {
                        capFace(innerBase+i,innerBase+i+1,cutAt(i+1));
                        capFace(innerBase+i,cutAt(i+1),cutAt(i));
                    }
                    else
                    {
                        capFace(cutAt(i),cutAt(i+1),innerBase+i+1);
                        capFace(cutAt(i),innerBase+i+1,innerBase+i);
                    }
                }
            }
            auto wall=[&](std::uint32_t from,std::uint32_t to)
            {
                const auto a=base+2*from,b=base+2*to;
                triangle(a,b+1,b);triangle(a,a+1,b+1);
            };
            for (std::uint32_t i=0;i<polygon.size();++i)
            {
                const auto next=(i+1)%std::uint32_t(polygon.size());
                if (localTessellation &&
                    ((left && i==1) || (!left && i==steps)))
                {wall(i,innerBase+steps);wall(innerBase+steps,next);}
                else if (localTessellation && i==polygon.size()-1)
                {wall(i,innerBase);wall(innerBase,next);}
                else wall(i,next);
            }
        };
        prism(true);prism(false);
    }
    void finish()
    {
        auto &mesh=result.mesh;
        for (std::size_t corner=0;corner<mesh.mesh.indices.size();++corner)
            mesh.normals[mesh.mesh.indices[corner]]=add(mesh.normals[mesh.mesh.indices[corner]],
                                                        mesh.cornerNormals[corner]);
        for (auto &normal:mesh.normals)
            normal=length(normal)>1e-10?unit(normal):Vec3d{0,1,0};
        mesh.closedSolid=false;
    }
};

void buildPipe(Builder &builder,const PA::ProceduralSpline &spline)
{
    const auto &settings=builder.settings;
    builder.part(Role::PipeBody,0,0,builder.result.path.lengthMeters*.5,[&](const Part &part)
    {
        if (part.state!=State::Intact)
        {
            std::vector<Vec3d> points;
            for (unsigned i=0;i<=32;++i)
                points.push_back(Construction::frameAtDistance(builder.result.path,
                    builder.result.path.lengthMeters*i/32).position);
            builder.strand(std::move(points),settings.pipeRadiusMeters,
                           builder.result.path.frames.front().normal,part.state);
            return;
        }
        Construction::ConstructionSettings sweep;
        sweep.pathQuality=settings.quality;
        sweep.profile=Construction::ProfileKind::Circle;
        sweep.radiusMeters=settings.pipeRadiusMeters;
        sweep.profileChordErrorMeters=settings.lod==2?.008:settings.lod==1?.003:.001;
        sweep.maximumVertices=settings.maximumVertices;
        builder.appendSweep(Construction::buildConstruction(spline,sweep));
    });
    if (!settings.couplingsEnabled) return;
    const auto placements=Construction::repeatAlongPath(builder.result.path,
        {.spacingMeters=settings.couplingSpacingMeters,
         .offsetMeters=settings.couplingSpacingMeters*.5,
         .startPaddingMeters=settings.couplingStartPaddingMeters+settings.couplingWidthMeters*.5,
         .endPaddingMeters=settings.couplingEndPaddingMeters+settings.couplingWidthMeters*.5,
         .maximumParts=settings.maximumParts});
    builder.result.repeatedStations+=placements.size();
    for (std::size_t i=0;i<placements.size();++i)
    {
        const auto &frame=placements[i].frame;
        builder.part(Role::Coupling,std::uint32_t(i),0,frame.distanceMeters,[&](const Part &part)
        {
            const auto half=settings.couplingWidthMeters*.5;
            const auto start=add(frame.position,mul(frame.tangent,-half));
            const auto end=add(frame.position,mul(frame.tangent,half));
            const auto variation=settings.couplingVariationMeters*
                std::sin(double(i+1)*2.399963+double(settings.variationSeed)*.17);
            const auto radius=settings.pipeRadiusMeters+
                settings.couplingRadialThicknessMeters+variation;
            builder.tube({start,end},radius,Construction::ProfileKind::Ring,0,0,
                         settings.couplingRadialThicknessMeters+variation);
        });
    }
}
void buildRailway(Builder &builder)
{
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    if (s.gaugeMeters>=s.sleeperLengthMeters)
        throw std::runtime_error("Railway gauge must fit inside sleeper length");
    const auto left=offsetPath(path,0,-s.gaugeMeters*.5);
    const auto right=offsetPath(path,0,s.gaugeMeters*.5);
    (void)left;(void)right;
    const auto placements=Construction::repeatAlongPath(path,
        {.spacingMeters=s.sleeperSpacingMeters,.offsetMeters=s.sleeperSpacingMeters*.5,
         .endPaddingMeters=s.sleeperSpacingMeters*.25,.maximumParts=s.maximumParts});
    builder.result.repeatedStations+=placements.size();
    for (std::size_t i=0;i<placements.size();++i)
    {
        const auto &frame=placements[i].frame;
        builder.part(Role::Sleeper,std::uint32_t(i),0,frame.distanceMeters,
                     add(frame.position,mul(frame.normal,-s.sleeperHeightMeters*.6)),
                     [&](const Part &part)
        {
            auto center=add(frame.position,mul(frame.normal,-s.sleeperHeightMeters*.6));
            if (part.state==State::Loose || part.state==State::Detached)
                center=add(center,mul(frame.normal,-s.sleeperHeightMeters*.8));
            if (part.state==State::Broken)
                builder.brokenBoard(frame,s.sleeperWidthMeters,s.sleeperLengthMeters,
                                    s.sleeperHeightMeters,part);
            else if (part.state==State::Bent || part.state==State::Collapsed)
            {
                const auto tilted=unit(add(frame.binormal,mul(frame.normal,.15)));
                const auto normal=unit(cross(tilted,frame.tangent));
                builder.box(center,frame.tangent,normal,tilted,
                            s.sleeperWidthMeters,s.sleeperHeightMeters,s.sleeperLengthMeters);
            }
            else builder.box(center,frame.tangent,frame.normal,frame.binormal,
                             s.sleeperWidthMeters,s.sleeperHeightMeters,s.sleeperLengthMeters);
        });
    }
    const auto segments=std::max(std::size_t(1),
        std::size_t(std::ceil(path.lengthMeters/s.sleeperSpacingMeters)));
    for (int side:{-1,1})
        for (std::size_t i=0;i<segments;++i)
        {
            const double start=path.lengthMeters*double(i)/double(segments);
            const double end=path.lengthMeters*double(i+1)/double(segments);
            const auto middle=Construction::frameAtDistance(path,(start+end)*.5);
            const auto railCenter=add(middle.position,
                add(mul(middle.binormal,side*s.gaugeMeters*.5),
                    mul(middle.normal,s.railHeightMeters*.5)));
            builder.part(Role::Rail,std::uint32_t(i),side,(start+end)*.5,
                         railCenter,[&](const Part &part)
            {
                auto point=[&](double distance)
                {
                    const auto frame=Construction::frameAtDistance(path,distance);
                    double deformation=0;
                    if (part.state==State::Bent || part.state==State::Collapsed)
                    {
                        const auto fraction=(distance-start)/(end-start);
                        deformation=-.12*std::sin(std::numbers::pi*fraction);
                    }
                    if (part.state==State::Loose || part.state==State::Detached)
                        deformation-=s.railHeightMeters;
                    return add(frame.position,add(mul(frame.binormal,side*s.gaugeMeters*.5),
                        mul(frame.normal,s.railHeightMeters*.5+deformation)));
                };
                if (part.state==State::Broken)
                {
                    builder.tube({point(start),point(std::lerp(start,end,.44))},s.railWidthMeters*.5,
                                 Construction::ProfileKind::IBeam,s.railWidthMeters,s.railHeightMeters);
                    builder.tube({point(std::lerp(start,end,.56)),point(end)},s.railWidthMeters*.5,
                                 Construction::ProfileKind::IBeam,s.railWidthMeters,s.railHeightMeters);
                }
                else builder.tube({point(start),point((start+end)*.5),point(end)},s.railWidthMeters*.5,
                                  Construction::ProfileKind::IBeam,s.railWidthMeters,s.railHeightMeters);
            });
        }
    if (s.railJointsEnabled)
        for (int side:{-1,1})
            for (std::size_t i=1;i<segments;++i)
            {
                const auto station=path.lengthMeters*double(i)/double(segments);
                const auto frame=Construction::frameAtDistance(path,station);
                const auto center=add(frame.position,
                    add(mul(frame.binormal,side*s.gaugeMeters*.5),
                        mul(frame.normal,s.railHeightMeters*.45)));
                builder.part(Role::RailJoint,std::uint32_t(i),side,station,
                             center,
                             [&](const Part &part)
                {
                    builder.box(center,frame.tangent,frame.normal,frame.binormal,
                        s.railWidthMeters*2,s.railHeightMeters*.45,s.railWidthMeters*1.6);
                });
            }
}
void buildPier(Builder &builder,bool groundSupports=true)
{
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    const auto fit=fitCountSpacing(path.lengthMeters,s.boardWidthMeters,s.boardGapMeters);
    if (fit.count>builder.settings.maximumParts/2)
        throw std::runtime_error("Pier board count exceeds assembly part budget");
    for (std::size_t i=0;i<fit.count;++i)
    {
        const auto station=fit.actualWidthMeters*(i+.5)+s.boardGapMeters*i;
        const auto frame=Construction::frameAtDistance(path,station);
        builder.part(Role::DeckPlank,std::uint32_t(i),0,station,[&](const Part &part)
        {
            auto center=frame.position;
            if (part.state==State::Loose || part.state==State::Detached)
                center=add(center,mul(frame.normal,-s.boardThicknessMeters*1.7));
            if (part.state==State::Bent || part.state==State::Collapsed)
            {
                const auto tilted=unit(add(frame.binormal,mul(frame.normal,.15)));
                const auto normal=unit(cross(tilted,frame.tangent));
                builder.box(center,frame.tangent,normal,tilted,
                            fit.actualWidthMeters,s.boardThicknessMeters,s.widthMeters);
                return;
            }
            if (part.state==State::Broken)
                builder.brokenBoard(frame,fit.actualWidthMeters,s.widthMeters,s.boardThicknessMeters,
                                    part);
            else builder.plank(center,frame.tangent,frame.normal,frame.binormal,
                               fit.actualWidthMeters,s.boardThicknessMeters,s.widthMeters);
        });
    }
    builder.result.repeatedStations+=fit.count;
    for (int side:{-1,1})
    {
        const auto middle=Construction::frameAtDistance(path,path.lengthMeters*.5);
        const auto center=add(middle.position,
            add(mul(middle.binormal,side*s.widthMeters*.36),
                mul(middle.normal,-s.beamHeightMeters*.8)));
        builder.part(Role::Beam,0,side,path.lengthMeters*.5,center,[&](const Part &part)
        {
            std::vector<Vec3d> points;
            const auto count=std::clamp(std::size_t(std::ceil(path.lengthMeters/.25)),
                                        std::size_t(8),std::size_t(64));
            for (std::size_t i=0;i<=count;++i)
            {
                const auto frame=Construction::frameAtDistance(path,path.lengthMeters*i/count);
                points.push_back(add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.36),
                                                mul(frame.normal,-s.beamHeightMeters*.8))));
            }
            if (part.state==State::Bent || part.state==State::Collapsed)
                for (std::size_t i=1;i+1<points.size();++i)
                    points[i]=add(points[i],mul(path.frames.front().normal,
                        -.12*std::sin(std::numbers::pi*double(i)/double(points.size()-1))));
            if (part.state==State::Detached || part.state==State::Loose)
                for (auto &point:points)
                    point=add(point,mul(path.frames.front().normal,-s.beamHeightMeters));
            if (part.state==State::Broken)
            {
                const auto middle=points.size()/2;
                builder.tube(std::vector<Vec3d>(points.begin(),
                    points.begin()+std::ptrdiff_t(middle-1)),s.beamWidthMeters*.5,
                    Construction::ProfileKind::Rectangle,s.beamWidthMeters,s.beamHeightMeters);
                builder.tube(std::vector<Vec3d>(points.begin()+std::ptrdiff_t(middle+2),
                    points.end()),s.beamWidthMeters*.5,Construction::ProfileKind::Rectangle,
                    s.beamWidthMeters,s.beamHeightMeters);
            }
            else builder.tube(points,s.beamWidthMeters*.5,
                Construction::ProfileKind::Rectangle,s.beamWidthMeters,s.beamHeightMeters);
        });
    }
    const auto supports=Construction::repeatAlongPath(path,
        {.spacingMeters=s.supportSpacingMeters,.offsetMeters=s.supportSpacingMeters*.5,
         .endPaddingMeters=s.supportSpacingMeters*.25,.maximumParts=s.maximumParts});
    builder.result.repeatedStations+=supports.size();
    for (std::size_t i=0;i<supports.size();++i)
    {
        const auto &frame=supports[i].frame;
        builder.part(Role::CrossBeam,std::uint32_t(i),0,frame.distanceMeters,
                     add(frame.position,mul(frame.normal,-s.beamHeightMeters*.9)),
                     [&](const Part &part)
        {
            builder.box(add(frame.position,mul(frame.normal,-s.beamHeightMeters*.9)),
                frame.tangent,frame.normal,frame.binormal,
                s.beamWidthMeters,s.beamHeightMeters*.65,s.widthMeters*.95);
        });
        if (groundSupports)
            for (int side:{-1,1})
                builder.part(Role::Pile,std::uint32_t(i),side,frame.distanceMeters,
                             add(frame.position,
                                 add(mul(frame.binormal,side*s.widthMeters*.38),
                                     mul(frame.normal,-s.beamHeightMeters-
                                         std::max(1.5,s.heightMeters*1.5)*.5))),
                             [&](const Part &part)
                {
                    const auto top=add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.38),
                                                         mul(frame.normal,-s.beamHeightMeters)));
                    const auto bottom=add(top,mul(frame.normal,-std::max(1.5,s.heightMeters*1.5)));
                    builder.beam(bottom,top,s.beamWidthMeters,s.beamWidthMeters,frame.tangent,part.state);
                });
    }
    for (int side:{-1,1})
    {
        const auto middle=Construction::frameAtDistance(path,path.lengthMeters*.5);
        const auto fasciaCenter=add(middle.position,
            add(mul(middle.binormal,side*s.widthMeters*.5),
                mul(middle.normal,-s.boardThicknessMeters*.5)));
        builder.part(Role::Fascia,0,side,path.lengthMeters*.5,
                     fasciaCenter,[&](const Part &part)
        {
            const auto first=Construction::frameAtDistance(path,0);
            const auto last=Construction::frameAtDistance(path,path.lengthMeters);
            const auto offset=[&](const Construction::PathFrame &frame)
            {return add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.5),
                                           mul(frame.normal,-s.boardThicknessMeters*.5)));};
            builder.beam(offset(first),offset(last),s.boardThicknessMeters,
                         s.beamHeightMeters,first.normal,part.state);
        });
        for (std::size_t i=0;groundSupports && i+1<supports.size();++i)
        {
            const auto station=(supports[i].frame.distanceMeters+
                                supports[i+1].frame.distanceMeters)*.5;
            const auto frame=Construction::frameAtDistance(path,station);
            const auto braceCenter=add(frame.position,
                add(mul(frame.binormal,side*s.widthMeters*.38),
                    mul(frame.normal,-(s.heightMeters*1.2+s.beamHeightMeters)*.5)));
            builder.part(Role::Brace,std::uint32_t(i),side,
                station,braceCenter,
                [&](const Part &part)
            {
                const auto &a=supports[i].frame,&b=supports[i+1].frame;
                const auto start=add(a.position,add(mul(a.binormal,side*s.widthMeters*.38),
                                                   mul(a.normal,-s.heightMeters*1.2)));
                const auto end=add(b.position,add(mul(b.binormal,side*s.widthMeters*.38),
                                                 mul(b.normal,-s.beamHeightMeters)));
                builder.beam(start,end,s.beamWidthMeters*.5,s.beamWidthMeters*.5,
                             a.normal,part.state);
            });
        }
    }
}
void buildRailing(Builder &builder)
{
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    const auto bays=std::max(std::size_t(1),
        std::size_t(std::ceil(path.lengthMeters/s.baySpacingMeters)));
    if (bays*8>s.maximumParts)
        throw std::runtime_error("Railing bay count exceeds assembly part budget");
    for (int side:{-1,1})
    {
        for (std::size_t i=0;i<=bays;++i)
        {
            const double station=path.lengthMeters*i/bays;
            const auto frame=Construction::frameAtDistance(path,station);
            const auto base=add(frame.position,mul(frame.binormal,side*s.widthMeters*.5));
            builder.part(Role::Post,std::uint32_t(i),side,station,
                         add(base,mul(frame.normal,s.heightMeters*.5)),[&](const Part &part)
            {
                auto top=add(base,mul(frame.normal,s.heightMeters));
                if (part.state==State::Bent || part.state==State::Collapsed)
                    top=add(top,mul(frame.tangent,s.heightMeters*.28));
                builder.beam(base,top,s.beamWidthMeters,s.beamWidthMeters,
                             frame.tangent,part.state);
            });
        }
        for (std::size_t i=0;i<bays;++i)
        {
            const double start=path.lengthMeters*i/bays,end=path.lengthMeters*(i+1)/bays;
            const auto a=Construction::frameAtDistance(path,start);
            const auto b=Construction::frameAtDistance(path,end);
            for (Role role:{Role::TopRail,Role::MiddleRail,Role::LowerRail})
            {
                if (role==Role::LowerRail && !s.lowerRailEnabled) continue;
                const double level=role==Role::TopRail?1.0:role==Role::MiddleRail?.53:.18;
                const auto middle=Construction::frameAtDistance(path,(start+end)*.5);
                const auto railPosition=add(middle.position,
                    add(mul(middle.binormal,side*s.widthMeters*.5),
                        mul(middle.normal,s.heightMeters*level)));
                builder.part(role,std::uint32_t(i),side,(start+end)*.5,
                             railPosition,[&](const Part &part)
                {
                    const auto point=[&](const Construction::PathFrame &frame)
                    {return add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.5),
                                                   mul(frame.normal,s.heightMeters*level)));};
                    builder.beam(point(a),point(b),s.beamWidthMeters*.8,
                                 s.beamWidthMeters*.8,a.normal,part.state);
                });
            }
            const auto center=Construction::frameAtDistance(path,(start+end)*.5);
            const auto foot=add(center.position,mul(center.binormal,side*s.widthMeters*.5));
            builder.part(Role::Baluster,std::uint32_t(i),side,(start+end)*.5,
                         add(foot,mul(center.normal,s.heightMeters*.5)),
                         [&](const Part &part)
            {
                builder.beam(foot,add(foot,mul(center.normal,s.heightMeters)),
                             s.beamWidthMeters*.5,s.beamWidthMeters*.5,center.tangent,part.state);
            });
        }
    }
    builder.result.repeatedStations+=bays+1;
}
void buildBeamBridge(Builder &builder,bool truss)
{
    buildPier(builder);
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    const auto bays=std::max(std::size_t(1),
        std::size_t(std::ceil(path.lengthMeters/s.baySpacingMeters)));
    for (int side:{-1,1})
    {
        for (std::size_t i=0;i<bays;++i)
        {
            const double start=path.lengthMeters*i/bays,end=path.lengthMeters*(i+1)/bays;
            const auto a=Construction::frameAtDistance(path,start);
            const auto b=Construction::frameAtDistance(path,end);
            const auto point=[&](const Construction::PathFrame &frame,double level)
            {return add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.52),
                                           mul(frame.normal,level)));};
            const auto middle=Construction::frameAtDistance(path,(start+end)*.5);
            builder.part(Role::Girder,std::uint32_t(i),side,(start+end)*.5,
                point(middle,-s.beamHeightMeters),
                [&](const Part &part)
            {builder.beam(point(a,-s.beamHeightMeters),point(b,-s.beamHeightMeters),
                          s.beamWidthMeters,s.beamHeightMeters,a.normal,part.state);});
            if (truss)
            {
                builder.part(Role::TopRail,std::uint32_t(i),side,(start+end)*.5,
                    point(middle,s.heightMeters),
                    [&](const Part &part)
                {builder.beam(point(a,s.heightMeters),point(b,s.heightMeters),
                              s.beamWidthMeters*.7,s.beamWidthMeters*.7,a.normal,part.state);});
                for (int diagonal=0;diagonal<2;++diagonal)
                    builder.part(Role::Brace,std::uint32_t(i*2+diagonal),side,
                        (start+end)*.5,point(middle,s.heightMeters*.5),
                        [&](const Part &part)
                    {
                        Vec3d from,to;
                        if (s.kind==Kind::TrussWarren)
                        {from=diagonal?point(middle,s.heightMeters):point(a,0);
                         to=diagonal?point(b,0):point(middle,s.heightMeters);}
                        else if (s.kind==Kind::TrussPratt)
                        {from=diagonal?point(middle,s.heightMeters):point(a,s.heightMeters);
                         to=diagonal?point(b,0):point(middle,0);}
                        else
                        {from=diagonal?point(middle,0):point(a,0);
                         to=diagonal?point(b,s.heightMeters):point(middle,s.heightMeters);}
                        builder.beam(from,to,s.beamWidthMeters*.55,s.beamWidthMeters*.55,
                                     a.normal,part.state);
                    });
            }
        }
    }
    buildRailing(builder);
}
void buildSuspension(Builder &builder)
{
    buildPier(builder,false);
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    const auto length=path.lengthMeters;
    if (s.sagMeters>=s.towerHeightMeters)
        throw std::runtime_error("Suspension sag must stay below tower height");
    for (int side:{-1,1})
    {
        const auto point=[&](double station)
        {
            const auto frame=Construction::frameAtDistance(path,station);
            return add(frame.position,add(mul(frame.binormal,side*s.widthMeters*.52),
                mul(frame.normal,s.towerHeightMeters-parabolicSag(station,length,s.sagMeters))));
        };
        builder.part(Role::Cable,0,side,length*.5,point(length*.5),[&](const Part &part)
        {
            std::vector<Vec3d> curve;
            for (unsigned i=0;i<=32;++i) curve.push_back(point(length*i/32));
            builder.strand(std::move(curve),s.cableRadiusMeters,
                           path.frames.front().normal,part.state);
        });
        for (unsigned i=0;i<2;++i)
        {
            const auto station=i?length:0.0;
            const auto frame=Construction::frameAtDistance(path,station);
            const auto base=add(frame.position,mul(frame.binormal,side*s.widthMeters*.52));
            builder.part(Role::Tower,i,side,station,
                         mul(add(base,point(station)),.5),[&](const Part &part)
            {builder.beam(base,point(station),s.beamWidthMeters*1.5,
                          s.beamWidthMeters*1.5,frame.tangent,part.state);});
        }
        const auto hangers=Construction::repeatAlongPath(path,
            {.spacingMeters=s.hangerSpacingMeters,.offsetMeters=s.hangerSpacingMeters*.5,
             .endPaddingMeters=s.hangerSpacingMeters*.25,.maximumParts=s.maximumParts});
        builder.result.repeatedStations+=hangers.size();
        for (std::size_t i=0;i<hangers.size();++i)
        {
            const auto &frame=hangers[i].frame;
            const auto foot=add(frame.position,mul(frame.binormal,side*s.widthMeters*.52));
            builder.part(Role::Hanger,std::uint32_t(i),side,frame.distanceMeters,
                         mul(add(foot,point(frame.distanceMeters)),.5),
                         [&](const Part &part)
            {
                builder.beam(foot,point(frame.distanceMeters),s.cableRadiusMeters*1.2,
                             s.cableRadiusMeters*1.2,frame.tangent,part.state);
            });
        }
    }
    for (unsigned end=0;end<2;++end)
    {
        const auto station=end?length:0.0;
        const auto frame=Construction::frameAtDistance(path,station);
        builder.part(Role::Anchor,end,0,station,
                     add(frame.position,mul(frame.normal,s.towerHeightMeters)),[&](const Part &part)
        {
            const auto top=add(frame.position,mul(frame.normal,s.towerHeightMeters));
            builder.beam(add(top,mul(frame.binormal,-s.widthMeters*.52)),
                         add(top,mul(frame.binormal,s.widthMeters*.52)),
                         s.beamWidthMeters*.8,s.beamWidthMeters*.8,
                         frame.normal,part.state);
        });
    }
}
void buildRopeBridge(Builder &builder)
{
    const auto &s=builder.settings;
    const auto &path=builder.result.path;
    const auto length=path.lengthMeters;
    const auto a=solveCatenaryA(length,s.sagMeters);
    const auto deckPoint=[&](const Construction::PathFrame &frame)
    {return add(frame.position,mul(frame.normal,-catenaryHeight(frame.distanceMeters,
                                                                  length,s.sagMeters,a)));};
    const auto stations=Construction::repeatAlongPath(path,
        {.spacingMeters=s.boardWidthMeters+s.boardGapMeters,
         .offsetMeters=(s.boardWidthMeters+s.boardGapMeters)*.5,
         .endPaddingMeters=s.boardWidthMeters*.25,.maximumParts=s.maximumParts});
    builder.result.repeatedStations+=stations.size();
    for (std::size_t i=0;i<stations.size();++i)
    {
        const auto &frame=stations[i].frame;
        builder.part(Role::DeckPlank,std::uint32_t(i),0,frame.distanceMeters,
                     deckPoint(frame),
                     [&](const Part &part)
        {
            auto adjusted=frame;
            adjusted.position=deckPoint(frame);
            const auto step=std::min(.02,length*.01);
            const auto before=Construction::frameAtDistance(path,
                std::max(0.0,frame.distanceMeters-step));
            const auto after=Construction::frameAtDistance(path,
                std::min(length,frame.distanceMeters+step));
            adjusted.tangent=unit(sub(deckPoint(after),deckPoint(before)));
            adjusted.binormal=unit(sub(frame.binormal,mul(adjusted.tangent,
                dot(frame.binormal,adjusted.tangent))));
            adjusted.normal=unit(cross(adjusted.binormal,adjusted.tangent));
            const double variation=s.irregularityMeters*std::sin(double(i)*2.399963);
            adjusted.position=add(adjusted.position,mul(adjusted.normal,variation));
            if (part.state==State::Loose || part.state==State::Detached)
                adjusted.position=add(adjusted.position,
                    mul(adjusted.normal,-s.boardThicknessMeters*2));
            if (part.state==State::Broken)
                builder.brokenBoard(adjusted,s.boardWidthMeters,s.widthMeters,
                                    s.boardThicknessMeters,part);
            else builder.plank(adjusted.position,adjusted.tangent,adjusted.normal,
                               adjusted.binormal,s.boardWidthMeters,
                               s.boardThicknessMeters,s.widthMeters);
        });
    }
    for (int side:{-1,1})
    {
        for (Role role:{Role::Cable,Role::HandRope})
        {
            const auto middle=Construction::frameAtDistance(path,length*.5);
            const auto center=add(deckPoint(middle),
                add(mul(middle.binormal,side*s.widthMeters*.48),
                    mul(middle.normal,role==Role::HandRope?s.heightMeters:0)));
            builder.part(role,0,side,length*.5,center,[&](const Part &part)
            {
                std::vector<Vec3d> curve;
                for (unsigned i=0;i<=32;++i)
                {
                    auto frame=Construction::frameAtDistance(path,length*i/32);
                    auto point=deckPoint(frame);
                    point=add(point,add(mul(frame.binormal,side*s.widthMeters*.48),
                        mul(frame.normal,role==Role::HandRope?s.heightMeters:0)));
                    curve.push_back(point);
                }
                builder.strand(std::move(curve),
                    s.cableRadiusMeters*(role==Role::HandRope?.8:1),
                    path.frames.front().normal,part.state);
            });
        }
        const auto tieStep=std::max(std::size_t(1),stations.size()/14);
        for (std::size_t i=0;i<stations.size();i+=tieStep)
        {
            const auto &frame=stations[i].frame;
            const auto foot=add(deckPoint(frame),mul(frame.binormal,side*s.widthMeters*.48));
            builder.part(Role::Hanger,std::uint32_t(i),side,frame.distanceMeters,
                         add(foot,mul(frame.normal,s.heightMeters*.5)),
                         [&](const Part &part)
            {
                builder.beam(foot,add(foot,mul(frame.normal,s.heightMeters)),
                             s.cableRadiusMeters*.9,s.cableRadiusMeters*.9,
                             frame.tangent,part.state);
            });
        }
        for (unsigned i=0;i<2;++i)
        {
            auto frame=Construction::frameAtDistance(path,i?length:0);
            const auto foot=add(frame.position,mul(frame.binormal,side*s.widthMeters*.48));
            builder.part(Role::Anchor,i,side,frame.distanceMeters,
                         add(foot,mul(frame.normal,s.heightMeters*.5)),[&](const Part &part)
            {
                builder.beam(foot,add(foot,mul(frame.normal,s.heightMeters)),
                             s.beamWidthMeters,s.beamWidthMeters,frame.tangent,part.state);
            });
        }
    }
}
}

Result build(const PA::ProceduralSpline &spline,const Settings &settings)
{
    const auto started=std::chrono::steady_clock::now();
    PA::ProceduralSpline fitted=spline;
    if (settings.kind==Kind::BeamBridge || settings.kind==Kind::TrussWarren ||
        settings.kind==Kind::TrussPratt || settings.kind==Kind::TrussHowe ||
        settings.kind==Kind::SuspensionBridge || settings.kind==Kind::RopePlankBridge)
    {
        const auto original=Construction::samplePath(fitted,settings.quality);
        const double scale=settings.spanMeters/original.lengthMeters;
        const auto origin=fitted.points.front().position;
        for (auto &point:fitted.points)
        {
            point.position=add(origin,mul(sub(point.position,origin),scale));
            point.incomingHandle=mul(point.incomingHandle,scale);
            point.outgoingHandle=mul(point.outgoingHandle,scale);
        }
    }
    Builder builder(fitted,settings);
    switch (settings.kind)
    {
    case Kind::PipeCouplings: buildPipe(builder,spline); break;
    case Kind::Railway:
    case Kind::BrokenRailway: buildRailway(builder); break;
    case Kind::WoodenPier:
    case Kind::DamagedPier: buildPier(builder); break;
    case Kind::Railing:
    case Kind::BrokenRailing: buildRailing(builder); break;
    case Kind::Catwalk: buildPier(builder); buildRailing(builder); break;
    case Kind::BeamBridge: buildBeamBridge(builder,false); break;
    case Kind::TrussWarren:
    case Kind::TrussPratt:
    case Kind::TrussHowe: buildBeamBridge(builder,true); break;
    case Kind::SuspensionBridge: buildSuspension(builder); break;
    case Kind::RopePlankBridge: buildRopeBridge(builder); break;
    default: throw std::runtime_error("Assembly family is not implemented yet");
    }
    builder.finish();
    builder.result.generationCpuMilliseconds=
        std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    builder.result.mesh.milliseconds=builder.result.generationCpuMilliseconds;
    return std::move(builder.result);
}
}
