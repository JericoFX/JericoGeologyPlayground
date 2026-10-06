#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 for(auto family:{"columnar_basalt","jointed_granite"}){
  Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v12";r.family=family;r.extractorVersion=r.family=="columnar_basalt"?"poly-csg-v1":"paris-mc-v1";r.wear=0;
  CHECK(validateRecipe(r,makeGrid(r,64)).empty());
  auto unused=r;unused.layerThickness=.04;CHECK(validateRecipe(unused,makeGrid(unused,64)).empty());
  auto f=makeField(r);CHECK(fieldValue(f,{0,r.family=="columnar_basalt"?-.3*r.size:0,0},Stage::Stratified)<0);
  auto ex=r.family=="columnar_basalt"?extractPolyhedral(f,Stage::Stratified):extract(f,makeGrid(r,64),Stage::Stratified);
  auto report=validate(ex.mesh,r.size);std::cout<<family<<" accepted="<<report.accepted<<" ambiguous="<<ex.ambiguousCells<<" triangles="<<ex.mesh.indices.size()/3<<std::endl;
  CHECK(report.accepted);CHECK(!ex.ambiguousCells);CHECK(!ex.boundaryTouched);CHECK(report.components==1);
  for(auto p:ex.mesh.positions)CHECK(finite(p));
  if(r.family=="jointed_granite"){auto fine=extract(f,makeGrid(r,128),Stage::Stratified);std::cout<<"fine accepted="<<validate(fine.mesh,r.size).accepted<<" ambiguous="<<fine.ambiguousCells<<std::endl;CHECK(validate(fine.mesh,r.size).accepted);CHECK(!fine.ambiguousCells);}

  if(r.family=="jointed_granite"){unsigned planar=0;for(std::size_t i=0;i<ex.mesh.indices.size();i+=3){auto a=ex.mesh.normals[ex.mesh.indices[i]],b=ex.mesh.normals[ex.mesh.indices[i+1]],c=ex.mesh.normals[ex.mesh.indices[i+2]];planar+=(length(a-b)<.004&&length(a-c)<.004);}std::cout<<"quiet face triangles="<<planar<<std::endl;CHECK(planar>ex.mesh.indices.size()/3*.20);}
  auto again=r.family=="columnar_basalt"?extractPolyhedral(makeField(r),Stage::Stratified):extract(makeField(r),makeGrid(r,64),Stage::Stratified);
  CHECK(ex.mesh.positions==again.mesh.positions);CHECK(ex.mesh.indices==again.mesh.indices);
  Recipe scaled=r;scaled.size*=1.5;scaled.layerThickness*=1.5;auto sf=makeField(scaled);
  for(int x=-3;x<=3;x++)for(int y=-3;y<=3;y++)for(int z=-3;z<=3;z++){
   Vec3 p{x*.2,y*.2,z*.2};CHECK(std::abs(fieldValue(sf,p*1.5,Stage::Stratified)-1.5*fieldValue(f,p,Stage::Stratified))<1e-10);
   CHECK(finite(evaluate(f,p,Stage::Stratified).gradient));CHECK(resistance(f,p)==.5);
  }
  for(int k=0;k<5;k++){
   Recipe changed=r;if(k==0)changed.studySpacing=.3;if(k==1){changed.roundness=.2;changed.columnCount=3;}if(k==2){changed.chipping=1;changed.columnLean=-.15;}if(k==3){changed.roundness=.06;changed.columnCount=7;}if(k==4)changed.jointDepth=0;
   if(k==4&&r.family=="jointed_granite")continue;
   auto altered=makeField(changed);auto e=r.family=="columnar_basalt"?extractPolyhedral(altered,Stage::Stratified):extract(altered,makeGrid(r,64),Stage::Stratified);
   std::cout<<family<<" bound="<<k<<" accepted="<<validate(e.mesh,r.size).accepted<<" ambiguous="<<e.ambiguousCells<<std::endl;
   CHECK(validate(e.mesh,r.size).accepted);CHECK(!e.ambiguousCells);CHECK(e.mesh.positions!=ex.mesh.positions);
  }
  for(unsigned seed:{117U,305U}){auto seeded=r;seeded.seed=seed;auto field=makeField(seeded);for(auto stage:{Stage::Base,Stage::Stratified}){auto output=r.family=="columnar_basalt"?extractPolyhedral(field,stage):extract(field,makeGrid(seeded,128),stage);CHECK(validate(output.mesh,seeded.size).accepted);CHECK(!output.ambiguousCells);CHECK(!output.boundaryTouched);CHECK(output.mesh.positions!=ex.mesh.positions);CHECK(output.maxContractionDistance<=1e-5*seeded.size);CHECK(output.contractedFaces==2*output.contractedEdges);}}
  for(int k=0;k<7;k++){auto bad=r;if(k==0)bad.studySpacing=.29;if(k==1)bad.roundness=NAN;if(k==2)bad.chipping=2;if(k==3)bad.columnCount=8;if(k==4)bad.columnLean=1;if(k==5)bad.jointDepth=-.1;if(k==6)bad.extractorVersion="bad";CHECK(!validateRecipe(bad,makeGrid(bad,64)).empty());}

 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
