#include "jerico/materials.hpp"
#include "jerico/png.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <cstring>
#include <cctype>
#include <numeric>

namespace jerico {
namespace {
using F3=std::array<float,3>;
float sat(float x){return std::clamp(x,0.f,1.f);} float lerp(float a,float b,float t){return (1-t)*a+t*b;}
V3 blend(V3 a,V3 b,double t){return {a[0]*(1-t)+b[0]*t,a[1]*(1-t)+b[1]*t,a[2]*(1-t)+b[2]*t};}
V3 vadd(V3 a,V3 b){return {a[0]+b[0],a[1]+b[1],a[2]+b[2]};}
V3 vsub(V3 a,V3 b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
V3 vmul(V3 a,double s){return {a[0]*s,a[1]*s,a[2]*s};}
double vdot(V3 a,V3 b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
V3 vcross(V3 a,V3 b){return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};}
V3 unit(V3 a){double n=std::sqrt(vdot(a,a));return n>1e-20?vmul(a,1/n):V3{0,0,1};}
// Adapted from Blender 4.3.2 Cycles util/hash.h (Apache-2.0) and
// kernel/svm/noise.h (BSD-3-Clause), not an alternative noise algorithm.
// Copyright 2009-2010 Sony Pictures Imageworks Inc.; 2011-2022 Blender Foundation.
uint32_t rot(uint32_t x,unsigned k){return (x<<k)|(x>>(32-k));}
uint32_t hash3(uint32_t x,uint32_t y,uint32_t z){
 uint32_t a=0xdeadbeefU+12+13,b=a,c=a;a+=x;b+=y;c+=z;
 c^=b;c-=rot(b,14);a^=c;a-=rot(c,11);b^=a;b-=rot(a,25);c^=b;c-=rot(b,16);a^=c;a-=rot(c,4);b^=a;b-=rot(a,14);c^=b;c-=rot(b,24);return c;
}
float fade(float t){return t*t*t*(t*(t*6-15)+10);}
float grad(uint32_t hash,float x,float y,float z){int h=hash&15;float u=h<8?x:y,v=h<4?y:((h==12||h==14)?x:z);return ((h&1)?-u:u)+((h&2)?-v:v);}
float snoise(F3 p){
 for(float&v:p){float correction=std::abs(v)>=1000000.f?.5f:0;v=std::fmod(v,100000.f)+correction;}
 int X=int(std::floor(p[0])),Y=int(std::floor(p[1])),Z=int(std::floor(p[2]));float fx=p[0]-X,fy=p[1]-Y,fz=p[2]-Z;
 float v[8];for(int i=0;i<8;i++)v[i]=grad(hash3(X+(i&1),Y+((i>>1)&1),Z+((i>>2)&1)),fx-(i&1),fy-((i>>1)&1),fz-((i>>2)&1));
 float x=fade(fx),y=fade(fy),z=fade(fz),x1=1-x,y1=1-y,z1=1-z;
 return .9820f*(z1*(y1*(v[0]*x1+v[1]*x)+y*(v[2]*x1+v[3]*x))+z*(y1*(v[4]*x1+v[5]*x)+y*(v[6]*x1+v[7]*x)));
}
float fbm(F3 p,float detail,float rough){
 float fscale=1,amp=1,maxamp=0,sum=0;
 for(int i=0;i<=int(detail);i++){F3 q={p[0]*fscale,p[1]*fscale,p[2]*fscale};sum+=snoise(q)*amp;maxamp+=amp;amp*=rough;fscale*=2;}
 float rmd=detail-std::floor(detail);if(rmd){float sum2=sum+snoise({p[0]*fscale,p[1]*fscale,p[2]*fscale})*amp;return lerp(.5f*sum/maxamp+.5f,.5f*sum2/(maxamp+amp)+.5f,rmd);}
 return .5f*sum/maxamp+.5f;
}
using Stop=std::pair<double,V3>;
V3 ramp(double f,std::initializer_list<Stop> stops){
 // Blender ColorBand compiles 257 entries (256 intervals); Cycles linearly
 // interpolates that table, including the small smoothing around stop corners.
 auto exact=[&](float x){auto it=stops.begin();if(x<=float(it->first))return it->second;auto prev=it++;for(;it!=stops.end();++it){if(x<=float(it->first)){float t=(x-float(prev->first))/(float(it->first)-float(prev->first));V3 out;for(int c=0;c<3;c++)out[c]=lerp(float(prev->second[c]),float(it->second[c]),t);return out;}prev=it;}return prev->second;};
 float x=sat(float(f))*256;int i=int(x);float t=x-i;V3 a=exact(i/256.f),b=exact(std::min(256,i+1)/256.f),out;for(int c=0;c<3;c++)out[c]=lerp(float(a[c]),float(b[c]),t);return out;
}
float smooth(float x){x=sat(x);return x*x*x*(x*(6*x-15)+10);}
float region(const Json&r,V3 p){if(r.at("shape")=="box"){float d=std::numeric_limits<float>::max();for(int i=0;i<3;i++)d=std::min(d,std::min(float(p[i])-r["min"][i].get<float>(),r["max"][i].get<float>()-float(p[i])));return d;}float d=0;for(int i=0;i<3;i++){float delta=float(p[i])-r["center"][i].get<float>();d+=delta*delta;}return r["radius"].get<float>()-std::sqrt(d);}
float directed_wear(const Json&part,V3 p,bool only_fine=false){
 // FineWear is an adapter profile, not an inference from free R2 metadata.
 // The R2 source preserves unknown config keys but never interprets this one.
 if(only_fine&&part.value("_material_family","")!="fine")return 0;
 std::string side=part.value("_surface_side","");float result=0;
 for(const auto&c:part.value("edge_wear",Json::array())){if(c.value("side","")!=side||(only_fine&&(!c.contains("fine_breakup")||!c["fine_breakup"].is_boolean()||!c["fine_breakup"].get<bool>())))continue;float distance=1e30f;
  for(const auto&seg:c.at("boundary_segments_xz")){float dx=seg[1][0].get<double>()-seg[0][0].get<double>(),dz=seg[1][1].get<double>()-seg[0][1].get<double>(),den=dx*dx+dz*dz;if(den<=1e-20f)continue;float vx=float(p[0])-seg[0][0].get<float>(),vz=float(p[2])-seg[0][1].get<float>(),f=sat((vx*dx+vz*dz)/den),rx=vx-f*dx,rz=vz-f*dz;distance=std::min(distance,std::sqrt(rx*rx+rz*rz));}
  float edge=1-smooth(distance/c.value("width_m",.015f)),phi=region(c.at("region"),p),feather=c.value("feather_m",0.f),mask=feather?smooth(phi/feather):float(phi>=0);
  if(!c.value("override_protection",false))for(const auto&r:c.value("protected_regions",Json::array()))mask*=float(region(r,p)<0);
  result=std::max(result,edge*mask*c.value("amount",1.f));
 }return result;
}
// SHA-256's first word is the stable furniture grain seed used in the source.
uint32_t sha_seed(const std::string&s){
 static constexpr uint32_t k[]={0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2};
 std::vector<uint8_t>d(s.begin(),s.end());uint64_t bits=d.size()*8;d.push_back(0x80);while(d.size()%64!=56)d.push_back(0);for(int i=7;i>=0;i--)d.push_back((bits>>(i*8))&255);
 uint32_t h[]={0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};auto rr=[](uint32_t x,int n){return (x>>n)|(x<<(32-n));};
 for(size_t j=0;j<d.size();j+=64){uint32_t w[64];for(int i=0;i<16;i++)w[i]=uint32_t(d[j+4*i])<<24|uint32_t(d[j+4*i+1])<<16|uint32_t(d[j+4*i+2])<<8|d[j+4*i+3];for(int i=16;i<64;i++)w[i]=w[i-16]+(rr(w[i-15],7)^rr(w[i-15],18)^(w[i-15]>>3))+w[i-7]+(rr(w[i-2],17)^rr(w[i-2],19)^(w[i-2]>>10));uint32_t a=h[0],b=h[1],c=h[2],e=h[4],f=h[5],g=h[6],hh=h[7],dd=h[3];for(int i=0;i<64;i++){uint32_t t=hh+(rr(e,6)^rr(e,11)^rr(e,25))+((e&f)^(~e&g))+k[i]+w[i],u=(rr(a,2)^rr(a,13)^rr(a,22))+((a&b)^(a&c)^(b&c));hh=g;g=f;f=e;e=dd+t;dd=c;c=b;b=a;a=t+u;}h[0]+=a;h[1]+=b;h[2]+=c;h[3]+=dd;h[4]+=e;h[5]+=f;h[6]+=g;h[7]+=hh;}return h[0];
}
}

double blender_noise(V3 position,double scale,double detail,double roughness,V3 axis,V3 offset){F3 p;for(int i=0;i<3;i++)p[i]=(float(position[i])*float(axis[i])+float(offset[i]))*float(scale);return fbm(p,float(detail),float(roughness));}
MaterialSample sample_material(const Json&part,const std::string&material,V3 position){
 V3 p={float(position[0]),float(position[1]),float(position[2])};MaterialSample s;std::string key=material;bool furniture=part.value("_material_family","")=="furniture"||part.contains("grain_axis");
 auto noise=[&](double scale,double detail,double rough,V3 axis=V3{1,1,1},V3 offset=V3{0,0,0}){return float(blender_noise(p,scale,detail,rough,axis,offset));};
 if(part.value("glass_study",false)&&(key=="glass"||key=="glass_edge")){
  s.base_color={.30,.40,.35};s.roughness=.12;s.transmission=.96;s.ior=1.52;s.coat=.12;
  if(key=="glass_edge"){s.base_color={.48,.63,.55};s.roughness=.16;s.transmission=.58;return s;}
  auto b=part.value("glass_bounds",Json({-.591,.591,-.011,1.261}));float x0=b[0],x1=b[1],z0=b[2],z1=b[3];float edge=std::min({float(p[0])-x0,x1-float(p[0]),float(p[2])-z0,z1-float(p[2])});
  float streak=noise(1,3,.60,{47,3,2.2}),dust=sat(1-edge/.085f)*(.38f+streak*.6f),bottom=sat(1-(float(p[2])-z0)/.19f);dust=std::max(dust,bottom*(streak*.55f));
  float scratch=0;
  // Cache only parameter-derived line coefficients, never an input image. This
  // preserves Python Random(203) exactly while avoiding 624-word MT setup at
  // every texel and normal-derivative sample. Each thread has independent state.
  struct ScratchLine{float ax,az,dx,dz,den;};static thread_local std::array<ScratchLine,11> lines;static thread_local std::array<double,4> lastBounds={std::numeric_limits<double>::quiet_NaN(),0,0,0};auto bounds=b.get<std::array<double,4>>();
  if(bounds!=lastBounds){PythonRandom rng(203);for(auto&line:lines){line.ax=rng.uniform(bounds[0]+.04,bounds[1]-.04);line.az=rng.uniform(bounds[2]+.04,bounds[3]-.04);double dx=rng.uniform(-.025,.025),dz=rng.uniform(.015,.075);line.dx=dx;line.dz=dz;line.den=dx*dx+dz*dz;}lastBounds=bounds;}
  for(const auto&line:lines){float vx=float(p[0])-line.ax,vz=float(p[2])-line.az,f=sat((vx*line.dx+vz*line.dz)/line.den),xx=vx-f*line.dx,zz=vz-f*line.dz;scratch=std::max(scratch,sat(1-std::sqrt(xx*xx+zz*zz)/.00024f));}
  s.roughness=.095f+(dust*.30f+scratch*.5f);s.base_color=blend({.30,.40,.35},{.31,.29,.22},dust);s.height=noise(900,2,.6)*dust;s.bump_strength=.18;s.bump_distance=.000025;return s;
 }
 if(key=="painted_steel"||key=="dark_steel"){
  auto data=part.value("weather",Json::object());auto b=data.value("bounds",part.value("panel_bounds",Json({-1,1,-.03,.03,0,2.1})));
  float edge=sat(1-std::min({float(p[0])-b[0].get<float>(),b[1].get<float>()-float(p[0]),float(p[2])-b[4].get<float>(),b[5].get<float>()-float(p[2])})/data.value("edge_width_m",.013f));
  float bottom=sat(1-float(p[2])/data.value("bottom_height_m",.57f));int64_t seed=recipe_seed(data.value("seed",Json(29)));auto mod=[](int64_t x,int n){return (x%n+n)%n;};V3 off={mod(mod(seed,997)-29,997)*.137,mod(mod(seed,991)-29,991)*.071,mod(mod(seed,983)-29,983)*.053};
  float macro=noise(8.2,5,.73,{1,1,1.5},off),meso=noise(51,3,.7,{1,1,1},off),micro=noise(600,2,.68,{1,1,1},off),streak=noise(1,3,.66,{65,55,3.2},off);
  float field=((macro+bottom*.18f)+edge*.13f)+streak*.04f;float mask=ramp(field,{{.58,{0,0,0}},{.65,{1,1,1}}})[0];
  V3 paint=ramp(meso,{{.23,{.010,.012,.012}},{.53,{.021,.025,.024}},{.82,{.039,.045,.042}}});
  if(data.value("finish","weathered_gray")=="weathered_gray")paint=ramp(meso,{{.23,{.052,.059,.057}},{.53,{.082,.090,.086}},{.82,{.12,.129,.123}}});
  if(key=="dark_steel")paint=ramp(meso,{{.2,{.009,.011,.012}},{.8,{.032,.036,.034}}});
  V3 rust=ramp(meso,{{.16,{.012,.008,.005}},{.47,{.040,.023,.012}},{.67,{.10,.049,.018}},{.86,{.17,.084,.034}}});float wear=edge*std::max(0.f,micro-.42f)*.42f;
  wear=std::max(wear,directed_wear(part,p)*.84f);s.base_color=blend(blend(paint,rust,mask),{.068,.073,.068},wear);s.roughness=lerp(.76f,.96f,mask);s.metallic=lerp(.28f,.04f,mask);s.height=micro*.60f+mask*.4f;s.bump_strength=.5;s.bump_distance=.00045;
  if(part.value("_surface_side","")=="front"){
   float edgefine=directed_wear(part,p,true);if(edgefine>0){float modulation=noise(140,2.5,.72,{1,1,1.25})*.76f+noise(1250,2,.73)*.24f,broken=ramp(modulation,{{.40,{.06,.06,.06}},{.53,{1,1,1}}})[0],maskfine=edgefine*broken;s.base_color=blend(s.base_color,{.135,.145,.132},maskfine);s.roughness=lerp(s.roughness,.48f,maskfine);s.metallic=lerp(s.metallic,.70f,maskfine);}
  }return s;
 }
 if(key=="cut_metal"||key=="glass_edge"){s.base_color=key=="cut_metal"?V3{.026,.031,.029}:V3{.18,.25,.23};s.roughness=.88;s.metallic=key=="cut_metal"?.62:0;float n=noise(210,3,.72);if(key=="cut_metal")s.base_color=ramp(n,{{.2,{.012,.016,.017}},{.8,{.051,.058,.054}}});else{s.transmission=.65;s.ior=1.52;}s.height=n;s.bump_strength=.3;s.bump_distance=.0003;return s;}
 if(part.contains("wood_finish")&&(key=="wood"||key=="damp_wood"||key=="wood_exposed"||key=="wood_rough")){
  auto finish=part["wood_finish"];float age=finish["age"],wet=finish["wetness"],ref=.58+double(age)*.29-double(wet)*.35;bool axis=finish["grain_axis"]=="X";
  float fine=noise(1,4,.69,axis?V3{5,460,460}:V3{460,460,5}),grain=noise(1,3,.63,axis?V3{2.3,140,140}:V3{140,140,2.3}),mixed=grain*((1-.28f)+.28f*fine);
  V3 c0=blend({.026,.012,.004},{.013,.011,.0075},age),c1=blend({.082,.038,.012},{.040,.031,.020},age),c2=blend({.17,.093,.038},{.082,.065,.041},age),c3=blend({.25,.148,.066},{.135,.109,.072},age);
  if(key=="wood_exposed"){c0=vmul(c0,1.10);c1=vmul(c1,1.10);c2=vmul(c2,1.10);c3=vmul(c3,1.10);}
  float bottom=1-sat((p[2]+.12f)/.52f),height=finish["height_m"],top=sat((p[2]-(height-.18f))/.22f)*.62f,patch=noise(6.8,3,.64,{1.4,1,.35}),mask=std::max(bottom,top)*lerp(.42f,1,sat((patch-.2f)/.6f)),dark=(1-wet*.20f)-mask*(wet*.65f);
  s.base_color=vmul(ramp(mixed,{{.22,c0},{.38,c1},{.52,c2},{.70,c3}}),dark);float rlo=std::max(.18f,ref-.065f),rhi=std::min(.94f,ref+.08f);s.roughness=ramp(patch,{{.2,{rlo,rlo,rlo}},{.8,{rhi,rhi,rhi}}})[0]-mask*(wet*(.13f+age*.07f));s.height=fine;s.bump_strength=.30+age*.22;s.bump_distance=.00032+age*.00072;return s;
 }
 if(furniture){
  if(key=="wood"||key=="wood_0"||key=="wood_1"||key=="wood_2"){
   V3 original=p;int axis=key=="wood"?2:key.back()-'0';if(part.contains("grain_axis")){auto a=part["grain_axis"].get<V3>();axis=std::max_element(a.begin(),a.end(),[](double x,double y){return std::abs(x)<std::abs(y);})-a.begin();auto seed=sha_seed(part.value("id",""));V3 shift={(seed%173)*.137,((seed/173)%113)*.153,((seed/193)%149)*.127};for(int i=0;i<3;i++)p[i]=float(p[i])+float(shift[i]);}
   bool legacy=part.value("_material_revision","1956")=="1951";V3 scale={115,115,115};scale[axis]=2.8;V3 abrasion={120,120,120};abrasion[axis]=9;float grain=noise(1,3,.72,scale),patch=legacy?noise(25,4,.7):noise(1,4,.7,abrasion);V3 timber=ramp(grain,{{.25,{.010,.007,.004}},{.43,{.026,.019,.011}},{.57,{.055,.039,.022}},{.74,{.095,.069,.040}}});double mask=legacy?ramp(patch,{{.36,{.05,.05,.05}},{.51,{.12,.12,.12}},{.56,{.82,.82,.82}},{.64,{1,1,1}}})[0]:ramp(patch,{{.36,{.025,.025,.025}},{.53,{.055,.055,.055}},{.62,{.40,.40,.40}},{.73,{.88,.88,.88}}})[0];double z=part.value("_world_position",Json(original))[2];s.base_color=vmul(blend({.009,.010,.008},timber,mask),lerp(.46f,1,sat(z/.3f)));s.roughness=legacy?lerp(.70f,.91f,mask):lerp(.82f,.94f,mask);s.height=grain;s.bump_strength=legacy?.34:.28;s.bump_distance=legacy?.0008:.0005;s.specular=legacy?.25:.10;return s;
  }
  if(key=="wood_edge"){s.base_color=ramp(noise(63,3,.65),{{.24,{.025,.017,.010}},{.65,{.052,.036,.019}},{.84,{.088,.060,.033}}});s.roughness=.87;return s;}
  if(key=="iron"||key=="rust"){float n=noise(74,3,.73);s.base_color=ramp(n,{{.23,{.013,.014,.013}},{.47,{.042,.039,.030}},{.62,{.095,.041,.014}},{.82,{.22,.11,.042}}});s.roughness=.63;s.metallic=.56;s.height=n;s.bump_strength=.33;s.bump_distance=.0005;return s;}
  if(key=="paper"){s.base_color={.075,.063,.041};s.roughness=.94;return s;}
  if(key=="grime"){s.base_color={.006,.005,.004};s.roughness=.98;return s;}
 }
 if(key=="wood"||key=="wood_x"||key=="wood_rough"||key=="wood_rough_x"){
  bool horizontal=key=="wood_x"||key=="wood_rough_x";if(!horizontal&&part.contains("vertices")&&!part["vertices"].empty()){V3 mn={1e30,1e30,1e30},mx={-1e30,-1e30,-1e30};for(const auto&v:part["vertices"])for(int i=0;i<3;i++){mn[i]=std::min(mn[i],v[i].get<double>());mx[i]=std::max(mx[i],v[i].get<double>());}horizontal=mx[0]-mn[0]>mx[2]-mn[2];}
  if(key.find("rough")!=std::string::npos){float grain=noise(1,5,.78,horizontal?V3{4.5,34,34}:V3{34,34,4.5}),micro=noise(1,4,.76,horizontal?V3{23,370,370}:V3{370,370,23}),mix=grain*(.62f+.38f*micro);s.base_color=ramp(mix,{{.18,{.0015,.0014,.0012}},{.33,{.005,.0045,.0037}},{.46,{.013,.011,.0085}},{.64,{.032,.027,.018}}});s.roughness=.94;s.height=micro;s.bump_strength=.6;s.bump_distance=.0017;
  }else{float grain=noise(1,4,.7,horizontal?V3{2.8,95,95}:V3{95,95,2.8});s.base_color=vmul(ramp(grain,{{.25,{.006,.0048,.0037}},{.43,{.018,.012,.008}},{.57,{.038,.027,.017}},{.75,{.075,.056,.037}}}),lerp(.48f,1,sat(float(p[2])/.34f)));s.roughness=.88;s.height=grain;s.bump_strength=.4;s.bump_distance=.0009;}return s;
 }
 if(key=="iron"||key=="rust"){
  float n=noise(42,4,.77);s.base_color=key=="iron"?ramp(n,{{.20,{.006,.007,.007}},{.52,{.015,.014,.013}},{.62,{.040,.020,.010}},{.75,{.11,.043,.013}},{.87,{.19,.075,.028}}}):ramp(n,{{.18,{.045,.022,.010}},{.5,{.12,.051,.019}},{.8,{.23,.11,.039}}});s.roughness=.74;s.metallic=key=="iron"?.38:.13;s.height=noise(550,2,.7);s.bump_strength=.4;s.bump_distance=.00065;s.base_color=blend(s.base_color,{.095,.105,.099},directed_wear(part,p));
  if(part.value("_surface_side","")=="front"){float edgefine=directed_wear(part,p,true);if(edgefine>0){float modulation=noise(140,2.5,.72,{1,1,1.25})*.76f+noise(1250,2,.73)*.24f,broken=ramp(modulation,{{.40,{.06,.06,.06}},{.53,{1,1,1}}})[0],maskfine=edgefine*broken;s.base_color=blend(s.base_color,{.135,.145,.132},maskfine);s.roughness=lerp(s.roughness,.48f,maskfine);s.metallic=lerp(s.metallic,.70f,maskfine);}}return s;
 }
 if(key=="glass"){s.base_color={.10,.13,.12};s.roughness=.16;s.transmission=.88;s.ior=1.52;s.height=noise(1,3,.6,{70,70,3});s.bump_strength=.3;s.bump_distance=.00022;return s;}
 if(key=="grime"){s.base_color={.025,.022,.017};s.roughness=.97;return s;}
 if(key=="concrete"){s.base_color={.033,.035,.034};s.roughness=.92;return s;}
 throw std::invalid_argument("Unsupported procedural material: "+key);
}

V3 sample_material_normal(const Json&part,const std::string&material,V3 position,
                          V3 geometric_normal,V3 shading_normal,double footprint_m,
                          const std::array<V3,3>&object_to_world){
 if(!(footprint_m>0)||!std::isfinite(footprint_m))throw std::invalid_argument("Normal footprint must be finite and positive");
 const V3 N=unit(shading_normal);auto sample=sample_material(part,material,position);
 if(sample.bump_distance==0||sample.bump_strength<=0)return N;
 auto to_world=[&](V3 v){return vadd(vadd(vmul(object_to_world[0],v[0]),vmul(object_to_world[1],v[1])),vmul(object_to_world[2],v[2]));};
 auto to_object=[&](V3 v){return V3{vdot(v,object_to_world[0]),vdot(v,object_to_world[1]),vdot(v,object_to_world[2])};};
 V3 Ng=unit(to_world(unit(geometric_normal))),axis;
 // Cycles differential_from_compact / make_orthonormals, in WORLD space.
 // Transform the stencil back to Object coordinates before sampling heights.
 if(Ng[0]!=Ng[1]||Ng[0]!=Ng[2])axis={Ng[2]-Ng[1],Ng[0]-Ng[2],Ng[1]-Ng[0]};
 else axis={Ng[2]-Ng[1],Ng[0]+Ng[2],-Ng[1]-Ng[0]};
 axis=unit(axis);V3 dx=vmul(to_object(axis),footprint_m),dy=vmul(to_object(vcross(Ng,axis)),footprint_m);
 double hx=sample_material(part,material,vadd(position,dx)).height;
 double hy=sample_material(part,material,vadd(position,dy)).height;
 V3 Rx=vcross(dy,N),Ry=vcross(N,dx);double det=vdot(dx,Rx);
 V3 gradient=vadd(vmul(Rx,hx-sample.height),vmul(Ry,hy-sample.height));
 double sign=det>0?1:(det<0?-1:0);
 V3 bumped=vsub(vmul(N,std::abs(det)),vmul(gradient,sample.bump_distance*sign));
 double length=std::sqrt(vdot(bumped,bumped));if(length==0)return N;
 bumped=vmul(bumped,1/length);
 return unit(blend(N,bumped,std::max(0.,sample.bump_strength)));
}

namespace {
struct Image {int size;std::vector<V3> pixels;explicit Image(int n,V3 value={}):size(n),pixels(size_t(n)*n,value){};};
float srgb(float v){return v<=.0031308f?12.92f*v:1.055f*std::pow(v,1/2.4f)-.055f;}
void tga(const std::filesystem::path&file,const Image&im,bool color){std::ofstream f(file,std::ios::binary);unsigned char h[18]={};h[2]=2;h[12]=im.size&255;h[13]=(im.size>>8)&255;h[14]=h[12];h[15]=h[13];h[16]=24;f.write(reinterpret_cast<char*>(h),18);for(auto p:im.pixels){unsigned char b[3];for(int c=0;c<3;c++)b[2-c]=std::lround(sat(color?srgb(float(p[c])):float(p[c]))*255);f.write(reinterpret_cast<char*>(b),3);}if(!f)throw std::runtime_error("Cannot write "+file.string());}
void pfm(const std::filesystem::path&file,const Image&im){std::ofstream f(file,std::ios::binary);f<<"PF\n"<<im.size<<" "<<im.size<<"\n-1.0\n";for(auto p:im.pixels)for(double v:p){float x=v;uint32_t bits;std::memcpy(&bits,&x,4);unsigned char b[4]={static_cast<unsigned char>(bits),static_cast<unsigned char>(bits>>8),static_cast<unsigned char>(bits>>16),static_cast<unsigned char>(bits>>24)};f.write(reinterpret_cast<char*>(b),4);}if(!f)throw std::runtime_error("Cannot write "+file.string());}
struct Chart {size_t face;double u0=0,v0=0,du=0,dv=0;int x=0,y=0,w=0,h=0;std::vector<V2> uv;V2 anchor{};};
bool pack(std::vector<Chart>&charts,int n,double scale,int pad){std::vector<size_t>order(charts.size());std::iota(order.begin(),order.end(),0);for(auto&c:charts){c.w=std::max(1,int(std::ceil(c.du*scale)))+1+2*pad;c.h=std::max(1,int(std::ceil(c.dv*scale)))+1+2*pad;}std::stable_sort(order.begin(),order.end(),[&](size_t a,size_t b){return charts[a].h>charts[b].h;});int x=0,y=0,row=0;for(auto i:order){auto&c=charts[i];if(c.w>n||c.h>n)return false;if(x+c.w>n){x=0;y+=row;row=0;}if(y+c.h>n)return false;c.x=x;c.y=y;x+=c.w;row=std::max(row,c.h);}return true;}
std::string safe_name(std::string s){for(char&c:s)if(!(std::isalnum(static_cast<unsigned char>(c))||c=='_'||c=='-'||c=='.'))c='_';return s;}
double part_area(const Json&p){double area=0;for(const auto&f:p["faces"]){if(f.size()<3)continue;V3 a=p["vertices"][f[0].get<size_t>()];for(size_t k=1;k+1<f.size();k++){V3 b=p["vertices"][f[k].get<size_t>()],c=p["vertices"][f[k+1].get<size_t>()];V3 n=vcross(vsub(b,a),vsub(c,a));area+=std::sqrt(vdot(n,n))*.5;}}return area;}
}
Json bake_material_textures(Json&scene,const std::string&directory,int resolution){
 if(resolution<16||resolution>16384){throw std::invalid_argument("Texture resolution must be 16..16384");}
 std::filesystem::path out=std::filesystem::absolute(directory);std::filesystem::create_directories(out);
 double maxarea=0,maxextent=0;for(const auto&p:scene["parts"]){maxarea=std::max(maxarea,part_area(p));V3 mn={1e30,1e30,1e30},mx={-1e30,-1e30,-1e30};for(const auto&v:p["vertices"])for(int i=0;i<3;i++){mn[i]=std::min(mn[i],v[i].get<double>());mx[i]=std::max(mx[i],v[i].get<double>());}for(int i=0;i<3;i++)maxextent=std::max(maxextent,mx[i]-mn[i]);}double minimum_density=resolution/std::max(1e-12,maxextent);
 std::string generator=scene.value("metadata",Json::object()).value("generator","");bool furniture=generator.find("furniture")!=std::string::npos||generator.find("Furniture")!=std::string::npos;
 const std::string profile=scene.value("metadata",Json::object()).value("native_material_profile","");const bool fine=profile=="finewear_1921";
 Json report={{"baked_parts",0},{"producer","native C++ procedural sampler and atlas rasterizer"},{"images_are_generated",true},{"pixel_format","PNG16_UNORM"},{"furniture_default_material_revision","1956"},{"normal_method","Cycles forward height stencil in world geometric-normal basis; mean metric UV texel footprint; no camera parameters"},{"probes",Json::array()},{"parts",Json::array()}};size_t partindex=0;
 for(auto&part:scene["parts"]){
  if(part["faces"].empty()){continue;}
  int n=std::max(32,int(std::ceil(resolution*std::sqrt(part_area(part)/std::max(1e-20,maxarea)))));n=std::min(n,resolution);constexpr int pad=3;
  std::array<V3,3> objectRotation={V3{1,0,0},V3{0,1,0},V3{0,0,1}};
  if(scene.contains("nodes")){V3 origin=world_point(scene,part["node"],{0,0,0});for(int axis=0;axis<3;axis++){V3 endpoint={0,0,0};endpoint[axis]=1;objectRotation[axis]=vsub(world_point(scene,part["node"],endpoint),origin);}}
  std::vector<Chart> charts;
  for(size_t fi=0;fi<part["faces"].size();fi++){
   Chart c;c.face=fi;for(const auto&v:part["uvs"][fi])c.uv.push_back(v.get<V2>());
   // Rotate only the independent texture chart. The original metric UV layer
   // is never changed. Long-edge alignment prevents diagonal bevel slivers
   // from wasting their narrow raster dimension.
   V2 axis={1,0};double longest=0;
   for(size_t i=0;i<c.uv.size();i++){V2 d={c.uv[(i+1)%c.uv.size()][0]-c.uv[i][0],c.uv[(i+1)%c.uv.size()][1]-c.uv[i][1]};double length=d[0]*d[0]+d[1]*d[1];if(length>longest){longest=length;double inv=1/std::sqrt(length);axis={d[0]*inv,d[1]*inv};}}
   for(auto&v:c.uv){V2 rotated={axis[0]*v[0]+axis[1]*v[1],-axis[1]*v[0]+axis[0]*v[1]};v=rotated;}
   c.u0=c.v0=1e30;double maxu=-1e30,maxv=-1e30,signedArea=0;
   for(size_t i=0;i<c.uv.size();i++){auto v=c.uv[i];c.u0=std::min(c.u0,v[0]);c.v0=std::min(c.v0,v[1]);maxu=std::max(maxu,v[0]);maxv=std::max(maxv,v[1]);auto next=c.uv[(i+1)%c.uv.size()];signedArea+=v[0]*next[1]-next[0]*v[1];}
   c.du=maxu-c.u0;c.dv=maxv-c.v0;
   auto polygon=c.uv;if(signedArea<0)std::reverse(polygon.begin()+1,polygon.end());auto triangle=triangulate(polygon).front();
   for(auto i:triangle){c.anchor[0]+=polygon[i][0]/3;c.anchor[1]+=polygon[i][1]/3;}
   charts.push_back(c);
  }
  while(!pack(charts,n,minimum_density,pad)){if(n>=16384)throw std::runtime_error("Too many faces for atlas");n=std::min(16384,n*2);}double lo=0,hi=1;while(pack(charts,n,hi,pad)&&hi<1e9)hi*=2;for(int i=0;i<40;i++){double mid=(lo+hi)/2;if(pack(charts,n,mid,pad))lo=mid;else hi=mid;}double scale=lo;if(scale<=0)throw std::runtime_error("Degenerate UV atlas");pack(charts,n,scale,pad);
  Image color(n),params(n),surface(n),normal(n,{.5,.5,1}),height(n);std::vector<int>cover(size_t(n)*n,-1),owner(size_t(n)*n,-1);Json texuv=Json::array(),probeFaces=Json::array();size_t filled=0;std::array<double,6> constantValues{};std::array<bool,6> constantFlags={true,true,true,true,true,true};bool haveConstants=false;std::vector<size_t> facePixels(charts.size(),0);Json materialPart=part;materialPart["_material_family"]=furniture?"furniture":(fine?"fine":"steel");
  for(const auto&c:charts){
   double scaleU=(c.w-2*pad-1)/std::max(1e-20,c.du),scaleV=(c.h-2*pad-1)/std::max(1e-20,c.dv);
   // Align an actual interior triangle centroid with a texel centre. One
   // extra interior texel contains the phase shift while preserving gutters.
   double phaseU=.5-(c.anchor[0]-c.u0)*scaleU,phaseV=.5-(c.anchor[1]-c.v0)*scaleV;phaseU-=std::floor(phaseU);phaseV-=std::floor(phaseV);
   Json puv=Json::array();for(auto uv:c.uv)puv.push_back({(c.x+pad+phaseU+(uv[0]-c.u0)*scaleU)/n,(c.y+pad+phaseV+(uv[1]-c.v0)*scaleV)/n});texuv.push_back(puv);
   for(int y=c.y;y<c.y+c.h;y++)for(int x=c.x;x<c.x+c.w;x++)owner[size_t(y)*n+x]=int(c.face);
   const auto&face=part["faces"][c.face];std::string key=part.value("material","iron");if(part.contains("face_materials")&&c.face<part["face_materials"].size())key=part["face_materials"][c.face];
   std::vector<V2>uv=c.uv;std::vector<size_t>indices(uv.size());std::iota(indices.begin(),indices.end(),0);double signedarea=0;for(size_t i=0;i<uv.size();i++)signedarea+=uv[i][0]*uv[(i+1)%uv.size()][1]-uv[(i+1)%uv.size()][0]*uv[i][1];if(signedarea<0){std::reverse(uv.begin()+1,uv.end());std::reverse(indices.begin()+1,indices.end());}
   auto triangles=triangulate(uv);bool probed=false;
   for(auto tri:triangles){for(auto&i:tri)i=indices[i];V3 a=part["vertices"][face[tri[0]].get<size_t>()],b=part["vertices"][face[tri[1]].get<size_t>()],d=part["vertices"][face[tri[2]].get<size_t>()];V2 ua=c.uv[tri[0]],ub=c.uv[tri[1]],ud=c.uv[tri[2]];double det=(ub[0]-ua[0])*(ud[1]-ua[1])-(ub[1]-ua[1])*(ud[0]-ua[0]);if(std::abs(det)<1e-22)continue;
    V3 delta1=vsub(b,a),delta2=vsub(d,a),N=unit(vcross(delta1,delta2));if(signedarea<0)N=vmul(N,-1);V3 T=unit(vmul(vsub(vmul(delta1,ud[1]-ua[1]),vmul(delta2,ub[1]-ua[1])),1/det)),B=unit(vmul(vsub(vmul(delta2,ub[0]-ua[0]),vmul(delta1,ud[0]-ua[0])),1/det));
    materialPart["_surface_side"]=part.contains("face_material_sides")?part["face_material_sides"][c.face].get<std::string>():(N[1]<-.9999?"front":(N[1]>.9999?"back":""));
    for(int y=c.y+pad;y<c.y+c.h-pad;y++)for(int x=c.x+pad;x<c.x+c.w-pad;x++){
     double u=c.u0+(x+.5-c.x-pad-phaseU)/scaleU,v=c.v0+(y+.5-c.y-pad-phaseV)/scaleV,beta=((u-ua[0])*(ud[1]-ua[1])-(v-ua[1])*(ud[0]-ua[0]))/det,gamma=((ub[0]-ua[0])*(v-ua[1])-(ub[1]-ua[1])*(u-ua[0]))/det,alpha=1-beta-gamma;if(alpha<-1e-8||beta<-1e-8||gamma<-1e-8)continue;size_t index=size_t(y)*n+x;if(cover[index]>=0)continue;V3 pos=vadd(vadd(vmul(a,alpha),vmul(b,beta)),vmul(d,gamma));
     if(furniture&&scene.contains("nodes"))materialPart["_world_position"]=world_point(scene,part["node"],pos);
     auto s=sample_material(materialPart,key,pos);std::array<double,6> values={s.ior,s.specular,s.coat,s.roughness,s.metallic,s.transmission};if(!haveConstants){constantValues=values;haveConstants=true;}else{for(size_t k=0;k<values.size();k++)if(std::abs(values[k]-constantValues[k])>1e-12)constantFlags[k]=false;}color.pixels[index]=s.base_color;params.pixels[index]={s.roughness,s.metallic,s.transmission};surface.pixels[index]={s.coat,s.specular,(s.ior-1)/2};height.pixels[index]={s.height*s.bump_distance,s.bump_strength,s.height};
     if(s.bump_distance&&s.bump_strength){V3 Ns=N;if(part.contains("normals")){const auto&ns=part["normals"][c.face];Ns=unit(vadd(vadd(vmul(ns[tri[0]].get<V3>(),alpha),vmul(ns[tri[1]].get<V3>(),beta)),vmul(ns[tri[2]].get<V3>(),gamma)));}double footprint=.5*(1/scaleU+1/scaleV);V3 bumped=sample_material_normal(materialPart,key,pos,N,Ns,footprint,objectRotation);V3 Ts=unit(vsub(T,vmul(Ns,vdot(T,Ns)))),Bs=unit(vcross(Ns,Ts));if(vdot(Bs,B)<0)Bs=vmul(Bs,-1);normal.pixels[index]={vdot(bumped,Ts)*.5+.5,vdot(bumped,Bs)*.5+.5,vdot(bumped,Ns)*.5+.5};}
     cover[index]=int(c.face);filled++;facePixels[c.face]++;
     if(!probed&&x>=c.x+pad+std::max(0,(c.w-2*pad)/3)&&y>=c.y+pad+std::max(0,(c.h-2*pad)/3)){
      Json args=materialPart;for(const char*k:{"faces","uvs","texture_uvs","textures"})args.erase(k);Json probe={{"id",part.value("id","")+".face"+std::to_string(c.face)},{"part",args},{"family",furniture?"furniture":(fine?"fine":(part.contains("wood_finish")?"damp":(part.value("glass_study",false)?"glass":"steel")))},{"material",key},{"position",pos},{"pixel",{x,y}},{"base_color",s.base_color},{"roughness",s.roughness},{"metallic",s.metallic},{"transmission",s.transmission},{"height",s.height},{"normal",normal.pixels[index]}};
      if(probeFaces.size()<12){probeFaces.push_back(probe);}
      probed=true;
     }
    }
   }
  }
  for(size_t fi=0;fi<facePixels.size();fi++)if(facePixels[fi]==0)throw std::runtime_error("Native atlas has an uncovered face: "+part.value("id",std::string("part"))+" face "+std::to_string(fi));
  // Extrude chart edge texels through three-pixel gutters, without cross-chart bleed.
  for(int iter=0;iter<pad;iter++){auto next=cover;for(int y=0;y<n;y++)for(int x=0;x<n;x++){size_t i=size_t(y)*n+x;if(cover[i]>=0||owner[i]<0)continue;for(auto step:std::array<std::array<int,2>,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){int xx=x+step[0],yy=y+step[1];if(xx<0||xx>=n||yy<0||yy>=n)continue;size_t j=size_t(yy)*n+xx;if(cover[j]!=owner[i])continue;color.pixels[i]=color.pixels[j];params.pixels[i]=params.pixels[j];surface.pixels[i]=surface.pixels[j];normal.pixels[i]=normal.pixels[j];height.pixels[i]=height.pixels[j];next[i]=cover[j];break;}}cover.swap(next);}
  std::string name=std::to_string(partindex++)+"."+safe_name(part.value("id","part"));Json textures={{"base_color",(out/(name+".base_color.png")).string()},{"material_params",(out/(name+".material_params.png")).string()},{"surface",(out/(name+".surface.png")).string()},{"normal",(out/(name+".normal.png")).string()},{"height",(out/(name+".height.pfm")).string()},{"base_color_linear",(out/(name+".base_color.pfm")).string()},{"resolution",n},{"normal_convention","OpenGL +Y tangent"},{"material_params_channels",Json::array({"roughness","metallic","transmission"})},{"surface_channels",Json::array({"coat","specular","(ior-1)/2"})}};
  textures["normal_gltf"]=(out/(name+".normal_gltf.png")).string();textures["orm"]=(out/(name+".orm.png")).string();textures["transmission"]=(out/(name+".transmission.png")).string();textures["specular_weight"]=(out/(name+".specular_weight.png")).string();textures["pixel_format"]="PNG16_UNORM";
  write_png16(textures["base_color"],n,n,color.pixels,true);write_png16(textures["material_params"],n,n,params.pixels);write_png16(textures["surface"],n,n,surface.pixels);write_png16(textures["normal"],n,n,normal.pixels);
  {Image scratch(n);for(size_t i=0;i<scratch.pixels.size();i++)scratch.pixels[i]={1,params.pixels[i][0],params.pixels[i][1]};write_png16(textures["orm"],n,n,scratch.pixels);for(size_t i=0;i<scratch.pixels.size();i++)scratch.pixels[i]={params.pixels[i][2],params.pixels[i][2],params.pixels[i][2]};write_png16(textures["transmission"],n,n,scratch.pixels);for(size_t i=0;i<scratch.pixels.size();i++)scratch.pixels[i]={normal.pixels[i][0],1-normal.pixels[i][1],normal.pixels[i][2]};write_png16(textures["normal_gltf"],n,n,scratch.pixels);std::vector<double> alpha(scratch.pixels.size());for(size_t i=0;i<scratch.pixels.size();i++){scratch.pixels[i]={1,1,1};alpha[i]=2*surface.pixels[i][1];}write_png16(textures["specular_weight"],n,n,scratch.pixels,false,alpha);}
  if(scene.value("metadata",Json::object()).value("emit_tga_diagnostics",false)){tga(out/(name+".base_color.tga"),color,true);tga(out/(name+".material_params.tga"),params,false);tga(out/(name+".surface.tga"),surface,false);tga(out/(name+".normal.tga"),normal,false);}
  pfm(textures["height"].get<std::string>(),height);pfm(textures["base_color_linear"].get<std::string>(),color);
  Json constants=Json::object();std::array<const char*,6> constantNames={"ior","specular","coat","roughness","metallic","transmission"};if(haveConstants)for(size_t k=0;k<constantNames.size();k++)if(constantFlags[k])constants[constantNames[k]]=constantValues[k];part["material_constants"]=constants;
  if(furniture)part["material_provenance"]={{"family","furniture"},{"revision",part.value("_material_revision","1956")},{"source","JericoWoodFurnitureCheckpoint_20261006_"+part.value("_material_revision","1956")+"/blender_adapter.py"}};

  part["texture_uvs"]=texuv;part["textures"]=textures;for(auto&p:probeFaces){p["textures"]=textures;report["probes"].push_back(p);}report["parts"].push_back({{"id",part["id"]},{"resolution",n},{"texels_per_metre",scale},{"covered_pixels",filled},{"material_constants",constants},{"material_provenance",part.value("material_provenance",Json::object())},{"uv_islands",charts.size()},{"face_covered_pixels",facePixels},{"empty_islands",std::count(facePixels.begin(),facePixels.end(),size_t(0))},{"textures",textures}});
 }
 report["baked_parts"]=report["parts"].size();scene["metadata"]["native_material_bake"]={{"generator","C++"},{"maximum_resolution",resolution},{"metric_uvs_preserved",true},{"pixel_format","PNG16_UNORM"},{"normal_filter","cycles_forward_texel_footprint"},{"normal_camera_dependent",false},{"texture_uv_layer","NativeTextureUV"},{"native_images",true}};return report;
}

} // namespace jerico
