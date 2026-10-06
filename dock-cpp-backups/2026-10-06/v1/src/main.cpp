#include "jerico/scene.hpp"
#include "jerico/evaluate.hpp"
#include "jerico/materials.hpp"
#include "jerico/interfaces.hpp"
#include "jerico/fine_wear.hpp"
#include "jerico/export.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
using namespace jerico;

int main(int argc, char** argv) {
 try {
  std::string family="fracture", recipe_path, out, glb, obj;
  bool evaluate=false;
  int bake=0;
  for(int i=1;i<argc;i++) {
   const std::string option=argv[i];
   if(option=="--help") {
    std::cout << "Usage: jerico_procedural --family fracture|industrial|finewear|wood|singlepane|damp|furniture --recipe input.json --out output.json [--evaluate 1] [--bake 1024] [--glb asset.glb] [--obj asset.obj]\n";
    return 0;
   }
   if(i+1>=argc) throw std::runtime_error("Missing argument value: "+option);
   const std::string value=argv[++i];
   if(option=="--family") family=value;
   else if(option=="--recipe") recipe_path=value;
   else if(option=="--out") out=value;
   else if(option=="--glb") glb=value;
   else if(option=="--obj") obj=value;
   else if(option=="--evaluate") {
    if(value!="0"&&value!="1"&&value!="false"&&value!="true")
     throw std::runtime_error("--evaluate expects 0,1,false,true");
    evaluate=(value=="1"||value=="true");
   } else if(option=="--bake") bake=std::stoi(value);
   else throw std::runtime_error("Unknown option: "+option);
  }
  if(recipe_path.empty()||out.empty()) throw std::runtime_error("Recipe and output paths required; use --help");
  std::ifstream input(recipe_path);
  if(!input) throw std::runtime_error("Cannot open recipe: "+recipe_path);
  Json recipe; input>>recipe;
  if(recipe.contains("seed")) recipe_seed(recipe["seed"]);
  Json scene;
  if(family=="fracture") scene=build_fracture(recipe);
  else if(family=="industrial") scene=build_industrial(recipe);
  else if(family=="furniture") scene=build_furniture(recipe);
  else if(family=="wood") scene=build_wood(recipe);
  else if(family=="singlepane") scene=build_singlepane(recipe);
  else if(family=="damp") scene=build_damp_wood(recipe);
  else if(family=="finewear") scene=build_fine_wear(recipe);
  else throw std::runtime_error("Unknown family: "+family);
  auto errors=family=="furniture"?validate_furniture(scene):validate_scene(scene);
  if(!errors.empty()) throw std::runtime_error("Source mesh validation failed: "+errors.dump());
  errors=validate_interfaces(scene);
  if(!errors.empty()) throw std::runtime_error("Interaction contract failed: "+errors.dump());
  if(evaluate) {
   scene=evaluate_scene(scene);
   errors=validate_scene(scene);
   if(!errors.empty()) throw std::runtime_error("Evaluated mesh validation failed: "+errors.dump());
  }
  auto folder=std::filesystem::path(out).parent_path();
  if(folder.empty()) folder=".";
  std::filesystem::create_directories(folder);
  if(bake>0) {
   if(!evaluate) throw std::runtime_error("Texture baking requires --evaluate 1");
   const auto report=bake_material_textures(scene,folder.string(),bake);
   std::ofstream report_file(folder/"material-report.json");
   report_file<<report.dump(2)<<'\n';
   errors=validate_scene(scene);
   if(!errors.empty()) throw std::runtime_error("Baked mesh validation failed: "+errors.dump());
  }
  if(!glb.empty()) {
   if(!evaluate||bake<=0) throw std::runtime_error("GLB export requires --evaluate 1 and --bake N");
   write_glb(scene,glb);
  }
  if(!obj.empty()) {
   if(!evaluate||bake<=0) throw std::runtime_error("OBJ/MTL export requires --evaluate 1 and --bake N");
   write_obj_mtl(scene,obj);
  }
  std::ofstream file(out);
  if(!file) throw std::runtime_error("Cannot write scene: "+out);
  file<<scene.dump(2)<<'\n';
  if(!file) throw std::runtime_error("Incomplete scene write: "+out);
  std::cout<<"C++ generated "<<scene["parts"].size()<<" parts; "
           <<(evaluate?"native evaluated mesh":"source mesh")
           <<" and interaction contracts validated. Visual parity is tracked separately.\n";
  return 0;
 } catch(const std::exception& error) {
  std::cerr<<error.what()<<'\n';
  return 1;
 }
}
