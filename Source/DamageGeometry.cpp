#include "DamageGeometry.hpp"

#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <manifold/manifold.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <numeric>
#include <numbers>
#include <stdexcept>

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
                              std::map<std::uint32_t,Jerico::Guid> &identities)
{
    manifold::MeshGL64 result;
    result.numProp=3;
    result.vertProperties.reserve(mesh.positions.size()*3);
    for (const auto p:mesh.positions)
        result.vertProperties.insert(result.vertProperties.end(),{p.x,p.y,p.z});
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
    TriangleMesh cutter;
    cutter.positions.reserve(2*sides+2);
    for (double z:{-depth,exteriorMargin})
        for (std::uint32_t i=0;i<sides;++i)
        {
            const auto angle=2*std::numbers::pi*i/sides;
            cutter.positions.push_back(localToWorld(projector,
                footprint.radius.x*std::cos(angle),
                footprint.radius.y*std::sin(angle),z));
        }
    const auto bottomCenter=std::uint32_t(cutter.positions.size());
    cutter.positions.push_back(localToWorld(projector,0,0,-depth));
    const auto topCenter=std::uint32_t(cutter.positions.size());
    cutter.positions.push_back(localToWorld(projector,0,0,exteriorMargin));
    for (std::uint32_t i=0;i<sides;++i)
    {
        const auto next=(i+1)%sides;
        cutter.indices.insert(cutter.indices.end(),{
            i,next,sides+next,i,sides+next,sides+i,
            bottomCenter,next,i,topCenter,sides+i,sides+next});
        cutter.triangleRegions.insert(cutter.triangleRegions.end(),
                                      {freshInterior,freshInterior,freshInterior,freshInterior});
    }
    return cutter;
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
    if (maximumTriangles<256 || maximumTriangles>240000 ||
        host.indices.size()/3>maximumTriangles ||
        host.indices.size()/3+256>maximumTriangles ||
        host.positions.size()>maximumTriangles)
        throw std::runtime_error("Damage cavity mesh budget exceeded");
    if (!PA::validateClosedMesh(host).hasValue())
        throw std::runtime_error("Damage cavity requires a closed host");
    if (!area.enabled || area.amount==0 || area.footprint.depth==0)
        return host;
    projectorCoordinates(area.projector,area.projector.center);
    sampleDamageDecal({},area.footprint);
    if (!std::isfinite(area.amount) || area.amount<0 || area.amount>1)
        throw std::runtime_error("Damage cavity amount outside [0,1]");
    const auto cutter=cutterMesh(area,freshInteriorRegion);
    if (separateBounds(host,cutter)) return host;
    checkCancel();
    std::map<std::uint32_t,Jerico::Guid> identities;
    const manifold::Manifold nativeHost(nativeMesh(host,identities));
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
}
