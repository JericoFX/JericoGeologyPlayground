#include <iostream>
#if __has_include("SplineDamageIO.hpp")
#include "SplineDamageIO.hpp"
#include "SolidGeometry.hpp"
#include "SplineDamageVolumeTests.hpp"
#include "SplineSurfaceFitTests.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <fstream>
#include <limits>
#include <set>
#include <stdexcept>
using namespace Playground;
using namespace Playground::Construction;
using Json=nlohmann::json;
namespace {
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
template<class F> void reject(F f,const char* message){try{f();}catch(const std::exception&){return;}throw std::runtime_error(message);}
bool equal(Vec3d a,Vec3d b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
double volume(const TriangleMesh&mesh){double result=0;for(std::size_t i=0;i<mesh.indices.size();i+=3){
 const auto a=mesh.positions[mesh.indices[i]],b=mesh.positions[mesh.indices[i+1]],c=mesh.positions[mesh.indices[i+2]];
 result+=(a.x*(b.y*c.z-b.z*c.y)+a.y*(b.z*c.x-b.x*c.z)+a.z*(b.x*c.y-b.y*c.x))/6;}return result;}
void same(const DamagePathCutter&a,const DamagePathCutter&b){
 require(a.enabled==b.enabled&&a.seed==b.seed&&a.radius==b.radius&&a.depth==b.depth&&a.amount==b.amount&&a.irregularity==b.irregularity&&a.separationMeters==b.separationMeters,"all damage scalars and 64-bit seed survive");
 require(equal(a.projector.center,b.projector.center)&&equal(a.projector.tangent,b.projector.tangent)&&equal(a.projector.bitangent,b.projector.bitangent)&&equal(a.projector.normal,b.projector.normal)&&equal(a.projector.scale,b.projector.scale)&&a.projector.rotationRadians==b.projector.rotationRadians,"entire projector survives");
 require(a.path.size()==b.path.size(),"all path points survive");for(size_t i=0;i<a.path.size();++i)require(equal(a.path[i],b.path[i]),"path coordinates survive");
}
PA::ProceduralSpline line(){
 PA::ProceduralSpline s;s.splineGuid=Solid::region(8301,1);s.name="Persistence fixture";s.interpolation=PA::SplineInterpolation::Polyline;
 for(unsigned i=0;i<2;++i){PA::SplineControlPoint p;p.pointGuid=Solid::region(8301,i+2);p.position={0,0,3.*i};s.points.push_back(p);}return s;
}
}
int main(int argc,char**argv){
 try{
  SplineSurfaceFitTests::run();
  const auto folder=argc>1?std::filesystem::path(argv[1]):std::filesystem::temp_directory_path()/"JericoSplineDamageIO";
  std::filesystem::create_directories(folder);
  const auto spline=line();ConstructionSettings original;original.radiusMeters=.08;original.breakSeed=991;original.metresPerTileU=.75;
  const auto intact=buildConstruction(spline,original);const auto frame=frameAtDistance(intact.path,1.5);
  PathDamageRule groove;groove.id="spline-damage-1";
  auto&c=groove.cutter;c.projector.center={frame.position.x+frame.normal.x*original.radiusMeters,frame.position.y+frame.normal.y*original.radiusMeters,frame.position.z+frame.normal.z*original.radiusMeters};
  c.projector.normal=frame.normal;c.projector.tangent=frame.tangent;c.projector.bitangent=frame.binormal;
  c.path={{-.3,0,0},{.3,0,0}};c.radius=.035;c.depth=.12;c.irregularity=.3;c.seed=777;c.amount=.85;
  original.damagePaths.push_back(groove);
  auto disabled=groove;disabled.id="spline-damage-2";disabled.cutter.enabled=false;disabled.cutter.seed=std::numeric_limits<std::uint64_t>::max();
  disabled.cutter.projector.center={2,3,4};disabled.cutter.projector.rotationRadians=.31;disabled.cutter.projector.scale={1.1,.9,1.2};disabled.cutter.path={{-.2,0,0},{0,.04,0},{.2,0,0}};
  original.damagePaths.push_back(disabled);
  const auto source=buildConstruction(spline,original);require(meshHash(source.mesh.mesh)!=meshHash(intact.mesh.mesh),"fixture really gouges geometry");
  auto spatial=original;
  auto path=line();path.splineGuid=Solid::region(8302,1);path.name="Spatial damage";
  path.interpolation=PA::SplineInterpolation::CatmullRom;path.points.clear();
  for(const auto p:std::vector<Vec3d>{{-.4,.01,-.007},{-.1,-.015,-.003},{.2,.015,-.015},{.4,0,-.003}}){
   PA::SplineControlPoint point;point.pointGuid=Solid::region(8302,path.points.size()+2);point.position=p;path.points.push_back(point);}
  spatial.damagePaths[0].spatialPath=path;
  const auto spatialBuilt=buildConstruction(spline,spatial);
  require(meshHash(spatialBuilt.mesh.mesh)!=meshHash(source.mesh.mesh),"canonical noncoplanar damage spline must change the real cut, not be ignored");
  auto decal=spatial;decal.damagePaths[0].operation=DamageVolumeOperation::Union;
  const auto raised=buildConstruction(spline,decal);
  require(volume(raised.mesh.mesh)>volume(intact.mesh.mesh),"spline decal operation must add geometry instead of cutting the host");
  require(std::any_of(raised.mesh.regions.begin(),raised.mesh.regions.end(),[](const auto&r){return r.role==SurfaceRole::DecalSurface;}),"added decal needs explicit provenance");
  require(freshInteriorCornerMask(raised.mesh).empty(),"added decal must not be labeled as exposed interior material");
  auto loop=decal.damagePaths[0];loop.spatialPath->closed=true;loop.cutter.radius=.025;loop.cutter.depth=.025;
  loop.cutter.irregularity=.75;
  const Vec3d loopPoints[]={{.3,0,0},{0,.3,0},{-.3,0,0},{0,-.3,0}};
  for(std::size_t i=0;i<4;++i)loop.spatialPath->points[i].position=loopPoints[i];
  const auto loopVolume=buildSplineDamageVolume(loop,Solid::region(8303,1));
  require(SplineDamageVolumeTests::connected(loopVolume),"closed spatial decal must remain one connected loop");
  std::set<std::uint32_t> loopVertices;std::set<std::pair<std::uint32_t,std::uint32_t>> loopEdges;
  for(std::size_t i=0;i<loopVolume.indices.size();i+=3)for(unsigned c=0;c<3;++c){
   const auto a=loopVolume.indices[i+c],b=loopVolume.indices[i+(c+1)%3];loopVertices.insert(a);loopEdges.insert(std::minmax(a,b));}
  require(std::int64_t(loopVertices.size())-std::int64_t(loopEdges.size())+std::int64_t(loopVolume.indices.size()/3)==0,
   "closed decal must form a torus without a cap or extra handle at the seam");
  const auto decalText=serializeSplineDamage(decal.damagePaths);
  require(Json::parse(decalText).at("version")==3,"volume union must use a version old readers reject instead of silently cutting");
  auto decalReplay=decal;decalReplay.damagePaths=deserializeSplineDamage(decalText);
  require(decalReplay.damagePaths[0].operation==DamageVolumeOperation::Union,"decal operation must survive persistence");
  const auto rebuiltDecal=buildConstruction(spline,decalReplay);
  require(meshHash(rebuiltDecal.mesh.mesh)==meshHash(raised.mesh.mesh)&&rebuiltDecal.cornerUV==raised.cornerUV,"raised decal geometry and UVs replay exactly");
  decalReplay.damagePaths[0].cutter.enabled=false;
  require(meshHash(buildConstruction(spline,decalReplay).mesh.mesh)==meshHash(intact.mesh.mesh),"disabling a raised decal restores the host");
  decalReplay=decal;decalReplay.damagePaths[0].cutter.amount=0;
  require(meshHash(buildConstruction(spline,decalReplay).mesh.mesh)==meshHash(intact.mesh.mesh),"zero raised decal amount restores the host");
  auto invalidDecal=Json::parse(decalText);invalidDecal["version"]=2;
  reject([&]{deserializeSplineDamage(invalidDecal.dump());},"operation fields in older schema must not be ignored");
  invalidDecal=Json::parse(decalText);invalidDecal["paths"][0]["operation"]="Unknown";
  reject([&]{deserializeSplineDamage(invalidDecal.dump());},"unknown decal composition must fail");
  const auto spatialText=serializeSplineDamage(spatial.damagePaths);
  const auto spatialReplay=deserializeSplineDamage(spatialText);
  require(spatialReplay[0].spatialPath==spatial.damagePaths[0].spatialPath,"canonical spline IDs, controls, handles, interpolation and roll must persist");
  auto restoredSpatial=spatial;restoredSpatial.damagePaths=spatialReplay;
  const auto replaySpatial=buildConstruction(spline,restoredSpatial);
  require(meshHash(replaySpatial.mesh.mesh)==meshHash(spatialBuilt.mesh.mesh)&&replaySpatial.cornerUV==spatialBuilt.cornerUV,"spatial damage geometry and UVs replay exactly");
  for(auto&rule:restoredSpatial.damagePaths)rule.cutter.enabled=false;
  require(meshHash(buildConstruction(spline,restoredSpatial).mesh.mesh)==meshHash(intact.mesh.mesh),"disabling spatial damage restores the exact host");
  restoredSpatial=spatial;restoredSpatial.damagePaths[0].cutter.amount=0;
  require(meshHash(buildConstruction(spline,restoredSpatial).mesh.mesh)==meshHash(intact.mesh.mesh),"zero spatial amount restores the exact host");
  auto badSpatial=Json::parse(spatialText);badSpatial["paths"][0]["spatialSpline"]["interpolation"]=.5;
  reject([&]{deserializeSplineDamage(badSpatial.dump());},"fractional spline mode must be rejected");
  const auto text=serializeSplineDamage(original.damagePaths);const auto parsed=deserializeSplineDamage(text);require(parsed.size()==2,"two operations restored");
  for(size_t i=0;i<parsed.size();++i){require(parsed[i].id==original.damagePaths[i].id,"operation IDs survive");same(parsed[i].cutter,original.damagePaths[i].cutter);}
  const auto file=folder/"damage.json";saveSplineDamage(original.damagePaths,file);
  auto restored=original;restored.damagePaths.clear();const auto replay=loadSplineDamage(spline,restored,file);
  require(meshHash(replay.mesh.mesh)==meshHash(source.mesh.mesh),"file load replays geometry exactly");
  require(replay.cornerUV==source.cornerUV&&replay.bakeCornerUV==source.bakeCornerUV,"file load replays surface and bake UVs");
  require(restored.breakSeed==991&&restored.radiusMeters==original.radiusMeters&&restored.metresPerTileU==.75,"damage load retains host settings");
  require(nextSplineDamageId(restored.damagePaths)=="spline-damage-3","new operation cannot collide with loaded IDs");
  auto seedChanged=original;seedChanged.damagePaths[0].cutter.seed=778;
  require(meshHash(buildConstruction(spline,seedChanged).mesh.mesh)!=meshHash(source.mesh.mesh),"contour seed changes real geometry");
  saveSplineDamage({},folder/"empty.json");const auto clear=loadSplineDamage(spline,restored,folder/"empty.json");
  require(restored.damagePaths.empty()&&meshHash(clear.mesh.mesh)==meshHash(intact.mesh.mesh),"empty operation file clears damage and restores intact geometry");
  auto invalid=Json::parse(text);invalid["paths"][0]["amount"]=2;std::ofstream(folder/"invalid.json")<<invalid;
  restored=original;const auto before=serializeSplineDamage(restored.damagePaths);
  reject([&]{loadSplineDamage(spline,restored,folder/"invalid.json");},"invalid file must be rejected");
  require(serializeSplineDamage(restored.damagePaths)==before&&meshHash(buildConstruction(spline,restored).mesh.mesh)==meshHash(source.mesh.mesh),"invalid file cannot mutate settings or geometry");
  auto open=original;open.capStart=false;const auto openBefore=serializeSplineDamage(open.damagePaths);
  reject([&]{loadSplineDamage(spline,open,file);},"topology invalid for host must reject before commit");
  require(serializeSplineDamage(open.damagePaths)==openBefore&&!open.capStart,"failed host build preserves state");
  reject([&]{deserializeSplineDamage("{}");},"wrong file shape rejected");
  for(const auto value:std::vector<Json>{-1,1.5,1e80}){auto bad=Json::parse(text);bad["paths"][0]["contourSeed"]=value;reject([&]{deserializeSplineDamage(bad.dump());},"lossy seed rejected");}
  auto bad=Json::parse(text);bad["version"]=5;reject([&]{deserializeSplineDamage(bad.dump());},"unknown schema rejected");
  bad=Json::parse(text);bad["paths"][1]["id"]=bad["paths"][0]["id"];reject([&]{deserializeSplineDamage(bad.dump());},"duplicate IDs rejected");
  auto nan=original.damagePaths;nan[0].cutter.depth=std::numeric_limits<double>::infinity();reject([&]{saveSplineDamage(nan,file);},"invalid save rejected");
  require(serializeSplineDamage(loadSplineDamage(file))==text,"failed save preserves previous file");
  reject([&]{deserializeSplineDamage(std::string(1024*1024+1,' '));},"oversized file rejected");
  std::ofstream(folder/"proof.json")<<Json{{"accepted",true},{"geometryHash",meshHash(source.mesh.mesh)},{"intactHash",meshHash(intact.mesh.mesh)},{"operations",2},{"uint64Seed",parsed[1].cutter.seed},{"emptyRestoresGeometry",true},{"invalidLoadPreservesState",true},{"hostFailurePreservesState",true},{"uvRoundtrip",true}}.dump(2);
  std::cout<<"PASS spline damage file/geometry/UV/parameter/seed roundtrip, empty state, collision-safe IDs and failed-load rollback\n";return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
#else
int main(){std::cerr<<"FAIL: Spline Construction has no shared damage persistence API\n";return 1;}
#endif
