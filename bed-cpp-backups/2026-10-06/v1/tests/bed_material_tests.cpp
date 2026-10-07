#include "jerico/bed.hpp"
#include "jerico/export.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include <set>
using namespace jerico;
void need(bool q,const std::string&s){if(!q)throw std::runtime_error(s);}
int main(int argc,char**argv){try{std::filesystem::path out=argc>1?argv[1]:"bed-material-tests";auto s=evaluate_bed(build_bed(Json::object()));bake_bed_materials(s,out.string(),64);std::set<std::string> family,paths;for(auto&p:s["parts"]){family.insert(p["material_shared_id"]);paths.insert(p["textures"]["base_color"]);for(auto&f:p["texture_uvs"])for(auto&uv:f)for(auto&x:uv)need(x>=0&&x<=1,"UV range");}need(family.size()==3&&paths.size()==3,"only three shared atlases");need(validate_scene(s).empty(),"baked scene valid");write_glb(s,(out/"bed.glb").string());write_obj_mtl(s,(out/"bed.obj").string());std::ifstream in(out/"bed.glb",std::ios::binary);in.seekg(12);uint32_t n;in.read(reinterpret_cast<char*>(&n),4);in.seekg(20);std::string js(n,' ');in.read(js.data(),n);auto j=Json::parse(js);need(j["materials"].size()==3,"GLB material dedup");need(j["images"].size()==18,"GLB image dedup");std::ofstream(out/"scene.json")<<s.dump();std::cout<<"Three shared native atlases, GLB/OBJ exports and texture UVs passed\n";}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
