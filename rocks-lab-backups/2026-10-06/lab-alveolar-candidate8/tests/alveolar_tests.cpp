#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 HeightfieldRecipe r;r.profile="alveolar_blocks";r.cells=256;r.width=r.height=1025;r.size=2;r.relief=.55;r.backDepth=.3;r.irregularity=.65;r.fractures=.7;r.jointDepth=.85;
 bool supported=true;try{validateHeightfieldRecipe(r);}catch(const std::invalid_argument&){supported=false;}
 CHECK(supported);
 auto a=proceduralHeightfield(r);CHECK(a.samples.size()==1025U*1025U);CHECK(a.clippedLow==0&&a.clippedHigh==0);
 auto host=r;host.profile="massive_oblique";host.blockScale=.288;host.jointSpacing=.16;host.jointDepth=.15;host.fractures=.175;
 auto b=proceduralHeightfield(host);CHECK(a.samples!=b.samples);
 r.jointDepth=0;CHECK(proceduralHeightfield(r).samples==b.samples);
 std::cout<<"alveolar profile erosion and zero-depth identity PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
