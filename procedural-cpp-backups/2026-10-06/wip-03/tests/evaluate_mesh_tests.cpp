#include "jerico/evaluate.hpp"
#include <fstream>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace jerico;
int main(int argc,char**argv){
 const std::filesystem::path root=argc>1?argv[1]:"fixtures";size_t failed=0,passed=0;
 for(const std::string path:{"furniture/cabinet_closed.json","furniture/workbench_closed.json","damp/old_intact.json","damp/age_above_threshold.json","wood/singlepane_broken_base.json","wood/reinforced_timber_door_aged.json"}){
  std::ifstream in(root/path);if(!in){std::cerr<<"Missing fixture "<<path<<'\n';return 1;}Json s;in>>s;
  for(const auto&p:s["parts"]){try{auto e=evaluate_part(p);Json isolated=s;isolated["parts"]=Json::array({e});isolated["sockets"]=Json::array();isolated["grips"]=Json::array();auto errors=validate_scene(isolated);if(!errors.empty())throw std::runtime_error(errors.dump());if(p.value("bevel",0.)>0&&!e.contains("native_bevel"))throw std::runtime_error("requested bevel missing evaluation metadata");
    if(e.contains("face_material_sides")){
     if(e["face_material_sides"].size()!=e["faces"].size())throw std::runtime_error("directed material slots lost face alignment");
     for(const auto&side:e["face_material_sides"])if(side!="front"&&side!="back"&&side!="")throw std::runtime_error("invalid directed material side");
    }
    for(auto&v:isolated["parts"][0]["vertices"])for(auto&component:v)component=double(static_cast<float>(component.get<double>()));
    errors=validate_scene(isolated);if(!errors.empty())throw std::runtime_error("float32 export collapses geometry: "+errors.dump());
    for(const auto* candidate:{&e,&isolated["parts"][0]}){
     const auto&vertices=(*candidate)["vertices"];
     for(size_t fi=0;fi<(*candidate)["faces"].size();++fi){const auto&face=(*candidate)["faces"][fi];for(size_t k=1;k+1<face.size();++k){const V3 a=vertices[face[0].get<size_t>()],b=vertices[face[k].get<size_t>()],c=vertices[face[k+1].get<size_t>()];const V3 ng=norm(cross(sub(b,a),sub(c,a)));for(size_t loop:{size_t(0),k,k+1})if(dot((*candidate)["normals"][fi][loop].get<V3>(),ng)<=0)throw std::runtime_error("loop normal crosses source/float32 geometric hemisphere at face "+std::to_string(fi));}}
    }
    ++passed;}
   catch(const std::exception&error){std::cerr<<path<<" :: "<<p["id"].get<std::string>()<<" :: "<<error.what()<<'\n';++failed;}
  }
 }
 std::cout<<"Evaluated mesh fixtures: "<<passed<<" passed, "<<failed<<" failed\n";return failed?1:0;
}
