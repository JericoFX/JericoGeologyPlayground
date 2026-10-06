#include "jerico/scene.hpp"
#include "jerico/damage.hpp"
#include "jerico/evaluate.hpp"
#include "jerico/interfaces.hpp"
#include "jerico/materials.hpp"
#include "jerico/export.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace jerico;
int main(int argc,char**argv){try{
 if(argc!=3)throw std::runtime_error("usage: manual_driver case.json output-directory");
 Json request;std::ifstream(argv[1])>>request;const std::filesystem::path directory=argv[2];
 Json scene=build_industrial(request.at("recipe"));size_t index=scene["parts"].size();for(size_t i=0;i<scene["parts"].size();++i)if(scene["parts"][i]["id"]=="door.sheet")index=i;
 if(index==scene["parts"].size())throw std::runtime_error("door.sheet missing");
 Json part=scene["parts"][index];for(const auto&cut:request.at("manual_cuts"))part=carve_panel(part,cut.at("polygon_xz"),cut.value("side",std::string("front")),cut.value("depth",Json(nullptr)));
 scene["parts"][index]=part;scene["metadata"]["coverage_case"]=request.at("case");scene["metadata"]["manual_cut_requests"]=request.at("manual_cuts");
 auto errors=validate_scene(scene);if(!errors.empty())throw std::runtime_error("source validation: "+errors.dump());errors=validate_interfaces(scene);if(!errors.empty())throw std::runtime_error("interface validation: "+errors.dump());
 std::filesystem::create_directories(directory);std::ofstream(directory/"source.json")<<scene.dump(2);
 scene=evaluate_scene(scene);errors=validate_scene(scene);if(!errors.empty())throw std::runtime_error("evaluated validation: "+errors.dump());
 const auto report=bake_material_textures(scene,directory.string(),64);std::ofstream(directory/"material-report.json")<<report.dump(2);
 write_glb(scene,(directory/"asset.glb").string());std::ofstream(directory/"scene.json")<<scene.dump(2);
 std::cout<<"Native direct carve_panel API export passed for "<<request["case"]<<'\n';return 0;
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
