#include "jerico/damage.hpp"
#include <cmath>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace jerico;
static void require(bool pass,const std::string&message){if(!pass)throw std::runtime_error(message);}
static Json execute(const Json&r){
 const auto mode=r.at("mode").get<std::string>();
 if(mode=="irregular")return irregular_polygon(r.at("center").get<V2>(),r.at("radii").get<V2>(),r.at("seed").get<int64_t>(),r.value("vertices",13),r.value("jaggedness",.25),r.value("positive_offset",uint64_t(0)));
 if(mode=="crack")return crack_profiles(r.at("center").get<V2>(),r.at("length"),r.at("width"),r.at("depth"),r.at("seed").get<int64_t>(),r.value("direction",V2{0,1}),r.value("branches",2),r.value("segments",6),r.value("positive_offset",uint64_t(0)));
 if(mode=="point")return point_in_panel(r.at("part"),r.at("point").get<V3>());
 if(mode=="sequence"){
  Json part=r.at("part"),parts=Json::array(),occupied=Json::array();
  for(const auto&cut:r.at("cuts")){part=carve_panel(part,cut.at("polygon"),cut.value("side",std::string("front")),cut.value("depth",Json(nullptr)));parts.push_back(part);}
  for(const auto&p:r.value("queries",Json::array()))occupied.push_back(point_in_panel(part,p.get<V3>()));
  return Json{{"parts",parts},{"occupancy",occupied}};
 }
 throw std::runtime_error("unknown test request mode");
}
int main(int argc,char**argv){try{
 if(argc==4&&std::string(argv[1])=="--evaluate"){
  Json requests,results=Json::array();std::ifstream input(argv[2]);input>>requests;
  for(const auto&r:requests){try{results.push_back({{"value",execute(r)}});}catch(const std::exception&e){results.push_back({{"error",e.what()}});}}
  std::ofstream output(argv[3]);output<<results.dump();return 0;
 }
 Builder b(normalize_recipe(Json::object()));
 const Json original=b.box("panel","leaf",0,1,-.05,.05,0,2,"painted_metal",.0015,"Panel"),snapshot=original;
 const Json polygon={{.3,.7},{.7,.7},{.7,1.3},{.3,1.3}};
 auto cut=carve_panel(original,polygon);
 require(cut.contains("cut_math"),"native panel subtraction metadata is missing");
 require(original==snapshot,"input panel mutated");
 require(std::abs(cut["cut_math"]["remaining_volume"].get<double>()-.176)<1e-12,"through subtraction volume mismatch");
 require(!point_in_panel(cut,{.431,0,.937}),"hole occupancy wrong");
 require(point_in_panel(cut,{.131,0,.937}),"solid occupancy wrong");
 require(irregular_polygon({.4,1},{.2,.4},19).size()==13,"irregular polygon missing");
 require(crack_profiles({.5,1},.48,.006,.025,41).size()==4,"crack profiles missing");
 for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
  bool rejected=false;try{point_in_panel(original,{0,value,0});}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite occupancy query accepted");
  rejected=false;try{irregular_polygon({0,value},{1,1},1);}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite irregular center accepted");
  rejected=false;try{crack_profiles({0,1},.48,.006,value,41);}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite crack depth accepted");
 }
 std::cout<<"damage_tests: native subtraction, occupancy and procedural cutter smoke checks passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"damage_tests: FAIL: "<<e.what()<<'\n';return 1;}}
