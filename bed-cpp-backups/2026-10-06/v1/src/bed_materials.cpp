#include "jerico/bed.hpp"
#include "jerico/materials.hpp"
#include "jerico/png.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <map>
#include <stdexcept>
namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
double sat(double x){return std::clamp(x,0.,1.);}double smooth(double x){x=sat(x);return x*x*(3-2*x);}V3 mix(V3 a,V3 b,double x){return add(mul(a,1-x),mul(b,x));}
uint32_t hash(const std::string&s){uint32_t h=2166136261;for(unsigned char c:s){h^=c;h*=16777619;}return h;}
struct Pixel {V3 color;double rough,metal,height;};
Pixel sample(const std::string&family,double u,double v,double phase){
 double macro=blender_noise({u*4.6+phase,v*4.6,1.27},1,3,.72),meso=blender_noise({u*28+phase,v*28,8.2},1,3,.72),fine=blender_noise({u*171+phase,v*171,3.7},1,2,.63);
 if(family=="iron"){
  double chips=smooth((meso-.52)/.14)*smooth((macro-.32)/.19),rust=smooth((macro-.47)/.16)*smooth((meso-.37)/.25),pit=smooth((fine-.63)/.12)*rust;
  V3 c=mix({.025,.030,.031},{.059,.063,.058},macro);c=mix(c,{.19,.117,.055},chips*.70);c=mix(c,{.098,.045,.019},rust*.76);c=mix(c,{.23,.205,.152},chips*(1-rust)*.25);c=mul(c,.8+.32*fine);
  return {c,std::clamp(.57+.27*rust+.08*meso,.35,.94),.73*(1-rust*.9),.00035*(meso-.5)-.00048*pit};
 }
 double stain=smooth((macro-.445)/.13),speck=smooth((meso-.535)/.16),warp=.5+.5*std::sin(u*2*pi*103+.3*std::sin(v*29)),weft=.5+.5*std::sin(v*2*pi*109+.4*std::sin(u*31));
 double weave=warp*weft;bool blanket=family=="blanket";V3 base=blanket?V3{.070,.070,.045}:V3{.225,.197,.148};base=mul(base,.72+.54*meso);base=mix(base,blanket?V3{.022,.025,.016}:V3{.040,.034,.022},stain*.84);base=mix(base,{.019,.017,.013},speck*.44);base=mul(base,.87+.22*weave);double seamWear=smooth((v-.81)/.025);base=mul(base,1-.32*seamWear);base=mix(base,blanket?V3{.16,.145,.10}:V3{.30,.25,.18},seamWear*smooth((fine-.55)/.18)*.30);
 return {base,blanket?.96:.91,0,(weave-.5)*(blanket?.00058:.00030)+(fine-.5)*.00020};
}
Json atlas(const std::string&f,const std::filesystem::path&dir,int n,int64_t seed){
 size_t count=size_t(n)*n;std::vector<V3> color(count),normal(count),orm(count),surface(count),trans(count,{0,0,0}),spec(count,{1,1,1}),params(count);std::vector<double> heights(count),alpha(count,.5);double phase=double(seed%10000)*.091;
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){size_t i=size_t(y)*n+x;auto p=sample(f,(x+.5)/n,(y+.5)/n,phase);color[i]=p.color;orm[i]={1,p.rough,p.metal};params[i]={p.rough,p.metal,0};heights[i]=p.height;surface[i]={f=="iron"?.04:0,.25,.25};}
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){auto h=[&](int xx,int yy){return heights[size_t(std::clamp(yy,0,n-1))*n+std::clamp(xx,0,n-1)];};V3 nn=norm({-(h(x+1,y)-h(x-1,y))*n/2,-(h(x,y+1)-h(x,y-1))*n/2,1});normal[size_t(y)*n+x]=add(mul(nn,.5),{.5,.5,.5});}
 Json t=Json::object();for(auto key:{"base_color","normal","orm","surface","transmission","specular_weight","material_params"})t[key]=(dir/("bed."+f+"."+key+".png")).string();write_png16(t["base_color"],n,n,color,true);write_png16(t["normal"],n,n,normal);write_png16(t["orm"],n,n,orm);write_png16(t["surface"],n,n,surface);write_png16(t["transmission"],n,n,trans);write_png16(t["specular_weight"],n,n,spec,false,alpha);write_png16(t["material_params"],n,n,params);t["resolution"]=n;t["normal_convention"]="OpenGL +Y tangent";t["pixel_format"]="PNG16_UNORM";return t;
}
}
void bake_bed_materials(Json&s,const std::string&directory,int n){
 if(n<32||n>2048)throw std::invalid_argument("Bed atlas resolution 32..2048");std::filesystem::create_directories(directory);std::map<std::string,Json> atlases;int64_t seed=recipe_seed(s["recipe"]["seed"]);
 for(auto&p:s["parts"]){std::string f=p["material"];if(f!="iron"&&f!="linen"&&f!="blanket")throw std::invalid_argument("unsupported bed material "+f);if(!atlases.count(f))atlases[f]=atlas(f,directory,n,seed);p["textures"]=atlases[f];p["material_shared_id"]="bed."+f;p["material_constants"]={{"ior",1.5},{"specular",.25},{"coat_roughness",.25}};
  Json uvs=Json::array();uint32_t h=hash(p["id"].get<std::string>());double offsetu=.17*(h%7)/7.,offsetv=.17*((h/7)%11)/11.;
   V3 lo{1e20,1e20,1e20},hi{-1e20,-1e20,-1e20};for(auto&v:p["vertices"])for(int k=0;k<3;k++){lo[k]=std::min(lo[k],v[k].get<double>());hi[k]=std::max(hi[k],v[k].get<double>());}
  for(size_t fi=0;fi<p["faces"].size();fi++){auto&face=p["faces"][fi];V3 a=p["vertices"][face[0].get<size_t>()],b=p["vertices"][face[1].get<size_t>()],c=p["vertices"][face[2].get<size_t>()],ng=norm(cross(sub(b,a),sub(c,a)));int major=0;for(int k=1;k<3;k++)if(std::abs(ng[k])>std::abs(ng[major]))major=k;int x=major==0?1:0,y=major==2?1:2;
   double du=std::max(.55,(hi[x]-lo[x])*1.25),dv=std::max(.55,(hi[y]-lo[y])*1.25);Json uv=Json::array();bool seam=p.value("role","")=="PerimeterPiping"||p.value("role","")=="PillowPiping"||p.value("role","")=="AttachedEdgeThread";for(auto i:face){V3 q=p["vertices"][i.get<size_t>()];double vv=f=="iron"?.01+offsetv+.78*(q[y]-lo[y])/dv:(seam?.84+.12*(q[y]-lo[y])/dv:.01+offsetv*.5+.70*(q[y]-lo[y])/dv);uv.push_back({.01+offsetu+.78*(q[x]-lo[x])/du,vv});}uvs.push_back(uv);
  }p["texture_uvs"]=uvs;
 }
 s["metadata"]["native_material_bake"]={{"generator","C++"},{"native_images",true},{"shared_material_count",atlases.size()},{"maximum_resolution",n},{"family","bed"},{"pixel_format","PNG16_UNORM"},{"atlas_policy","One atlas per iron/linen/blanket family, shared by every part"},{"source_images",false},{"normal_camera_dependent",false}};
}
}
