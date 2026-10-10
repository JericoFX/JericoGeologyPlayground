#include "../core/src/alveolar_surface.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do{if(!(x))throw std::runtime_error(#x);}while(0)
HeightfieldRecipe recipe(){HeightfieldRecipe r;r.profile="alveolar_blocks";r.cells=256;r.width=r.height=1025;r.size=2;r.relief=.55;r.backDepth=.3;r.irregularity=.65;r.fractures=.7;r.jointDepth=.85;return r;}
bool invalid(const HeightfieldRecipe&r){try{validateHeightfieldRecipe(r);return false;}catch(const std::invalid_argument&){return true;}}
int main(){try{
 auto r=recipe();
 for(unsigned seed:{0U,48U,117U,305U,4294967295U}){
  r.seed=seed;auto host=proceduralHeightfield(alveolarHostRecipe(r));auto study=makeAlveolarStudy(r),replay=makeAlveolarStudy(r);CHECK(study.guide.mask==replay.guide.mask);CHECK(study.patches.size()==3);CHECK(study.patches.size()==replay.patches.size());
  for(unsigned i=0;i<study.patches.size();i++){auto&p=study.patches[i];CHECK(p.x==replay.patches[i].x&&p.y==replay.patches[i].y);CHECK(alveolarMaskAt(study.guide,p.x,p.y)>=.95);CHECK(p.cavities.size()>=7&&p.cavities.size()<=11);for(unsigned j=0;j<i;j++)CHECK(std::hypot(p.x-study.patches[j].x,p.y-study.patches[j].y)>=1.4*r.patchRadiusMeters);}
  auto a=alveolarSamples(r,host,study),b=alveolarSamples(r,host,replay);CHECK(a.samples==b.samples);CHECK(a.clippedLow==0&&a.clippedHigh==0);std::uint64_t changed=0,protectedNodes=0;
  for(unsigned y=0;y<r.height;y++)for(unsigned x=0;x<r.width;x++){auto k=std::size_t(y)*r.width+x;double xx=r.size*(double(x)/(r.width-1)-.5),yy=r.size*(.5-double(y)/(r.height-1)),mask=alveolarMaskAt(study.guide,xx,yy),coverage=alveolarCoverage(study,xx,yy);CHECK(a.samples[k]<=host.samples[k]);CHECK(a.samples[k]>=6553);CHECK((host.samples[k]-a.samples[k])/65535.<=.075*r.jointDepth+1./65535.);if(mask*coverage==0)CHECK(a.samples[k]==host.samples[k]);changed+=a.samples[k]!=host.samples[k];protectedNodes+=mask==0;}
  CHECK(changed>0);auto spec=HeightfieldSpec{r.width,r.height,r.cells,r.size,r.relief,r.backDepth,true};auto mesh=heightfieldSolid(a.samples,spec).mesh,hostMesh=heightfieldSolid(host.samples,spec).mesh;auto report=validate(mesh,r.size);CHECK(report.accepted&&report.components==1&&!report.badEdges&&!report.badVertices&&!report.degenerateFaces);
  for(auto v:mesh.normals)CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));for(auto v:mesh.cornerNormals)CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
  std::uint64_t protectedFootprints=0;for(unsigned y=0;y<=r.cells;y++)for(unsigned x=0;x<=r.cells;x++){
   bool allProtected=true;int sx=int(x*4),sy=int((r.cells-y)*4);for(int yy=std::max(0,sy-2);yy<=std::min(1024,sy+2)&&allProtected;yy++)for(int xx=std::max(0,sx-2);xx<=std::min(1024,sx+2);xx++)if(alveolarMaskAt(study.guide,r.size*(xx/1024.-.5),r.size*(.5-yy/1024.))!=0){allProtected=false;break;}
   if(allProtected){protectedFootprints++;CHECK(mesh.positions[y*(r.cells+1)+x]==hostMesh.positions[y*(r.cells+1)+x]);}
  }CHECK(protectedFootprints>0);
  auto zero=r;zero.jointDepth=0;auto z=proceduralHeightfield(zero);CHECK(z.samples==host.samples);auto zm=heightfieldSolid(z.samples,spec).mesh;CHECK(zm.positions==hostMesh.positions&&zm.indices==hostMesh.indices&&zm.normals==hostMesh.normals&&zm.cornerNormals==hostMesh.cornerNormals);
  zero=r;zero.patchCount=0;CHECK(proceduralHeightfield(zero).samples==host.samples);
  CHECK(alveolarCut(r,study,study.patches[0].x,study.patches[0].y,.099)==0);CHECK(alveolarMaskAt(study.guide,100,100)==0);
  std::cout<<"seed="<<seed<<" patches="<<study.patches.size()<<" changed="<<changed<<" protected_samples="<<protectedNodes<<" protected_filter_footprints="<<protectedFootprints<<" topology/replay/zero-identity PASS\n";
 }
 for(int mode=0;mode<10;mode++){auto bad=recipe();switch(mode){case 0:bad.patchCount=9;break;case 1:bad.patchRadiusMeters=.194;break;case 2:bad.alveolusRadiusMeters=.04;break;case 3:bad.patchRadiusMeters=std::numeric_limits<double>::quiet_NaN();break;case 4:bad.alveolusRadiusMeters=std::numeric_limits<double>::infinity();break;case 5:bad.width=1024;break;case 6:bad.cells=128;break;case 7:bad.patchRadiusMeters=0;break;case 8:bad.jointDepth=-1;break;case 9:bad.irregularity=2;break;}CHECK(invalid(bad));}
 auto minimum=recipe();minimum.alveolusRadiusMeters=(4*(minimum.size/minimum.cells))/.54;minimum.patchRadiusMeters=3*minimum.alveolusRadiusMeters;CHECK(!invalid(minimum));minimum.alveolusRadiusMeters=std::nextafter(minimum.alveolusRadiusMeters,0.);CHECK(invalid(minimum));
 for(unsigned seed:{0U,4294967295U})for(double extreme:{0.,1.}){auto e=recipe();e.seed=seed;e.irregularity=e.fractures=e.jointDepth=extreme;e.patchCount=8;e.patchRadiusMeters=.4*e.size;e.alveolusRadiusMeters=e.patchRadiusMeters/3;auto s=proceduralHeightfield(e);CHECK(s.clippedLow==0&&s.clippedHigh==0);CHECK(validate(heightfieldSolid(s.samples,{e.width,e.height,e.cells,e.size,e.relief,e.backDepth,true}).mesh,e.size).accepted);}
 std::cout<<"finite bounds, minimum supports, count cap, extreme seeds/controls PASS\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
