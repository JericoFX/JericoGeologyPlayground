#include "../core/src/surface_families.cpp"
#include <iostream>
#include <stdexcept>
using namespace rocks;
#define CHECK(x) do { if(!(x)) throw std::runtime_error(#x); } while(0)
// A step at either support endpoint or plateau shoulder must fail the quadratic
// convergence check. Test the real scalar path with two sites, so relevance=1.
int main(){try{
 double previous=-1;
 for(unsigned seed:{0U,48U,117U,305U,4294967295U}){
  HeightfieldRecipe r;r.profile="slate_foliated";r.seed=seed;r.size=.64;r.layerSpacingMeters=.075;r.plateLengthMeters=.28;r.fractures=.7;
  SlateStudy original(r),one(r);auto j=original.faces.joints.front();
  for(auto candidate:original.faces.joints)if(smooth((r.fractures-.75*random(r.seed,candidate.key+23500))/.25)>.8){j=candidate;break;}
  one.faces.sites={original.faces.sites[j.a],original.faces.sites[j.b]};j.a=0;j.b=1;one.faces.joints={j};
  double center=r.size*(random(r.seed,j.key+23700)-.5),length=r.plateLengthMeters;
  auto value=[&](double t){double along=center+t,x=-j.nx*j.offset-j.ny*along,y=-j.ny*j.offset+j.nx*along;CHECK(one.faces.relevance(j,one.faces.query(x,y))==1);return one.incision(x,y);};
  double peak=value(0);CHECK(peak>.025);CHECK(value(-length)==0&&value(length)==0);
  double span[2],signature=0;
  for(int side:{-1,1}){
   double inside=0,outside=length;
   for(int k=0;k<60;k++){double middle=(inside+outside)*.5;if(value(side*middle)>0)inside=middle;else outside=middle;}
   double endpoint=(inside+outside)*.5;span[side==1]=endpoint;
   double h=length*1e-3,full=value(side*(endpoint-h)),half=value(side*(endpoint-h*.5));
   CHECK(full>0);CHECK(half/full>.20&&half/full<.30);
   // Find the plateau shoulder independently of the production random extents.
   double flat=0,ramp=endpoint;
   for(int k=0;k<60;k++){double middle=(flat+ramp)*.5;if(peak-value(side*middle)<peak*1e-14)flat=middle;else ramp=middle;}
   double shoulder=(flat+ramp)*.5,drop=peak-value(side*(shoulder+h)),halfDrop=peak-value(side*(shoulder+h*.5));
   CHECK(drop>0);CHECK(halfDrop/drop>.20&&halfDrop/drop<.30);
   CHECK(value(side*(endpoint+h))==0);
   for(int k=0;k<=100;k++){double t=side*length*k/100.;double a=value(t);CHECK(a==value(t));CHECK(a>=0&&a<=peak*(1+1e-14));signature+=(k+1)*a;}
  }
  CHECK(span[0]+span[1]<length);CHECK(std::abs(span[1]-span[0])>length*.02);
  CHECK(signature!=previous);previous=signature;
  std::cout<<"seed="<<seed<<" controlled support, unequal ends, smooth endpoint/plateau shoulders and exact scalar replay PASS\n";
 }
}catch(std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
