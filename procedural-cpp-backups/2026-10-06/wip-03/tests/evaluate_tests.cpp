#include "jerico/evaluate.hpp"
#include <iostream>
#include <cmath>
#include <stdexcept>
using namespace jerico;
int main(){try{Builder b(normalize_recipe(Json::object()));b.node("frame",{0,0,0},"Frame",nullptr);b.box("cube","frame",-1,1,-1,1,-1,1,"iron",.1);auto e=evaluate_scene(b.scene);if(e["parts"][0]["vertices"].size()<=8)throw std::runtime_error("bevel added no geometry");auto errors=validate_scene(e);if(!errors.empty())throw std::runtime_error(errors.dump());for(auto&v:e["parts"][0]["vertices"])for(double x:v)if(x<-1-1e-10||x>1+1e-10)throw std::runtime_error("bevel outside bounds");for(const auto&part:e["parts"]){for(size_t fi=0;fi<part["faces"].size();++fi){const auto&face=part["faces"][fi];V3 a=part["vertices"][face[0].get<size_t>()],area{};for(size_t k=1;k+1<face.size();++k)area=add(area,cross(sub(part["vertices"][face[k].get<size_t>()].get<V3>(),a),sub(part["vertices"][face[k+1].get<size_t>()].get<V3>(),a)));V3 expected=norm(area);for(const auto&normal:part["normals"][fi])if(dot(expected,normal.get<V3>())<1-1e-10)throw std::runtime_error("Source flat cube unexpectedly has smoothed bevel corner normals");}}
std::cout<<"native evaluated cube closed, finite, bounded; source flat shading preserved\n";}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
