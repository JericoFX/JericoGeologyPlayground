#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
#include <set>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
// Removing a family dispatch, suppressing its structural response, or breaking
// deterministic field evaluation must fail these real public-API tests.
int main(int argc,char**argv){try{
 const std::string profile=argc>1?argv[1]:"shale_laminated";
 HeightfieldRecipe r;r.profile=profile;r.size=.64;r.relief=.12;r.backDepth=.08;r.width=r.height=513;r.cells=128;r.layerSpacingMeters=.06;
 auto a=proceduralHeightfield(r);CHECK(a.samples==proceduralHeightfield(r).samples);CHECK(a.clippedLow==0&&a.clippedHigh==0);CHECK(a.rawMax-a.rawMin>.12);CHECK(a.rawMin>=.05&&a.rawMax<=.95);
 auto ex=generateHeightfield(r);auto report=validate(ex.mesh,r.size);CHECK(report.accepted);CHECK(report.components==1&&report.badEdges==0&&report.badVertices==0);CHECK(report.duplicateFaces==0&&report.degenerateFaces==0);
 for(auto n:ex.mesh.cornerNormals)CHECK(finite(n)&&std::abs(length(n)-1)<1e-10);
 for(unsigned i=0;i<(r.cells+1)*(r.cells+1);i++)CHECK(ex.mesh.positions[i].z+r.backDepth>=r.backDepth);
 auto b=r;b.seed=117;CHECK(a.samples!=proceduralHeightfield(b).samples);
 b=r;b.directionDegrees=31;CHECK(a.samples!=proceduralHeightfield(b).samples);
 b=r;b.jointDepth=0;auto flat=proceduralHeightfield(b);CHECK(a.samples!=flat.samples);
 for(std::size_t i=0;i<a.samples.size();i++)CHECK(a.samples[i]<=flat.samples[i]);
 b=r;b.relief*=.5;CHECK(a.samples==proceduralHeightfield(b).samples);auto half=generateHeightfield(b);for(unsigned i=0;i<(r.cells+1)*(r.cells+1);i++)CHECK(ex.mesh.positions[i].z*.5==half.mesh.positions[i].z);
 for(unsigned seed:{0U,48U,117U,305U,4294967295U}){b=r;b.seed=seed;auto s=proceduralHeightfield(b);CHECK(s.clippedLow+s.clippedHigh==0);CHECK(validate(generateHeightfield(b).mesh,b.size).accepted);}
 std::cout<<profile<<": deterministic morphology, depth control and closed topology PASS\n";
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
