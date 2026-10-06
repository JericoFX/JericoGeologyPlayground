#include <iostream>
#if !__has_include("jerico/dock_materials.hpp")
int main() { std::cerr << "FAIL: dock material baker has not been implemented\n"; return 1; }
#else
#include "jerico/dock_materials.hpp"
#include "jerico/evaluate.hpp"
#include "jerico/dock_geometry.hpp"
#include "jerico/export.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <limits>
#include <set>
#include <stdexcept>
#include <zlib.h>
using namespace jerico;
namespace {
void require(bool condition,const char* message) { if(!condition) throw std::runtime_error(message); }
std::vector<unsigned char> bytes(const std::string& path) {
 std::ifstream input(path,std::ios::binary); require(bool(input),"missing texture file");
 return {std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
}
struct Image { int width=0,height=0,channels=0; std::vector<unsigned char> data; V3 pixel(int x,int y,bool srgb=false) const {
 const size_t i=size_t(height-1-y)*(width*channels*2+1)+1+x*channels*2; V3 p{};
 for(int c=0;c<3;c++){ double value=(data.at(i+c*2)*256+data.at(i+c*2+1))/65535.;p[c]=srgb?(value<=.04045?value/12.92:std::pow((value+.055)/1.055,2.4)):value; }return p;
} };
Image png(const std::string& path) {
 auto b=bytes(path); auto be=[&](size_t i){return uint32_t(b.at(i))<<24|uint32_t(b.at(i+1))<<16|uint32_t(b.at(i+2))<<8|b.at(i+3);};
 require(b.size()>33&&b[0]==137&&b[1]=='P'&&b[24]==16,"expected native PNG16");
 Image image; image.width=int(be(16));image.height=int(be(20));image.channels=b[25]==6?4:3;
 std::vector<unsigned char> compressed;
 for(size_t i=8;i<b.size();){size_t n=be(i);require(i+n+12<=b.size(),"PNG chunk exceeds file");if(std::string(b.begin()+i+4,b.begin()+i+8)=="IDAT")compressed.insert(compressed.end(),b.begin()+i+8,b.begin()+i+8+n);i+=n+12;}
 uLongf decoded=size_t(image.height)*(image.width*image.channels*2+1);image.data.resize(decoded);
 require(uncompress(image.data.data(),&decoded,compressed.data(),compressed.size())==Z_OK&&decoded==image.data.size(),"independent PNG decode failed");
 for(int row=0;row<image.height;row++)require(image.data[size_t(row)*(image.width*image.channels*2+1)]==0,"unexpected PNG filter");
 return image;
}
Json make_scene(int count=30) {
 Builder builder({{"seed",2718}}); builder.node("frame",{0,0,0},"Dock",nullptr);
 for(int i=0;i<count;i++){
  auto& p=builder.box("deck."+std::to_string(i),"frame",0,2,-.1,.1,-.035,.035,"wood",0,"Plank");
  p["timber_frame"]={{"origin",{0,0,0}},{"u",{1,0,0}},{"v",{0,1,0}},{"w",{0,0,1}}};
  p["timber_length"]=2.;p["timber_width"]=.2;p["timber_thickness"]=.07;p["fractured"]=i%4==0;
 }
 builder.cylinder("fastener","frame",{0,0,0},{0,0,.07},.012,"iron",8,0);
 builder.scene["metadata"]["family"]="dock";
 return evaluate_scene(builder.scene);
}
void verify_uvs(const Json& before,const Json& after) {
 for(size_t i=0;i<after["parts"].size();i++){
  const auto& part=after["parts"][i];const auto& original=before["parts"][i];
  for(const char* key:{"vertices","faces","normals","uvs"})require(part[key]==original[key],"material bake changed geometry, normals or metric UVs");
  require(part["texture_uvs"].size()==part["faces"].size(),"texture UV face count mismatch");
  for(size_t f=0;f<part["faces"].size();f++){
   const auto& uv=part["texture_uvs"][f];require(uv.size()==part["faces"][f].size(),"texture UV corner count mismatch");double area=0;
   for(size_t j=0;j<uv.size();j++){float u=uv[j][0],v=uv[j][1],u1=uv[(j+1)%uv.size()][0],v1=uv[(j+1)%uv.size()][1];require(std::isfinite(u)&&std::isfinite(v)&&u>0&&u<1&&v>0&&v<1,"UV not safely inside shared atlas");area+=double(u)*v1-double(u1)*v;}
   require(std::abs(area)>1e-10,"UV collapsed after float32 export");
  }
 }
}
}
int main(int argc,char** argv) { try {
 const std::filesystem::path root=argc>1?argv[1]:"/tmp/jerico-dock-material-tests";std::filesystem::create_directories(root);
 auto source=make_scene(),scene=source;
 bake_dock_materials(scene,(root/"a").string(),64);verify_uvs(source,scene);
 std::set<std::string> woodIds,woodPaths;std::set<std::string> allPaths;
 for(const auto& part:scene["parts"]){
  require(part.contains("material_shared_id"),"shared material ID missing");
  const auto& t=part.at("textures");
  for(const char* key:{"base_color","normal","orm","transmission","surface","specular_weight","material_params"}){require(t.contains(key),"GLB/OBJ material texture key missing");allPaths.insert(t[key].get<std::string>());auto image=png(t[key]);require(image.width==64&&image.height==64,"resolution not honored");}
  require(part.at("material_constants").at("ior").get<double>()>=1,"valid material IOR missing");
  if(part["material"]=="wood"){woodIds.insert(part["material_shared_id"]);woodPaths.insert(t["base_color"]);}
 }
 require(woodIds.size()>=4&&woodIds.size()<=6,"wood family must contain 4-6 deterministic variants for many boards");
 require(woodPaths.size()==woodIds.size(),"wood textures are not shared by material family");
 require(allPaths.size()<=7*9,"texture count grows with pieces instead of bounded families");
 require(scene["metadata"]["native_material_bake"]["native_images"]==true,"native GLB compatibility metadata missing");
 auto shuffled=source;std::reverse(shuffled["parts"].begin(),shuffled["parts"].end());bake_dock_materials(shuffled,(root/"b").string(),64);
 for(size_t i=0;i<scene["parts"].size();i++){
  const auto& a=scene["parts"][i];const auto& b=shuffled["parts"][scene["parts"].size()-1-i];
  require(a["material_shared_id"]==b["material_shared_id"]&&a["texture_uvs"]==b["texture_uvs"],"family or UV depends on iteration order");
  require(bytes(a["textures"]["base_color"])==bytes(b["textures"]["base_color"]),"bake is not byte deterministic");
 }
 auto rotated=make_scene(1);rotated["parts"].erase(1);auto original=rotated;
 V3 u=norm({.7,.5,.2}),v=norm(cross({0,0,1},u)),w=cross(u,v),origin={17,-4,9};
 auto& rp=rotated["parts"][0];for(auto& vertex:rp["vertices"]){V3 p=vertex;vertex=add(origin,add(mul(u,p[0]),add(mul(v,p[1]),mul(w,p[2]))));}
 rp["timber_frame"]={{"origin",origin},{"u",u},{"v",v},{"w",w}};
 rp["uvs"]=metric_uvs(rp["vertices"],rp["faces"]);rp=evaluate_part(rp);
 bake_dock_materials(original,(root/"orientation-a").string(),64);bake_dock_materials(rotated,(root/"orientation-b").string(),64);
 for(size_t f=0;f<rp["texture_uvs"].size();f++)for(size_t c=0;c<rp["texture_uvs"][f].size();c++)for(int k=0;k<2;k++)require(std::abs(rp["texture_uvs"][f][c][k].get<double>()-original["parts"][0]["texture_uvs"][f][c][k].get<double>())<1e-10,"wood grain did not follow timber frame rotation");
 auto& first=scene["parts"][0];auto normal=png(first["textures"]["normal"]),color=png(first["textures"]["base_color"]),orm=png(first["textures"]["orm"]);
 double variation=0,along=0,across=0,red=0,blue=0;V3 prev=color.pixel(2,2,true);
 for(int y=4;y<32;y++)for(int x=4;x<60;x++){
  auto n=normal.pixel(x,y),c=color.pixel(x,y,true),o=orm.pixel(x,y);double length=0;for(int k=0;k<3;k++){length+=std::pow(2*n[k]-1,2);variation+=std::abs(c[k]-prev[k]);}prev=c;
  red+=c[0];blue+=c[2];
  require(std::abs(std::sqrt(length)-1)<.00006&&n[2]>.5,"normal PNG is not unit tangent-space +Z");
  require(o[0]>.99&&o[1]>.25&&o[1]<=1&&o[2]<.001,"wood ORM physical channel bounds");along+=std::abs(n[0]-.5);across+=std::abs(n[1]-.5);
 }
 require(variation>1,"weathered wood albedo is flat");require(across>along*1.2,"long grain orientation is missing from normal map");
 require(blue/red>.84,"weathered wood is still beige rather than desaturated gray");
 Builder irregular({{"seed",91}});irregular.node("frame",{0,0,0},"Dock",nullptr);
 dock_timber(irregular,"splintered.board",{0,0,1.5},{2.4,.8,1.8},{0,1,0},.18,.075,"DeckBoard",91,true,true,.9);
 dock_timber(irregular,"wet.pile",{0,0,-.7},{0,0,1.6},{1,0,0},.2,.2,"Pile",91,false,false,.7);
 auto irregularScene=evaluate_scene(irregular.scene),irregularSource=irregularScene;
 bake_dock_materials(irregularScene,(root/"irregular").string(),64);verify_uvs(irregularSource,irregularScene);
 std::set<std::string> faceRegions;for(const auto& region:irregularScene["parts"][0]["material_provenance"]["face_regions"])faceRegions.insert(region);
 require(faceRegions.count("long_grain")&&faceRegions.count("end_grain")&&faceRegions.count("exposed"),"fracture does not expose different wood regions");
 std::set<std::string> pileRegions;for(const auto& region:irregularScene["parts"][1]["material_provenance"]["face_regions"])pileRegions.insert(region);
 require(pileRegions.count("wet_grain"),"vertical pile has no lower wet-weathering region");
 const auto pileColor=png(irregularScene["parts"][1]["textures"]["base_color"]);double lower=0,upper=0;
 for(int y=48;y<53;y++)for(int x=6;x<15;x++){auto low=pileColor.pixel(x,y,true),high=pileColor.pixel(x+40,y,true);lower+=low[0]+low[1]+low[2];upper+=high[0]+high[1]+high[2];}
 require(lower<upper*.72,"lower pile weathering is not visibly darker");
 write_glb(irregularScene,(root/"irregular.glb").string());
 const auto glb=bytes((root/"irregular.glb").string());require(glb.size()>20&&std::string(glb.begin(),glb.begin()+4)=="glTF","native GLB did not accept dock material/UV contract");
 auto bad=make_scene(1);bad["parts"][0]["timber_frame"]["u"]={0,0,0};bool rejected=false;try{bake_dock_materials(bad,(root/"bad").string(),64);}catch(const std::exception&){rejected=true;}require(rejected,"degenerate frame accepted");
 bad=make_scene(1);rejected=false;try{bake_dock_materials(bad,(root/"bad-resolution").string(),0);}catch(const std::exception&){rejected=true;}require(rejected,"zero resolution accepted");
 std::cout<<"Dock materials: shared deterministic families, PNG16 channels, frame-aligned UVs, geometry preservation and validation passed\n";return 0;
 }catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;} }
#endif
