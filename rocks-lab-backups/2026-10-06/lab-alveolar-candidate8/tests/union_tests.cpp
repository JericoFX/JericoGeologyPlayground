#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
std::vector<Plane> box(Vec3 lo,Vec3 hi){return {{{1,0,0},hi.x},{{-1,0,0},-lo.x},{{0,1,0},hi.y},{{0,-1,0},-lo.y},{{0,0,1},hi.z},{{0,0,-1},-lo.z}};}
int main(){try{
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v4";r.family="strata";r.extractorVersion="poly-csg-v1";r.wear=0;
 RockField f{r,box({-.8,-.8,-.8},{.8,.8,.8}),{},0,{}};
 f.masses={box({-.5,-.5,-.5},{.5,.1,.5}),box({-.3,-.1,-.5},{.5,.5,.3})};
 CHECK(fieldValue(f,{0,.4,0},Stage::Stratified)<0);CHECK(fieldValue(f,{-.4,.4,0},Stage::Stratified)>0);
 CHECK((evaluate(f,{.5,.5,.1},Stage::Stratified).gradient==Vec3{1,0,0}));
 auto mesh=extractPolyhedral(f,Stage::Stratified).mesh;auto report=validate(mesh,2);CHECK(report.accepted);CHECK(std::abs(report.signedVolume-(.6+.384-.128))<1e-8);
 CHECK(mesh.positions==extractPolyhedral(f,Stage::Stratified).mesh.positions);
 for(std::size_t i=0;i<mesh.indices.size();i+=3){auto a=mesh.positions[mesh.indices[i]],b=mesh.positions[mesh.indices[i+1]],c=mesh.positions[mesh.indices[i+2]];auto p=(a+b+c)/3,n=normalized(cross(b-a,c-a));CHECK(fieldValue(f,p+n*2e-7,Stage::Stratified)>0);CHECK(fieldValue(f,p-n*2e-7,Stage::Stratified)<0);}
 r.generatorVersion="rocks-v5";auto strata=makeField(r);CHECK(strata.masses.size()>=4);CHECK(strata.fractures.empty());CHECK(validate(extractPolyhedral(strata,Stage::Stratified).mesh,r.size).accepted);
 r.family="cliff";auto cliff=makeField(r);CHECK(cliff.masses.size()>=4);CHECK(cliff.fractures.empty());CHECK(validate(extractPolyhedral(cliff,Stage::Stratified).mesh,r.size).accepted);
 r.family="common_rock";r.detail=0;auto coarse=makeField(r);r.detail=1;auto chipped=makeField(r);CHECK(chipped.fractures.size()>coarse.fractures.size());CHECK(chipped.fractures==makeField(r).fractures);auto cm=extractPolyhedral(chipped,Stage::Stratified).mesh;CHECK(validate(cm,r.size).accepted);CHECK(validate(cm,r.size).signedVolume<validate(extractPolyhedral(coarse,Stage::Stratified).mesh,r.size).signedVolume);
 std::cout<<"bounded positive union analytic volume and strata fixture PASS\n";
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
