#include "DamageGeometry.hpp"

#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <manifold/manifold.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <numbers>
#include <set>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace Playground
{
namespace
{
namespace PA=Jerico::Tools::ProceduralAuthoring;
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d multiply(Vec3d a,double v) {return {a.x*v,a.y*v,a.z*v};}
Vec3d localToWorld(const DamageProjector &projector,double u,double v,double w)
{
    const auto cosine=std::cos(projector.rotationRadians);
    const auto sine=std::sin(projector.rotationRadians);
    const double x=cosine*u+sine*v;
    const double y=-sine*u+cosine*v;
    return add(projector.center,add(multiply(projector.tangent,x*projector.scale.x),
        add(multiply(projector.bitangent,y*projector.scale.y),
            multiply(projector.normal,w*projector.scale.z))));
}
manifold::MeshGL64 nativeMesh(const TriangleMesh &mesh,
                              std::map<std::uint32_t,Jerico::Guid> &identities,
                              bool mergeCoincident=false)
{
    manifold::MeshGL64 result;
    result.numProp=3;
    result.vertProperties.reserve(mesh.positions.size()*3);
    for (const auto p:mesh.positions)
        result.vertProperties.insert(result.vertProperties.end(),{p.x,p.y,p.z});
    if (mergeCoincident)
    {
        std::map<std::tuple<double,double,double>,std::uint32_t> firstAtPosition;
        for (std::uint32_t i=0;i<mesh.positions.size();++i)
        {
            const auto p=mesh.positions[i];
            const auto [it,inserted]=firstAtPosition.try_emplace(
                std::tuple{p.x,p.y,p.z},i);
            if (!inserted)
            {
                result.mergeFromVert.push_back(i);
                result.mergeToVert.push_back(it->second);
            }
        }
    }
    std::map<Jerico::Guid,std::vector<std::size_t>> groups;
    for (std::size_t face=0;face<mesh.triangleRegions.size();++face)
        groups[mesh.triangleRegions[face]].push_back(face);
    const auto firstId=manifold::Manifold::ReserveIDs(std::uint32_t(groups.size()));
    std::uint32_t ordinal=0;
    for (const auto &[id,faces]:groups)
    {
        result.runIndex.push_back(result.triVerts.size());
        result.runOriginalID.push_back(firstId+ordinal);
        identities[firstId+ordinal++]=id;
        for (auto face:faces)
            for (unsigned corner=0;corner<3;++corner)
                result.triVerts.push_back(mesh.indices[3*face+corner]);
    }
    result.runIndex.push_back(result.triVerts.size());
    return result;
}
TriangleMesh fromNative(const manifold::MeshGL64 &output,
                        const std::map<std::uint32_t,Jerico::Guid> &identities)
{
    std::vector<std::size_t> parents(output.NumVert());
    std::iota(parents.begin(),parents.end(),0);
    auto root=[&](std::size_t index)
    {
        while (parents[index]!=index)
        {parents[index]=parents[parents[index]];index=parents[index];}
        return index;
    };
    for (std::size_t i=0;i<output.mergeFromVert.size();++i)
        parents[root(output.mergeFromVert[i])]=root(output.mergeToVert[i]);
    TriangleMesh result;
    std::map<std::size_t,std::uint32_t> remap;
    for (const auto index:output.triVerts)
    {
        const auto merged=root(index);
        auto [it,inserted]=remap.try_emplace(merged,std::uint32_t(result.positions.size()));
        if (inserted)
            result.positions.push_back({output.vertProperties[merged*output.numProp],
                output.vertProperties[merged*output.numProp+1],
                output.vertProperties[merged*output.numProp+2]});
        result.indices.push_back(it->second);
    }
    result.triangleRegions.resize(result.indices.size()/3);
    for (std::size_t run=0;run<output.runOriginalID.size();++run)
        for (std::size_t face=output.runIndex[run]/3;face<output.runIndex[run+1]/3;++face)
            result.triangleRegions[face]=identities.at(output.runOriginalID[run]);
    return result;
}
bool separateBounds(const TriangleMesh &host,const TriangleMesh &cutter)
{
    Vec3d hostLow{1e30,1e30,1e30},hostHigh{-1e30,-1e30,-1e30};
    Vec3d cutLow=hostLow,cutHigh=hostHigh;
    auto include=[](Vec3d p,Vec3d &low,Vec3d &high)
    {
        low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
    };
    for (const auto p:host.positions) include(p,hostLow,hostHigh);
    for (const auto p:cutter.positions) include(p,cutLow,cutHigh);
    return cutHigh.x<hostLow.x || cutLow.x>hostHigh.x ||
           cutHigh.y<hostLow.y || cutLow.y>hostHigh.y ||
           cutHigh.z<hostLow.z || cutLow.z>hostHigh.z;
}
TriangleMesh cutterMesh(const DamageArea &area,Jerico::Guid freshInterior)
{
    constexpr std::uint32_t sides=64;
    const auto &projector=area.projector;
    const auto &footprint=area.footprint;
    const double depth=footprint.depth*area.amount;
    const double exteriorMargin=std::max(1e-4,
        std::min(footprint.radius.x*projector.scale.x,
                 footprint.radius.y*projector.scale.y)*.01)/projector.scale.z;
    const double rimDepth=std::min(depth*.4,
        std::min(footprint.radius.x,footprint.radius.y)*area.boundary.rimFraction);
    std::vector<double> levels{-depth};
    if (rimDepth>0) levels.push_back(-rimDepth);
    levels.push_back(exteriorMargin);
    TriangleMesh cutter;
    const bool shapedFloor=area.boundary.irregularity>0;
    cutter.positions.reserve((levels.size()+std::uint32_t(shapedFloor))*sides+2);
    for (std::size_t ring=0;ring<levels.size();++ring)
        for (std::uint32_t i=0;i<sides;++i)
        {
            const auto angle=2*std::numbers::pi*i/sides;
            const auto radial=damageBoundaryRadiusScale(angle,area.boundary)*
                (ring+1==levels.size()?1+area.boundary.rimFraction:1);
            cutter.positions.push_back(localToWorld(projector,
                radial*footprint.radius.x*std::cos(angle),
                radial*footprint.radius.y*std::sin(angle),levels[ring]));
        }
    const auto floorBase=std::uint32_t(cutter.positions.size());
    auto effectiveFootprint=footprint;
    effectiveFootprint.depth=depth;
    if (shapedFloor)
        for (std::uint32_t i=0;i<sides;++i)
        {
            const auto angle=2*std::numbers::pi*i/sides;
            const auto radial=.55*damageBoundaryRadiusScale(angle,area.boundary);
            const auto x=radial*footprint.radius.x*std::cos(angle);
            const auto y=radial*footprint.radius.y*std::sin(angle);
            cutter.positions.push_back(localToWorld(projector,
                x,y,-damageCavityFloorDepth({x,y,0},effectiveFootprint,area.boundary)));
        }
    const auto bottomCenter=std::uint32_t(cutter.positions.size());
    cutter.positions.push_back(localToWorld(projector,0,0,
        -damageCavityFloorDepth({},effectiveFootprint,area.boundary)));
    const auto topCenter=std::uint32_t(cutter.positions.size());
    cutter.positions.push_back(localToWorld(projector,0,0,levels.back()));
    for (std::uint32_t i=0;i<sides;++i)
    {
        const auto next=(i+1)%sides;
        for (std::uint32_t ring=0;ring+1<levels.size();++ring)
        {
            const auto base=ring*sides;
            cutter.indices.insert(cutter.indices.end(),{
                base+i,base+next,base+sides+next,
                base+i,base+sides+next,base+sides+i});
            cutter.triangleRegions.insert(cutter.triangleRegions.end(),
                                          {freshInterior,freshInterior});
        }
        const auto topBase=std::uint32_t(levels.size()-1)*sides;
        if (shapedFloor)
        {
            cutter.indices.insert(cutter.indices.end(),{
                floorBase+i,next,i,floorBase+i,floorBase+next,next,
                bottomCenter,floorBase+next,floorBase+i});
            cutter.triangleRegions.insert(cutter.triangleRegions.end(),
                                          {freshInterior,freshInterior,freshInterior});
        }
        else
        {
            cutter.indices.insert(cutter.indices.end(),{bottomCenter,next,i});
            cutter.triangleRegions.push_back(freshInterior);
        }
        cutter.indices.insert(cutter.indices.end(),{topCenter,topBase+i,topBase+next});
        cutter.triangleRegions.push_back(freshInterior);
    }
    return cutter;
}
TriangleMesh pathSegmentMesh(const DamagePathCutter &cutter,Vec3d first,Vec3d second,
                             Jerico::Guid freshInterior)
{
    const auto sides=std::uint32_t(damagePathSegmentOutline(cutter,first,second).size());
    const std::uint32_t rings=cutter.irregularity>0?damagePathDepthIntervals+1:2;
    const auto exteriorMargin=std::max(1e-4,cutter.radius*.01)/cutter.projector.scale.z;
    const auto depth=cutter.depth*cutter.amount;
    TriangleMesh mesh;
    mesh.positions.reserve(rings*sides+2);
    for (std::uint32_t ring=0;ring<rings;++ring)
    {
        const auto fraction=double(ring)/(rings-1);
        const auto outline=damagePathSegmentOutline(cutter,first,second,fraction);
        const auto z=std::lerp(-depth,exteriorMargin,fraction);
        for (const auto point:outline)
            mesh.positions.push_back(localToWorld(cutter.projector,point.x,point.y,z));
    }
    const auto bottomCenter=std::uint32_t(mesh.positions.size());
    if (cutter.irregularity==0)
    {
        mesh.positions.push_back(localToWorld(cutter.projector,
            (first.x+second.x)*.5,(first.y+second.y)*.5,-depth));
        mesh.positions.push_back(localToWorld(cutter.projector,
            (first.x+second.x)*.5,(first.y+second.y)*.5,exteriorMargin));
    }
    for (std::uint32_t i=0;i<sides;++i)
    {
        const auto next=(i+1)%sides;
        for (std::uint32_t ring=0;ring+1<rings;++ring)
        {
            const auto base=ring*sides;
            mesh.indices.insert(mesh.indices.end(),{
                base+i,base+next,base+sides+next,
                base+i,base+sides+next,base+sides+i});
            mesh.triangleRegions.insert(mesh.triangleRegions.end(),2,freshInterior);
        }
        if (cutter.irregularity==0)
        {
            mesh.indices.insert(mesh.indices.end(),{
                bottomCenter,next,i,bottomCenter+1,
                (rings-1)*sides+i,(rings-1)*sides+next});
            mesh.triangleRegions.insert(mesh.triangleRegions.end(),2,freshInterior);
        }
    }
    if (cutter.irregularity>0)
    {
        const auto cap=[&](std::uint32_t ring,bool bottom)
        {
            const auto outline=damagePathSegmentOutline(cutter,first,second,
                                                         bottom?0:1);
            std::vector<std::uint32_t> remaining(sides);
            std::iota(remaining.begin(),remaining.end(),0);
            const auto orient=[&](std::uint32_t a,std::uint32_t b,std::uint32_t c)
            {
                const auto &p=outline[a],&q=outline[b],&r=outline[c];
                return (q.x-p.x)*(r.y-p.y)-(q.y-p.y)*(r.x-p.x);
            };
            while (remaining.size()>2)
            {
                bool clipped=false;
                for (std::size_t i=0;i<remaining.size();++i)
                {
                    const auto a=remaining[(i+remaining.size()-1)%remaining.size()];
                    const auto b=remaining[i];
                    const auto c=remaining[(i+1)%remaining.size()];
                    if (orient(a,b,c)<=1e-12) continue;
                    bool contains=false;
                    for (const auto point:remaining)
                        if (point!=a && point!=b && point!=c &&
                            orient(a,b,point)>=-1e-12 &&
                            orient(b,c,point)>=-1e-12 &&
                            orient(c,a,point)>=-1e-12)
                            {contains=true;break;}
                    if (contains) continue;
                    const auto base=ring*sides;
                    if (bottom)
                        mesh.indices.insert(mesh.indices.end(),
                                            {base+c,base+b,base+a});
                    else
                        mesh.indices.insert(mesh.indices.end(),
                                            {base+a,base+b,base+c});
                    mesh.triangleRegions.push_back(freshInterior);
                    remaining.erase(remaining.begin()+std::ptrdiff_t(i));
                    clipped=true;
                    break;
                }
                if (!clipped)
                    throw std::runtime_error("Fracture cap cannot be triangulated");
            }
        };
        cap(0,true);
        cap(rings-1,false);
    }
    return mesh;
}
void separateFractureChunks(TriangleMesh &mesh,const DamagePathCutter &cutter)
{
    if (cutter.separationMeters==0) return;
    if (cutter.path.size()!=2)
        throw std::invalid_argument("Chunk separation requires one straight fracture path");
    std::vector<std::size_t> parents(mesh.positions.size());
    std::iota(parents.begin(),parents.end(),0);
    const auto root=[&](std::size_t index)
    {
        while (parents[index]!=index)
        {parents[index]=parents[parents[index]];index=parents[index];}
        return index;
    };
    for (std::size_t i=0;i<mesh.indices.size();i+=3)
    {
        const auto first=root(mesh.indices[i]);
        parents[root(mesh.indices[i+1])]=first;
        parents[root(mesh.indices[i+2])]=first;
    }
    std::set<std::size_t> chunks;
    for (std::size_t i=0;i<mesh.positions.size();++i) chunks.insert(root(i));
    if (chunks.size()<2)
        throw std::runtime_error("Chunk separation requires a through-fracture");
    const auto dx=cutter.path[1].x-cutter.path[0].x;
    const auto dy=cutter.path[1].y-cutter.path[0].y;
    const auto pathLength=std::hypot(dx,dy);
    const auto side=localToWorld(cutter.projector,-dy/pathLength,dx/pathLength,0);
    const auto sideVector=multiply(add(side,multiply(cutter.projector.center,-1)),
        1/std::hypot(std::hypot(side.x-cutter.projector.center.x,
                                side.y-cutter.projector.center.y),
                     side.z-cutter.projector.center.z));
    const auto localCenter=localToWorld(cutter.projector,
        (cutter.path[0].x+cutter.path[1].x)*.5,
        (cutter.path[0].y+cutter.path[1].y)*.5,0);
    std::map<std::size_t,std::pair<double,std::size_t>> centers;
    for (std::size_t i=0;i<mesh.positions.size();++i)
    {
        const auto p=mesh.positions[i];
        const auto offset=add(p,multiply(localCenter,-1));
        const auto sideDistance=offset.x*sideVector.x+offset.y*sideVector.y+
                                offset.z*sideVector.z;
        auto &entry=centers[root(i)];
        entry.first+=sideDistance;
        ++entry.second;
    }
    for (std::size_t i=0;i<mesh.positions.size();++i)
    {
        const auto &entry=centers.at(root(i));
        const auto direction=entry.first>=0?1.0:-1.0;
        mesh.positions[i]=add(mesh.positions[i],multiply(sideVector,
            direction*cutter.separationMeters*.5));
    }
}
}

TriangleMesh cutDamageCavity(const TriangleMesh &host,const DamageArea &area,
                             Jerico::Guid freshInteriorRegion,std::size_t maximumTriangles,
                             const Jerico::CancellationToken *cancel)
{
    auto checkCancel=[&]
    {
        if (cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
    };
    checkCancel();
    const auto cutterTriangles=256+
        (area.boundary.rimFraction>0 ? 128 : 0)+
        (area.boundary.irregularity>0 ? 128 : 0);
    if (maximumTriangles<cutterTriangles || maximumTriangles>240000 ||
        host.indices.size()/3>maximumTriangles ||
        host.indices.size()/3+cutterTriangles>maximumTriangles ||
        host.positions.size()>maximumTriangles)
        throw std::runtime_error("Damage cavity mesh budget exceeded");
    if (!PA::validateClosedMesh(host).hasValue())
        throw std::runtime_error("Damage cavity requires a closed host");
    if (!area.enabled || area.amount==0 || area.footprint.depth==0)
        return host;
    projectorCoordinates(area.projector,area.projector.center);
    sampleDamageDecal({},area.footprint);
    auto effectiveFootprint=area.footprint;
    effectiveFootprint.depth*=area.amount;
    sampleDamageCavityField({0,0,-effectiveFootprint.depth*.5},
                            effectiveFootprint,area.boundary);
    if (!std::isfinite(area.amount) || area.amount<0 || area.amount>1)
        throw std::runtime_error("Damage cavity amount outside [0,1]");
    const auto cutter=cutterMesh(area,freshInteriorRegion);
    if (separateBounds(host,cutter)) return host;
    checkCancel();
    std::map<std::uint32_t,Jerico::Guid> identities;
    const manifold::Manifold nativeHost(nativeMesh(host,identities,true));
    const manifold::Manifold nativeCutter(nativeMesh(cutter,identities));
    if (nativeHost.Status()!=manifold::Manifold::Error::NoError ||
        nativeCutter.Status()!=manifold::Manifold::Error::NoError)
        throw std::runtime_error("Damage cavity input is not manifold");
    const auto cut=nativeHost-nativeCutter;
    if (cut.Status()!=manifold::Manifold::Error::NoError || cut.IsEmpty())
        throw std::runtime_error("Damage cavity boolean failed");
    checkCancel();
    const auto output=cut.GetMeshGL64();
    if (output.NumTri()>maximumTriangles || output.NumVert()>maximumTriangles)
        throw std::runtime_error("Damage cavity output exceeds mesh budget");
    auto result=fromNative(output,identities);
    if (!PA::validateClosedMesh(result).hasValue())
        throw std::runtime_error("Damage cavity output is not closed");
    checkCancel();
    return result;
}
TriangleMesh cutDamagePath(const TriangleMesh &host,const DamagePathCutter &cutter,
                           Jerico::Guid freshInteriorRegion,std::size_t maximumTriangles,
                           const Jerico::CancellationToken *cancel)
{
    auto checkCancel=[&]
    {
        if (cancel && cancel->isCancellationRequested())
            throw std::runtime_error("Cancelled");
    };
    checkCancel();
    if (maximumTriangles>240000 || host.indices.size()/3>maximumTriangles ||
        host.positions.size()>maximumTriangles)
        throw std::runtime_error("Damage path mesh budget exceeded");
    if (!PA::validateClosedMesh(host).hasValue())
        throw std::runtime_error("Damage path requires a closed host");
    if (!cutter.enabled || cutter.amount==0) return host;
    if (cutter.path.size()<2 || cutter.path.size()>maximumDamagePathPoints ||
        host.indices.size()/3+(cutter.path.size()-1)*
            (cutter.irregularity>0?864:72)>maximumTriangles)
        throw std::runtime_error("Damage path mesh budget exceeded");
    sampleDamagePathField(cutter.projector.center,cutter);
    std::map<std::uint32_t,Jerico::Guid> identities;
    const manifold::Manifold nativeHost(nativeMesh(host,identities,true));
    if (nativeHost.Status()!=manifold::Manifold::Error::NoError)
        throw std::runtime_error("Damage path host is not manifold (status "+
            std::to_string(int(nativeHost.Status()))+")");
    manifold::Manifold combined;
    bool intersects=false;
    for (std::size_t i=1;i<cutter.path.size();++i)
    {
        checkCancel();
        const auto segment=pathSegmentMesh(cutter,cutter.path[i-1],cutter.path[i],
                                           freshInteriorRegion);
        if (separateBounds(host,segment)) continue;
        manifold::Manifold nativeSegment(nativeMesh(segment,identities));
        if (nativeSegment.Status()!=manifold::Manifold::Error::NoError)
            throw std::runtime_error("Damage path cutter is not manifold");
        combined=intersects ? combined+nativeSegment : nativeSegment;
        if (combined.Status()!=manifold::Manifold::Error::NoError)
            throw std::runtime_error("Damage path union failed");
        intersects=true;
    }
    if (!intersects) return host;
    checkCancel();
    const auto cut=nativeHost-combined;
    if (cut.Status()!=manifold::Manifold::Error::NoError || cut.IsEmpty())
        throw std::runtime_error("Damage path boolean failed");
    checkCancel();
    const auto output=cut.GetMeshGL64();
    if (output.NumTri()>maximumTriangles || output.NumVert()>maximumTriangles)
        throw std::runtime_error("Damage path output exceeds mesh budget");
    auto result=fromNative(output,identities);
    separateFractureChunks(result,cutter);
    if (!PA::validateClosedMesh(result).hasValue())
        throw std::runtime_error("Damage path output is not closed");
    checkCancel();
    return result;
}
std::vector<std::uint8_t> freshInteriorCornerMask(const MeshResult &mesh)
{
    if (!mesh.mesh.triangleRegions.empty() &&
        mesh.mesh.triangleRegions.size()!=mesh.mesh.indices.size()/3)
        throw std::invalid_argument("Fresh material requires a region per triangle");
    std::set<Jerico::Guid> fresh;
    for (const auto &region:mesh.regions)
        if (region.role==SurfaceRole::DamageInterior)
            fresh.insert(region.id);
    if (fresh.empty()) return {};
    if (mesh.mesh.triangleRegions.empty())
        throw std::invalid_argument("Fresh material requires triangle regions");
    std::vector<std::uint8_t> result(mesh.mesh.indices.size());
    for (std::size_t corner=0;corner<result.size();++corner)
        result[corner]=fresh.contains(mesh.mesh.triangleRegions[corner/3]) ? 1 : 0;
    return result;
}
}
