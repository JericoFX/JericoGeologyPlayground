#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v3";r.extractorVersion="paris-mc-v1";r.family="irregular_beds";r.seed=48;r.wear=0;r.detail=1;
 auto f=makeField(r);auto e=extract(f,makeGrid(r,64),Stage::Stratified);auto q=validate(e.mesh,r.size);
 std::cout<<"triangles="<<e.mesh.indices.size()/3<<" ambiguous="<<e.ambiguousCells<<" degenerate="<<q.degenerateFaces<<" badEdges="<<q.badEdges<<" badVertices="<<q.badVertices<<" volume="<<q.signedVolume<<std::endl;
 CHECK(e.ambiguousCells==0);CHECK(q.accepted);CHECK(q.components==1);CHECK(!e.boundaryTouched);CHECK(e.mesh.indices.size()/3<26000);CHECK(q.signedVolume>1.8);
 for(int x=-6;x<=6;x++)for(int y=-4;y<=4;y++){
  Vec3 p{r.size*x/20.,r.size*y/20.,-.3*r.size};CHECK(fieldValue(f,p,Stage::Stratified)==fieldValue(f,p,Stage::Base));
 }
 for(int x=-7;x<=7;x++)for(int y=-6;y<=6;y++)for(int z=-7;z<=7;z++){
  Vec3 p{r.size*x/16.,r.size*y/16.,r.size*z/16.};CHECK(fieldValue(f,p,Stage::Stratified)>=fieldValue(f,p,Stage::Base));
 }
 std::cout<<"erosion field safeguards PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
