#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 for(unsigned seed:{48,117,305}){Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v7";r.family="weathered_boulder";r.extractorVersion="paris-mc-v1";r.seed=seed;r.wear=0;r.detail=1;
 auto f=makeField(r);CHECK(fieldValue(f,{0,0,0},Stage::Stratified)<0);
 for(int x=-5;x<=5;x++)for(int y=-5;y<=5;y++)for(int z=-5;z<=5;z++){Vec3 p{r.size*x/10,r.size*y/10,r.size*z/10};auto a=evaluate(f,p,Stage::Stratified);CHECK(std::isfinite(a.value));CHECK(finite(a.gradient));CHECK(std::abs(a.value-fieldValue(f,p,Stage::Base))<=.06*r.size);CHECK(a.value==fieldValue(makeField(r),p,Stage::Stratified));}
 for(auto cells:{64,128})for(auto stage:{Stage::Base,Stage::Stratified}){auto e=extract(f,makeGrid(r,cells),stage);auto report=validate(e.mesh,r.size);std::cout<<"weathered seed="<<seed<<" cells="<<cells<<" stage="<<int(stage)<<" ambiguous="<<e.ambiguousCells<<" accepted="<<report.accepted<<" triangles="<<e.mesh.indices.size()/3<<'\n';CHECK(!e.ambiguousCells);CHECK(report.accepted);CHECK(!e.boundaryTouched);CHECK(e.maxContractionDistance<=1e-5*r.size);CHECK(e.contractedFaces==2*e.contractedEdges);for(auto p:e.mesh.positions)CHECK(std::abs(fieldValue(f,p,stage))<1e-8*r.size);CHECK(e.mesh.positions==extract(f,makeGrid(r,cells),stage).mesh.positions);
 }
 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
