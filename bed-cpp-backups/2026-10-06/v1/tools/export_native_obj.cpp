#include "jerico/export.hpp"
#include <fstream>
#include <iostream>
int main(int argc,char**argv){try{if(argc!=3)throw std::runtime_error("Usage: export_native_obj native-scene.json output.obj");std::ifstream input(argv[1]);if(!input)throw std::runtime_error("Cannot open native scene");jerico::Json scene;input>>scene;jerico::write_obj_mtl(scene,argv[2]);std::cout<<"Exported native OBJ/MTL in world-space Z-up metres; glass transmission is not preserved\n";return 0;}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
