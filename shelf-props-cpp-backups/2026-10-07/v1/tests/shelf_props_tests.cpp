#include "jerico/shelf_props.hpp"
#include "jerico/export.hpp"
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>
using namespace jerico;
void need(bool b,const std::string&s){if(!b)throw std::runtime_error(s);}
int main(int argc,char**argv){try{
 auto s=build_shelf_props(Json::object());
 need(s.contains("metadata")&&s["metadata"].contains("items")&&s["metadata"]["items"].size()==3,"missing mug, lantern and bottle generators");
 need(validate_scene(s).empty(),validate_scene(s).dump());
 for(auto&p:s["parts"])if(p["role"]=="GlobeGuard"){
  V3 low{},high{};for(int k=0;k<8;k++){low=add(low,p["vertices"][k].get<V3>());high=add(high,p["vertices"][32*8+k].get<V3>());}low=mul(low,1./8);high=mul(high,1./8);
  need(low[2]<=.052&&std::hypot(low[0],low[1])<=.031,"lower lantern guard attachment floats");
  need(high[2]>=.182&&std::hypot(high[0],high[1])<=.039,"upper lantern guard attachment floats");
 }

 need(s.dump()==build_shelf_props(Json::object()).dump(),"seed instability");
 auto other=build_shelf_props({{"seed",418}});need(s.dump()!=other.dump(),"seed has no effect");
 for(const auto&item:s["metadata"]["items"]){need(item["grab"]["group_part_ids"].size()>0,"grab group empty");need(item["obb"]["half"][2].get<double>()>0,"empty OBB");}
 for(auto key:{"mug_scale","lantern_scale","bottle_scale"})for(double x:{.70,1.30}){auto q=build_shelf_props({{key,x},{"wear",1.0},{"seed",-1}});need(validate_scene(q).empty(),"invalid range edge mesh: "+std::string(key)+validate_scene(q).dump());}
 for(auto q:{Json{{"wear",-1}},Json{{"mug_scale",0}},Json{{"seed",2.5}},Json{{"segments",8}},Json{{"unknown",1}}}){bool thrown=false;try{build_shelf_props(q);}catch(...){thrown=true;}need(thrown,"invalid recipe accepted");}
 auto e=evaluate_shelf_props(s);need(validate_scene(e).empty(),validate_scene(e).dump());
 for(auto&p:e["parts"]){for(auto&v:p["vertices"])for(auto x:v)need(std::isfinite(float(x.get<double>())),"float32 position invalid");for(auto&f:p["normals"])for(auto&n:f){double l=0;for(auto x:n){float t=x;need(std::isfinite(t),"float32 normal invalid");l+=t*t;}need(std::abs(l-1)<1e-5,"normal unit");}}
 if(argc>1){bake_shelf_props(e,argv[1],64);std::set<std::string> shared;for(auto&p:e["parts"]){shared.insert(p["material_shared_id"]);need(p["texture_uvs"].size()==p["faces"].size(),"UV face count");}need(shared.size()<=5,"per-part atlas regression");write_glb(e,std::string(argv[1])+"/props.glb");write_obj_mtl(e,std::string(argv[1])+"/props.obj");}
 std::cout<<"PASS geometry topology, recipe ranges, stability, float32 normals and shared export\n";
}catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}}
