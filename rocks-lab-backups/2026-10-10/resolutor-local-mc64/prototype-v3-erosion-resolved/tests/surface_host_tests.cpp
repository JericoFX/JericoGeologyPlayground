#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 for(auto profile:{"slate_foliated","limestone_dissolution"})for(unsigned seed:{48U,117U,305U}){
  HeightfieldRecipe r;r.profile=profile;r.seed=seed;r.size=.64;r.relief=.12;r.backDepth=.075;r.cells=128;r.width=r.height=513;r.layerSpacingMeters=.075;r.plateLengthMeters=.28;r.directionDegrees=-20;r.jointDepth=0;r.fractures=.7;r.irregularity=.65;
  auto host=r;const bool slate=r.profile=="slate_foliated";host.profile=slate?"vertical_channels":"massive_oblique";host.blockScale=std::clamp(slate?r.plateLengthMeters/(r.size*2.1):r.channelSpacingMeters/r.size*1.8,.12,.6);host.jointSpacing=std::clamp(slate?r.layerSpacingMeters/r.size*2:r.channelSpacingMeters/r.size,.08,.6);host.jointDepth=.15;host.fractures=r.fractures*.25;
  CHECK(proceduralHeightfield(r).samples==proceduralHeightfield(host).samples);
  auto a=generateHeightfield(r),b=generateHeightfield(host);CHECK(a.mesh.positions==b.mesh.positions);CHECK(a.mesh.indices==b.mesh.indices);CHECK(a.mesh.cornerNormals==b.mesh.cornerNormals);
  r.jointDepth=.85;auto changed=proceduralHeightfield(r);CHECK(changed.samples!=proceduralHeightfield(host).samples);CHECK(validate(generateHeightfield(r).mesh,r.size).accepted);
  std::cout<<profile<<" seed="<<seed<<" zero-detail inherited outcrop identity PASS\n";
 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
