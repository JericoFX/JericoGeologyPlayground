#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 for(auto version:{"rocks-v4","rocks-v5"})for(auto family:{"common_rock","cliff","strata"}){
  std::vector<Plane> previous;
  for(unsigned seed:{48,117,305}){
   Recipe r;r.schemaVersion=2;r.generatorVersion=version;r.extractorVersion="poly-csg-v1";r.family=family;r.seed=seed;r.wear=0;
   CHECK(validateRecipe(r,makeGrid(r,128)).empty());auto f=makeField(r);
   auto detailed=r;detailed.detail=1;CHECK(makeField(detailed).fractures!=f.fractures);
   CHECK(f.planes==makeField(r).planes);CHECK(f.fractures==makeField(r).fractures);CHECK(f.planes!=previous);previous=f.planes;
   CHECK(fieldValue(f,{0,0,0},Stage::Stratified)<0);
   if(r.family=="common_rock")for(auto cut:f.fractures)CHECK(cut.size()==1);
   CHECK(validate(extractPolyhedral(makeField(detailed),Stage::Stratified).mesh,r.size).accepted);
   for(auto stage:{Stage::Base,Stage::Stratified}){
    auto mesh=extractPolyhedral(f,stage).mesh;auto report=validate(mesh,r.size);
    std::cout<<family<<" seed="<<seed<<" stage="<<int(stage)<<" accepted="<<report.accepted<<" faces="<<mesh.indices.size()/3<<std::endl;
    CHECK(report.accepted);CHECK(report.components==1);CHECK(mesh.indices.size()<100000);CHECK(mesh.cornerNormals.size()==mesh.indices.size());
    CHECK(mesh.positions==extractPolyhedral(f,stage).mesh.positions);
    for(std::size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.positions[mesh.indices[i]],b=mesh.positions[mesh.indices[i+1]],c=mesh.positions[mesh.indices[i+2]];auto p=(a+b+c)/3;auto n=normalized(cross(b-a,c-a));CHECK(dot(n,mesh.cornerNormals[i])>.999999);CHECK(fieldValue(f,p+n*(1e-7*r.size),stage)>0);CHECK(fieldValue(f,p-n*(1e-7*r.size),stage)<0);}
   }
  }
 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
