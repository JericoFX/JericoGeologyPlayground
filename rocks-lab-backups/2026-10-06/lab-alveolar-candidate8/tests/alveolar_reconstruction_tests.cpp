#include "../core/src/alveolar_surface.hpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
int main(){try{
 HeightfieldRecipe r;r.profile="alveolar_blocks";r.seed=48;r.cells=256;r.width=1026;r.height=1025;r.size=2;r.relief=.55;r.backDepth=.3;r.jointDepth=.85;r.irregularity=.65;r.fractures=.7;
 auto s=makeAlveolarStudy(r);auto host=proceduralHeightfield(alveolarHostRecipe(r));auto a=alveolarSamples(r,host,s);auto spec=HeightfieldSpec{r.width,r.height,r.cells,r.size,r.relief,r.backDepth,true};auto hm=heightfieldSolid(host.samples,spec).mesh,em=heightfieldSolid(a.samples,spec).mesh;
 unsigned geometricProtected=0,geometricOnlyChanged=0,reconstructionProtected=0,reconstructionChanged=0;double geometricMax=0;
 for(unsigned y=0;y<=r.cells;y++)for(unsigned x=0;x<=r.cells;x++){
  // The 513 guide aligns with this physical box, but the 1026 source does not.
  bool boxZero=true;for(int gy=std::max(0,2*int(r.cells-y)-1);gy<=std::min(512,2*int(r.cells-y)+1);gy++)for(int gx=std::max(0,2*int(x)-1);gx<=std::min(512,2*int(x)+1);gx++)if(s.guide.mask[gy*513+gx]!=0)boxZero=false;
  double sx=double(x)*(r.width-1)/r.cells,sy=double(r.cells-y)*(r.height-1)/r.cells,halfX=.5*(r.width-1.)/r.cells,halfY=.5*(r.height-1.)/r.cells;
  int x0=int(std::floor(std::max(0.,sx-halfX))),x1=int(std::ceil(std::min(r.width-1.,sx+halfX))),y0=int(std::floor(std::max(0.,sy-halfY))),y1=int(std::ceil(std::min(r.height-1.,sy+halfY)));
  bool reconstructionZero=true;for(int py=y0;py<=y1;py++)for(int px=x0;px<=x1;px++)if(alveolarMaskAt(s.guide,r.size*(double(px)/(r.width-1)-.5),r.size*(.5-double(py)/(r.height-1)))!=0)reconstructionZero=false;
  auto k=y*(r.cells+1)+x;double delta=hm.positions[k].z-em.positions[k].z;
  if(boxZero){geometricProtected++;geometricOnlyChanged+=delta!=0;geometricMax=std::max(geometricMax,delta);}
  if(reconstructionZero){reconstructionProtected++;reconstructionChanged+=delta!=0;CHECK(hm.positions[k]==em.positions[k]);}
 }
 CHECK(geometricOnlyChanged>0);CHECK(reconstructionProtected>0);CHECK(reconstructionChanged==0);
 std::cout<<"nonaligned_source=1026x1025 geometric_box_protected="<<geometricProtected<<" geometric_box_boundary_effects="<<geometricOnlyChanged<<" max_boundary_mm="<<geometricMax*1000<<" complete_reconstruction_protected="<<reconstructionProtected<<" complete_reconstruction_changed="<<reconstructionChanged<<" PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
