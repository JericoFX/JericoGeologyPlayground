#include "jerico/bookshelf.hpp"
#include "jerico/evaluate.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
using namespace jerico;
namespace {
Json inspect(const Json& s) {
 Json issues=Json::array(); std::map<std::string,const Json*> items;
 for(const auto&i:s.at("metadata").at("items"))items[i.at("id").get<std::string>()]=&i;
 for(const auto&p:s.at("parts")) {
  std::string id=p.value("item_id",""); if(id.empty())continue;
  auto it=items.find(id);if(it==items.end()){issues.push_back({{"kind","missing_item"},{"part",p.at("id")}});continue;}
  const auto&o=it->second->at("obb");auto c=o.at("center").get<V3>(),h=o.at("half").get<V3>();auto a=o.at("axes").get<std::array<V3,3>>();
  double worst=0;V3 point{};int axis=-1;
  for(const auto&jv:p.at("vertices")){auto v=jv.get<V3>();for(int k=0;k<3;++k){double excess=std::abs(dot(sub(v,c),a[k]))-h[k];if(excess>worst){worst=excess;point=v;axis=k;}}}
  if(worst>.0010001)issues.push_back({{"kind","mesh_outside_obb"},{"item",id},{"part",p.at("id")},{"excess_m",worst},{"axis",axis},{"point",point}});
 }
 return issues;
}
Json by_id(const Json&s){Json o=Json::object();for(const auto&p:s.at("parts"))o[p.at("id").get<std::string>()]=p;return o;}
}
int main(int argc,char**argv){try{
 std::string out=argc>1?argv[1]:"verification";bool dump_scenes=argc>2&&std::string(argv[2])=="--dump-scenes";std::filesystem::create_directories(out);Json report={{"cases",Json::array()},{"invalid_inputs",Json::array()},{"seed_checks",Json::array()}};
 std::vector<std::pair<std::string,Json>> cases;
 for(auto n:{"reference_four","narrow_four","archive_five"}){std::ifstream f(std::string("recipes/")+n+".json");Json r;if(!f)throw std::runtime_error("Run test from project root");f>>r;cases.push_back({n,r});}
 for(auto kv:std::vector<std::pair<std::string,std::vector<double>>>{{"width",{.90,2.1}},{"height",{1.45,2.7}},{"depth",{.30,.55}},{"shelf_thickness",{.025,.055}},{"post_width",{.05,.095}},{"wear",{0,1}},{"fill",{0,.01,.25,.5,1}},{"book_scale",{.7,1.15}},{"compartments",{3,5,6}}})for(double v:kv.second)cases.push_back({kv.first+"_"+std::to_string(v),Json{{kv.first,v}}});
 cases.push_back({"compact_dense",Json{{"width",.9},{"height",1.45},{"depth",.30},{"post_width",.095},{"shelf_thickness",.055},{"fill",1},{"book_scale",1.15}}});
 cases.push_back({"six_scaled",Json{{"compartments",6},{"height",2.10},{"book_scale",1.15}}});
 cases.push_back({"large_dense",Json{{"width",2.1},{"height",2.7},{"depth",.55},{"compartments",6},{"fill",1},{"book_scale",1.15},{"wear",1}}});
 for(int seed:{0,1,-1,719,1302,99991})cases.push_back({"seed_"+std::to_string(seed),Json{{"seed",seed}}});
 size_t errors=0;
 for(const auto& c:cases){Json result={{"name",c.first},{"patch",c.second}};try{auto s=build_bookshelf(c.second);auto e=inspect(s);result["layout_errors"]=validate_bookshelf(s);result["geometry_obb_errors"]=e;result["items"]=s["metadata"]["items"].size();result["accepted"]=true;result["ok"]=e.empty()&&result["layout_errors"].empty();if(!result["ok"].get<bool>())++errors;if(c.first=="reference_four"||c.first=="narrow_four"||c.first=="archive_five"){auto eval=evaluate_scene(s);result["native_mesh_errors"]=validate_scene(eval);result["evaluated_obb_errors"]=inspect(eval);if(!result["native_mesh_errors"].empty()||!result["evaluated_obb_errors"].empty()){if(result["ok"].get<bool>())++errors;result["ok"]=false;}if(dump_scenes)std::ofstream(out+"/"+c.first+"_geometry.json")<<eval.dump();}}catch(const std::exception&e){result["accepted"]=false;result["error"]=e.what();result["ok"]=false;++errors;}report["cases"].push_back(result);std::cout<<c.first<<": "<<result["ok"]<<"\n";}
 for(auto kv:std::vector<std::pair<std::string,Json>>{{"width",.89},{"width",2.11},{"height",1.44},{"height",2.71},{"depth",.29},{"depth",.56},{"shelf_thickness",.024},{"shelf_thickness",.056},{"post_width",.049},{"post_width",.096},{"wear",-.01},{"wear",1.01},{"fill",-.01},{"fill",1.01},{"book_scale",.69},{"book_scale",1.16},{"compartments",2},{"compartments",7},{"compartments",3.5},{"width","bad"},{"width",nullptr},{"width",true},{"seed",1.5},{"seed","bad"},{"seed",nullptr}}){bool rejected=false;std::string what;try{build_bookshelf(Json{{kv.first,kv.second}});}catch(const std::exception&e){rejected=true;what=e.what();}report["invalid_inputs"].push_back({{"key",kv.first},{"value",kv.second},{"rejected",rejected},{"error",what}});if(!rejected)++errors;}
 auto a=build_bookshelf(Json::object()),again=build_bookshelf(Json::object()),empty=build_bookshelf(Json{{"fill",0}}),changed=build_bookshelf(Json{{"seed",418}});auto fullparts=by_id(a),emptyparts=by_id(empty),changedparts=by_id(changed);bool deterministic=a==again,independent=true,seed_effect=false;for(auto it=emptyparts.begin();it!=emptyparts.end();++it){if(!fullparts.contains(it.key())||fullparts[it.key()]!=it.value())independent=false;}for(auto it=fullparts.begin();it!=fullparts.end();++it){if(changedparts.contains(it.key())&&it.value()["vertices"]!=changedparts[it.key()]["vertices"])seed_effect=true;}report["seed_checks"]={{"identical_recipe_bitwise_json",deterministic},{"frame_unchanged_by_fill",independent},{"seed_changes_geometry",seed_effect}};if(!deterministic||!independent||!seed_effect)++errors;
 report["failed_cases"]=errors;report["status"]=errors?"FAIL":"PASS";std::ofstream(out+"/layout_review.json")<<report.dump(2);std::cout<<report["status"]<<": "<<errors<<" failures; report "<<out<<"/layout_review.json\n";return errors?1:0;
 }catch(const std::exception&e){std::cerr<<"FAIL "<<e.what()<<'\n';return 2;}}
