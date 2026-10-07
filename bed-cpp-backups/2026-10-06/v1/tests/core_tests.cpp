#include "jerico/scene.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <stdexcept>
#include <limits>
using namespace jerico;
void require(bool x,const char* message){if(!x)throw std::runtime_error(message);}
int main(int argc,char**argv){
 if(argc>1){for(const char*name:{"random-reference.json","random-offset-reference.json"}){std::ifstream f(std::filesystem::path(argv[1])/name);Json cases;f>>cases;for(auto&c:cases){PythonRandom r(c["seed"].get<int64_t>(),c.value("offset",uint64_t(0)));for(auto&v:c["values"])require(r.random()==v.get<double>(),"Python RNG full stream mismatch");}}}

 PythonRandom rng(29); require(rng.random()==0.5481190538116991,"Python seed29 random mismatch");
 Builder b(normalize_recipe(Json::object()));b.node("frame",{0,0,0},"Frame",nullptr); b.box("box","frame",-1,1,-.1,.1,0,2);
 require(b.scene["parts"][0]["vertices"].size()==8,"box vertices");require(b.scene["parts"][0]["faces"].size()==8,"box faces");
 require(validate_scene(b.scene).empty(),"box invalid");
 size_t missed=0;
 for(const auto& field: {std::string("schema_version"),std::string("units"),std::string("transform")}){
  Json invalid=b.scene;
  if(field=="schema_version")invalid["schema_version"]=99;
  else if(field=="units")invalid["units"]="cm";
  else invalid["nodes"][0]["rotation_z"]=std::numeric_limits<double>::infinity();
  if(validate_scene(invalid).empty()){std::cerr<<"accepted invalid "<<field<<"\n";++missed;}
 }
 Json sweep=b.scene;sweep["metadata"]["leaf_envelopes"]=Json::array({{{"node","frame"},{"rect",{{0,0},{1,0},{1,1},{0,1}}},{"z",{0,1}},{"maximum_degrees",1},{"angle_sign",1}}});
 sweep["metadata"]["frame_envelopes"]=Json::array({{{"id","collision.fixture"},{"rect",{{0,0},{1,0},{1,1},{0,1}}},{"z",{0,1}}}});
 if(validate_scene(sweep).empty()){std::cerr<<"accepted leaf/frame sweep collision\n";++missed;}
 require(missed==0,"source validation contract omitted guards");
 std::cout<<"core tests passed\n";
}
