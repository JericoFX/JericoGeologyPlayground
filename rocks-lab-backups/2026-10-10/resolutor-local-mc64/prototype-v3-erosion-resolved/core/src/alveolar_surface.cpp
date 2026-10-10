// Compact blind cavity patches guided only by the inherited mathematical host.
// Artist-directed morphology, not a geological solver or image sampler.
#include "alveolar_surface.hpp"
#include <array>
#include <limits>
#include <stdexcept>
namespace rocks {
namespace {
constexpr double pi=3.14159265358979323846;
double smooth(double t){t=std::clamp(t,0.,1.);return t*t*(3-2*t);}
std::uint32_t hash(std::uint32_t x){x^=x>>16;x*=0x7feb352dU;x^=x>>15;x*=0x846ca68bU;x^=x>>16;return x;}
double random(std::uint32_t seed,unsigned key){return double(hash(seed+0x9e3779b9U*key))/4294967295.;}
// Lower envelope of unit-curvature parabolas, exact squared Euclidean transform.
std::vector<double> distance1D(const std::vector<double>&f){
 const int n=int(f.size());std::vector<int> v(n);std::vector<double> z(n+1),d(n);int k=0;v[0]=0;z[0]=-std::numeric_limits<double>::infinity();z[1]=std::numeric_limits<double>::infinity();
 for(int q=1;q<n;q++){
  double s;while(true){int j=v[k];s=((f[q]+double(q)*q)-(f[j]+double(j)*j))/(2.*(q-j));if(s>z[k]||k==0)break;k--;}
  k++;v[k]=q;z[k]=s;z[k+1]=std::numeric_limits<double>::infinity();
 }
 k=0;for(int q=0;q<n;q++){while(z[k+1]<q)k++;double dx=q-v[k];d[q]=dx*dx+f[v[k]];}return d;
}
}
LocalPlane fitAlveolarPlane(const std::vector<double>&h,unsigned n,unsigned x,unsigned y,unsigned axial,unsigned diagonal){
 if(n<2||h.size()!=std::size_t(n)*n||axial==0||diagonal==0||x<std::max(axial,diagonal)||y<std::max(axial,diagonal)||x+std::max(axial,diagonal)>=n||y+std::max(axial,diagonal)>=n)throw std::invalid_argument("invalid local-plane stencil");
 const int a=int(axial),b=int(diagonal);const std::array<std::pair<int,int>,9> offsets={{{0,0},{a,0},{-a,0},{0,a},{0,-a},{b,b},{b,-b},{-b,b},{-b,-b}}};
 LocalPlane p;double xx=0,yy=0;
 for(auto[dx,dy]:offsets){double u=double(dx)/(n-1),v=double(dy)/(n-1),z=h[std::size_t(int(y)+dy)*n+int(x)+dx];p.intercept+=z/9.;p.slopeX+=u*z;p.slopeY+=v*z;xx+=u*u;yy+=v*v;}
 p.slopeX/=xx;p.slopeY/=yy;
 for(auto[dx,dy]:offsets){double z=h[std::size_t(int(y)+dy)*n+int(x)+dx],error=z-p.intercept-p.slopeX*dx/(n-1.)-p.slopeY*dy/(n-1.);p.residual+=error*error/9.;}
 p.residual=std::sqrt(p.residual);return p;
}
std::vector<double> euclideanProtectedDistance(const std::vector<unsigned char>&p,unsigned w,unsigned h,double pitch){
 if(!w||!h||p.size()!=std::size_t(w)*h||!std::isfinite(pitch)||pitch<=0)throw std::invalid_argument("invalid distance grid");
 if(std::none_of(p.begin(),p.end(),[](unsigned char v){return v!=0;}))return std::vector<double>(p.size(),std::numeric_limits<double>::infinity());
 double far=double(w)*w+double(h)*h+1;std::vector<double> tmp(p.size()),out(p.size()),line(w);
 for(unsigned y=0;y<h;y++){for(unsigned x=0;x<w;x++)line[x]=p[y*w+x]?0:far;auto d=distance1D(line);for(unsigned x=0;x<w;x++)tmp[y*w+x]=d[x];}
 line.resize(h);for(unsigned x=0;x<w;x++){for(unsigned y=0;y<h;y++)line[y]=tmp[y*w+x];auto d=distance1D(line);for(unsigned y=0;y<h;y++)out[y*w+x]=std::sqrt(d[y])*pitch;}return out;
}
AlveolarGuide makeAlveolarGuide(const std::vector<double>&h,unsigned n,double size,unsigned axial,unsigned diagonal){
 if(n<2||h.size()!=std::size_t(n)*n||!std::isfinite(size)||size<=0||axial==0||diagonal==0||2*std::max(axial,diagonal)>=n)throw std::invalid_argument("invalid guide dimensions");
 AlveolarGuide g;g.side=n;g.size=size;g.protectedNodes.assign(h.size(),1);g.residual.assign(h.size(),0);g.slope.assign(h.size(),0);g.mask.resize(h.size());unsigned margin=std::max(axial,diagonal);
 for(unsigned y=margin;y<n-margin;y++)for(unsigned x=margin;x<n-margin;x++){
  auto p=fitAlveolarPlane(h,n,x,y,axial,diagonal);auto k=std::size_t(y)*n+x;g.residual[k]=p.residual;g.slope[k]=std::hypot(p.slopeX,p.slopeY);g.protectedNodes[k]=(p.residual>.012||g.slope[k]>1.8||h[k]<.35);
 }
 g.distance=euclideanProtectedDistance(g.protectedNodes,n,n,size/(n-1.));for(std::size_t k=0;k<h.size();k++)g.mask[k]=smooth((g.distance[k]-.015*size)/(.010*size));return g;
}
double alveolarMaskAt(const AlveolarGuide&g,double x,double y){
 if(!std::isfinite(x)||!std::isfinite(y)||g.side<2||g.mask.size()!=std::size_t(g.side)*g.side||!std::isfinite(g.size)||g.size<=0)throw std::invalid_argument("invalid mask query");
 if(std::abs(x)>g.size*.5||std::abs(y)>g.size*.5)return 0;
 double u=std::clamp((x/g.size+.5)*(g.side-1),0.,double(g.side-1)),v=std::clamp((.5-y/g.size)*(g.side-1),0.,double(g.side-1));unsigned i=unsigned(u),j=unsigned(v),ii=std::min(i+1,g.side-1),jj=std::min(j+1,g.side-1);double a=u-i,b=v-j;
 return (1-b)*((1-a)*g.mask[j*g.side+i]+a*g.mask[j*g.side+ii])+b*((1-a)*g.mask[jj*g.side+i]+a*g.mask[jj*g.side+ii]);
}
HeightfieldRecipe alveolarHostRecipe(const HeightfieldRecipe&r){auto host=r;host.profile="massive_oblique";host.blockScale=.288;host.jointSpacing=.16;host.jointDepth=.15;host.fractures=r.fractures*.25;return host;}
void validateAlveolarRecipe(const HeightfieldRecipe&r){
 auto positive=[](double x){return std::isfinite(x)&&x>0;};
 if(r.width-1<4*r.cells||r.height-1<4*r.cells)throw std::invalid_argument("alveolar source requires at least four sampling intervals per mesh cell");
 if(r.patchCount>8||!positive(r.patchRadiusMeters)||!positive(r.alveolusRadiusMeters)||r.patchRadiusMeters<3*r.alveolusRadiusMeters||r.patchRadiusMeters>.4*r.size)throw std::invalid_argument("invalid alveolar count or compact patch supports");
 double pitch=std::max(r.size/r.cells,r.size/(std::min(r.width,r.height)-1.));
 if(.54*r.alveolusRadiusMeters<4*pitch)throw std::invalid_argument("alveolar minimum realized half-support requires four mesh cells");
}
AlveolarStudy makeAlveolarStudy(const HeightfieldRecipe&r){
 validateHeightfieldRecipe(r);AlveolarStudy study;auto guideRecipe=alveolarHostRecipe(r);guideRecipe.width=guideRecipe.height=513;auto codes=proceduralHeightfield(guideRecipe);std::vector<double> h;h.reserve(codes.samples.size());for(auto v:codes.samples)h.push_back(v/65535.);study.guide=makeAlveolarGuide(h,513,r.size);
 for(unsigned candidate=0;candidate<2048&&study.patches.size()<r.patchCount;candidate++){
  unsigned key=hash(candidate+41000);double x=r.size*(random(r.seed,key)-.5),y=r.size*(random(r.seed,key+1)-.5);if(alveolarMaskAt(study.guide,x,y)<.95)continue;
  bool separated=true;for(const auto&p:study.patches)if(std::hypot(x-p.x,y-p.y)<1.4*r.patchRadiusMeters){separated=false;break;}if(!separated)continue;
  AlveolarPatch p{x,y,r.patchRadiusMeters,.8+.45*random(r.seed,key+2),2*pi*random(r.seed,key+3),{}};
  unsigned count=7+unsigned(5*random(r.seed,key+4));count=std::min(count,11U);
  for(unsigned i=0;i<count;i++){
   unsigned c=hash(key+i*37+42000);double angle=2*pi*random(r.seed,c),radial=p.radius*std::sqrt(random(r.seed,c+1))*.60,dx=radial*std::cos(angle),dy=radial*std::sin(angle)*p.aspect,cs=std::cos(p.angle),sn=std::sin(p.angle);
   p.cavities.push_back({x+cs*dx-sn*dy,y+sn*dx+cs*dy,r.alveolusRadiusMeters*(.8+.45*random(r.seed,c+2)),.75+.65*random(r.seed,c+3),2*pi*random(r.seed,c+4),2.2+1.6*random(r.seed,c+5),.15+.20*random(r.seed,c+6),.45+.55*random(r.seed,c+7),.9+.2*random(r.seed,c+8)});
  }
  study.patches.push_back(p);
 }
 return study;
}
double alveolarCoverage(const AlveolarStudy&s,double x,double y){
 double outside=1;
 for(const auto&p:s.patches){double dx=x-p.x,dy=y-p.y,cs=std::cos(p.angle),sn=std::sin(p.angle),u=(cs*dx+sn*dy)/p.radius,v=(-sn*dx+cs*dy)/(p.radius*p.aspect),rho=std::hypot(u,v);if(rho>=1)continue;
  double envelope=1-smooth((rho-.55)/.45),uncut=1;
  for(const auto&c:p.cavities){dx=x-c.x;dy=y-c.y;cs=std::cos(c.angle);sn=std::sin(c.angle);u=(cs*dx+sn*dy)/c.radius;v=(-sn*dx+cs*dy)/(c.radius*c.aspect);if(u<0)u/=c.left;
   rho=std::pow(std::pow(std::abs(u),c.exponent)+std::pow(std::abs(v),c.exponent),1/c.exponent);if(rho>=1)continue;double phi=1-smooth((rho-c.floor)/(1-c.floor));uncut*=1-c.amplitude*phi;
  }
  outside*=1-envelope*(.15+.85*(1-uncut));
 }
 return 1-outside;
}
double alveolarCut(const HeightfieldRecipe&r,const AlveolarStudy&s,double x,double y,double host){
 if(r.jointDepth==0||r.patchCount==0)return 0;
 double mask=alveolarMaskAt(s.guide,x,y);if(mask==0)return 0;
 return std::min(.075*r.jointDepth*mask*alveolarCoverage(s,x,y),std::max(0.,host-.10));
}
HeightfieldSamples alveolarSamples(const HeightfieldRecipe&r,const HeightfieldSamples&host,const AlveolarStudy&s){
 if(host.samples.size()!=std::size_t(r.width)*r.height)throw std::invalid_argument("alveolar host dimensions mismatch");
 if(r.jointDepth==0||r.patchCount==0)return host;
 HeightfieldSamples out;out.samples.reserve(host.samples.size());
 for(unsigned y=0;y<r.height;y++)for(unsigned x=0;x<r.width;x++){
  auto code=host.samples[std::size_t(y)*r.width+x];double base=code/65535.,cut=alveolarCut(r,s,r.size*(double(x)/(r.width-1)-.5),r.size*(.5-double(y)/(r.height-1)),base),height=base-cut;
  out.rawMin=std::min(out.rawMin,height);out.rawMax=std::max(out.rawMax,height);out.clippedLow+=height<0;out.clippedHigh+=height>1;out.samples.push_back(cut==0?code:std::uint16_t(std::llround(height*65535.)));
 }
 return out;
}
}
