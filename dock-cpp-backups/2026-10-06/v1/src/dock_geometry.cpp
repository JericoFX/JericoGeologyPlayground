#include "jerico/dock_geometry.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>
namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
uint64_t hashid(const std::string&s,int64_t seed){uint64_t h=1469598103934665603ULL^uint64_t(seed);for(unsigned char c:s){h^=c;h*=1099511628211ULL;}return h;}
double unit(uint64_t v){v^=v>>30;v*=0xbf58476d1ce4e5b9ULL;v^=v>>27;v*=0x94d049bb133111ebULL;v^=v>>31;return double(v>>11)/9007199254740992.;}
double taper(double t,double a,double b){if(t<=a||t>=b)return 0;return std::pow(std::sin(pi*(t-a)/(b-a)),1.3);}
}
Json& dock_timber(Builder& builder,const std::string& id,V3 a,V3 b,V3 width_axis,
 double width,double thickness,const std::string& role,int64_t seed,
 bool broken_start,bool broken_end,double wear,bool horizontal_top){
 const double length=std::sqrt(dot(sub(b,a),sub(b,a)));
 if(!std::isfinite(length)||length<.03||!std::isfinite(width)||width<.012||!std::isfinite(thickness)||thickness<.012)throw std::invalid_argument("invalid timber dimensions");
 const auto u=norm(sub(b,a));const auto v=norm(sub(width_axis,mul(u,dot(width_axis,u))));const auto w=norm(cross(u,v));
 auto h=hashid(id,seed);auto random=[&](int n){return unit(h+uint64_t(n)*0x9e3779b97f4a7c15ULL);};
 const double phase=random(1)*2*pi,phase2=random(2)*2*pi;
 const double bevel=std::min({.004,thickness*.12,width*.065});
 const double crackY=(random(5)-.5)*width*.42,crackWidth=std::min(.003+random(6)*.003,width*.045);
 const double crackStart=.05+random(7)*.3,crackEnd=std::min(.97,crackStart+.25+random(8)*.4);
 const double crackDepth=horizontal_top?0:std::min(thickness*.28,.007+random(9)*.011)*wear;
 const int segments=std::clamp(int(std::ceil(length/.16)),8,30);
 Json vertices=Json::array(),faces=Json::array(),profile=Json::array();constexpr int ringSize=11;
 for(int j=0;j<=segments;++j){const double t=double(j)/segments;
  const double chipAt=.12+.76*random(11),chipRadius=.045+.11*random(12);
  const double chip=std::pow(std::max(0.,1-std::abs(t-chipAt)/chipRadius),2)*(.002+.006*random(13))*wear;
  const double left=.5*width-(.0006+.0018*(.5+.5*std::sin(t*2.7*pi+phase)))*wear-chip;
  const double right=.5*width-(.0006+.0022*(.5+.5*std::sin(t*3.8*pi+phase2)))*wear;
  const double bow=horizontal_top?0:std::sin(t*pi)*(.0015+random(14)*.003)*std::sin(phase)*wear;
  const double roll=horizontal_top?0:std::sin(t*1.5*pi+phase2)*.0016*wear;
  const double groove=crackDepth*taper(t,crackStart,crackEnd);
  std::array<V2,ringSize> ring={V2{-left,-thickness*.5+bevel},V2{-left+bevel,-thickness*.5},V2{right-bevel,-thickness*.5},V2{right,-thickness*.5+bevel},V2{right,thickness*.5-bevel},V2{right-bevel,thickness*.5},V2{crackY+crackWidth,thickness*.5},V2{crackY,thickness*.5-groove},V2{crackY-crackWidth,thickness*.5},V2{-left+bevel,thickness*.5},V2{-left,thickness*.5-bevel}};
  profile.push_back({t,left,right,bow,roll,groove});
  for(int k=0;k<ringSize;++k){auto yz=ring[k];double x=length*t;if(j==0||j==segments){bool broken=j==0?broken_start:broken_end;double q=yz[0]/width;double irregular=broken?(.005+.20*std::pow(.5+.5*std::sin(q*13+phase),1.8)+.035*(.5+.5*std::sin(q*29+phase2))):(.001+random(k+90)*.003)*wear;irregular=std::min(irregular,length*.27);x+=(j==0?1:-1)*irregular;}
   yz[1]+=bow+roll*(yz[0]/width);vertices.push_back(add(a,add(mul(u,x),add(mul(v,yz[0]),mul(w,yz[1])))));
  }
 }
 for(int j=0;j<segments;++j)for(int k=0;k<ringSize;++k){int q=(k+1)%ringSize;size_t A=j*ringSize+k,B=j*ringSize+q,C=(j+1)*ringSize+q,D=(j+1)*ringSize+k;faces.push_back({A,B,C});faces.push_back({A,C,D});}
 for(int end:{0,segments}){V3 center{};for(int k=0;k<ringSize;++k)center=add(center,vertices[end*ringSize+k].get<V3>());center=mul(center,1./ringSize);size_t ci=vertices.size();vertices.push_back(center);for(int k=0;k<ringSize;++k){size_t A=end*ringSize+k,B=end*ringSize+(k+1)%ringSize;if(end==0)faces.push_back({ci,B,A});else faces.push_back({ci,A,B});}}
 auto&p=builder.mesh(id,"frame",vertices,faces,"wood",0,role);p["timber_frame"]={{"origin",a},{"u",u},{"v",v},{"w",w}};p["timber_length"]=length;p["timber_width"]=width;p["timber_thickness"]=thickness;p["timber_profile"]=profile;p["grain_axis"]=u;p["fractured"]=broken_start||broken_end;
 p["wood_geometry"]={{"method","seeded_grain_aligned_edge_deformation_and_chamfered_sections"},{"edge_displacement_m_max",.011},{"crack_depth_m",crackDepth},{"crack_interval_fraction",{crackStart,crackEnd}},{"broken_start",broken_start},{"broken_end",broken_end},{"horizontal_top",horizontal_top}};return p;
}
}
