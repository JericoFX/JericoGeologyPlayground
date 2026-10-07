#include "jerico/bookshelf_materials.hpp"
#include "jerico/png.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace jerico {
namespace {
constexpr double pi=3.14159265358979323846;
constexpr int variants=6;
constexpr std::array<const char*,7> texture_keys{{"base_color","normal","orm","transmission","surface","specular_weight","material_params"}};
double clamp(double x){return std::clamp(x,0.,1.);}
double smooth(double x){x=clamp(x);return x*x*(3-2*x);}
double mix(double a,double b,double t){return a+(b-a)*t;}
V3 mix(V3 a,V3 b,double t){return add(mul(a,1-t),mul(b,t));}
uint32_t hash(const std::string&s){uint32_t h=2166136261u;for(unsigned char c:s){h^=c;h*=16777619u;}return h;}
double random01(uint32_t h){h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;return double(h)/4294967296.;}
double noise(double x,double y,uint32_t seed){int64_t ix=int64_t(std::floor(x)),iy=int64_t(std::floor(y));double fx=smooth(x-ix),fy=smooth(y-iy);auto at=[&](int dx,int dy){return random01(uint32_t(ix+dx)*374761393u+uint32_t(iy+dy)*668265263u+seed*2246822519u);};return mix(mix(at(0,0),at(1,0),fx),mix(at(0,1),at(1,1),fx),fy);}
double fbm(double x,double y,uint32_t seed){return (noise(x,y,seed)+.45*noise(x*2.13,y*2.13,seed+17)+.20*noise(x*4.47,y*4.47,seed+31))/1.65;}
void need(bool b,const std::string&m){if(!b)throw std::invalid_argument("Bookshelf material bake: "+m);}
V3 vector3(const Json&j,const std::string&label){need(j.is_array()&&j.size()==3,label+": expected three-vector");V3 v{};for(int k=0;k<3;k++){need(j[k].is_number(),label+": nonnumeric value");v[k]=j[k].get<double>();need(std::isfinite(v[k]),label+": nonfinite value");}return v;}
bool is_wood(const std::string&m){return m=="wood"||m=="wood_exposed"||m=="wood_rough"||m=="damp_wood";}
bool is_iron(const std::string&m){return m=="iron"||m=="rust";}
struct Frame{V3 origin{},lo{1e30,1e30,1e30},hi{-1e30,-1e30,-1e30};std::array<V3,3>axes{{{1,0,0},{0,1,0},{0,0,1}}};};
Frame frame_of(const Json&p,const std::string&key){Frame f;const auto meta=p.value("metadata",Json::object());const auto j=p.value(key,meta.value(key,Json::object()));if(!key.empty()){
 need(j.is_object()&&j.contains("origin")&&j.contains("u")&&j.contains("v")&&j.contains("w"),p.value("id","")+": "+key+" is required");f.origin=vector3(j["origin"],key+" origin");const char*names[]={"u","v","w"};for(int k=0;k<3;k++){f.axes[k]=vector3(j[names[k]],names[k]);need(std::abs(dot(f.axes[k],f.axes[k])-1)<1e-5,key+": nonunit axis");}for(int k=0;k<3;k++)for(int l=k+1;l<3;l++)need(std::abs(dot(f.axes[k],f.axes[l]))<1e-5,key+": nonorthogonal axes");need(dot(cross(f.axes[0],f.axes[1]),f.axes[2])>.99999,key+": frame must be right-handed");}
 need(p.contains("vertices")&&p["vertices"].is_array()&&!p["vertices"].empty(),"vertices missing");for(const auto&jv:p["vertices"]){V3 v=sub(vector3(jv,"vertex"),f.origin);for(int k=0;k<3;k++){double x=dot(v,f.axes[k]);f.lo[k]=std::min(f.lo[k],x);f.hi[k]=std::max(f.hi[k],x);}}return f;}
struct Region{double low,high;};
constexpr std::array<Region,3> wood_regions{{{0,.70},{.70,.84},{.84,1}}};
struct Mapping{size_t index;std::string family,space;Json uv,regions;};
Mapping map_part(const Json&p,size_t index,int n,int64_t seed){
 const std::string mat=p.value("material","");const bool wood=is_wood(mat),iron=is_iron(mat),cover=mat=="book_cover",paper=mat=="book_pages",label=mat=="book_label",gilt=mat=="book_gilt";
 need(wood||iron||cover||paper||label||gilt,"unsupported material "+mat);need(p.contains("id")&&p["id"].is_string(),"part ID missing");const std::string id=p["id"];const auto meta=p.value("metadata",Json::object());const uint32_t h=hash(std::to_string(seed)+":"+id);const auto family=p.value("book_family",meta.value("book_family",Json()));need(!cover||(family.is_number_integer()&&family.get<int>()>=0&&family.get<int>()<variants),id+": book_family must be 0..5");
 const Frame f=frame_of(p,wood?"timber_frame":iron?"":"book_frame");const std::string fid=wood?"bookshelf.wood."+std::to_string(h%variants):cover?"bookshelf.cover."+std::to_string(family.get<int>()):paper?"bookshelf.paper.0":label?"bookshelf.label.0":gilt?"bookshelf.gilt.0":"bookshelf.iron.0";
 Mapping result{index,fid,wood?"timber_frame":iron?"world":"book_frame",Json::array(),Json::array()};
 need(p.contains("faces")&&p["faces"].is_array()&&!p["faces"].empty(),id+": no faces");need(p.contains("normals")&&p["normals"].is_array()&&p["normals"].size()==p["faces"].size(),id+": evaluated normals required");need(p.contains("uvs")&&p["uvs"].is_array()&&p["uvs"].size()==p["faces"].size(),id+": metric UVs required");if(p.contains("face_materials"))need(p["face_materials"].is_array()&&p["face_materials"].size()==p["faces"].size(),id+": face material count");
 const bool fractured=p.value("fractured",meta.value("fracture",false));
 for(size_t fi=0;fi<p["faces"].size();fi++){
  const auto&face=p["faces"][fi];need(face.is_array()&&face.size()>=3,id+": invalid face");need(p["normals"][fi].size()==face.size()&&p["uvs"][fi].size()==face.size(),id+": loop count mismatch");std::vector<V3> positions;positions.reserve(face.size());
  for(size_t j=0;j<face.size();j++){const auto&vi=face[j];need(vi.is_number_integer()&&vi.get<int64_t>()>=0&&vi.get<size_t>()<p["vertices"].size(),id+": invalid vertex index");positions.push_back(sub(p["vertices"][vi.get<size_t>()].get<V3>(),f.origin));V3 normal=vector3(p["normals"][fi][j],"loop normal");need(std::abs(dot(normal,normal)-1)<1e-4,id+": nonunit loop normal");const auto&uv=p["uvs"][fi][j];need(uv.is_array()&&uv.size()==2,id+": invalid metric UV");for(int c=0;c<2;c++)need(uv[c].is_number()&&std::isfinite(uv[c].get<double>()),id+": nonfinite metric UV");}
  V3 normal{};for(size_t j=1;j+1<positions.size();j++)normal=add(normal,cross(sub(positions[j],positions[0]),sub(positions[j+1],positions[0])));need(dot(normal,normal)>1e-30,id+": zero-area face");normal=norm(normal);V3 local{dot(normal,f.axes[0]),dot(normal,f.axes[1]),dot(normal,f.axes[2])};int dominant=0;for(int k=1;k<3;k++)if(std::abs(local[k])>std::abs(local[dominant])+1e-12)dominant=k;const int a=dominant==0?1:0,b=dominant==2?1:2;
  const std::string faceMat=p.contains("face_materials")?p["face_materials"][fi].get<std::string>():mat;const bool exposed=faceMat=="wood_exposed"||faceMat=="wood_edge"||(fractured&&std::abs(local[0])>.35);const int region=wood?(dominant==0?2:exposed?1:0):0;const Region limits=wood?wood_regions[region]:Region{0,1};const double padding=std::min(3./n,(limits.high-limits.low)*.15),u0=3./n,u1=1-3./n,v0=limits.low+padding,v1=limits.high-padding;
  const double spanA=f.hi[a]-f.lo[a],spanB=f.hi[b]-f.lo[b];need(spanA>0&&spanB>0,id+": degenerate dominant-plane bounds");double domainA=0,domainB=0;
  if(wood){domainA=std::max(region==2?.42:3.2,spanA*1.08);domainB=std::max(.42,spanB*1.08);}else if(cover||label){domainA=spanA*(label?1.:1.07);domainB=spanB*(label?1.:1.07);}else if(paper){domainA=std::max(dominant==0?.42:.085,spanA*1.05);domainB=std::max(.42,spanB*1.05);}else{domainA=std::max(.24,spanA*1.1);domainB=std::max(.36,spanB*1.1);}
  // This chart seed depends on local plane, never on face index or vertex order.
  // Curved and triangulated patches therefore retain continuous local coordinates.
  const uint32_t chart=h+uint32_t(dominant*137);const double offA=(domainA-spanA)*random01(chart+19),offB=(domainB-spanB)*random01(chart+211);Json uv=Json::array();
  for(V3 p3:positions){double x=(dot(p3,f.axes[a])-f.lo[a]+offA)/domainA,y=(dot(p3,f.axes[b])-f.lo[b]+offB)/domainB;uv.push_back({mix(u0,u1,clamp(x)),mix(v0,v1,clamp(y))});}
  double area=0;for(size_t j=0;j<uv.size();j++){float x=uv[j][0],y=uv[j][1],xx=uv[(j+1)%uv.size()][0],yy=uv[(j+1)%uv.size()][1];area+=double(x)*yy-double(xx)*y;}need(std::abs(area)>1e-15,id+": float32 texture UV collapsed");result.uv.push_back(std::move(uv));result.regions.push_back(wood?(region==2?"end_grain":region==1?"exposed":"blackened_long_grain"):paper?"sepia_leaf_edges":label?"aged_pseudotext":cover?"worn_cover":gilt?"worn_gilt":"rusted_iron");
 }return result;
}
struct Pixel{V3 color{};double roughness=.85,metallic=0,height=0,specular=.22;};
Pixel wood_pixel(double x,double y,int region,int variant){
 const uint32_t seed=1271u+uint32_t(variant*9739);const double macro=fbm(x*2.8,y*17,seed),fibre=fbm(x*5.2,y*155,seed+17),warp=.008*(noise(x*2.3,y*5,seed+29)-.5);const double rings=.5+.5*std::sin((y+warp)*pi*2*(90+variant*3));double grain=.62*fibre+.38*rings;
 double check=0;for(int j=0;j<7;j++){double center=.42*random01(seed+j*313),start=2.5*random01(seed+j*127),length=.24+1.1*random01(seed+j*419);double curve=center+.0015*std::sin(x*8+j)+.0018*std::sin(x*2.7+j);double width=.0006+.0011*random01(seed+j*823);check=std::max(check,(1-smooth(std::abs(y-curve)/width))*smooth((x-start)/.12)*smooth((start+length-x)/.16));}
 const double patch=fbm(x*9.4,y*40,seed+91),chip=fbm(x*27,y*118,seed+107);const double peeling=smooth((patch+.23*chip-.57)/.16)*smooth((grain-.18)/.45);const double dust=smooth((macro-.57)/.22)*.23;
 Pixel p;const double black=.010+.013*grain;const V3 coat{black*1.02,black,black*.92};V3 bare=mix(V3{.042,.032,.023},V3{.153,.113,.069},grain);p.color=mix(coat,bare,peeling*.90);p.color=mix(p.color,{.136,.132,.113},dust);p.color=mix(p.color,{.0028,.0026,.0021},check*.92);p.height=(grain-.5)*.00028+(fibre-.5)*.00012+peeling*.00014-check*.00075;p.roughness=std::clamp(.78+fibre*.13+peeling*.10,.68,.98);p.specular=.22;
 constexpr double shade[]={.83,1.1,.94,.77,1.18,.90};p.color=mul(p.color,shade[variant]);
 if(region==1){p.color=mix(bare,{.11,.095,.071},.30);p.color=mix(p.color,coat,smooth((patch-.49)/.20)*.68);p.roughness=.94;p.height*=1.35;}
 if(region==2){double dx=x-(.14+variant*.015),dy=y-.18,r=std::sqrt(dx*dx+dy*dy),angle=std::atan2(dy,dx);double ring=.5+.5*std::sin(r*(310+variant*12)+1.8*noise(x*18,y*18,seed)),split=std::pow(std::max(0.,std::cos(angle*9+.2*variant)),50)*smooth((r-.026)/.13);p.color=mix(V3{.025,.019,.013},V3{.087,.065,.041},ring*.7+fibre*.3);p.color=mix(p.color,{.005,.004,.003},split*.75);p.roughness=.93;p.height=(ring-.5)*.00027-split*.00065;}
 return p;
}
Pixel cover_pixel(double u,double v,int variant){
 constexpr std::array<V3,6> colors{{{.044,.024,.014},{.022,.032,.019},{.043,.014,.018},{.016,.025,.039},{.037,.031,.023},{.022,.023,.021}}};const uint32_t seed=1987u+uint32_t(variant*7643);double large=fbm(u*5.7,v*6.3,seed),grain=fbm(u*139,v*143,seed+11),fine=noise(u*370,v*355,seed+91);double d=std::min({u,1-u,v,1-v});double border=1-smooth((d-.016-.028*noise(u*16,v*17,seed+17))/.072);double abrasion=fbm(u*19,v*23,seed+23);double scar=smooth((large+.32*abrasion-.64)/.17);double flecks=smooth((fbm(u*80,v*87,seed+41)-.40)/.25);double wear=clamp((scar*.87+border*.64)*flecks);double scratch=0;for(int j=0;j<9;j++){double cx=.08+.84*random01(seed+j*293),cy=random01(seed+j*907),length=.035+.15*random01(seed+j*373);double dd=std::abs(u-cx-.003*std::sin(v*27+j));scratch=std::max(scratch,(1-smooth(dd/.0022))*smooth((v-cy)/.012)*smooth((cy+length-v)/.013));}
 Pixel p;p.color=mul(colors[variant],.63+.54*large+.23*grain);V3 faded=mix(V3{.081,.059,.036},V3{.223,.171,.108},abrasion);p.color=mix(p.color,faded,wear*.85);p.color=mix(p.color,{.177,.143,.100},scratch*.48);double grime=smooth((noise(u*8,v*7,seed+331)-.57)/.22);p.color=mul(p.color,1-.42*grime);p.roughness=std::clamp(.69+.13*grain+wear*.16,.65,.98);p.height=(grain-.5)*.000085+(fine-.5)*.000024-wear*.000075-scratch*.000055;p.specular=.24;return p;
}
Pixel paper_pixel(double x,double y){
 const double warp=.000075*(noise(x*10,y*9,291)-.5);double leaf=.5+.5*std::sin((x+warp)*2*pi*2240+.55*noise(x*120,y*2,331));double grooves=std::pow(leaf,5),fibres=noise(x*270,y*410,347),age=fbm(x*28,y*14,373),fox=smooth((fbm(x*160,y*145,397)-.68)/.14);Pixel p;p.color=mix(V3{.137,.092,.045},V3{.331,.249,.145},.39+.42*age+.19*fibres);p.color=mul(p.color,1-.40*grooves);p.color=mix(p.color,{.076,.038,.014},fox*.55);p.roughness=.94+.035*fibres;p.height=-grooves*.00010+(fibres-.5)*.000003;p.specular=.12;return p;
}
Pixel label_pixel(double u,double v){
 double age=fbm(u*7,v*9,881),fibre=noise(u*287,v*293,883),edge=1-smooth((std::min({u,1-u,v,1-v})-.018)/.13);Pixel p;p.color=mix(V3{.167,.112,.053},V3{.397,.306,.173},age*.69+.31*fibre);p.color=mul(p.color,1-edge*.43);double ink=0;
 // Deliberately nonlinguistic strokes, not fabricated authors or book titles.
 // A six-by-nine grid gives an old ledger label readable as marks at distance.
 if(u>.14&&u<.86&&v>.18&&v<.82){double gx=(u-.14)/.72*9,gy=(v-.18)/.64*6;int col=int(gx),row=int(gy);double x=gx-col,y=gy-row;uint32_t bits=hash(std::to_string(row)+":"+std::to_string(col));if((bits&7)!=0){bool stem=x>.13&&x<.23&&y>.17&&y<.72;bool top=(bits&8)&&x>.18&&x<.68&&y>.65&&y<.74;bool middle=(bits&16)&&x>.20&&x<.59&&y>.40&&y<.49;bool right=(bits&32)&&x>.60&&x<.69&&y>.24&&y<.61;bool foot=(bits&64)&&x>.19&&x<.72&&y>.17&&y<.25;ink=(stem||top||middle||right||foot)?1:0;}}
 double border=(std::abs(u-.09)<.004||std::abs(u-.91)<.004)&&v>.09&&v<.91?1:0;border=std::max(border,((std::abs(v-.09)<.004||std::abs(v-.91)<.004)&&u>.09&&u<.91)?1.:0.);ink=std::max(ink,border);ink*=.50+.38*noise(u*83,v*79,901);p.color=mix(p.color,{.029,.022,.012},ink);p.roughness=.96;p.height=(fibre-.5)*.000013-edge*.000009;p.specular=.10;return p;
}
Pixel metal_pixel(double x,double y,bool gilt){
 const uint32_t seed=gilt?773:551;double large=fbm(x*31,y*34,seed),fine=fbm(x*211,y*227,seed+17),pit=smooth((fine-.62)/.20);Pixel p;
 if(gilt){double worn=smooth((large-.31)/.39);p.color=mix(V3{.147,.101,.039},V3{.039,.030,.021},worn);p.roughness=.58+.24*large;p.metallic=.64*(1-worn*.76);p.height=(fine-.5)*.000018-pit*.000023;p.specular=.3;}
 else{double rust=smooth((large+.24*fine-.61)/.20),scale=fbm(x*83+y*31,y*97-x*23,seed+53),granule=noise(x*571,y*593,seed+71);V3 oxidised=mix(V3{.030,.022,.014},V3{.079,.042,.021},scale*.61+granule*.39);p.color=mix(V3{.016+.011*fine,.020+.010*fine,.021+.009*fine},oxidised,rust);p.color=mix(p.color,{.101,.088,.059},pit*rust*.41);p.color=mix(p.color,{.043,.044,.040},smooth((granule-.77)/.15)*(1-rust)*.35);p.roughness=.65+.27*rust+.04*granule;p.metallic=.72*(1-rust*.93);p.height=(fine-.5)*.000075+(granule-.5)*.000026+rust*.000053-pit*.00010;p.specular=.29;}return p;
}
struct Atlas{Json textures;};
Atlas bake_atlas(const std::string&family,const std::filesystem::path&directory,int n){
 const bool wood=family.find(".wood.")!=std::string::npos,cover=family.find(".cover.")!=std::string::npos,paper=family.find(".paper.")!=std::string::npos,label=family.find(".label.")!=std::string::npos,gilt=family.find(".gilt.")!=std::string::npos;const int variant=family.back()-'0';const size_t count=size_t(n)*n;
 std::vector<V3>colors(count),normals(count),orm(count),params(count),surface(count),transmission(count,{0,0,0}),specular(count,{1,1,1});std::vector<double>height(count),alpha(count);std::vector<int>regionIds(count);
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){const size_t i=size_t(y)*n+x;double u=(x+.5)/n,v=(y+.5)/n;int region=wood?(v<.70?0:v<.84?1:2):0;regionIds[i]=region;Region strip=wood?wood_regions[region]:Region{0,1};double pad=std::min(3./n,(strip.high-strip.low)*.15);double uu=clamp((u-3./n)/(1-6./n)),vv=clamp((v-strip.low-pad)/(strip.high-strip.low-2*pad));Pixel p=wood?wood_pixel(uu*(region==2?.42:3.2),vv*.42,region,variant):cover?cover_pixel(uu,vv,variant):paper?paper_pixel(uu*.085,vv*.42):label?label_pixel(uu,vv):metal_pixel(uu*.24,vv*.36,gilt);colors[i]=p.color;orm[i]={1,p.roughness,p.metallic};params[i]={p.roughness,p.metallic,0};surface[i]={0,p.specular,.25};height[i]=p.height;alpha[i]=p.specular*2;}
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){const size_t i=size_t(y)*n+x;int region=regionIds[i];Region strip=wood?wood_regions[region]:Region{0,1};double pad=std::min(3./n,(strip.high-strip.low)*.15),dx=(wood?(region==2?.42:3.2):paper?.085:label?.085:cover?.32:.24)/(n-6.),dy=(wood?.42:paper?.42:label?.10:cover?.42:.36)/(n*(strip.high-strip.low-2*pad));auto at=[&](int xx,int yy){xx=std::clamp(xx,0,n-1);yy=std::clamp(yy,0,n-1);size_t k=size_t(yy)*n+xx;return regionIds[k]==region?height[k]:height[i];};V3 normal=norm({-(at(x+1,y)-at(x-1,y))/(2*dx),-(at(x,y+1)-at(x,y-1))/(2*dy),1});normals[i]=add(mul(normal,.5),{.5,.5,.5});}
 Atlas result;auto&t=result.textures;t=Json::object();for(const char*k:texture_keys)t[k]=(directory/(family+"."+k+".png")).string();write_png16(t["base_color"],n,n,colors,true);write_png16(t["normal"],n,n,normals);write_png16(t["orm"],n,n,orm);write_png16(t["material_params"],n,n,params);write_png16(t["transmission"],n,n,transmission);write_png16(t["surface"],n,n,surface);write_png16(t["specular_weight"],n,n,specular,false,alpha);
 t["resolution"]=n;t["pixel_format"]="PNG16_UNORM";t["normal_convention"]="OpenGL +Y tangent";t["material_params_channels"]={"roughness","metallic","transmission"};t["surface_channels"]={"coat","specular","(ior-1)/2"};t["shared_family"]=family;return result;
}
}
void bake_bookshelf_materials(Json&scene,const std::string&directory,int resolution){
 need(resolution>=32&&resolution<=4096,"resolution must be in [32,4096]");need(!directory.empty(),"output directory is empty");need(scene.contains("parts")&&scene["parts"].is_array(),"scene parts must be an array");const int64_t seed=recipe_seed(scene.value("recipe",Json::object()).value("seed",Json(0)));std::vector<Mapping>mappings;std::map<std::string,Atlas>atlases;
 for(size_t i=0;i<scene["parts"].size();i++){auto m=map_part(scene["parts"][i],i,resolution,seed);atlases.emplace(m.family,Atlas{});mappings.push_back(std::move(m));}
 std::filesystem::create_directories(directory);for(auto&[family,atlas]:atlases)atlas=bake_atlas(family,directory,resolution);
 for(const auto&m:mappings){auto&p=scene["parts"][m.index];p["texture_uvs"]=m.uv;p["textures"]=atlases.at(m.family).textures;p["material_shared_id"]=m.family;p["material_constants"]={{"ior",1.5},{"coat",0},{"transmission",0},{"coat_roughness",.03}};p["material_provenance"]={{"family","bookshelf"},{"generator","native C++17"},{"shared_variant",m.family},{"grain_space",m.space},{"face_regions",m.regions},{"label_content","nonlinguistic procedural strokes"}};}
 scene["metadata"]["native_material_bake"]={{"generator","C++"},{"native_images",true},{"maximum_resolution",resolution},{"metric_uvs_preserved",true},{"pixel_format","PNG16_UNORM"},{"texture_uv_layer","NativeTextureUV"},{"family","bookshelf"},{"shared_material_count",atlases.size()},{"maximum_material_families",16},{"maximum_wood_variants",variants},{"maximum_cover_variants",variants},{"material_sharing","one atlas per family/variant; never per book or timber"},{"normal_filter","central texel footprint"},{"normal_camera_dependent",false}};
}
}
