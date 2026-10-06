#include "jerico/dock_deform.hpp"
#include "jerico/dock_geometry.hpp"
#include "jerico/evaluate.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace jerico;
namespace {
void require(bool value,const char*why){if(!value)throw std::runtime_error(why);}
template<class F>void rejects(F f,const char*why){bool rejected=false;try{f();}catch(const std::invalid_argument&){rejected=true;}require(rejected,why);}
Json board(V3 a={0,0,1.5},V3 b={2.4,0,1.5},V3 width={0,1,0}){
 Builder builder(Json{{"seed",17}});builder.node("frame",{0,0,0},"Dock",nullptr);
 return dock_timber(builder,"test.board",a,b,width,.18,.075,"DeckBoard",17,false,true,.8);
}
Json scene(const Json&part){Builder b(Json{{"seed",17}});b.node("frame",{0,0,0},"Dock",nullptr);b.scene["parts"].push_back(part);return b.scene;}
}
int main(){try{
 const Json original=board(),snapshot=original;
 const Json operation={{"amount_m",.002},{"seed",81},{"edge_band_m",.024},
  {"start_protection_m",.22},{"end_protection_m",.24},{"protected_intervals",{{.9,1.2}}}};
 const Json result=deform_timber_edges(original,operation);
 require(result["vertices"]!=original["vertices"],"edge operation must actually change geometry");
 require(original==snapshot,"operation mutated its input");
 require(result==deform_timber_edges(original,operation),"same seed/ID is not deterministic");
 auto different=operation;different["seed"]=82;
 require(result["vertices"]!=deform_timber_edges(original,different)["vertices"],"different seed does not vary damage");
 auto renamed=original;renamed["id"]="test.another_board";
 require(result["vertices"]!=deform_timber_edges(renamed,operation)["vertices"],"stable part ID does not vary damage");
 require(result["faces"]==original["faces"]&&result["uvs"]==original["uvs"],"operation altered connectivity or UVs");
 size_t protected_count=0,changed_count=0,center_count=0;
 for(size_t i=0;i<original["vertices"].size();++i){
  auto a=original["vertices"][i].get<V3>(),b=result["vertices"][i].get<V3>();
  if(a[0]<=.22||a[0]>=2.4-.24||(.9<=a[0]&&a[0]<=1.2)){
   require(original["vertices"][i]==result["vertices"][i],"protected vertex changed even by rounding");++protected_count;
  }
  if(std::abs(a[1])<=.09-.024){require(original["vertices"][i]==result["vertices"][i],"interior outside edge band changed");++center_count;}
  require(a[0]==b[0]&&a[2]==b[2],"longitudinal or height coordinate changed");
  require(std::abs(b[1]-a[1])<=.002+1e-12,"amplitude bound exceeded");
  if(a!=b)++changed_count;
 }
 require(protected_count>20&&center_count>10&&changed_count>20,"test did not exercise protected, interior and changed regions");
 for(const auto&face:original["faces"]){
  const size_t a=face[0],b=face[1],c=face[2];
  auto normal=[&](const Json&p){return cross(sub(p["vertices"][b].get<V3>(),p["vertices"][a].get<V3>()),sub(p["vertices"][c].get<V3>(),p["vertices"][a].get<V3>()));};
  require(dot(normal(original),normal(result))>0,"triangle inverted");
 }
 require(validate_scene(scene(result)).empty(),"deformed mesh failed closure, volume or UV validation");
 require(validate_scene(evaluate_scene(scene(result))).empty(),"deformed mesh failed evaluated normal validation");
 auto portable=result;for(auto& vertex:portable["vertices"])for(auto& coordinate:vertex)coordinate=double(float(coordinate.get<double>()));
 require(validate_scene(scene(portable)).empty(),"float32 positions collapse or invalidate mesh");
 for(int seed=0;seed<20;++seed){auto at_limit=operation;at_limit["seed"]=seed;at_limit["amount_m"]=.024*.20;
  require(validate_scene(scene(deform_timber_edges(original,at_limit))).empty(),"bounded extreme amplitude failed a seed");}
 auto zero=operation;zero["amount_m"]=0;require(deform_timber_edges(original,zero)==original,"amount zero is not exact identity");
 auto covered=operation;covered["protected_intervals"]={{0,2.4}};require(deform_timber_edges(original,covered)==original,"fully protected mask is not identity");
 auto support=original;support["protected_support"]=true;require(deform_timber_edges(support,operation)==support,"whole-part support was deformed");
 const Json turned=board({4,-3,1.5},{4,-.6,1.5},{-1,0,0});const Json moved=deform_timber_edges(turned,operation);
 for(size_t i=0;i<result["vertices"].size();++i){auto a=result["vertices"][i].get<V3>(),b=moved["vertices"][i].get<V3>();require(std::abs(b[0]-(4-a[1]))<1e-12&&std::abs(b[1]-(-3+a[0]))<1e-12&&a[2]==b[2],"local field changed under rigid transform");}
 auto bad=operation;bad["amount_m"]=.03;rejects([&]{deform_timber_edges(original,bad);},"excessive amplitude accepted");
 bad=operation;bad["seed"]=std::numeric_limits<uint64_t>::max();rejects([&]{deform_timber_edges(original,bad);},"unsigned overflowing seed accepted");
 bad=operation;bad["protected_intervals"]={{1.2,.9}};rejects([&]{deform_timber_edges(original,bad);},"reversed protection interval accepted");
 bad=operation;bad["amount_m"]=std::numeric_limits<double>::quiet_NaN();rejects([&]{deform_timber_edges(original,bad);},"NaN amplitude accepted");
 auto invalid=original;invalid["timber_frame"]["u"]={2,0,0};rejects([&]{deform_timber_edges(invalid,operation);},"nonunit frame accepted");
 invalid=original;invalid["faces"].erase(invalid["faces"].begin());invalid["uvs"].erase(invalid["uvs"].begin());rejects([&]{deform_timber_edges(invalid,operation);},"open input mesh accepted");
 invalid=original;invalid["uvs"][0][0][0]=std::numeric_limits<double>::infinity();rejects([&]{deform_timber_edges(invalid,operation);},"invalid UV accepted");
 invalid=evaluate_scene(scene(original))["parts"][0];rejects([&]{deform_timber_edges(invalid,operation);},"evaluated normals accepted and left stale");
 std::cout<<"PASS dock edge deformation: geometry, deterministic local field, exact masks/UVs, manifold, orientation, bounded displacement, input guards\n";
 return 0;
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
