#include "jerico/shelf_props.hpp"
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
double sat(double a){return std::clamp(a,0.,1.);}double smooth(double a){a=sat(a);return a*a*(3-2*a);}V3 mix(V3 a,V3 b,double t){return add(mul(a,1-t),mul(b,t));}
struct Pixel{V3 c{};double rough=.8,metal=0,trans=0,h=0;};
Pixel pixel(const std::string&mat,double u,double v,int64_t seed,double wear){
 double a=2*pi*u;V3 p{.9*std::cos(a),.9*std::sin(a),v*3.1};V3 off{double(seed%137)*.121,double(seed%97)*.077,double(seed%73)*.143};p=add(p,off);
 double macro=blender_noise(p,2.8,3,.65),grain=blender_noise(p,27,2,.62),micro=blender_noise(p,103,1,.7),chip=blender_noise(p,11,3,.62);
 double boundary=1-smooth((std::min(v,1-v)-.045)/.065);double rust=smooth((macro+.35*chip-.66)/.26)*wear;double pits=smooth((grain-.61)/.17)*wear;double scratch=std::pow(std::max(0.,std::sin(u*400+.7*std::sin(v*31))),32)*smooth((chip-.49)/.22)*.24*wear;
 Pixel o;
 if(mat=="tin"){double lost=smooth((macro+.38*grain-.69)/.21)*wear;V3 clean=mix(V3{.095,.098,.085},V3{.235,.230,.204},grain);V3 oxide=mix(V3{.033,.021,.011},V3{.103,.056,.025},chip);o.c=mix(clean,oxide,lost*.91);o.c=mul(o.c,1-.35*pits);o.c=mix(o.c,{.32,.30,.255},boundary*(.16+.44*chip)+scratch*.7);o.rough=.49+.30*lost+.08*micro;o.metal=.84*(1-.91*lost);o.h=(grain-.5)*.000065-pits*.00010+lost*.000018-scratch*.00003;}
 else if(mat=="iron"){V3 base=mix(V3{.014,.017,.017},V3{.035,.038,.035},grain);V3 oxide=mix(V3{.025,.017,.009},V3{.094,.048,.020},chip);o.c=mix(base,oxide,rust*.85);o.c=mix(o.c,{.12,.109,.083},boundary*grain*.23+scratch*.55);o.rough=.62+.30*rust+.04*micro;o.metal=.72*(1-.9*rust);o.h=(grain-.5)*.000045-pits*.000080+rust*.00004;}
 else{bool amber=mat=="glass_amber";double dirt=smooth((macro+.21*grain-.55)/.26)*wear;double crust=smooth((chip+.3*micro-.80)/.16)*wear;double streak=smooth((blender_noise({p[0]*2,p[1]*2,p[2]*.12},15,2,.65)-.53)/.20)*.35*wear;double film=sat(dirt*.67+crust*.40+streak*.45+boundary*.15*wear);V3 clear=amber?V3{.19,.125,.062}:V3{.63,.65,.57};V3 dust=amber?V3{.105,.075,.042}:V3{.22,.205,.167};o.c=mix(clear,dust,film);o.trans=(amber?.78:.93)*(1-.66*film);o.rough=.085+(amber?.39:.32)*film;o.h=(grain-.5)*.000009+crust*.00006;o.metal=0;}
 return o;
}
}
void bake_shelf_props(Json&s,const std::string&directory,int n){if(n<32||n>2048)throw std::invalid_argument("Shelf prop bake resolution 32..2048");std::filesystem::create_directories(directory);std::map<std::string,Json>maps;int64_t seed=recipe_seed(s["recipe"]["seed"]);double wear=s["recipe"]["wear"];
 for(auto&p:s["parts"]){std::string mat=p["material"];maps.emplace(mat,Json::object());}
 for(auto&entry:maps){const auto&mat=entry.first;size_t count=size_t(n)*n;std::vector<V3>colors(count),normal(count),orm(count),trans(count),surface(count,{0,.5,.255}),spec(count,{1,1,1}),params(count);std::vector<double>height(count),alpha(count,1);
  for(int y=0;y<n;y++)for(int x=0;x<n;x++){size_t i=size_t(y)*n+x;auto p=pixel(mat,(x+.5)/n,(y+.5)/n,seed,wear);colors[i]=p.c;orm[i]={1,p.rough,p.metal};params[i]={p.rough,p.metal,p.trans};trans[i]={p.trans,p.trans,p.trans};height[i]=p.h;}
  for(int y=0;y<n;y++)for(int x=0;x<n;x++){auto at=[&](int xx,int yy){xx=(xx+n)%n;yy=std::clamp(yy,0,n-1);return height[size_t(yy)*n+xx];};double dx=(at(x+1,y)-at(x-1,y))/(.36*2/n),dy=(at(x,y+1)-at(x,y-1))/(.23*2/n);normal[size_t(y)*n+x]=add(mul(norm({-dx,-dy,1}),.5),{.5,.5,.5});}
  Json&t=entry.second;for(auto key:{"base_color","normal","orm","transmission","surface","specular_weight","material_params"})t[key]=(std::filesystem::path(directory)/("shelfprops."+mat+"."+key+".png")).string();write_png16(t["base_color"],n,n,colors,true);write_png16(t["normal"],n,n,normal);write_png16(t["orm"],n,n,orm);write_png16(t["transmission"],n,n,trans);write_png16(t["surface"],n,n,surface);write_png16(t["specular_weight"],n,n,spec,false,alpha);write_png16(t["material_params"],n,n,params);t["resolution"]=n;t["pixel_format"]="PNG16_UNORM";t["normal_convention"]="OpenGL +Y tangent";t["shared_family"]="shelfprops."+mat;
 }
 for(auto&p:s["parts"]){std::string mat=p["material"];p["textures"]=maps.at(mat);p["material_shared_id"]="shelfprops."+mat;p["material_constants"]={{"ior",mat.find("glass")!=std::string::npos?1.51:1.5},{"coat",0},{"coat_roughness",.03}};p["material_provenance"]={{"generator","original native C++"},{"family",mat},{"shared",true},{"image_inputs",false},{"wear",wear},{"seed",seed}};}
 s["metadata"]["native_material_bake"]={{"generator","C++"},{"native_images",true},{"shared_material_count",maps.size()},{"resolution",n},{"pixel_format","PNG16_UNORM"},{"per_wire_atlas",false},{"transmission","KHR_materials_transmission with real shell walls"}};
}
}
