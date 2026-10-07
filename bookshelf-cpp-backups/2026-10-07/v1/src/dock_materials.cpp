#include "jerico/dock_materials.hpp"
#include "jerico/materials.hpp"
#include "jerico/png.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace jerico {
namespace {
constexpr int wood_variants=6;
constexpr double pi=3.14159265358979323846;
constexpr double long_end=.60,exposed_end=.70,wet_end=.86;
struct Region { double low,high; };
constexpr std::array<Region,4> regions{{{0,long_end},{long_end,exposed_end},{wet_end,1},{exposed_end,wet_end}}};
double clamp(double x){return std::clamp(x,0.,1.);}
double blend(double a,double b,double t){return a+(b-a)*t;}
V3 mix(V3 a,V3 b,double t){return add(mul(a,1-t),mul(b,t));}
double smooth(double x){x=clamp(x);return x*x*(3-2*x);}
uint32_t hash(const std::string& value){uint32_t h=2166136261u;for(unsigned char c:value){h^=c;h*=16777619u;}return h;}
double random01(uint32_t h){h^=h>>16;h*=0x7feb352du;h^=h>>15;h*=0x846ca68bu;h^=h>>16;return double(h)/4294967296.;}
void need(bool condition,const std::string& message){if(!condition)throw std::invalid_argument("Dock material bake: "+message);}
V3 vector3(const Json& value,const std::string& name){
 need(value.is_array()&&value.size()==3,name+" must be a three-vector");V3 result{};
 for(int i=0;i<3;i++){need(value[i].is_number(),name+" contains a non-number");result[i]=value[i].get<double>();need(std::isfinite(result[i]),name+" contains a nonfinite number");}return result;
}
struct Frame {V3 origin{},lo{1e30,1e30,1e30},hi{-1e30,-1e30,-1e30};std::array<V3,3> axes{{{1,0,0},{0,1,0},{0,0,1}}};};
Frame frame_of(const Json& part,bool iron){
 Frame frame;
 const Json metadata=part.value("metadata",Json::object());
 const Json f=part.value("timber_frame",metadata.value("timber_frame",Json::object()));
 if(!iron){
  need(f.is_object()&&f.contains("origin")&&f.contains("u")&&f.contains("v")&&f.contains("w"),part.value("id","")+": timber_frame is required");
  frame.origin=vector3(f["origin"],"frame origin");const char* names[]={"u","v","w"};
  for(int i=0;i<3;i++){frame.axes[i]=vector3(f[names[i]],names[i]);need(std::abs(dot(frame.axes[i],frame.axes[i])-1)<1e-5,"frame axis is not unit length");}
  for(int i=0;i<3;i++)for(int j=i+1;j<3;j++)need(std::abs(dot(frame.axes[i],frame.axes[j]))<1e-5,"frame is not orthogonal");
  need(dot(cross(frame.axes[0],frame.axes[1]),frame.axes[2])>.99999,"frame must be right-handed");
 }
 need(part.contains("vertices")&&part["vertices"].is_array()&&!part["vertices"].empty(),"part has no vertices");
 for(const auto& vertex:part["vertices"]){const V3 p=sub(vector3(vertex,"vertex"),frame.origin);for(int i=0;i<3;i++){double v=dot(p,frame.axes[i]);frame.lo[i]=std::min(frame.lo[i],v);frame.hi[i]=std::max(frame.hi[i],v);}}
 return frame;
}
struct Mapping {size_t part_index;std::string family;Json uv,regions;};
Mapping map_part(const Json& part,size_t index,int n,int64_t seed){
 const std::string material=part.value("material","");const bool iron=material=="iron"||material=="rust";
 need(iron||material=="wood"||material=="wood_exposed"||material=="damp_wood"||material=="wood_rough", "unsupported dock material: "+material);
 const std::string id=part.at("id");const uint32_t h=hash(std::to_string(seed)+":"+id);
 const Frame frame=frame_of(part,iron);
 Mapping result{index,iron?"dock.iron.0":"dock.wood."+std::to_string(h%wood_variants),Json::array(),Json::array()};
 need(part.contains("faces")&&part["faces"].is_array()&&!part["faces"].empty(),id+": no faces");
 need(part.contains("normals")&&part["normals"].size()==part["faces"].size(),id+": evaluated loop normals required");
 need(part.contains("uvs")&&part["uvs"].size()==part["faces"].size(),id+": metric UVs required");
 const Json metadata=part.value("metadata",Json::object());const bool fractured=part.value("fractured",metadata.value("fracture",false));
 for(size_t fi=0;fi<part["faces"].size();fi++){
  const auto& face=part["faces"][fi];need(face.is_array()&&face.size()>=3,id+": invalid face");
  need(part["normals"][fi].size()==face.size()&&part["uvs"][fi].size()==face.size(),id+": loop attribute count mismatch");
  std::vector<V3> positions;positions.reserve(face.size());
  for(const auto& vertex:face){need(vertex.is_number_integer()&&vertex.get<int64_t>()>=0&&vertex.get<size_t>()<part["vertices"].size(),id+": invalid vertex index");positions.push_back(sub(part["vertices"][vertex.get<size_t>()].get<V3>(),frame.origin));}
  V3 normal{};for(size_t j=1;j+1<positions.size();j++)normal=add(normal,cross(sub(positions[j],positions[0]),sub(positions[j+1],positions[0])));
  need(dot(normal,normal)>1e-30,id+": zero-area face");normal=norm(normal);
  V3 localNormal{dot(normal,frame.axes[0]),dot(normal,frame.axes[1]),dot(normal,frame.axes[2])};
  int dominant=0;for(int c=1;c<3;c++)if(std::abs(localNormal[c])>std::abs(localNormal[dominant])+1e-12)dominant=c;
  int a=dominant==0?1:0,b=dominant==2?1:2;
  const std::string face_material=part.contains("face_materials")?part["face_materials"].at(fi).get<std::string>():material;
  double alongCenter=0;for(V3 p:positions)alongCenter+=dot(p,frame.axes[0]);alongCenter/=positions.size();
  const Json damage=part.value("wood_geometry",Json::object());
  const double tipWidth=std::min(.12,(frame.hi[0]-frame.lo[0])*.08);
  const bool atBrokenTip=(damage.value("broken_start",fractured)&&alongCenter<frame.lo[0]+tipWidth)||
                         (damage.value("broken_end",fractured)&&alongCenter>frame.hi[0]-tipWidth);
  const bool exposed=face_material=="wood_exposed"||face_material=="wood_edge"||(fractured&&(std::abs(localNormal[0])>.35||atBrokenTip));
  const bool vertical=std::abs(frame.axes[0][2])>.75;
  const int region=iron?0:(dominant==0?2:(exposed?1:(vertical?3:0)));
  const Region limits=iron?Region{0,1}:regions[region];
  // Three texels of padding around each strip protect UV samples from the
  // neighboring grain mode at normal viewing mip levels. Faces intentionally
  // overlap in a shared material; these are not unique per-piece charts.
  const double pad=std::min(3./n,(limits.high-limits.low)*.18);
  const double u0=3./n,u1=1-3./n,v0=limits.low+pad,v1=limits.high-pad;
  const double spanA=frame.hi[a]-frame.lo[a],spanB=frame.hi[b]-frame.lo[b];
  const double domainA=iron?std::max(.18,spanA*1.10):(region==2?std::max(.44,spanA*1.10):(region==3?spanA:std::max(3.6,spanA*1.10)));
  const double domainB=iron?std::max(.18,spanB*1.10):std::max(.44,spanB*1.10);
  const uint32_t faceSeed=h+uint32_t(dominant*137)+(localNormal[dominant]>0?17u:53u);
  const double offsetA=(domainA-spanA)*random01(faceSeed+19),offsetB=(domainB-spanB)*random01(faceSeed+211);
  Json uv=Json::array();
  for(V3 p:positions){double x=(dot(p,frame.axes[a])-frame.lo[a]+offsetA)/domainA,y=(dot(p,frame.axes[b])-frame.lo[b]+offsetB)/domainB;if(region==3&&frame.axes[0][2]<0)x=1-x;uv.push_back({blend(u0,u1,clamp(x)),blend(v0,v1,clamp(y))});}
  // A dominant-plane projection of a valid planar polygon cannot collapse.
  // Validate the serialized precision as well, before producing any files.
  double area=0;for(size_t j=0;j<uv.size();j++){float x=uv[j][0],y=uv[j][1],xx=uv[(j+1)%uv.size()][0],yy=uv[(j+1)%uv.size()][1];area+=double(x)*yy-double(xx)*y;}
  need(std::abs(area)>1e-15,id+": float32 UV polygon collapsed");result.uv.push_back(std::move(uv));result.regions.push_back(iron?"iron":region==2?"end_grain":region==1?"exposed":region==3?"wet_grain":"long_grain");
 }
 return result;
}
struct Pixel {V3 color{};double roughness=.8,metallic=0,height=0,specular=.25;};
Pixel wood_pixel(double x,double y,int region,int variant){
 // The core damp-wood procedure supplies the substrate in physical metres.
 // Dock-specific radial fibres, weathering and checks are added analytically.
 const double age=.78+.035*variant,wet=.16+.09*variant;
 const Json part={{"wood_finish",{{"age",age},{"wetness",wet},{"grain_axis","X"},{"height_m",5.0}}}};
 const double offset=variant*13.719;
 const V3 p{x+offset,y+variant*.273,1.4+variant*.097};
 MaterialSample substrate=sample_material(part,"wood",p);
 const double macro=blender_noise({x*.48+offset,y*5.1,variant*.7},1,2,.65);
 const double fibre=blender_noise({x*1.65+offset,y*78.,variant*3.21},1,2,.64);
 const double warp=.016*(blender_noise({x*1.35+offset,y*4.,variant*.31},1,2,.6)-.5);
 const double ring=.5+.5*std::sin((y+warp)*2*pi*(68+variant*2.2)+std::sin(x*1.7)*.35);
 double grain=.79*fibre+.21*ring,knot=0;
 // Sparse, elongated knots bend fibres locally instead of bending every
 // growth line into the same repeated sine wave down the whole board.
 for(int k=0;k<2;k++){
  const uint32_t seed=uint32_t(variant*9871+k*739+311);
  const double cx=.25+3.1*random01(seed),cy=.055+.33*random01(seed+57);
  const double dx=(x-cx)*.14,dy=y-cy,radius=std::sqrt(dx*dx+dy*dy);
  const double mask=std::exp(-radius*radius/.0016);
  const double rings=.5+.5*std::sin(radius*640+2*fibre);
  grain=blend(grain,.62*fibre+.38*rings,mask*.80);
  knot=std::max(knot,std::exp(-radius*radius/.000055));
 }
 double check=0;
 for(int c=0;c<7;c++){
  const uint32_t seed=uint32_t(variant*9871+c*1123+157);
  const double center=.44*random01(seed),start=3.6*random01(seed+113),length=.25+1.8*random01(seed+229);
  const double curve=center+.0019*std::sin(x*12+c)+.0035*std::sin(x*3.8+c*1.7);
  const double width=.0008+.0018*random01(seed+331),distance=std::abs(y-curve);
  const double taper=smooth((x-start)/.10)*smooth((start+length-x)/.17);
  check=std::max(check,(1-smooth(distance/width))*taper);
 }
 const double substrateGray=dot(substrate.base_color,{.2126,.7152,.0722});
 const double tone=blend(.029,.076,grain);
 V3 silver{tone*1.015,tone,tone*.965};
 Pixel pixel;pixel.color=mix(mul({1.02,1.,.95},substrateGray*.8),silver,.74);
 // Exposed dock wood loses its warm finish. Broken silver fibres and uneven
 // oxidised patches are part of the albedo, not artificial white highlights.
 const double patch=blender_noise({x*4.6+offset,y*14.3,variant*1.9},1,3,.68);
 const double fibres=blender_noise({x*7.2+offset,y*118.,variant*2.7},1,2,.67);
 const double silvering=smooth((patch-.44)/.20)*smooth((fibres-.34)/.28);
 pixel.color=mix(pixel.color,{.174,.177,.169},silvering*.69);
 constexpr double shades[]={.72,1.08,.58,.91,1.20,.79};
 pixel.color=mul(pixel.color,shades[variant]);
 const double damp=smooth((macro-.40)/.35)*wet;
 pixel.color=mul(pixel.color,blend(1,.46,damp));
 pixel.color=mix(pixel.color,{.008,.008,.007},check*.88);
 pixel.color=mix(pixel.color,{.020,.018,.014},knot*.72);
 pixel.roughness=std::clamp(.84+fibre*.10-damp*.30,.35,.98);
 pixel.height=(grain-.5)*.00078+(fibre-.5)*.00043-check*.00085+silvering*.00023;
 pixel.specular=.22;
 if(region==1){pixel.color=mix(mul(pixel.color,1.32),{.106,.087,.060},.22);pixel.roughness=.94;pixel.height*=1.18;}
 if(region==3){
  const double irregular=.10*blender_noise({x*2.5,y*22.,variant*2.3},1,2,.7);
  const double wetBand=1-smooth((x/3.6-.22-irregular)/.20);
  pixel.color=mul(pixel.color,1-wetBand*.76);
  pixel.color=mix(pixel.color,{.018,.024,.019},wetBand*.17);
  pixel.roughness=std::clamp(pixel.roughness-wetBand*.20,.4,.96);
 }
 if(region==2){
  const double cx=.13+.045*variant,cy=.22+.02*std::sin(variant),dx=x-cx,dy=y-cy;
  const double radius=std::sqrt(dx*dx+dy*dy),angle=std::atan2(dy,dx);
  const double growth=.5+.5*std::sin(radius*(255+variant*11)+1.4*blender_noise({x*23,y*23,double(variant)},1,2,.6));
  const double split=std::pow(std::max(0.,std::cos(angle*7+variant*.7)),60)*smooth((radius-.03)/.09);
  const double n=blender_noise({x*61,y*61,double(variant)},1,2,.65);
  pixel.color=mix({.033,.031,.028},{.092,.088,.080},growth*.70+n*.30);pixel.color=mul(pixel.color,shades[variant]);pixel.color=mix(pixel.color,{.018,.017,.014},split*.7);
  pixel.roughness=.86+n*.10;pixel.height=(growth-.5)*.00042-split*.0006;
 }
 return pixel;
}
struct Atlas {Json textures;};
Atlas bake_atlas(const std::string& family,const std::filesystem::path& directory,int n){
 const bool iron=family=="dock.iron.0";const int variant=iron?0:family.back()-'0';
 const size_t count=size_t(n)*n;std::vector<V3> color(count),normal(count),params(count),orm(count),surface(count),transmission(count,{0,0,0}),specular(count,{1,1,1});
 std::vector<double> heights(count),alpha(count);std::vector<int> regionIds(count);
 const Json ironPart=Json::object();
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){
  const size_t i=size_t(y)*n+x;const double u=(x+.5)/n,v=(y+.5)/n;
  const int region=iron?0:(v<long_end?0:(v<exposed_end?1:(v<wet_end?3:2)));regionIds[i]=region;
  const Region strip=iron?Region{0,1}:regions[region];
  // The procedural field is clamped inside strip gutters. This is analytic
  // edge dilation, not transparent padding or an unbaked background.
  const double padding=std::min(3./n,(strip.high-strip.low)*.18);
  const double xx=clamp((u-3./n)/(1-6./n))*(iron?.18:(region==2?.44:3.6));
  const double yy=clamp((v-strip.low-padding)/(strip.high-strip.low-2*padding))*(iron?.18:.44);
  Pixel p;
  if(iron){auto s=sample_material(ironPart,"iron",{xx,yy,.037});p={s.base_color,s.roughness,s.metallic,s.height*s.bump_distance*s.bump_strength,s.specular};}
  else p=wood_pixel(xx,yy,region,variant);
  color[i]=p.color;params[i]={p.roughness,p.metallic,0};orm[i]={1,p.roughness,p.metallic};surface[i]={0,p.specular,.25};heights[i]=p.height;alpha[i]=2*p.specular;
 }
 for(int y=0;y<n;y++)for(int x=0;x<n;x++){
  const size_t i=size_t(y)*n+x;const int region=regionIds[i];const Region strip=iron?Region{0,1}:regions[region];
  auto height=[&](int xx,int yy){xx=std::clamp(xx,0,n-1);yy=std::clamp(yy,0,n-1);return regionIds[size_t(yy)*n+xx]==region?heights[size_t(yy)*n+xx]:heights[i];};
  const double padding=std::min(3./n,(strip.high-strip.low)*.18);
  const double dx=(iron?.18:(region==2?.44:3.6))/(n-6.);
  const double dy=(iron?.18:.44)/(n*(strip.high-strip.low-2*padding));
  const V3 value=norm({-(height(x+1,y)-height(x-1,y))/(2*dx),-(height(x,y+1)-height(x,y-1))/(2*dy),1});
  normal[i]=add(mul(value,.5),{.5,.5,.5});
 }
 Atlas atlas;auto& t=atlas.textures;t=Json::object();
 for(const char* key:{"base_color","normal","orm","transmission","surface","specular_weight","material_params"})t[key]=(directory/(family+"."+key+".png")).string();
 write_png16(t["base_color"],n,n,color,true);write_png16(t["normal"],n,n,normal);write_png16(t["orm"],n,n,orm);write_png16(t["transmission"],n,n,transmission);write_png16(t["surface"],n,n,surface);write_png16(t["specular_weight"],n,n,specular,false,alpha);write_png16(t["material_params"],n,n,params);
 t["resolution"]=n;t["pixel_format"]="PNG16_UNORM";t["normal_convention"]="OpenGL +Y tangent";t["material_params_channels"]={"roughness","metallic","transmission"};t["surface_channels"]={"coat","specular","(ior-1)/2"};t["shared_family"]=family;
 return atlas;
}
}
void bake_dock_materials(Json& scene,const std::string& directory,int resolution){
 need(resolution>=32&&resolution<=4096,"resolution must be in [32,4096]");need(!directory.empty(),"output directory is empty");
 need(scene.contains("parts")&&scene["parts"].is_array(),"scene parts must be an array");
 const int64_t seed=recipe_seed(scene.value("recipe",Json::object()).value("seed",Json(0)));
 std::vector<Mapping> mappings;std::map<std::string,Atlas> atlases;
 for(size_t i=0;i<scene["parts"].size();i++){auto mapping=map_part(scene["parts"][i],i,resolution,seed);atlases.emplace(mapping.family,Atlas{});mappings.push_back(std::move(mapping));}
 // Validate the entire scene first, so invalid later parts do not mutate
 // earlier parts or leave partially assigned texture layers behind.
 std::filesystem::create_directories(directory);
 for(auto& entry:atlases)entry.second=bake_atlas(entry.first,directory,resolution);
 for(const auto& mapping:mappings){auto& part=scene["parts"][mapping.part_index];part["texture_uvs"]=mapping.uv;part["textures"]=atlases.at(mapping.family).textures;part["material_shared_id"]=mapping.family;part["material_constants"]={{"ior",1.5},{"coat",0},{"transmission",0},{"coat_roughness",.03}};part["material_provenance"]={{"family","dock"},{"generator","native C++17"},{"shared_variant",mapping.family},{"grain_space","timber_frame"},{"face_regions",mapping.regions}};}
 scene["metadata"]["native_material_bake"]={{"generator","C++"},{"native_images",true},{"maximum_resolution",resolution},{"metric_uvs_preserved",true},{"pixel_format","PNG16_UNORM"},{"texture_uv_layer","NativeTextureUV"},{"family","dock"},{"shared_material_count",atlases.size()},{"maximum_wood_variants",wood_variants},{"material_sharing","one atlas per family/variant; never per timber"},{"normal_filter","central texel footprint"},{"normal_camera_dependent",false}};
}
}
