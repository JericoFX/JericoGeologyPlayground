#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 HeightfieldSpec spec;spec.width=2;spec.height=2;spec.cells=16;spec.size=2;spec.relief=.5;spec.backDepth=.3;
 std::vector<std::uint16_t> flat(4,32768),slope{0,65535,0,65535};
 for(auto samples:{flat,slope}){auto e=heightfieldSolid(samples,spec);auto report=validate(e.mesh,spec.size);CHECK(report.accepted);CHECK(report.components==1);CHECK(std::abs(report.signedVolume-2.2)<1e-8);CHECK(e.mesh.positions==heightfieldSolid(samples,spec).mesh.positions);CHECK(e.mesh.cornerNormals.size()==e.mesh.indices.size());}
 auto procedural=proceduralCliffHeightSamples(64,64,48);CHECK(procedural.size()==4096);CHECK(procedural==proceduralCliffHeightSamples(64,64,48));CHECK(procedural!=proceduralCliffHeightSamples(64,64,117));auto limits=std::minmax_element(procedural.begin(),procedural.end());CHECK(int(*limits.second)-int(*limits.first)>30000);
 HeightfieldSpec generated;generated.width=64;generated.height=64;generated.cells=64;CHECK(validate(heightfieldSolid(procedural,generated).mesh,generated.size).accepted);
 auto twelve=proceduralCliffHeightSamples(64,64,48,12);CHECK(twelve!=procedural);CHECK(twelve==proceduralCliffHeightSamples(64,64,48,12));CHECK(validate(heightfieldSolid(twelve,generated).mesh,generated.size).accepted);
 auto fragmented=proceduralCliffHeightSamples(64,64,48,12,true);CHECK(fragmented==proceduralCliffHeightSamples(64,64,48,12,true));std::size_t changed=0;for(std::size_t i=0;i<twelve.size();i++)changed+=twelve[i]!=fragmented[i];CHECK(changed>0);CHECK(changed<twelve.size()*.65);CHECK(validate(heightfieldSolid(fragmented,generated).mesh,generated.size).accepted);
 HeightfieldSpec alias;alias.width=1024;alias.height=2;alias.cells=128;std::vector<std::uint16_t> alternating(2048);for(unsigned i=0;i<2048;i++)alternating[i]=i%2?65535:0;auto antiAliased=heightfieldSolid(alternating,alias);double lo=1,hi=0;for(unsigned y=0;y<=128;y++)for(unsigned x=1;x<128;x++){double z=antiAliased.mesh.positions[y*129+x].z;lo=std::min(lo,z);hi=std::max(hi,z);}CHECK(hi-lo<.002);
 bool rejected=false;try{heightfieldSolid({0,1},spec);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
 spec.cells=0;rejected=false;try{heightfieldSolid(flat,spec);}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
 HeightfieldRecipe recipe;recipe.width=64;recipe.height=64;recipe.cells=32;
 auto stable=proceduralHeightfield(recipe);CHECK(stable.samples==proceduralHeightfield(recipe).samples);CHECK(stable.clippedLow==0&&stable.clippedHigh==0);
 auto altered=recipe;altered.fractures=0;auto unfractured=proceduralHeightfield(altered);std::size_t same=0;for(std::size_t i=0;i<stable.samples.size();i++)same+=stable.samples[i]==unfractured.samples[i];CHECK(same>stable.samples.size()/4);CHECK(same<stable.samples.size());
 for(int control=0;control<5;control++){altered=recipe;if(control==0)altered.seed++;if(control==1)altered.bedCount=6;if(control==2)altered.thicknessVariation=0;if(control==3)altered.tiltDegrees=20;if(control==4)altered.irregularity=0;CHECK(proceduralHeightfield(altered).samples!=stable.samples);CHECK(generateHeightfield(altered).mesh.positions!=generateHeightfield(recipe).mesh.positions);}
 auto stableMesh=generateHeightfield(recipe);altered=recipe;altered.relief*=.5;auto half=generateHeightfield(altered);CHECK(proceduralHeightfield(altered).samples==stable.samples);for(std::size_t i=0;i<(recipe.cells+1)*(recipe.cells+1);i++){CHECK(stableMesh.mesh.positions[i].x==half.mesh.positions[i].x);CHECK(stableMesh.mesh.positions[i].y==half.mesh.positions[i].y);CHECK(stableMesh.mesh.positions[i].z*.5==half.mesh.positions[i].z);}
 CHECK(validate(stableMesh.mesh,recipe.size).accepted);
 for(int control=0;control<7;control++){altered=recipe;if(control==0)altered.tiltDegrees=46;if(control==1)altered.fractures=-1;if(control==2)altered.irregularity=2;if(control==3)altered.thicknessVariation=2;if(control==4)altered.relief=NAN;if(control==5)altered.bedCount=25;if(control==6)altered.cells=257;bool bad=false;try{proceduralHeightfield(altered);}catch(const std::invalid_argument&){bad=true;}CHECK(bad);}
 altered=recipe;altered.fractures=0;auto fractureMesh=generateHeightfield(altered);std::size_t unchangedVertices=0;for(std::size_t i=0;i<stableMesh.mesh.positions.size();i++)unchangedVertices+=stableMesh.mesh.positions[i]==fractureMesh.mesh.positions[i];CHECK(unchangedVertices>stableMesh.mesh.positions.size()/4);CHECK(unchangedVertices<stableMesh.mesh.positions.size());
 // Frozen legacy uint16 hashes independently built from checkpoint af7c823.
 const std::uint64_t legacyHashes[]={1198471063844732077ULL,17737045475368427179ULL,394923978403433414ULL,3484276115356407293ULL,3958080576092066854ULL,16452679926296703924ULL,9953843331803197921ULL,13792105934897921424ULL,1458111521632048090ULL,10555013151765122550ULL,15060227053902125436ULL,5577754866052316290ULL};
 unsigned hashIndex=0;for(unsigned seed:{48,117,305})for(unsigned count:{5,12})for(bool fragments:{false,true}){std::uint64_t hash=1469598103934665603ULL;for(auto value:proceduralCliffHeightSamples(64,64,seed,count,fragments)){hash^=value;hash*=1099511628211ULL;}CHECK(hash==legacyHashes[hashIndex++]);}
 // Fixed mapping must not stretch constant or narrow-range inputs.
 HeightfieldSpec fixed;fixed.width=2;fixed.height=2;fixed.cells=4;fixed.fixedMapping=true;auto constant=heightfieldSolid({49151,49151,49151,49151},fixed);CHECK(std::abs(constant.mesh.positions[0].z-fixed.relief*49151/65535.)<1e-14);
 // Size changes only XY. Backing changes only back Z. Neither rerolls sample heights.
 altered=recipe;altered.size=3;auto larger=generateHeightfield(altered);CHECK(proceduralHeightfield(altered).samples==stable.samples);for(std::size_t i=0;i<stableMesh.mesh.positions.size();i++){CHECK(std::abs(larger.mesh.positions[i].x-1.5*stableMesh.mesh.positions[i].x)<1e-14);CHECK(std::abs(larger.mesh.positions[i].y-1.5*stableMesh.mesh.positions[i].y)<1e-14);CHECK(larger.mesh.positions[i].z==stableMesh.mesh.positions[i].z);}
 altered=recipe;altered.backDepth=.5;auto backed=generateHeightfield(altered);for(std::size_t i=0;i<(recipe.cells+1)*(recipe.cells+1);i++)CHECK(backed.mesh.positions[i]==stableMesh.mesh.positions[i]);CHECK(backed.mesh.positions.back().z==-.5);
 altered=recipe;altered.cells=16;CHECK(proceduralHeightfield(altered).samples==stable.samples);CHECK(generateHeightfield(altered).mesh.positions.size()!=stableMesh.mesh.positions.size());
 for(unsigned seed:{0U,4294967295U})for(double tilt:{-45.,45.}){altered=recipe;altered.seed=seed;altered.tiltDegrees=tilt;CHECK(validate(generateHeightfield(altered).mesh,altered.size).accepted);}
 std::cout<<"heightfield closed slab, analytic volume, determinism and invalid input PASS\n";
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
