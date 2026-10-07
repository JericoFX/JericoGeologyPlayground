#include "jerico/evaluate.hpp"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
using namespace jerico;
namespace {
void check_hemispheres(const Json& part,bool quantized) {
    auto vertices=part.at("vertices").get<std::vector<V3>>();
    if(quantized) for(auto&v:vertices)for(auto&x:v)x=static_cast<float>(x);
    for(size_t fi=0;fi<part.at("faces").size();++fi){
        const auto&f=part["faces"][fi];
        for(size_t k=1;k+1<f.size();++k){
            const V3 a=vertices[f[0].get<size_t>()],b=vertices[f[k].get<size_t>()],c=vertices[f[k+1].get<size_t>()];
            const V3 ng=norm(cross(sub(b,a),sub(c,a)));
            for(size_t loop:{size_t(0),k,k+1}){
                auto ns=part["normals"][fi][loop].get<V3>();
                if(quantized)for(auto&x:ns)x=static_cast<float>(x);
                if(dot(ns,ng)<=0)throw std::runtime_error(part["id"].get<std::string>()+" face "+std::to_string(fi)+" loop "+std::to_string(loop)+" incompatible normal, dot="+std::to_string(dot(ns,ng))+(quantized?" float32":" double"));
            }
        }
    }
}
}
int main(int argc,char**argv){try{
    const std::filesystem::path root=argc>1?argv[1]:"fixtures";
    size_t count=0;
    for(const auto& filename:{"wood/reinforced_timber_door_aged.json","furniture/cabinet_open.json"}){
        std::ifstream in(root/filename);Json scene;in>>scene;
        for(const auto&source:scene["parts"]){
            const std::string id=source["id"];
            if(id!="door.plank.0"&&id!="post.1.-1"&&id!="drawer.0.cup")continue;
            const auto part=evaluate_part(source);
            if(id=="drawer.0.cup"){
                // Blender 4.3.2, same native candidate geometry, all faces smooth,
                // Weighted Normal FACE_AREA, keep_sharp, weight 40, threshold .01.
                const Json oracle={{"0",{-0.000007291120709851384,-0.9961069226264954,0.08815304934978485}},
                                   {"300",{0.050197526812553406,0.9984836578369141,0.022593867033720016}},
                                   {"500",{-0.3692682385444641,0.47844868898391724,-0.7966980338096619}},
                                   {"600",{-0.7654661536216736,-0.6191742420196533,0.17517070472240448}}};
                for(const auto& entry:oracle.items()){
                    const auto actual=part["normals"][std::stoul(entry.key())][0].get<V3>();
                    const auto expected=entry.value().get<V3>();
                    if(std::sqrt(dot(sub(actual,expected),sub(actual,expected)))>.002)throw std::runtime_error("Blender weighted corner oracle mismatch at cup face "+entry.key());
                }
            }
            check_hemispheres(part,false);check_hemispheres(part,true);++count;
        }
    }
    if(count!=3)throw std::runtime_error("missing normal regression fixture");
    std::cout<<"evaluated normal regression: 3 real parts, double and float32 hemispheres passed\n";
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
