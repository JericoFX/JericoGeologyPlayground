#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v8";r.family="cliff_plateau";r.extractorVersion="poly-csg-v1";r.wear=0;
 auto unsupported=r;unsupported.detail=.5;CHECK(!validateRecipe(unsupported,makeGrid(unsupported,128)).empty());
 auto f=makeField(r);CHECK(f.masses.size()==10);CHECK(f.fractures.empty());std::set<std::size_t> footprintSizes;for(std::size_t i=1;i<f.masses.size();i++)footprintSizes.insert(f.masses[i].size());CHECK(footprintSizes.size()>1);CHECK(f.masses==makeField(r).masses);
 for(double x:{-.4,0.,.4})for(double y:{-.3,0.,.3})CHECK(fieldValue(f,{x*r.size,y*r.size,-.1*r.size},Stage::Stratified)<0);
 auto e=extractPolyhedral(f,Stage::Stratified);auto report=validate(e.mesh,r.size);CHECK(report.accepted);CHECK(e.mesh.positions==extractPolyhedral(f,Stage::Stratified).mesh.positions);
 for(std::size_t i=0;i<e.mesh.indices.size();i+=3){auto a=e.mesh.positions[e.mesh.indices[i]],b=e.mesh.positions[e.mesh.indices[i+1]],c=e.mesh.positions[e.mesh.indices[i+2]];auto p=(a+b+c)/3,n=normalized(cross(b-a,c-a));CHECK(fieldValue(f,p+n*2e-7,Stage::Stratified)>0);CHECK(fieldValue(f,p-n*2e-7,Stage::Stratified)<0);}
 std::cout<<"hierarchical plateau fixture PASS vertices="<<e.mesh.positions.size()<<" triangles="<<e.mesh.indices.size()/3<<'\n';
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
