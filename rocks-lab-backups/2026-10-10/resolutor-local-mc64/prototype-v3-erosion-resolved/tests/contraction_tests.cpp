#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)

Extraction fixture(std::vector<Vec3> positions,std::vector<std::uint32_t> indices){
 Extraction e;e.mesh.positions=std::move(positions);e.mesh.indices=std::move(indices);
 e.mesh.normals.resize(e.mesh.positions.size());e.mesh.resistance.resize(e.mesh.positions.size(),.5);
 for(std::size_t i=0;i<e.mesh.indices.size();i+=3){auto a=e.mesh.indices[i],b=e.mesh.indices[i+1],c=e.mesh.indices[i+2];auto n=cross(e.mesh.positions[b]-e.mesh.positions[a],e.mesh.positions[c]-e.mesh.positions[a]);for(auto v:{a,b,c})e.mesh.normals[v]=e.mesh.normals[v]+n;}
 for(auto&n:e.mesh.normals)n=normalized(n);
 return e;
}
void expectUnchangedRejection(Extraction e,double size=1){
 auto before=e.mesh;bool rejected=false;
 try{contractSubresolutionEdges(e,size);}catch(const std::runtime_error&){rejected=true;}
 CHECK(rejected);CHECK(e.mesh.positions==before.positions);CHECK(e.mesh.indices==before.indices);
 CHECK(e.contractedEdges==0);CHECK(e.contractedFaces==0);CHECK(e.maxContractionDistance==0);
}
int main(){try{
 // Endpoint neighbor sets meet the old vertex-only link check, but the
 // opposite edge also belongs to both links. Collapse would leave two
 // coincident triangles with opposite winding and destroy the tetrahedron.
 expectUnchangedRejection(fixture({{0,0,0},{1e-13,0,0},{0,1,0},{0,0,1}},
                                  {0,2,1,0,1,3,0,3,2,1,2,3}));
 for(double scale:{.1,1.,10.})
  expectUnchangedRejection(fixture({{0,0,0},{1e-7*scale,0,0},{0,1e-6*scale,0},{0,0,scale}},
                                   {0,2,1,0,1,3,0,3,2,1,2,3}),scale);
 // A genuinely contractible short edge: subdivide one tetrahedron edge.
 auto e=fixture({{0,0,0},{1,0,0},{0,1,0},{0,0,1},{1e-13,0,0}},
                {0,2,4,4,2,1,0,4,3,4,1,3,0,3,2,1,2,3});
 auto original=e.mesh.positions;contractSubresolutionEdges(e,1);
 CHECK(e.contractedEdges==1);CHECK(e.contractedFaces==2);CHECK(e.mesh.indices.size()==12);
 CHECK(e.maxContractionDistance>0&&e.maxContractionDistance<=1e-5);
 CHECK(validate(e.mesh,1).accepted);
 for(auto p:e.mesh.positions)CHECK(std::find(original.begin(),original.end(),p)!=original.end());
 // A thin triangle alone does not authorize moving a long edge endpoint.
 expectUnchangedRejection(fixture({{0,0,0},{1,0,0},{.5,1e-13,0},{0,0,1}},
                                  {0,2,1,0,1,3,0,3,2,1,2,3}));
 std::cout<<"bounded contraction tetrahedron rejection and legal collapse PASS\n";
 }catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
}
