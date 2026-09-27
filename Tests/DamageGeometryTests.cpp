#include "DamageGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <set>
#include <stdexcept>

using namespace Playground;
namespace PA=Jerico::Tools::ProceduralAuthoring;

namespace
{
void require(bool value,const char *message)
{if (!value) throw std::runtime_error(message);}
template<class F> void reject(F action,const char *message)
{
    bool rejected=false;
    try {action();} catch (const std::exception &) {rejected=true;}
    require(rejected,message);
}
double volume(const TriangleMesh &mesh)
{
    double sum=0;
    for (std::size_t t=0;t<mesh.indices.size();t+=3)
    {
        const auto a=mesh.positions[mesh.indices[t]];
        const auto b=mesh.positions[mesh.indices[t+1]];
        const auto c=mesh.positions[mesh.indices[t+2]];
        sum+=a.x*(b.y*c.z-b.z*c.y)+a.y*(b.z*c.x-b.x*c.z)+a.z*(b.x*c.y-b.y*c.x);
    }
    return sum/6;
}
std::size_t axialCrossings(const TriangleMesh &mesh,double x,double y)
{
    std::vector<double> hits;
    for (std::size_t t=0;t<mesh.indices.size();t+=3)
    {
        const auto a=mesh.positions[mesh.indices[t]];
        const auto b=mesh.positions[mesh.indices[t+1]];
        const auto c=mesh.positions[mesh.indices[t+2]];
        const auto d=(b.y-c.y)*(a.x-c.x)+(c.x-b.x)*(a.y-c.y);
        if (std::abs(d)<1e-12) continue;
        const auto u=((b.y-c.y)*(x-c.x)+(c.x-b.x)*(y-c.y))/d;
        const auto v=((c.y-a.y)*(x-c.x)+(a.x-c.x)*(y-c.y))/d;
        if (u<0 || v<0 || u+v>1) continue;
        hits.push_back(u*a.z+v*b.z+(1-u-v)*c.z);
    }
    std::sort(hits.begin(),hits.end());
    hits.erase(std::unique(hits.begin(),hits.end(),
        [](double a,double b){return std::abs(a-b)<1e-6;}),hits.end());
    return hits.size();
}
void checkCut(const TriangleMesh &source,const DamageArea &area,Jerico::Guid interior)
{
    const auto original=PA::hashTriangleMesh(source);
    const auto result=cutDamageCavity(source,area,interior,50000);
    require(PA::validateClosedMesh(result).hasValue(),"cavity must remain closed");
    require(volume(result)>0 && volume(result)<volume(source),
            "cavity must remove bounded geometric volume");
    require(std::set<Jerico::Guid>(result.triangleRegions.begin(),result.triangleRegions.end()).contains(interior),
            "fresh interior region must be written during the cut");
    const std::set<Jerico::Guid> originalRegions(source.triangleRegions.begin(),
                                                 source.triangleRegions.end());
    for (const auto region:result.triangleRegions)
        require(region==interior || originalRegions.contains(region),
                "subtraction must not lose source region provenance");
    require(PA::hashTriangleMesh(source)==original,"cavity edit must not change source mesh");
    require(PA::hashTriangleMesh(result)==
            PA::hashTriangleMesh(cutDamageCavity(source,area,interior,50000)),
            "same operation must regenerate identical topology");
    auto moved=area;
    moved.projector.center.x+=10;
    require(PA::hashTriangleMesh(cutDamageCavity(source,moved,interior,50000))==original,
            "moving the projector away must restore the uncut source");
}
}
int run()
{
    auto rockRecipe=preset(0);
    auto rock=evaluate(rockRecipe,12).mesh;
    require(PA::validateClosedMesh(rock).hasValue(),"rock fixture must be closed");
    DamageArea area;
    area.id="shared-cavity";
    area.projector.center=*std::max_element(rock.positions.begin(),rock.positions.end(),
        [](Vec3d a,Vec3d b){return a.z<b.z;});
    area.footprint.radius={.45,.35,1};
    area.footprint.depth=.45;
    const auto interior=Solid::region(443,10);
    checkCut(rock,area,interior);
    auto naturalArea=area;
    naturalArea.boundary.irregularity=.13;
    naturalArea.boundary.rimFraction=.2;
    naturalArea.boundary.seed=443;
    checkCut(rock,naturalArea,interior);
    DamagePathCutter rockPath;
    rockPath.projector.center=area.projector.center;
    rockPath.path={{-.35,0,0},{.35,0,0}};
    rockPath.radius=.08;
    rockPath.depth=.45;
    const auto crackedRock=cutDamagePath(rock,rockPath,interior,50000);
    require(PA::validateClosedMesh(crackedRock).hasValue() &&
            volume(crackedRock)>0 && volume(crackedRock)<volume(rock),
            "shared path cutter must remove bounded geometry from rock");
    auto pathRecipe=preset(0);
    auto rockCrack=defaultLayer(Kind::DamagePath);
    rockCrack.damageOperationId=11;
    rockCrack.translation={float(area.projector.center.x),float(area.projector.center.y),
                           float(area.projector.center.z)};
    rockCrack.direction={0,0,1};
    rockCrack.amplitude=.45f;
    rockCrack.width=.16f;
    rockCrack.damagePathPoints={{{-.35f,0},{.35f,0}}};
    pathRecipe.version=25;
    pathRecipe.layers.push_back(rockCrack);
    const auto pathRock=evaluate(pathRecipe,12);
    require(PA::validateClosedMesh(pathRock.mesh).hasValue() &&
            PA::hashTriangleMesh(pathRock.mesh)!=PA::hashTriangleMesh(rock),
            "geology path layer must produce a geometric crack");
    require(std::any_of(pathRock.regions.begin(),pathRock.regions.end(),
        [](const RegionInfo &region){return region.role==SurfaceRole::DamageInterior;}),
        "geology crack must retain fresh interior lineage");
    require(meshHash(pathRock.mesh)==meshHash(evaluate(deserialize(serialize(pathRecipe)),12).mesh),
            "geology crack path must survive recipe save and load");
    auto zeroPath=pathRecipe;
    zeroPath.layers.back().damageAmount=0;
    require(meshHash(evaluate(zeroPath,12).mesh)==meshHash(rock),
            "zero crack amount must restore the uncut geology source");
    auto movedPath=pathRecipe;
    movedPath.layers.back().translation[0]+=10;
    require(meshHash(evaluate(movedPath,12).mesh)==meshHash(rock),
            "moving editable crack path off rock must restore source geometry");
    auto oldVersionPath=pathRecipe;
    oldVersionPath.version=24;
    reject([&]{validate(oldVersionPath);},"geology crack path must require recipe v25");
    require(PA::hashTriangleMesh(cutDamageCavity(rock,naturalArea,interior,50000))!=
            PA::hashTriangleMesh(cutDamageCavity(rock,area,interior,50000)),
            "opt-in shared boundary must change the rock cut geometry");
    auto rockCut=defaultLayer(Kind::DamageCavity);
    rockCut.damageOperationId=10;
    rockCut.seed=443;
    rockCut.translation={float(area.projector.center.x),float(area.projector.center.y),
                         float(area.projector.center.z)};
    rockCut.direction={0,0,1};
    rockCut.projectorRadius={.45f,.35f};
    rockCut.amplitude=.45f;
    rockRecipe.version=23;
    rockRecipe.layers.push_back(rockCut);
    const auto rockResult=evaluate(rockRecipe,12);
    auto zeroCavity=rockRecipe;
    zeroCavity.version=25;
    zeroCavity.layers.back().damageAmount=0;
    require(meshHash(evaluate(zeroCavity,12).mesh)==meshHash(rock),
            "owner cavity amount zero must restore source geometry");
    zeroCavity.version=24;
    reject([&]{validate(zeroCavity);},
           "non-default cavity amount must require recipe v25");
    require(PA::validateClosedMesh(rockResult.mesh).hasValue(),
            "editable geology cavity must remain closed");
    require(std::any_of(rockResult.regions.begin(),rockResult.regions.end(),
        [](const RegionInfo &r){return r.role==SurfaceRole::DamageInterior;}),
        "geology recipe must persist fresh interior lineage");
    const auto freshMask=freshInteriorCornerMask(rockResult);
    require(freshMask.size()==rockResult.mesh.indices.size(),
            "fresh material mask must have one value per rendered corner");
    const auto freshId=std::find_if(rockResult.regions.begin(),rockResult.regions.end(),
        [](const RegionInfo &r){return r.role==SurfaceRole::DamageInterior;})->id;
    std::size_t freshCorners=0;
    for (std::size_t corner=0;corner<freshMask.size();++corner)
    {
        const auto expected=rockResult.mesh.triangleRegions[corner/3]==freshId;
        require(freshMask[corner]==std::uint8_t(expected),
                "fresh mask must follow region identity without bleeding to source faces");
        freshCorners+=freshMask[corner];
    }
    require(freshCorners>0 && freshCorners<freshMask.size(),
            "fresh material must affect only newly exposed faces");
    MeshResult withoutRegions;
    withoutRegions.mesh.indices={0,1,2};
    require(freshInteriorCornerMask(withoutRegions).empty() &&
            freshInteriorCornerMask(evaluate(preset(0),12)).empty(),
            "meshes without fresh regions must skip mask allocation");
    auto malformed=rockResult;
    malformed.mesh.triangleRegions.pop_back();
    reject([&]{freshInteriorCornerMask(malformed);},
           "partial triangle region streams must be rejected");
    require(meshHash(rockResult.mesh)==meshHash(evaluate(deserialize(serialize(rockRecipe)),12).mesh),
            "geology cavity must survive save and load");
    auto legacyRecipe=rockRecipe;
    legacyRecipe.version=22;
    legacyRecipe.layers.back().damageBoundaryIrregularity=0;
    legacyRecipe.layers.back().damageRimFraction=0;
    require(meshHash(evaluate(legacyRecipe,12).mesh)==
            meshHash(evaluate(deserialize(serialize(legacyRecipe)),12).mesh),
            "v22 circular cavity must retain a readable legacy recipe");
    legacyRecipe.layers.back().frequency=100;
    require(meshHash(evaluate(legacyRecipe,12).mesh)==
            meshHash(evaluate(deserialize(serialize(legacyRecipe)),12).mesh),
            "v22 cavity must keep its historical valid frequency range");
    legacyRecipe.layers.back().damageRimFraction=.2f;
    reject([&]{validate(legacyRecipe);},"soft rim must require recipe v23");
    const auto oldEvidence=std::filesystem::path(PLAYGROUND_ROOT)/
        "Evidence/2026-09-27-shared-damage-cavity";
    require(PA::hashTriangleMesh(evaluate(load(oldEvidence/"rock-cavity-recipe.json"),44).mesh)==
                2386827915866525017ULL &&
            PA::hashTriangleMesh(evaluate(load(oldEvidence/"panel-through-hole-recipe.json"),44).mesh)==
                2986865365891870130ULL,
            "v22 published rock and panel cavity topology must stay unchanged");
    auto collisionRecipe=rockRecipe;
    collisionRecipe.layers.back().seed=0;
    collisionRecipe.layers.back().damageOperationId=0xDA6A68;
    const auto collisionResult=evaluate(collisionRecipe,12);
    std::set<Jerico::Guid> collisionRegionIds;
    for (const auto &region:collisionResult.regions)
        require(collisionRegionIds.insert(region.id).second,
                "fresh interior ID must not alias an existing source region");
    auto invalidRecipe=rockRecipe;
    invalidRecipe.version=21;
    reject([&]{validate(invalidRecipe);},"cavity must require recipe v22");
    invalidRecipe=rockRecipe;
    invalidRecipe.layers.push_back(rockCut);
    reject([&]{validate(invalidRecipe);},"duplicate operation IDs must be rejected");
    invalidRecipe=rockRecipe;
    invalidRecipe.layers.back().projectorRadius[0]=0;
    reject([&]{validate(invalidRecipe);},"zero projector radius must be rejected");
    invalidRecipe=rockRecipe;
    invalidRecipe.layers.back().amplitude=std::numeric_limits<float>::infinity();
    reject([&]{validate(invalidRecipe);},"non-finite depth must be rejected");
    Jerico::CancellationSource stopped;
    stopped.requestCancellation();
    auto stoppedToken=stopped.token();
    reject([&]{evaluate(rockRecipe,12,-1,-1,&stoppedToken);},
           "cancelled geology cavity must not publish a result");
    rockRecipe.layers.back().translation[0]+=10;
    require(meshHash(evaluate(rockRecipe,12).mesh)==meshHash(rock),
            "moving geology cavity off rock must restore source geometry");

    Recipe slabRecipe;
    slabRecipe.base=Base::Plane;
    slabRecipe.planeSize={2,2};
    auto solid=defaultLayer(Kind::Solidify);
    solid.thickness=.3;
    slabRecipe.layers={solid};
    auto panel=evaluate(slabRecipe,8).mesh;
    require(PA::validateClosedMesh(panel).hasValue(),"panel fixture must be closed");
    double front=-1e9;
    for (const auto p:panel.positions) front=std::max(front,p.z);
    DamagePathCutter crack;
    crack.path={{-.7,0,0},{.7,0,0}};
    crack.radius=.08;
    crack.depth=.12;
    crack.projector.center={0,0,front};
    const auto crackedPanel=cutDamagePath(panel,crack,interior,50000);
    require(PA::validateClosedMesh(crackedPanel).hasValue() &&
            volume(crackedPanel)>0 && volume(crackedPanel)<volume(panel),
            "shared path cutter must remove a bounded channel from a wooden panel fixture");
    require(std::set<Jerico::Guid>(crackedPanel.triangleRegions.begin(),
                                  crackedPanel.triangleRegions.end()).contains(interior),
            "path cutter must mark fresh interior faces");
    require(PA::hashTriangleMesh(crackedPanel)==
            PA::hashTriangleMesh(cutDamagePath(panel,crack,interior,50000)),
            "same path cutter must regenerate identical geometry");
    require(sampleDamagePathField({0,0,front-.04},crack)<0 &&
            sampleDamagePathField({0,.3,front-.04},crack)>0,
            "shared path field must identify the same local channel cut by geometry");
    auto disabledCrack=crack;
    disabledCrack.enabled=false;
    require(sampleDamagePathField({0,0,front-.04},disabledCrack)>0 &&
            PA::hashTriangleMesh(cutDamagePath(panel,disabledCrack,interior,50000))==
                PA::hashTriangleMesh(panel),
            "disabled path must agree between the shared field and geometry");
    auto bent=crack;
    bent.path={{-.7,-.2,0},{0,-.2,0},{.7,.2,0}};
    const auto bentPanel=cutDamagePath(panel,bent,interior,50000);
    require(PA::validateClosedMesh(bentPanel).hasValue() &&
            volume(bentPanel)>0 && volume(bentPanel)<volume(panel),
            "joined crack segments must cut a closed panel");
    crack.projector.center.x+=10;
    require(PA::hashTriangleMesh(cutDamagePath(panel,crack,interior,50000))==
            PA::hashTriangleMesh(panel),
            "moving a non-destructive path cutter off the panel restores the source");
    crack.projector.center.x-=10;
    crack.depth=.32;
    require(axialCrossings(cutDamagePath(panel,crack,interior,50000),0,.013)==0,
            "deep path cutter must penetrate the panel without a phantom cap");
    reject([&]{cutDamagePath(panel,crack,interior,4);},
           "path cutter budget must reject before boolean work");
    auto invalidCrack=crack;
    invalidCrack.path[1]=invalidCrack.path[0];
    reject([&]{cutDamagePath(panel,invalidCrack,interior,50000);},
           "collapsed crack segment must be rejected");
    invalidCrack=crack;
    invalidCrack.path[1].z=.1;
    reject([&]{cutDamagePath(panel,invalidCrack,interior,50000);},
           "crack path outside projector plane must be rejected");
    area.projector.center={0,0,front};
    area.footprint.depth=.12;
    checkCut(panel,area,interior);
    const auto shallow=cutDamageCavity(panel,area,interior,50000);
    require(axialCrossings(shallow,.013,.007)==2,
            "shallow cavity must keep solid material behind its floor");

    require(!isThroughHole(.12,.3,.01) && isThroughHole(.32,.3,.01),
            "through-hole tier must follow measured panel thickness");
    area.footprint.depth=.32;
    const auto opening=cutDamageCavity(panel,area,interior,50000);
    require(PA::validateClosedMesh(opening).hasValue() && volume(opening)<volume(panel),
            "full-depth cutter must create closed through-hole geometry");
    require(axialCrossings(opening,.013,.007)==0,
            "through-hole must actually penetrate panel center");
    reject([&]{cutDamageCavity(panel,area,interior,4);},
           "mesh budget must reject before boolean work");
    area.projector.scale.x=0;
    reject([&]{cutDamageCavity(panel,area,interior,50000);},
           "zero projector scale must be rejected before boolean work");
}
int main()
{
    try {return run();}
    catch (const std::exception &error)
    {
        std::fprintf(stderr,"DamageGeometryTests: %s\n",error.what());
        return 1;
    }
}
