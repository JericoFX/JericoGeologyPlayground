#include "jerico/scene.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
using namespace jerico;
namespace fs=std::filesystem;
namespace {
size_t checked_numbers=0;double max_numeric_delta=0;
void require(bool value,const std::string&why){if(!value)throw std::runtime_error(why);}
void compare(const Json&actual,const Json&expected,const std::string&path=""){
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
Json load(const fs::path&path){std::ifstream in(path);require(bool(in),"fixture unavailable: "+path.string());Json j;in>>j;return j;}
Json without_glass(Json scene){auto&parts=scene["parts"];Json filtered=Json::array();for(const auto&p:parts)if(p["role"]!="GlassPane")filtered.push_back(p);return filtered;}
}
int main(int argc,char**argv){
 try{
  fs::path dir=argc>1?argv[1]:"fixtures/wood";size_t count=0;
  for(const auto&entry:load(dir/"manifest.json")){
   auto expected=load(dir/entry.at("file").get<std::string>());auto scene=entry["family"]=="wood"?build_wood(entry["recipe"]):build_singlepane(entry["recipe"]);
   require(scene.contains("parts"),"native wood must return authored mesh parts");compare(scene,expected);
   auto errors=validate_scene(scene);require(errors.empty(),"native validation failed "+entry["file"].get<std::string>()+": "+errors.dump());
   if(entry["family"]=="wood")require(scene["metadata"]["clearance"]["passed"],"moving wood clearance failed");
   else{require(scene["nodes"].size()==1&&scene["grips"].empty(),"fixed glass must not introduce leaf motion or grabs");require(!scene["metadata"]["clearance"]["applicable"].get<bool>(),"fixed frame reported fictitious motion validation");}
   std::cout<<"PASS fixture "<<entry["file"].get<std::string>()<<" parts="<<scene["parts"].size()<<'\n';++count;
  }
  Json recipe={{"preset","reinforced_timber"},{"seed",17}};auto a=build_wood(recipe),b=build_wood(recipe);require(a==b,"same-seed timber differs");
  recipe["angle"]=64;auto rotated=build_wood(recipe);require(a["parts"]==rotated["parts"],"rigid rotation changed timber local geometry");
  for(const std::string kind:{"door","window"}){
   auto broken=build_wood({{"kind",kind},{"damage","broken"}});size_t disabled=0;
   for(const auto&grip:broken["grips"])if(!grip["enabled"].get<bool>())++disabled;
   require(disabled>0,"broken wood retained unsupported grip");
  }
  auto intact=build_singlepane(Json::object()),empty=build_singlepane({{"glazing","empty"}}),broken=build_singlepane({{"glazing","broken"}});
  require(without_glass(intact)==without_glass(empty)&&without_glass(intact)==without_glass(broken),"glazing changes the fixed timber frame");
  require(!empty["sockets"].back()["enabled"].get<bool>(),"empty glazing socket must be disabled");require(broken["metadata"]["glass_hole"]["area_m2"].get<double>()>0,"broken pane must have real through-hole");
  require(broken==build_singlepane({{"glazing","broken"}}),"broken singlepane is nondeterministic");
  for(const Json&bad:std::vector<Json>{Json::array(),{{"preset","bad"}},{{"kind","door"}},{{"angle",1}},{{"glazing","cracked"}},{{"glazing",true}},{{"width",false}},{{"seed",true}}}){
   bool rejected=false;try{build_singlepane(bad);}catch(const std::exception&){rejected=true;}require(rejected,"invalid singlepane recipe accepted: "+bad.dump());
  }
  std::cout<<"PASS wood: "<<count<<" full scenes; "<<checked_numbers<<" numeric values; max delta="<<max_numeric_delta<<"; source topology, IDs, UV, materials, sockets/grips, clearances, determinism, rigid poses, supported broken-state interaction\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
