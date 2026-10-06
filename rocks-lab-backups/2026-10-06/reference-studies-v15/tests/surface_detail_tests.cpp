#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 Recipe r;r.schemaVersion=2;r.generatorVersion="rocks-v6";r.family="common_rock";r.extractorVersion="poly-csg-v1";r.wear=0;r.detail=1;
 auto f=makeField(r);CHECK(f.fractures.size()==3);auto e=extractPolyhedral(f,Stage::Stratified);
 CHECK(validate(e.mesh,r.size).accepted);CHECK(e.maxDisplacement>0);CHECK(e.maxDisplacement<=.008*r.size);CHECK(e.jacobianLowerBound>0);CHECK(e.flippedTriangles==0);CHECK(e.preservedSharpVertices>0);
 CHECK(e.mesh.indices.size()>3000);CHECK(e.mesh.indices.size()<105000);CHECK(e.mesh.positions==extractPolyhedral(f,Stage::Stratified).mesh.positions);
 r.detail=0;auto off=extractPolyhedral(makeField(r),Stage::Stratified);CHECK(off.maxDisplacement==0);CHECK(off.mesh.indices.size()<e.mesh.indices.size());
 auto difference=compareSurfaces(off.mesh,e.mesh);CHECK(difference.maxDistance>0);CHECK(difference.maxDistance<=.008*r.size+1e-8*r.size);
 for(std::size_t v=0;v<off.mesh.positions.size();v++){
  Vec3 first{};bool seen=false,sharp=false;
  for(std::size_t i=0;i<off.mesh.indices.size();i++)if(off.mesh.indices[i]==v){auto n=off.mesh.cornerNormals[i];if(!seen){first=n;seen=true;}else if(dot(first,n)<1-1e-8)sharp=true;}
  if(sharp)CHECK(length(off.mesh.positions[v]-e.mesh.positions[v])<1e-12*r.size);
 }
 auto unsupported=r;unsupported.extractorVersion="paris-mc-v1";CHECK(!validateRecipe(unsupported,makeGrid(unsupported,128)).empty());
 std::cout<<"v6 bounded shared-edge relief PASS vertices="<<e.mesh.positions.size()<<" triangles="<<e.mesh.indices.size()/3<<" maxDisplacement="<<e.maxDisplacement<<" jacobianLowerBound="<<e.jacobianLowerBound<<" sharpPinned="<<e.preservedSharpVertices<<'\n';
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
