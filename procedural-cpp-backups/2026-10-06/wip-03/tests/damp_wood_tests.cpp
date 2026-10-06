#include "jerico/scene.hpp"
#include "jerico/damage.hpp"
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
  fs::path dir=argc>1?argv[1]:"fixtures/damp";size_t count=0;
  for(const auto&entry:load(dir/"manifest.json")){
   auto expected=load(dir/entry.at("file").get<std::string>());auto scene=build_damp_wood(entry["recipe"]);compare(scene,expected);
   auto errors=validate_scene(scene);require(errors.empty(),"native validation failed "+entry["file"].get<std::string>()+": "+errors.dump());
   require(scene["metadata"]["wood_support_samples"].size()==12,"four frame members require twelve protected supporting core samples");
   for(const auto&sample:scene["metadata"]["wood_support_samples"]){bool supported=false;for(const auto&p:scene["parts"])if(p["id"]==sample["part_id"])supported=point_in_panel(p,sample["point"].get<V3>());require(supported,"damage destroyed protected structural core");}
   for(const auto&p:scene["parts"])if(p["role"]=="Frame"){
    require(p["face_materials"].size()==p["faces"].size(),"damage materials lost face alignment");require(p["wood_damage"]["removed_volume_m3"].get<double>()>0,"age damage removes no wood");
    bool exposed=false;for(const auto&m:p["face_materials"])if(m=="wood_exposed")exposed=true;require(exposed,"cut wood lacks exposed-material group");
   }
   std::cout<<"PASS fixture "<<entry["file"].get<std::string>()<<" parts="<<scene["parts"].size()<<'\n';++count;
  }
  auto base=build_damp_wood(Json::object());require(base==build_damp_wood(Json::object()),"same seed differs");
  auto wet=build_damp_wood({{"wetness",1}}),dry=build_damp_wood({{"wetness",0}});
  require(wet["parts"].size()==dry["parts"].size(),"wetness changes part count");
  for(size_t i=0;i<wet["parts"].size();++i)for(const std::string key:{"vertices","faces","uvs"})require(wet["parts"][i][key]==dry["parts"][i][key],"wetness alters geometry: "+key);
  for(const std::string family:{"old","new"}){
   Json common={{"family",family}};auto a=build_damp_wood(common);common["glazing"]="broken";auto b=build_damp_wood(common);common["glazing"]="empty";auto c=build_damp_wood(common);require(without_glass(a)==without_glass(b)&&without_glass(a)==without_glass(c),"glazing alters wet timber frame");
   require(c["grips"].empty(),"fixed timber introduces grasp runtime");require(!c["sockets"].back()["enabled"].get<bool>(),"empty glass socket active");
  }
  for(const auto&bad:std::vector<Json>{Json::array(),{{"family","ancient"}},{{"family",false}},{{"age",true}},{{"age",-.001}},{{"age",1.001}},{{"wetness",false}},{{"wetness",2}},{{"wetness",nullptr}},{{"width",.2}},{{"unknown",4}}}){bool rejected=false;try{build_damp_wood(bad);}catch(const std::exception&){rejected=true;}require(rejected,"bad damp recipe accepted: "+bad.dump());}
  std::cout<<"PASS damp wood: "<<count<<" full scenes; "<<checked_numbers<<" numeric values; max delta="<<max_numeric_delta<<"; exposed wood materials, approved glass reuse, support samples, age threshold, wetness independence, determinism and input ranges\n";
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 1;}
}
