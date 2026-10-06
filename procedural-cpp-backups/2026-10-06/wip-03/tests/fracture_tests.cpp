#include "jerico/scene.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>
using jerico::Json;
namespace {
int assertions=0;
void require(bool ok,const std::string&message) { ++assertions; if(!ok) throw std::runtime_error(message); }
void compare(const Json&expected,const Json&actual,const std::string&path="$") {
 if(expected.is_object()) {
  require(actual.is_object(),path+": object required");
  for(auto it=expected.begin();it!=expected.end();++it) {
   if(path=="$.metadata" && it.key()=="generator") continue;
   require(actual.contains(it.key()),path+": missing "+it.key());
   compare(it.value(),actual.at(it.key()),path+"."+it.key());
  }
  require(expected.size()==actual.size(),path+": key count differs");
 } else if(expected.is_array()) {
  require(actual.is_array()&&actual.size()==expected.size(),path+": array mismatch");
  for(size_t i=0;i<expected.size();++i)compare(expected[i],actual[i],path+"["+std::to_string(i)+"]");
 } else if(expected.is_number()) {
  require(actual.is_number(),path+": number required");
  require(std::isfinite(actual.get<double>())&&std::abs(expected.get<double>()-actual.get<double>())<=2e-12,path+": numeric mismatch");
  if(expected.is_number_integer()&&actual.is_number_integer()) require(expected==actual,path+": integer mismatch");
 } else require(expected==actual,path+": literal mismatch");
}
double cross(const jerico::V2&a,const jerico::V2&b,const jerico::V2&c){return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);}
bool contains(const Json&p,const jerico::V2&q){
 bool inside=false;
 for(size_t i=0;i<p.size();++i){auto a=p[i].get<jerico::V2>(),b=p[(i+1)%p.size()].get<jerico::V2>();
  if(std::abs(cross(a,b,q))<=1e-10*std::hypot(a[0]-b[0],a[1]-b[1])&&q[0]>=std::min(a[0],b[0])-1e-10&&q[0]<=std::max(a[0],b[0])+1e-10&&q[1]>=std::min(a[1],b[1])-1e-10&&q[1]<=std::max(a[1],b[1])+1e-10)return true;
  if((a[1]>q[1])!=(b[1]>q[1])&&q[0]<a[0]+(q[1]-a[1])*(b[0]-a[0])/(b[1]-a[1]))inside=!inside;
 }return inside;
}
bool in_glass(const Json&s,const jerico::V2&q){for(const auto&p:s["parts"])if(p["role"]=="GlassPane"&&p.contains("footprint_xz")&&contains(p["footprint_xz"],q))return true;return false;}
Json non_glass(const Json&s){Json r=Json::array();for(const auto&p:s["parts"])if(p["role"]!="GlassPane")r.push_back(p);return r;}
void integrity(const Json&s) {
 const auto&m=s.at("metadata").at("glass_fracture");
 require(std::abs(m["source_partition_area_m2"].get<double>()-m["pane_area_m2"].get<double>())<1e-10,"partition coverage");
 require(std::abs(m["retained_area_m2"].get<double>()+m["removed_area_m2"].get<double>()-m["pane_area_m2"].get<double>())<1e-10,"area balance");
 require(!in_glass(s,m["impact_xz"].get<jerico::V2>()),"impact must be open hole");
 for(const auto&q:m["gap_samples_xz"])require(!in_glass(s,q.get<jerico::V2>()),"gap sample contains glass");
 for(const auto&q:m["solid_samples_xz"])require(in_glass(s,q.get<jerico::V2>()),"solid sample misses glass");
 for(const auto&p:s["parts"])if(p["role"]=="GlassPane"){
  require(p["support_overlap_area_m2"].get<double>()>0,"fragment lacks rebate support");
  require(p["perimeter_bearing_length_m"].get<double>()>.001,"fragment bearing too short");
  require(p["bevel"].get<double>()==0,"glass edges must be sharp");
  require(p["faces"].size()==p["face_materials"].size(),"material cardinality");
  std::map<std::pair<size_t,size_t>,int> edges;double volume=0;
  for(const auto&face:p["faces"]){
   for(size_t i=0;i<face.size();++i)++edges[{face[i].get<size_t>(),face[(i+1)%face.size()].get<size_t>()}];
   const auto a=p["vertices"][face[0].get<size_t>()].get<jerico::V3>();
   for(size_t i=1;i+1<face.size();++i){const auto b=p["vertices"][face[i].get<size_t>()].get<jerico::V3>(),c=p["vertices"][face[i+1].get<size_t>()].get<jerico::V3>();
    volume+=jerico::dot(a,jerico::cross(b,c))/6;
   }
  }
  for(const auto&e:edges)require(e.second==1&&edges[{e.first.second,e.first.first}]==1,"fragment must be closed and coherently wound");
  require(std::abs(volume-p["footprint_area_m2"].get<double>()*.004)<1e-11,"fragment volume");
 }
}
}
int main(int argc,char**argv) {
 try {
  const Json base={{"preset","industrial_window"},{"glazing","broken"}};
  const auto scene=jerico::build_fracture(base);
  require(scene["metadata"].contains("glass_fracture"),"native fracture metadata is missing");
  integrity(scene);
  compare(scene,jerico::build_fracture(base));
  for(const char*state:{"intact","empty","broken"}){Json r=base;r["glazing"]=state;auto actual=jerico::build_fracture(r),industrial=jerico::build_industrial(r);
   require(non_glass(actual)==non_glass(industrial),"fracture altered frame");require(actual["sockets"]==industrial["sockets"],"fracture altered sockets");
   require(actual["recipe"]["width"]==1.16,"industrial default width");
  }
  const Json invalid=Json::array({{{"energy",true}},{{"energy",-.1}},{{"energy",1.01}},{{"energy",".66"}},{{"ray_count",true}},{{"ray_count",12.0}},{{"ray_count",11}},{{"ray_count",41}},{{"impact",{.31,.5}}},{{"impact",{0,.24}}},{{"impact",{0,.76}}},{{"impact",{false,.5}}},{{"impact",{0,.5,0}}}});
  for(const auto&update:invalid){auto r=base;r.update(update);bool rejected=false;try{jerico::build_fracture(r);}catch(const std::exception&){rejected=true;}require(rejected,"invalid fracture controls accepted: "+update.dump());}
  for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){auto r=base;r["energy"]=value;bool rejected=false;try{jerico::build_fracture(r);}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite energy accepted");}
  Json r=base;r.update({{"width",.65},{"height",.8},{"energy",.1},{"ray_count",12},{"impact",{.3,.25}},{"seed",29}});
  auto fallback=jerico::build_fracture(r);integrity(fallback);require(fallback["metadata"]["glass_fracture"].contains("safety_fallback"),"known extreme configuration must activate overlap fallback");
  if(argc>1){std::ifstream input(argv[1]);require(bool(input),"reference fixture file unreadable");Json records;input>>records;for(const auto&record:records){
   if(record.contains("error")){bool rejected=false;try{jerico::build_fracture(record["recipe"]);}catch(const std::exception&e){rejected=true;require(e.what()==record["error"].get<std::string>(),"native rejection differs from source: "+record["name"].get<std::string>());}require(rejected,"source-rejected recipe accepted: "+record["name"].get<std::string>());}
   else compare(record["scene"],jerico::build_fracture(record["recipe"]),"$");
  }}
  std::cout<<"fracture_tests: "<<assertions<<" assertions passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"fracture_tests: FAIL: "<<e.what()<<"\n";return 1;}
}
