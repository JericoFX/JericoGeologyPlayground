#include "jerico/fine_wear.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace jerico;
static void require(bool value,const char*message){if(!value)throw std::runtime_error(message);}
static Json evaluate(const Json&r){
 const auto mode=r.at("mode").get<std::string>();
 if(mode=="contour")return contour_segments(r.at("config"));
 if(mode=="build")return build_fine_wear(r.at("recipe"));
 if(mode=="apply")return apply_fine_wear_operations(r.at("scene"),r.at("operations"));
 throw std::runtime_error("unknown fine wear test mode");
}
int main(int argc,char**argv){try{
 if(argc==4&&std::string(argv[1])=="--evaluate"){
  Json requests,results=Json::array();std::ifstream input(argv[2]);input>>requests;
  for(const auto&r:requests){try{results.push_back({{"value",evaluate(r)}});}catch(const std::exception&e){results.push_back({{"error",e.what()}});}}
  std::ofstream output(argv[3]);output<<results.dump();return 0;
 }
 Json config={{"width_m",.025},{"region",{{"shape","box"},{"min",{-.01,-.1,.02}},{"max",{.03,.1,.08}}}},{"boundary_segments_xz",{{{0,0},{0,.05}},{{0,.05},{0,.1}},{{0,.1},{.1,.1}},{{.1,.1},{.1,0}},{{.1,0},{0,0}}}}};
 auto snapshot=config;auto reduced=contour_segments(config);require(reduced.size()==3,"native fine contour merge/culling missing");require(config==snapshot,"fine contour config mutated");
 std::cout<<"fine_wear_tests: exact contour reduction and immutable config passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"fine_wear_tests: FAIL: "<<e.what()<<'\n';return 1;}}
