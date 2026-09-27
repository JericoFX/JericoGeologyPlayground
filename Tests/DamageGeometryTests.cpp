#include "DamageGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>

#include <algorithm>
#include <cmath>
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
int main()
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
    auto rockCut=defaultLayer(Kind::DamageCavity);
    rockCut.damageOperationId=10;
    rockCut.seed=443;
    rockCut.translation={float(area.projector.center.x),float(area.projector.center.y),
                         float(area.projector.center.z)};
    rockCut.direction={0,0,1};
    rockCut.projectorRadius={.45f,.35f};
    rockCut.amplitude=.45f;
    rockRecipe.version=22;
    rockRecipe.layers.push_back(rockCut);
    const auto rockResult=evaluate(rockRecipe,12);
    require(PA::validateClosedMesh(rockResult.mesh).hasValue(),
            "editable geology cavity must remain closed");
    require(std::any_of(rockResult.regions.begin(),rockResult.regions.end(),
        [](const RegionInfo &r){return r.role==SurfaceRole::DamageInterior;}),
        "geology recipe must persist fresh interior lineage");
    require(meshHash(rockResult.mesh)==meshHash(evaluate(deserialize(serialize(rockRecipe)),12).mesh),
            "geology cavity must survive save and load");
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
