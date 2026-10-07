#include "jerico/export.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Usage: export_native_scene native-scene.json output.glb");std::ifstream input(argv[1]);if(!input)throw std::runtime_error("Cannot open native scene");jerico::Json scene;input>>scene;const auto errors=jerico::validate_scene(scene);if(!errors.empty())throw std::runtime_error("Native scene validation failed: "+errors.dump());jerico::write_glb(scene,argv[2]);std::cout<<"Exported existing native C++ scene without regeneration or repair\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
