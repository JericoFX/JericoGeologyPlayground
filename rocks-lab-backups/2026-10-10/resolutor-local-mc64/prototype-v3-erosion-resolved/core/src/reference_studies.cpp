// Independent artist-directed mathematical studies. Reference images are never read.
#include "reference_studies.hpp"
#include <array>
#include <optional>
#include <stdexcept>
namespace rocks { namespace {
constexpr double pi=3.14159265358979323846;
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
double rnd(unsigned seed,unsigned key){return double(hash(seed+0x9e3779b9U*key))/4294967295.;}
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
double noise(double x,double y,unsigned seed){int ix=int(std::floor(x)),iy=int(std::floor(y));double fx=smooth(x-ix),fy=smooth(y-iy),v=0;for(int i=0;i<2;i++)for(int j=0;j<2;j++)v+=(2*rnd(seed,hash(unsigned(ix+i))^hash(unsigned(iy+j)*277U))-1)*(i?fx:1-fx)*(j?fy:1-fy);return v;}
double wrap(double a){return std::remainder(a,2*pi);}
double matrix(double x,double y,const HeightfieldRecipe&r){return .15+r.irregularity*(.027*noise(x/r.size*11,y/r.size*11,r.seed+42000)+.022*noise(x/r.size*91,y/r.size*91,r.seed+42001)+.010*noise(x/r.size*157,y/r.size*157,r.seed+42002));}
struct Lobe {double x,y,angle,aspect,radius,extent,phase,height;unsigned id;};
struct Lava {
 const HeightfieldRecipe&r;std::vector<Lobe> lobes;
 explicit Lava(const HeightfieldRecipe&v):r(v){
  // Independent lobes occupy irregular overlapping regions, not a global stripe field.
  const std::array<std::array<double,2>,9> centers{{{-.38,.30},{.11,.31},{.45,.12},{-.23,-.08},{.25,-.31},{-.46,-.42},{-.04,-.49},{.45,.45},{.05,.02}}};
  for(unsigned k=0;k<r.lobeCount;k++){auto p=centers[k%centers.size()];lobes.push_back({r.size*(p[0]+.09*(rnd(r.seed,43000+k)-.5)),r.size*(p[1]+.09*(rnd(r.seed,43100+k)-.5)),std::atan2(-p[1],-p[0])+.95*(rnd(r.seed,43200+k)-.5),.66+.49*rnd(r.seed,43300+k),r.lobeRadiusMeters*(.76+.50*rnd(r.seed,43400+k)),2.25+.65*rnd(r.seed,43500+k),2*pi*rnd(r.seed,43600+k),.20+.11*rnd(r.seed,43700+k),k});}
 }
 double operator()(double x,double y)const{
  double base=matrix(x,y,r),top=base;
  for(const auto&l:lobes){double dx=x-l.x,dy=y-l.y,cs=std::cos(l.angle),sn=std::sin(l.angle),u=cs*dx+sn*dy,v=(-sn*dx+cs*dy)/l.aspect;
   double rho=std::hypot(u,v);if(rho>l.radius*1.27)continue;
   double theta=std::atan2(v,u),angular=wrap(theta),end=smooth((l.extent*.5-std::abs(angular))/.23);if(end==0)continue;
   double warped=rho+r.irregularity*(.055*l.radius*std::sin(2.2*theta+l.phase)+.025*l.radius*std::sin(5.1*theta-l.phase));
   double fade=smooth((warped-r.ridgeSpacingMeters*.75)/(r.ridgeSpacingMeters*.55))*smooth((l.radius-warped)/(r.ridgeSpacingMeters*.8));if(fade==0)continue;
   double local=base+.065*end*fade;
   int near=int(std::floor(warped/r.ridgeSpacingMeters));
   for(int k=near-1;k<=near+1;k++){if(k<1)continue;unsigned key=l.id*101+unsigned(k);
    double rad=(k+.50*(rnd(r.seed,44000+key)-.5))*r.ridgeSpacingMeters;
    double width=r.ridgeWidthMeters*(.83+.34*rnd(r.seed,44100+key))*(1+r.irregularity*(.25*std::sin(5*theta+l.phase+k)+.30*noise(x/(r.ridgeSpacingMeters*.8),y/(r.ridgeSpacingMeters*.8),r.seed+44700+key)));
    double displacement=r.irregularity*r.ridgeSpacingMeters*.10*noise(x/(r.ridgeSpacingMeters*1.2),y/(r.ridgeSpacingMeters*1.2),r.seed+44800+key);
    double d=(warped-rad-displacement)/width;if(std::abs(d)>=1)continue;
    double arch=std::pow(std::max(0.,1-d*d),.62);
    // Short cross-fractures follow arc length and terminate at this single cord.
    double arc=theta*rad,period=r.ridgeSpacingMeters*(.78+.70*rnd(r.seed,44200+key)),crack=0;
    int seg=int(std::floor(arc/period));
    for(int j=seg-1;j<=seg+1;j++){unsigned ck=hash(key*119U+unsigned(j));double center=(j+.18+.64*rnd(r.seed,44300+ck))*period;double cw=std::max(r.size/r.cells*1.15,width*.21);double q=std::abs(arc-center)/cw;crack=std::max(crack,(.2+.8*rnd(r.seed,44400+ck))*std::max(0.,1-q));}
    double crest=l.height*(.83+.20*rnd(r.seed,44500+key))*(1+r.irregularity*.30*noise(x/(width*2),y/(width*2),r.seed+44550+key))*arch*(1-r.fractures*.70*crack);
    crest+=r.irregularity*.035*noise(x/width*1.8,y/width*1.8,r.seed+44600+key)*arch;
    local=std::max(local,base+(crest+.035)*end*fade);
   }
   top=std::max(top,local);
  }
  return base+r.jointDepth*(top-base);
 }
};
struct Plane{double nx,ny,offset,slope;};
struct Fragment{double x,y,rx,ry,angle,height,tiltX,tiltY;std::vector<Plane> sides;};
struct Scree{
 const HeightfieldRecipe&r;std::vector<Fragment> stones;
 explicit Scree(const HeightfieldRecipe&v):r(v){for(unsigned k=0;k<r.fragmentCount;k++){
  double tier=k<20?1.:(k<65?.43:.22),radius=r.fragmentScaleMeters*tier*(.61+.66*rnd(r.seed,45000+k));
  Fragment f{r.size*1.04*(rnd(r.seed,45100+k)-.5),r.size*1.04*(rnd(r.seed,45200+k)-.5),radius,radius*(.55+.72*rnd(r.seed,45300+k)),2*pi*rnd(r.seed,45400+k),(.17+.31*rnd(r.seed,45500+k))*std::pow(tier,.63),.16*(rnd(r.seed,45600+k)-.5),.16*(rnd(r.seed,45700+k)-.5),{}};
  unsigned faces=4+unsigned(3*rnd(r.seed,45800+k));for(unsigned j=0;j<faces;j++){double a=2*pi*(j+.20*(rnd(r.seed,46000+k*7+j)-.5))/faces;f.sides.push_back({std::cos(a),std::sin(a),.72+.28*rnd(r.seed,47000+k*7+j),.58+.72*rnd(r.seed,48000+k*7+j)});}stones.push_back(f);
 }}
 double operator()(double x,double y)const{double base=matrix(x,y,r),top=base;
  for(const auto&f:stones){double dx=x-f.x,dy=y-f.y;if(std::abs(dx)>std::max(f.rx,f.ry)*2.2||std::abs(dy)>std::max(f.rx,f.ry)*2.2)continue;double c=std::cos(f.angle),s=std::sin(f.angle),u=(c*dx+s*dy)/f.rx,v=(-s*dx+c*dy)/f.ry;
   double cap=f.height+f.tiltX*u+f.tiltY*v;
   // Convex intersections of unequal planes give broad sloping faces and angular rims.
   for(const auto&p:f.sides)cap=std::min(cap,(p.offset-p.nx*u-p.ny*v)*p.slope);
   if(cap<=0)continue;
   double chipped=cap+r.irregularity*.017*noise(x/r.size*83,y/r.size*83,r.seed+49000)-r.fractures*.022*std::max(0.,1-std::abs(v+.38*u-.31)/.10)*smooth((u-.12)/.2)*smooth((.88-u)/.2);
   top=std::max(top,base+std::max(0.,chipped));
  }return base+r.jointDepth*(top-base);
 }
};
}
bool isReferenceStudy(const std::string&p){return p=="lava_rope_lobes"||p=="angular_scree";}
void validateReferenceStudy(const HeightfieldRecipe&r){
 auto range=[](double x,double a,double b){return std::isfinite(x)&&x>=a&&x<=b;};double pitch=r.size/r.cells;
 if(r.width-1<4*r.cells||r.height-1<4*r.cells)throw std::invalid_argument("reference study source needs four intervals per mesh cell");
 if(r.profile=="lava_rope_lobes"&&(!range(r.ridgeWidthMeters,2*pitch,.15*r.size)||!range(r.ridgeSpacingMeters,2.1*r.ridgeWidthMeters,.25*r.size)||!range(r.lobeRadiusMeters,3*r.ridgeSpacingMeters,.65*r.size)||r.lobeCount<3||r.lobeCount>9))throw std::invalid_argument("lava features invalid or underresolved");
 if(r.profile=="angular_scree"&&(!range(r.fragmentScaleMeters,10*pitch,.28*r.size)||r.fragmentCount<20||r.fragmentCount>180))throw std::invalid_argument("scree features invalid or underresolved");
}
HeightfieldSamples referenceStudySamples(const HeightfieldRecipe&r){
 HeightfieldSamples out;out.samples.reserve(std::size_t(r.width)*r.height);std::optional<Lava> lava;std::optional<Scree> scree;if(r.profile=="lava_rope_lobes")lava.emplace(r);else scree.emplace(r);double a=r.directionDegrees*pi/180,c=std::cos(a),s=std::sin(a);
 for(unsigned y=0;y<r.height;y++)for(unsigned x=0;x<r.width;x++){double px=r.size*(double(x)/(r.width-1)-.5),py=r.size*(.5-double(y)/(r.height-1)),u=c*px-s*py,v=s*px+c*py,h=r.profile=="lava_rope_lobes"?(*lava)(u,v):(*scree)(u,v);out.rawMin=std::min(out.rawMin,h);out.rawMax=std::max(out.rawMax,h);out.clippedLow+=h<0;out.clippedHigh+=h>1;out.samples.push_back(std::uint16_t(std::llround(std::clamp(h,0.,1.)*65535)));}
 return out;
}
}
