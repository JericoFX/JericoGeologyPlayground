#include "WebLod.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace Playground::Web
{
namespace
{
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double value) {return {a.x*value,a.y*value,a.z*value};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3d a) {return std::sqrt(dot(a,a));}
Vec3d unit(Vec3d a)
{
    const auto size=length(a);
    if (size<1e-12 || !std::isfinite(size))
        throw std::runtime_error("Web edge has undefined direction");
    return mul(a,1/size);
}
struct Point2 {double x{},y{};};
Point2 project(Vec3d world,const CardPatch &patch)
{
    const auto local=sub(world,patch.center);
    return {dot(local,patch.axisU),dot(local,patch.axisV)};
}
double distanceToSegment(Point2 point,Point2 start,Point2 end)
{
    const auto x=end.x-start.x,y=end.y-start.y;
    const auto squared=x*x+y*y;
    if (squared<1e-20) return std::hypot(point.x-start.x,point.y-start.y);
    const auto t=std::clamp(((point.x-start.x)*x+(point.y-start.y)*y)/squared,0.0,1.0);
    return std::hypot(point.x-start.x-t*x,point.y-start.y-t*y);
}
bool active(const Edge &edge,const Settings &settings)
{return edge.state==EdgeState::Intact && !settings.brokenEdges.contains(edge.id);}
bool withinArea(Point2 point,const Graph &graph,const Settings &settings,
                const CardPatch &patch)
{
    if (settings.authoring!=Authoring::Area) return true;
    const auto patchIndex=patch.index==std::numeric_limits<unsigned>::max()?0:patch.index;
    const auto &polygon=graph.areaMasks.at(patchIndex);
    bool inside=false;
    for (std::size_t i=0,j=polygon.size()-1;i<polygon.size();j=i++)
    {
        const auto a=project(polygon[i],patch);
        const auto b=project(polygon[j],patch);
        if ((a.y>point.y)!=(b.y>point.y) &&
            point.x<(b.x-a.x)*(point.y-a.y)/(b.y-a.y)+a.x)
            inside=!inside;
    }
    return inside;
}
double edgeDistance(Point2 sample,const Edge &edge,const Graph &graph,
                    const CardPatch &patch)
{
    const auto a=project(graph.nodes[edge.start],patch);
    const auto b=project(graph.nodes[edge.end],patch);
    if (!edge.sagged) return distanceToSegment(sample,a,b);
    const auto mid=project(edge.sagPoint,patch);
    return std::min(distanceToSegment(sample,a,mid),
                    distanceToSegment(sample,mid,b));
}
std::uint8_t encodeNormal(double value)
{return std::uint8_t(std::lround(std::clamp(value*.5+.5,0.0,1.0)*255));}
void appendSweep(MeshResult &mesh,std::vector<std::array<float,2>> &uv,
                 const Construction::ConstructionResult &piece,Jerico::Guid guid)
{
    const auto base=std::uint32_t(mesh.mesh.positions.size());
    mesh.mesh.positions.insert(mesh.mesh.positions.end(),
        piece.mesh.mesh.positions.begin(),piece.mesh.mesh.positions.end());
    mesh.normals.insert(mesh.normals.end(),piece.mesh.normals.begin(),piece.mesh.normals.end());
    mesh.field.insert(mesh.field.end(),piece.mesh.field.begin(),piece.mesh.field.end());
    for (auto index:piece.mesh.mesh.indices) mesh.mesh.indices.push_back(base+index);
    mesh.mesh.triangleRegions.insert(mesh.mesh.triangleRegions.end(),
        piece.mesh.mesh.indices.size()/3,guid);
    mesh.cornerNormals.insert(mesh.cornerNormals.end(),
        piece.mesh.cornerNormals.begin(),piece.mesh.cornerNormals.end());
    uv.insert(uv.end(),piece.cornerUV.begin(),piece.cornerUV.end());
}
void appendQuad(MeshResult &mesh,std::vector<std::array<float,2>> &uv,
                const std::array<Vec3d,4> &positions,
                const std::array<std::array<float,2>,4> &coordinates,
                Vec3d normal,Jerico::Guid guid)
{
    const auto base=std::uint32_t(mesh.mesh.positions.size());
    mesh.mesh.positions.insert(mesh.mesh.positions.end(),positions.begin(),positions.end());
    mesh.normals.insert(mesh.normals.end(),4,normal);
    mesh.field.insert(mesh.field.end(),4,1.0f);
    constexpr unsigned corners[]={0,1,2,0,2,3};
    for (auto corner:corners)
    {mesh.mesh.indices.push_back(base+corner);
     mesh.cornerNormals.push_back(normal);
     uv.push_back(coordinates[corner]);}
    mesh.mesh.triangleRegions.insert(mesh.mesh.triangleRegions.end(),2,guid);
}
TextureLevel rasterLevel(const Graph &graph,const Settings &settings,
                         const std::vector<CardPatch> &patches,unsigned size,
                         const std::atomic_bool *cancelled)
{
    TextureLevel result;
    result.width=size*unsigned(patches.size());
    result.height=size;
    const auto pixels=std::size_t(result.width)*result.height;
    result.coverageRgba.resize(pixels*4);
    result.normalRgba.resize(pixels*4);
    result.distanceRgba.resize(pixels*4);
    std::vector<float> signedDistance(pixels,std::numeric_limits<float>::infinity());
    double minimumPixels=std::numeric_limits<double>::infinity();
    std::size_t occupied{};
    for (std::size_t patchIndex=0;patchIndex<patches.size();++patchIndex)
    {
        const auto &patch=patches[patchIndex];
        const auto stepU=(patch.maxU-patch.minU)/size;
        const auto stepV=(patch.maxV-patch.minV)/size;
        const auto pixelHalfDiagonal=.5*std::hypot(stepU,stepV);
        for (unsigned y=0;y<size;++y)
        {
            if (cancelled && cancelled->load())
                throw std::runtime_error("Web bake cancelled");
            for (unsigned x=0;x<size;++x)
            {
                const Point2 center{patch.minU+(x+.5)*stepU,
                                    patch.minV+(y+.5)*stepV};
                const auto pixel=std::size_t(y)*result.width+patchIndex*size+x;
                std::uint64_t coveredSamples{};
                double nearest=std::numeric_limits<double>::infinity();
                if (!withinArea(center,graph,settings,patch))
                {
                    const auto offset=pixel*4;
                    result.normalRgba[offset]=128;result.normalRgba[offset+1]=128;
                    result.normalRgba[offset+2]=255;
                    result.distanceRgba[offset]=255;
                    result.distanceRgba[offset+1]=255;
                    result.distanceRgba[offset+2]=255;
                    continue;
                }
                for (const auto &edge:graph.edges)
                {
                    if (!active(edge,settings) ||
                        (patch.index!=std::numeric_limits<unsigned>::max() &&
                         edge.patch!=patch.index)) continue;
                    const auto radius=edge.radiusMeters;
                    minimumPixels=std::min(minimumPixels,
                        2*radius*size/std::max(patch.maxU-patch.minU,patch.maxV-patch.minV));
                    const auto distance=edgeDistance(center,edge,graph,patch);
                    nearest=std::min(nearest,distance-radius);
                    if (distance>radius+pixelHalfDiagonal) continue;
                    for (unsigned sampleY=0;sampleY<settings.supersamples;++sampleY)
                        for (unsigned sampleX=0;sampleX<settings.supersamples;++sampleX)
                        {
                            const Point2 sample{
                                patch.minU+(x+(sampleX+.5)/settings.supersamples)*stepU,
                                patch.minV+(y+(sampleY+.5)/settings.supersamples)*stepV};
                            if (withinArea(sample,graph,settings,patch) &&
                                edgeDistance(sample,edge,graph,patch)<=radius)
                                coveredSamples|=std::uint64_t(1)<<
                                    (sampleY*settings.supersamples+sampleX);
                        }
                }
                const auto coverage=double(std::popcount(coveredSamples))/
                    double(settings.supersamples*settings.supersamples);
                const auto alpha=std::uint8_t(std::lround(coverage*255));
                occupied+=alpha>0;
                const auto offset=pixel*4;
                signedDistance[pixel]=float(nearest);
                result.coverageRgba[offset]=alpha;
                result.coverageRgba[offset+1]=alpha;
                result.coverageRgba[offset+2]=alpha;
                result.coverageRgba[offset+3]=alpha;
                result.normalRgba[offset+3]=alpha;
                const double spread=.25;
                const auto encoded=std::uint8_t(std::lround(
                    std::clamp(.5+nearest/(2*spread),0.0,1.0)*255));
                result.distanceRgba[offset]=encoded;
                result.distanceRgba[offset+1]=encoded;
                result.distanceRgba[offset+2]=encoded;
                result.distanceRgba[offset+3]=alpha;
            }
        }
        for (unsigned y=0;y<size;++y)
            for (unsigned x=0;x<size;++x)
            {
                const auto pixel=std::size_t(y)*result.width+patchIndex*size+x;
                const auto offset=pixel*4;
                if (!result.coverageRgba[offset+3])
                {result.normalRgba[offset]=128;result.normalRgba[offset+1]=128;
                 result.normalRgba[offset+2]=255;continue;}
                const auto left=std::size_t(y)*result.width+patchIndex*size+
                    (x?x-1:x);
                const auto right=std::size_t(y)*result.width+patchIndex*size+
                    std::min(x+1,size-1);
                const auto below=std::size_t(y?y-1:y)*result.width+patchIndex*size+x;
                const auto above=std::size_t(std::min(y+1,size-1))*result.width+
                    patchIndex*size+x;
                const auto distance=[&](std::size_t index)
                {return std::isfinite(signedDistance[index])?
                    signedDistance[index]:signedDistance[pixel];};
                const auto gx=(distance(right)-distance(left))/
                    ((right==left?1.0:double((x<size-1?1:0)+(x>0?1:0)))*stepU);
                const auto gy=(distance(above)-distance(below))/
                    ((above==below?1.0:double((y<size-1?1:0)+(y>0?1:0)))*stepV);
                const auto normal=unit(Vec3d{-settings.normalStrength*gx,
                    -settings.normalStrength*gy,1});
                result.normalRgba[offset]=encodeNormal(normal.x);
                result.normalRgba[offset+1]=encodeNormal(normal.y);
                result.normalRgba[offset+2]=encodeNormal(normal.z);
            }
    }
    result.occupiedFraction=double(occupied)/double(pixels);
    result.minimumRepresentedWidthPixels=minimumPixels;
    return result;
}
}
Result build(const Settings &settings,bool geometryDebug,
             const std::atomic_bool *cancelled)
{
    const auto started=std::chrono::steady_clock::now();
    if (cancelled && cancelled->load())
        throw std::runtime_error("Web bake cancelled");
    if (settings.resolution<64 || settings.resolution>1024 ||
        (settings.resolution&(settings.resolution-1)) ||
        !(settings.supersamples==1 || settings.supersamples==2 || settings.supersamples==4 ||
          settings.supersamples==8) || settings.maximumCards<1 ||
        settings.maximumCards>4 || settings.physicalEdgeBudget>50 ||
        !std::isfinite(settings.geometryDistance) ||
        !std::isfinite(settings.hybridDistance) ||
        !std::isfinite(settings.shaderOnlyDistance) ||
        !std::isfinite(settings.cullDistance) ||
        settings.geometryDistance<0 ||
        settings.hybridDistance<settings.geometryDistance ||
        settings.shaderOnlyDistance<settings.hybridDistance ||
        settings.cullDistance<settings.shaderOnlyDistance ||
        !std::isfinite(settings.maximumPlaneErrorMeters) ||
        !std::isfinite(settings.maximumRmsErrorMeters) ||
        settings.maximumPlaneErrorMeters<=0 || settings.maximumRmsErrorMeters<=0 ||
        !std::isfinite(settings.minimumRadiusPixels) ||
        settings.minimumRadiusPixels<0 ||
        !std::isfinite(settings.normalStrength) || settings.normalStrength<0 ||
        settings.normalStrength>10)
        throw std::runtime_error("Web bake resolution, supersampling or budget is invalid");
    Result result;
    result.graph=makeGraph(settings);
    for (auto &edge:result.graph.edges)
        if (settings.brokenEdges.contains(edge.id) && edge.state==EdgeState::Intact)
            edge.state=EdgeState::Broken;
    const auto activeTotal=std::count_if(result.graph.edges.begin(),
        result.graph.edges.end(),[&](const Edge &edge){return active(edge,settings);});
    constexpr std::size_t maximumDebugStrands=256;
    const auto debugCount=std::min<std::size_t>(maximumDebugStrands,activeTotal);
    result.sourceTruncated=geometryDebug && activeTotal>maximumDebugStrands;
    std::size_t activeOrdinal{};
    for (const auto &edge:result.graph.edges)
    {
        if (cancelled && cancelled->load())
            throw std::runtime_error("Web bake cancelled");
        if (!active(edge,settings)) continue;
        ++result.activeEdgeCount;
        result.minimumRadiusMeters=result.minimumRadiusMeters==0?edge.radiusMeters:
            std::min(result.minimumRadiusMeters,edge.radiusMeters);
        if (!geometryDebug) continue;
        const auto selected=((activeOrdinal+1)*debugCount)/activeTotal>
                            (activeOrdinal*debugCount)/activeTotal;
        ++activeOrdinal;
        if (!selected) continue;
        ++result.sourceRenderedEdges;
        PA::ProceduralSpline spline;
        spline.splineGuid=Solid::region(2401,edgeToken(edge.id));
        spline.name=edge.id;
        spline.interpolation=PA::SplineInterpolation::Polyline;
        const std::vector<Vec3d> points=edge.sagged?
            std::vector<Vec3d>{result.graph.nodes[edge.start],edge.sagPoint,
                result.graph.nodes[edge.end]}:
            std::vector<Vec3d>{result.graph.nodes[edge.start],
                result.graph.nodes[edge.end]};
        for (unsigned node=0;node<points.size();++node)
        {
            PA::SplineControlPoint point;
            point.pointGuid=Solid::region(2403,std::uint64_t(edgeToken(edge.id))+node);
            point.position=points[node];
            spline.points.push_back(point);
        }
        Construction::ConstructionSettings tube;
        tube.radiusMeters=edge.radiusMeters;
        tube.profileChordErrorMeters=edge.radiusMeters*.08;
        tube.maximumVertices=8192;
        const auto swept=Construction::buildConstruction(spline,tube);
        const auto guid=Solid::region(2402,edgeToken(edge.id));
        appendSweep(result.source,result.sourceUV,swept,guid);
    }
    if (settings.physicalEdgeBudget)
    {
        std::vector<const Edge *> candidates;
        for (const auto &edge:result.graph.edges)
            if (active(edge,settings)) candidates.push_back(&edge);
        const auto count=std::min<std::size_t>(settings.physicalEdgeBudget,candidates.size());
        for (std::size_t i=0;i<count;++i)
            result.physicalEdgeIds.push_back(
                candidates[i*candidates.size()/count]->id);
    }
    result.source.closedSolid=false;
    const auto all=fitPlane(result.graph,std::numeric_limits<unsigned>::max(),
                            settings.cardPaddingMeters);
    const bool oneCard=all.maximumPlaneErrorMeters<=settings.maximumPlaneErrorMeters &&
        all.rmsPlaneErrorMeters<=settings.maximumRmsErrorMeters;
    if (oneCard)
    {
        result.patches.push_back(all);
        result.patches.back().index=std::numeric_limits<unsigned>::max();
    }
    else
    {
        unsigned maximumPatch=0;
        for (const auto &edge:result.graph.edges)
            maximumPatch=std::max(maximumPatch,edge.patch);
        if (maximumPatch+1>settings.maximumCards)
            result.fallbackReason="Web needs more locally planar cards than the configured budget";
        else for (unsigned patch=0;patch<=maximumPatch;++patch)
        {
            auto candidate=fitPlane(result.graph,patch,settings.cardPaddingMeters);
            if (candidate.maximumPlaneErrorMeters>settings.maximumPlaneErrorMeters ||
                candidate.rmsPlaneErrorMeters>settings.maximumRmsErrorMeters)
                result.fallbackReason="Local web patch exceeds plane-fit error";
            result.patches.push_back(candidate);
        }
    }
    if (geometryDebug || !result.fallbackReason.empty())
    for (const auto &edge:result.graph.edges)
    {
        if (!active(edge,settings)) continue;
        const auto &plane=oneCard?all:fitPlane(result.graph,edge.patch,0);
        const auto appendRibbon=[&](Vec3d start,Vec3d end)
        {
            const auto across=unit(cross(plane.normal,unit(sub(end,start))));
            const auto half=edge.radiusMeters;
            appendQuad(result.ribbons,result.ribbonUV,
                {sub(start,mul(across,half)),sub(end,mul(across,half)),
                 add(end,mul(across,half)),add(start,mul(across,half))},
                {{{0,0},{1,0},{1,1},{0,1}}},plane.normal,
                Solid::region(2402,edgeToken(edge.id)));
            const auto marker=edge.id.find("/Spoke_");
            if (marker!=std::string::npos && settings.structuralStrandCount)
            {
                const auto spoke=unsigned(std::stoul(edge.id.substr(edge.id.rfind('_')+1)));
                const auto count=std::min(settings.structuralStrandCount,settings.spokes);
                bool selected=false;
                for (unsigned strand=0;strand<count;++strand)
                    selected|=spoke==strand*settings.spokes/count;
                if (selected)
                    appendQuad(result.structural,result.structuralUV,
                        {sub(start,mul(across,half)),sub(end,mul(across,half)),
                         add(end,mul(across,half)),add(start,mul(across,half))},
                        {{{0,0},{1,0},{1,1},{0,1}}},plane.normal,
                        Solid::region(2406,edgeToken(edge.id)));
            }
        };
        const auto start=result.graph.nodes[edge.start],end=result.graph.nodes[edge.end];
        if (edge.sagged)
        {appendRibbon(start,edge.sagPoint);appendRibbon(edge.sagPoint,end);}
        else appendRibbon(start,end);
    }
    result.ribbons.closedSolid=false;
    result.structural.closedSolid=false;
    if (geometryDebug)
    for (const auto &edge:result.graph.edges)
    {
        if (std::find(result.physicalEdgeIds.begin(),result.physicalEdgeIds.end(),
                      edge.id)==result.physicalEdgeIds.end()) continue;
        const auto plane=fitPlane(result.graph,edge.patch,0);
        const auto append=[&](Vec3d a,Vec3d b)
        {
            const auto across=unit(cross(plane.normal,unit(sub(b,a))));
            const auto half=std::max(edge.radiusMeters*2,.012);
            appendQuad(result.coarsePhysical,result.coarsePhysicalUV,
                {sub(a,mul(across,half)),sub(b,mul(across,half)),
                 add(b,mul(across,half)),add(a,mul(across,half))},
                {{{0,0},{1,0},{1,1},{0,1}}},plane.normal,
                Solid::region(2405,edgeToken(edge.id)));
        };
        const auto start=result.graph.nodes[edge.start],end=result.graph.nodes[edge.end];
        if (edge.sagged) {append(start,edge.sagPoint);append(edge.sagPoint,end);}
        else append(start,end);
    }
    result.coarsePhysical.closedSolid=false;
    if (result.activeEdgeCount==0)
        result.fallbackReason="Web has no visible source edges";
    result.bakeResolution=settings.resolution;
    constexpr std::uint64_t maximumEdgePixelChecks=40'000'000;
    constexpr std::uint64_t maximumPotentialSampleChecks=1'000'000'000;
    const auto patches=std::max(std::size_t(1),result.patches.size());
    const auto baseChecks=[&]
    {return std::uint64_t(result.bakeResolution)*result.bakeResolution*
            patches*result.graph.edges.size();};
    while (baseChecks()>maximumEdgePixelChecks &&
           result.bakeResolution>64)
        result.bakeResolution/=2;
    result.bakeSupersamples=settings.supersamples;
    while (baseChecks()*result.bakeSupersamples*result.bakeSupersamples>
           maximumPotentialSampleChecks && result.bakeSupersamples>1)
        result.bakeSupersamples/=2;
    result.bakeBudgetReduced=result.bakeResolution!=settings.resolution ||
        result.bakeSupersamples!=settings.supersamples;
    if (baseChecks()>maximumEdgePixelChecks && result.fallbackReason.empty())
        result.fallbackReason="Web exceeds CPU raster budget at minimum resolution";
    if (result.fallbackReason.empty())
    {
        double minimumPixels=std::numeric_limits<double>::infinity();
        for (const auto &patch:result.patches)
            for (const auto &edge:result.graph.edges)
                if (active(edge,settings) &&
                    (patch.index==std::numeric_limits<unsigned>::max() ||
                     edge.patch==patch.index))
                    minimumPixels=std::min(minimumPixels,
                        2*edge.radiusMeters*result.bakeResolution/
                        std::max(patch.maxU-patch.minU,patch.maxV-patch.minV));
        if (minimumPixels<settings.minimumRadiusPixels &&
            settings.style==Style::Heavy)
            result.fallbackReason="Strand width is below card pixel policy";
    }
    if (result.fallbackReason.empty())
    {
        for (std::size_t i=0;i<result.patches.size();++i)
        {
            const auto &patch=result.patches[i];
            const auto position=[&](double u,double v)
            {return add(patch.center,add(mul(patch.axisU,u),mul(patch.axisV,v)));};
            const float left=float(i)/float(result.patches.size());
            const float right=float(i+1)/float(result.patches.size());
            appendQuad(result.card,result.cardUV,
                {position(patch.minU,patch.minV),position(patch.maxU,patch.minV),
                 position(patch.maxU,patch.maxV),position(patch.minU,patch.maxV)},
                {{{left,0},{right,0},{right,1},{left,1}}},patch.normal,
                Solid::region(2404,i+1));
        }
        result.card.closedSolid=false;
        auto rasterSettings=settings;
        rasterSettings.supersamples=result.bakeSupersamples;
        for (unsigned size=result.bakeResolution;size>=1;size/=2)
        {
            auto mip=rasterLevel(result.graph,rasterSettings,result.patches,size,cancelled);
            result.textureBytes+=mip.coverageRgba.size()+mip.normalRgba.size()+
                mip.distanceRgba.size();
            result.mips.push_back(std::move(mip));
        }
        result.cardAccepted=true;
    }
    result.bakeCpuMilliseconds=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-started).count();
    return result;
}
double segmentPhi(std::array<double,2> sample,std::array<double,2> start,
                  std::array<double,2> end,double radius)
{
    if (!std::isfinite(radius) || radius<=0)
        throw std::runtime_error("Invalid web edge radius");
    for (double value:{sample[0],sample[1],start[0],start[1],end[0],end[1]})
        if (!std::isfinite(value)) throw std::runtime_error("Non-finite web field input");
    return distanceToSegment({sample[0],sample[1]},
        {start[0],start[1]},{end[0],end[1]})-radius;
}
std::uint64_t textureHash(const TextureLevel &level)
{
    std::uint64_t hash=14695981039346656037ull;
    auto bytes=[&](const auto &values)
    {
        const auto *data=reinterpret_cast<const unsigned char *>(values.data());
        for (std::size_t i=0;i<values.size()*sizeof(values[0]);++i)
        {hash^=data[i];hash*=1099511628211ull;}
    };
    bytes(level.coverageRgba);bytes(level.normalRgba);bytes(level.distanceRgba);
    return hash;
}
}
