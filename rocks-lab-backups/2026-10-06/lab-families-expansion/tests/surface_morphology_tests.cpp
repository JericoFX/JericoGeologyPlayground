#include "rocks/mesh.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
int main(int argc,char**argv){try{
 std::string profile=argc>1?argv[1]:"slate_foliated";HeightfieldRecipe r;r.profile=profile;r.size=.64;r.relief=.12;r.backDepth=.08;r.width=r.height=513;r.cells=128;r.layerSpacingMeters=.05;r.plateLengthMeters=.34;r.thicknessVariation=.4;r.irregularity=0;r.fractures=0;r.jointDepth=.85;r.directionDegrees=90;
 auto a=proceduralHeightfield(r);double along=0,across=0;for(unsigned y=1;y<r.height;y++)for(unsigned x=1;x<r.width;x++){double dx=int(a.samples[y*r.width+x])-int(a.samples[y*r.width+x-1]),dy=int(a.samples[y*r.width+x])-int(a.samples[(y-1)*r.width+x]);along+=dx*dx;across+=dy*dy;}
 std::cout<<profile<<" along/across gradient energy="<<along/across<<'\n';
 // Removing finite wedge ends degenerates slate into near-continuous ribs.
 // This is a shape regression criterion, not a geological naturalness score.
 std::vector<double> spans;for(unsigned y=r.height/8;y<r.height*7/8;y++){auto first=a.samples.begin()+y*r.width;auto mm=std::minmax_element(first,first+r.width);spans.push_back(double(*mm.second-*mm.first)/65535.);}
 std::sort(spans.begin(),spans.end());double medianSpan=spans[spans.size()/2];std::cout<<"median along-plate scalar span="<<medianSpan<<'\n';
 if(profile=="slate_foliated"&&medianSpan<.12)throw std::runtime_error("slate lacks substantial finite plate-end geometry without optional cracks/noise");
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
