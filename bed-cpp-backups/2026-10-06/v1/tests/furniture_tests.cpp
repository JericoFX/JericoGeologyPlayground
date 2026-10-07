#include "jerico/scene.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <set>
#include <stdexcept>
#include <string>
using namespace jerico;
namespace fs=std::filesystem;
static void require(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
static size_t checked_numbers=0;static double max_numeric_delta=0;
static void compare(const Json& actual,const Json& expected,const std::string&path=""){
 if(path=="/metadata/generator")return;
 if(actual.is_number()&&expected.is_number()){
  double a=actual.get<double>(),e=expected.get<double>(),delta=std::abs(a-e);++checked_numbers;max_numeric_delta=std::max(max_numeric_delta,delta);
  require(std::isfinite(a)&&std::isfinite(e)&&delta<=2e-12,"numeric mismatch "+path+": "+actual.dump()+" versus "+expected.dump());return;
 }
 require(actual.type()==expected.type(),"type mismatch "+path);
 if(expected.is_object()){
  require(actual.size()==expected.size(),"object shape mismatch "+path);
  for(auto it=expected.begin();it!=expected.end();++it){require(actual.contains(it.key()),"missing field "+path+"/"+it.key());compare(actual.at(it.key()),it.value(),path+"/"+it.key());}
 }else if(expected.is_array()){
  require(actual.size()==expected.size(),"array size mismatch "+path+" "+std::to_string(actual.size())+" versus "+std::to_string(expected.size()));
  for(size_t i=0;i<expected.size();++i)compare(actual[i],expected[i],path+"/"+std::to_string(i));
 }else require(actual==expected,"value mismatch "+path+": "+actual.dump()+" versus "+expected.dump());
}
static Json load(const fs::path&path){std::ifstream in(path);require(bool(in),"fixture unavailable: "+path.string());Json j;in>>j;return j;}
int main(int argc,char**argv){
 try{
  fs::path dir=argc>1?argv[1]:"fixtures/furniture";size_t count=0;
  for(const auto&entry:load(dir/"manifest.json")){
   auto expected=load(dir/entry.at("file").get<std::string>());auto scene=build_furniture(expected.at("recipe"));
   require(scene.contains("parts"),"build_furniture must return authored mesh parts");compare(scene,expected);
   require(validate_furniture(scene).empty(),"native validation failed "+entry["file"].get<std::string>()+": "+validate_furniture(scene).dump());
   const auto sweep=sweep_furniture_clearance(scene);require(sweep["passed"],"full-travel drawer clearance failed");
   if(entry.contains("clearance"))compare(sweep,entry["clearance"],"/clearance");
   if(!scene["metadata"]["drawers"].empty())require(std::abs(sweep["runner_gap_m"].get<double>()-.003)<1e-10,"runner bearing gap must be 3 mm");
   for(const auto&part:scene["parts"])if(part.contains("bounds"))for(const auto&v:part["vertices"])for(size_t k=0;k<3;++k)require(v[k].get<double>()>=part["bounds"][0][k].get<double>()-1e-8&&v[k].get<double>()<=part["bounds"][1][k].get<double>()+1e-8,"board exceeds requested bounds: "+part["id"].get<std::string>());
   std::cout<<"PASS fixture "<<entry["file"].get<std::string>()<<" parts="<<scene["parts"].size()<<'\n';++count;
  }
  auto a=build_furniture({{"seed",5}}),b=build_furniture({{"seed",5}}),c=build_furniture({{"seed",6}});
  require(a==b,"same-seed scene differs");require(a["nodes"]==c["nodes"]&&a["sockets"]==c["sockets"],"wear seed changed mechanism placement");require(a["parts"]!=c["parts"],"seed must vary surface geometry");
  auto closed=build_furniture(Json::object()),open=build_furniture({{"open_fraction",.45}});require(closed["parts"]==open["parts"],"opening deforms authored geometry");
  auto single=build_furniture({{"drawer_count",1},{"open_fraction",1}});
  const auto single_drawer=single["metadata"]["drawers"][0];
  require(std::abs(single["nodes"][1]["translation"][1].get<double>()-(single_drawer["closed_origin"][1].get<double>()-single_drawer["travel_m"].get<double>()))<1e-12,"single drawer cannot open through full travel");
  for(const auto&dr:closed["metadata"]["drawers"]){
   std::set<std::string> roles;
   for(const auto&p:closed["parts"])if(p["node"]==dr["node"])roles.insert(p["role"]);
   for(const std::string role:{"DrawerFront","DrawerLeft","DrawerRight","DrawerBack","DrawerBottom","CupHandle"})require(roles.count(role),"drawer shell missing "+role);
  }
  for(const auto&grip:closed["grips"]){
   const auto support=closed["metadata"]["grip_supports"][grip["id"].get<std::string>()];
   bool contact=false;
   for(const auto&p:closed["parts"])if(p["id"]==support)for(const auto&v:p["vertices"])if(v==grip["position"])contact=true;
   require(contact,"cup grasp position must be on actual authored surface");
  }
  auto optional=build_furniture({{"preset","workbench"},{"drawer",false},{"shelf",false}});require(optional["metadata"]["drawers"].empty(),"drawer=false still adds drawer");for(const auto&p:optional["parts"])require(p["role"]!="ShelfPlank","shelf=false still adds shelf planks");
  std::vector<Json> invalid={Json::array(),{{"width",std::numeric_limits<double>::quiet_NaN()}},{{"width",true}},{{"drawer_count",0}},{{"drawer_count",true}},{{"depth",.1}},{{"open_fraction",2}},{{"preset","bed"}},{{"surprise",1}},{{"height",.5},{"drawer_count",8}},{{"drawer",1}},{{"shelf",nullptr}},{{"seed",1.5}}};
  for(const auto&recipe:invalid){bool rejected=false;try{build_furniture(recipe);}catch(const std::exception&){rejected=true;}require(rejected,"invalid recipe accepted: "+recipe.dump());}
  auto collision=build_furniture(Json::object());const auto dr=collision["metadata"]["drawers"][0];auto o=dr["closed_origin"].get<V3>();double width=dr["outer_hi"][0].get<double>()-dr["outer_lo"][0].get<double>();
  collision["metadata"]["static_envelopes"].push_back({{"id","thin_blocker"},{"lo",{o[0]-width/4,o[1]-.08,o[2]+.02}},{"hi",{o[0]+width/4,o[1]-.0799,o[2]+.03}}});require(!sweep_furniture_clearance(collision)["passed"].get<bool>(),"continuous sweep missed thin obstruction");
  collision=closed;collision["metadata"]["static_envelopes"].push_back({{"id","runner.injected"},{"lo",{o[0]-.30,o[1]+.02,o[2]+.03}},{"hi",{o[0]+.30,o[1]+.3,o[2]+.05}}});require(!sweep_furniture_clearance(collision)["passed"].get<bool>(),"sweep ignored colliding runner");
  std::cout<<"PASS furniture: "<<count<<" complete scene fixtures; "<<checked_numbers<<" numeric values; max delta="<<max_numeric_delta<<"; determinism, rigid poses, bounds, optional components, ranges and continuous clearance regressions\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
