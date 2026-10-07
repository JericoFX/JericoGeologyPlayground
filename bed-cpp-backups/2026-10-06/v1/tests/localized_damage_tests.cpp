#include "jerico/localized_damage.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace jerico;
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static Json evaluate(const Json&r){
 const auto mode=r.at("mode").get<std::string>();
 if(mode=="apply")return apply_operations(r.at("scene"),r.at("operations"));
 if(mode=="selection")return selection_weight(r.at("part_id"),r.at("side"),r.at("point").get<V3>(),r.at("selection"));
 if(mode=="edge_distance")return edge_distance(r.at("part"),r.at("point").get<V3>(),r.value("side",std::string("")));
 if(mode=="edge_weight")return edge_wear_weight(r.at("part"),r.at("side"),r.at("point").get<V3>(),r.at("selection"));
 if(mode=="smootherstep")return smootherstep(r.at("value"));
 throw std::runtime_error("unknown localized test mode");
}
int main(int argc,char**argv){try{
 if(argc==4&&std::string(argv[1])=="--evaluate"){
  Json requests,results=Json::array();std::ifstream input(argv[2]);input>>requests;
  for(const auto&r:requests){try{results.push_back({{"value",evaluate(r)}});}catch(const std::exception&e){results.push_back({{"error",e.what()}});}}
  std::ofstream output(argv[3]);output<<results.dump();return 0;
 }
 Builder b(normalize_recipe(Json::object()));b.node("leaf",{0,0,0},"Leaf",nullptr);b.scene["metadata"]["leaf_envelopes"]=Json::array();b.scene["metadata"]["frame_envelopes"]=Json::array();b.box("panel","leaf",0,1,-.05,.05,0,2,"painted_steel");
 Json op={{"part_id","panel"},{"side","front"},{"type","cut"},{"region",{{"shape","box"},{"min",{.15,-.06,.7}},{"max",{.45,.06,1.3}}}}};
 auto original=b.scene;auto scene=apply_operations(original,Json::array({op}));
 require(scene["parts"][0].contains("cuts"),"localized native cut operation missing");
 require(original==b.scene,"input scene mutated");
 require(scene["metadata"]["damage_operations"][0]["status"]=="applied","applied report missing");
 require(!point_in_panel(scene["parts"][0],{.3,-.049,1}),"cut exterior occupancy incorrect");
 require(point_in_panel(scene["parts"][0],{.3,.049,1}),"opposite surface removed");
 for(double value:{std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::infinity()}){
  auto invalid=op;invalid["depth"]=value;bool rejected=false;try{apply_operations(original,Json::array({invalid}));}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite localized depth accepted");
  rejected=false;try{selection_weight("panel","front",{0,value,0},op);}catch(const std::exception&){rejected=true;}require(rejected,"nonfinite selector point accepted");
 }
 auto oversized_seed=op;oversized_seed["seed"]=std::numeric_limits<uint64_t>::max();bool seed_rejected=false;try{apply_operations(original,Json::array({oversized_seed}));}catch(const std::exception&e){seed_rejected=std::string(e.what())=="seed must be a signed 64-bit integer";}require(seed_rejected,"operation uint64 seed was not explicitly rejected");
 std::cout<<"localized_damage_tests: selection, immutable operations and shallow occupancy passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"localized_damage_tests: FAIL: "<<e.what()<<'\n';return 1;}}
