#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
#include <limits>
using namespace rocks;
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
static std::uint64_t digest(const HeightfieldSamples&s){std::uint64_t h=1469598103934665603ULL;for(auto v:s.samples){h^=v;h*=1099511628211ULL;}return h;}
int main(){try{
 HeightfieldRecipe inactive;inactive.profile="angular_scree";inactive.width=inactive.height=257;inactive.cells=64;inactive.fragmentScaleMeters=.4;auto activeOnly=proceduralHeightfield(inactive);inactive.lobeCount=std::numeric_limits<std::uint32_t>::max();CHECK(proceduralHeightfield(inactive).samples==activeOnly.samples);
 HeightfieldRecipe positional{117,65,65,16};CHECK(positional.seed==117);CHECK(positional.width==65);CHECK(positional.height==65);CHECK(positional.cells==16);
 const std::vector<std::pair<std::string,std::uint64_t>> baselines={{"stratified",17141506850881561337ULL},{"massive_oblique",17562711200393448930ULL},{"vertical_channels",7612288368902441834ULL}};
 for(auto [profile,expected]:baselines){HeightfieldRecipe r;r.profile=profile;r.width=r.height=129;r.cells=32;CHECK(digest(proceduralHeightfield(r))==expected);}
 for(auto profile:{"lava_rope_lobes","angular_scree"}){
  HeightfieldRecipe r;r.profile=profile;r.width=r.height=513;r.cells=128;r.relief=.55;
  bool recognized=true;try{validateHeightfieldRecipe(r);}catch(const std::invalid_argument&){recognized=false;}CHECK(recognized);
  auto a=proceduralHeightfield(r),b=proceduralHeightfield(r);CHECK(a.samples==b.samples);CHECK(a.samples.size()==513*513);CHECK(a.clippedLow==0&&a.clippedHigh==0);CHECK(a.rawMin>0&&a.rawMax<1);CHECK(a.rawMax-a.rawMin>.2);
  auto rotated=r;rotated.directionDegrees=35;CHECK(proceduralHeightfield(rotated).samples!=a.samples);
  auto unbroken=r;unbroken.fractures=0;CHECK(proceduralHeightfield(unbroken).samples!=a.samples);
  auto changed=r;changed.seed++;CHECK(proceduralHeightfield(changed).samples!=a.samples);
  auto control=r;control.jointDepth=0;auto c=proceduralHeightfield(control);CHECK(c.samples!=a.samples);CHECK(c.rawMax-c.rawMin<.12);
  auto extracted=generateHeightfield(r);auto report=validate(extracted.mesh,r.size);CHECK(report.accepted);CHECK(report.components==1);CHECK(report.badEdges==0&&report.badVertices==0&&report.degenerateFaces==0);for(auto p:extracted.mesh.positions)CHECK(finite(p));
  auto bad=r;bad.width=10;bool rejected=false;try{validateHeightfieldRecipe(bad);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
  std::cout<<profile<<" accepted=1 deterministic=1 range="<<a.rawMin<<","<<a.rawMax<<" triangles="<<extracted.mesh.indices.size()/3<<"\n";
 }
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
