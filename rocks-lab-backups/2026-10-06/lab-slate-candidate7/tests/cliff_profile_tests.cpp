#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do {if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 HeightfieldRecipe r;r.width=96;r.height=96;r.cells=48;
 auto legacy=proceduralHeightfield(r).samples;
 // Independently compiled from c566766 before the v11 extension (64x64 defaults).
 const std::uint64_t v10Hashes[]={394923978403433414ULL,9953843331803197921ULL,15060227053902125436ULL};unsigned hi=0;
 for(unsigned seed:{48U,117U,305U}){HeightfieldRecipe frozen;frozen.seed=seed;frozen.width=frozen.height=64;std::uint64_t hash=1469598103934665603ULL;for(auto value:proceduralHeightfield(frozen).samples){hash^=value;hash*=1099511628211ULL;}CHECK(hash==v10Hashes[hi++]);}

 for(auto profile:{"massive_oblique","vertical_channels"}){
  r.profile=profile;auto a=proceduralHeightfield(r);CHECK(a.samples!=legacy);CHECK(a.samples==proceduralHeightfield(r).samples);CHECK(a.clippedLow==0&&a.clippedHigh==0);CHECK(a.rawMax-a.rawMin>.32);CHECK(validate(generateHeightfield(r).mesh,r.size).accepted);
  auto b=r;b.jointSpacing+=.001;CHECK(proceduralHeightfield(b).samples!=a.samples);b=r;b.jointDepth=0;auto uncut=proceduralHeightfield(b).samples;std::size_t equal=0;for(std::size_t i=0;i<uncut.size();i++){CHECK(a.samples[i]<=uncut[i]);equal+=a.samples[i]==uncut[i];}CHECK(equal>uncut.size()/5);CHECK(equal<uncut.size()*.92);
  for(int k=0;k<5;k++){b=r;if(k==0)b.seed=117;if(k==1)b.directionDegrees=30;if(k==2)b.jointSpacing=.12;if(k==3)b.blockScale=.5;if(k==4)b.irregularity=0;CHECK(proceduralHeightfield(b).samples!=a.samples);}
  b=r;b.bedCount=5;b.thicknessVariation=0;b.tiltDegrees=-25;b.fragmentedContacts=true;CHECK(proceduralHeightfield(b).samples==a.samples);
  b=r;b.relief*=.5;auto first=generateHeightfield(r),half=generateHeightfield(b);CHECK(proceduralHeightfield(b).samples==a.samples);for(unsigned i=0;i<(r.cells+1)*(r.cells+1);i++)CHECK(first.mesh.positions[i].z*.5==half.mesh.positions[i].z);
  b=r;b.fractures=.5;auto fewer=proceduralHeightfield(b).samples;b=r;b.jointDepth*=.5;auto shallower=proceduralHeightfield(b).samples;CHECK(fewer!=shallower);std::size_t fullSame=0,halfSame=0;for(std::size_t i=0;i<a.samples.size();i++){fullSame+=a.samples[i]==fewer[i];halfSame+=a.samples[i]==shallower[i];}CHECK(fullSame>halfSame);
  b=r;b.directionDegrees=90;auto rotated=proceduralHeightfield(b).samples;for(unsigned y=0;y<r.height;y++)for(unsigned x=0;x<r.width;x++)CHECK(std::abs(int(rotated[y*r.width+x])-int(a.samples[(r.height-1-x)*r.width+y]))<=1);
  b=r;b.size=3;b.backDepth=.4;CHECK(proceduralHeightfield(b).samples==a.samples);
  double dx=0,dy=0;for(unsigned y=1;y<r.height;y++)for(unsigned x=1;x<r.width;x++){double sx=int(a.samples[y*r.width+x])-int(a.samples[y*r.width+x-1]),sy=int(a.samples[y*r.width+x])-int(a.samples[(y-1)*r.width+x]);dx+=sx*sx;dy+=sy*sy;}CHECK(dx>0&&dy>0);if(r.profile=="vertical_channels")CHECK(dx>dy);
  for(unsigned seed:{48U,117U,305U}){b=r;b.seed=seed;auto sample=proceduralHeightfield(b);CHECK(sample.samples==proceduralHeightfield(b).samples);CHECK(sample.clippedLow==0&&sample.clippedHigh==0);CHECK(validate(generateHeightfield(b).mesh,b.size).accepted);}
  for(unsigned seed:{0U,4294967295U}){b=r;b.seed=seed;b.directionDegrees=-90;b.jointSpacing=.08;b.jointDepth=1;b.blockScale=.12;CHECK(validate(generateHeightfield(b).mesh,b.size).accepted);}
  for(int k=0;k<5;k++){b=r;if(k==0)b.profile="unknown";if(k==1)b.directionDegrees=NAN;if(k==2)b.jointSpacing=0;if(k==3)b.blockScale=2;if(k==4)b.jointDepth=1.1;bool rejected=false;try{proceduralHeightfield(b);}catch(std::invalid_argument&){rejected=true;}CHECK(rejected);}
 }
 // Explicit clipping is permitted at supported extremes and must remain visible.
 HeightfieldRecipe clipped;clipped.profile="massive_oblique";clipped.seed=2179694280U;clipped.blockScale=.12;clipped.jointSpacing=.08;clipped.directionDegrees=-90;clipped.jointDepth=1;clipped.width=41;clipped.height=29;clipped.cells=12;
 auto clippedSamples=proceduralHeightfield(clipped);CHECK(clippedSamples.clippedLow==1);CHECK(clippedSamples.rawMin<0);CHECK(*std::min_element(clippedSamples.samples.begin(),clippedSamples.samples.end())==0);CHECK(validate(generateHeightfield(clipped).mesh,clipped.size).accepted);
 std::cout<<"distinct profile determinism, finite ranges, local incisions, fixed mapping PASS\n";
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
