#include "jerico/export.hpp"
#include "jerico/evaluate.hpp"
#include "jerico/materials.hpp"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <zlib.h>
using namespace jerico;
namespace {
struct DecimalComma:std::numpunct<char>{char do_decimal_point()const override{return ',';}};
void require(bool ok,const std::string&message){if(!ok)throw std::runtime_error(message);}
std::string read(const std::filesystem::path&path){std::ifstream f(path,std::ios::binary);require(bool(f),"missing file "+path.string());return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
uint32_t be(const std::string&b,size_t i){return uint32_t(static_cast<unsigned char>(b.at(i)))<<24|uint32_t(static_cast<unsigned char>(b.at(i+1)))<<16|uint32_t(static_cast<unsigned char>(b.at(i+2)))<<8|static_cast<unsigned char>(b.at(i+3));}
std::vector<unsigned char> png_rows(const std::filesystem::path&p){auto b=read(p);require(b.size()>33&&b[1]=='P'&&b[2]=='N'&&b[3]=='G'&&b[24]==16&&b[25]==2,"RGB16 PNG required");std::string z;for(size_t at=8;at+12<=b.size();){auto n=be(b,at);if(b.substr(at+4,4)=="IDAT")z+=b.substr(at+8,n);at+=12+n;}std::vector<unsigned char> raw((size_t(be(b,16))*6+1)*be(b,20));uLongf length=raw.size();require(uncompress(raw.data(),&length,reinterpret_cast<const unsigned char*>(z.data()),z.size())==Z_OK&&length==raw.size(),"independent PNG inflate");const size_t stride=size_t(be(b,16))*6+1;for(size_t y=0;y<be(b,20);++y){const unsigned filter=raw[y*stride];require(filter<=4,"PNG filter range");for(size_t x=1;x<stride;++x){const int left=x>=7?raw[y*stride+x-6]:0,up=y?raw[(y-1)*stride+x]:0,ul=y&&x>=7?raw[(y-1)*stride+x-6]:0;const int p=left+up-ul;const int pa=std::abs(p-left),pb=std::abs(p-up),pc=std::abs(p-ul);const int predictor=filter==0?0:filter==1?left:filter==2?up:filter==3?(left+up)/2:pa<=pb&&pa<=pc?left:pb<=pc?up:ul;raw[y*stride+x]=static_cast<unsigned char>(raw[y*stride+x]+predictor);}raw[y*stride]=0;}return raw;}
}
int main(int argc,char**argv){try{
 const std::filesystem::path root=argc>1?argv[1]:"/tmp/jerico-obj-test";std::filesystem::create_directories(root);
 Builder b(normalize_recipe(Json::object()));b.node("frame",{1,2,3},"Frame",nullptr,.2);b.node("leaf",{.3,-.4,.5},"Leaf","frame",.4);b.box("glass.panel","leaf",-1,1,-.03,.03,0,2,"glass",0);b.box("steel.panel","frame",-1,1,-.04,.04,0,2,"painted_steel",0);
 auto scene=evaluate_scene(b.scene);bake_material_textures(scene,(root/"native").string(),32);std::ofstream(root/"scene.json")<<scene.dump(2);
 const auto saved_locale=std::locale();std::locale::global(std::locale(saved_locale,new DecimalComma));
 try{write_obj_mtl(scene,(root/"asset.obj").string());}catch(...){std::locale::global(saved_locale);throw;}
 std::locale::global(saved_locale);auto obj=read(root/"asset.obj"),mtl=read(root/"asset.mtl"),notes=read(root/"asset.README.txt");
 require(mtl.find("Ni 1.52\n")!=std::string::npos,"OBJ/MTL numeric transport inherited host decimal-comma locale");
 require(obj.find("mtllib asset.mtl")!=std::string::npos,"relative MTL reference missing");require(notes.find("transmission")!=std::string::npos&&notes.find("Z-up")!=std::string::npos,"OBJ limitations/basis not documented");
 std::vector<V3> positions,normals;std::vector<V2> uvs;std::istringstream stream(obj);std::string line;size_t faces=0,objects=0;
 while(std::getline(stream,line)){std::istringstream row(line);std::string kind;row>>kind;if(kind=="v"||kind=="vn"){V3 value{};row>>value[0]>>value[1]>>value[2];for(double x:value)require(std::isfinite(x),"nonfinite OBJ attribute");if(kind=="v")positions.push_back(value);else{require(std::abs(dot(value,value)-1)<1e-10,"OBJ normal not unit");normals.push_back(value);}}else if(kind=="vt"){V2 uv{};row>>uv[0]>>uv[1];require(uv[0]>=0&&uv[0]<=1&&uv[1]>=0&&uv[1]<=1,"OBJ atlas UV bounds");uvs.push_back(uv);}else if(kind=="o")++objects;else if(kind=="f"){std::vector<std::array<size_t,3>> corners;std::string token;while(row>>token){std::replace(token.begin(),token.end(),'/',' ');std::istringstream indices(token);std::array<size_t,3> c{};indices>>c[0]>>c[1]>>c[2];require(c[0]>0&&c[0]<=positions.size()&&c[1]>0&&c[1]<=uvs.size()&&c[2]>0&&c[2]<=normals.size(),"OBJ index bounds");corners.push_back(c);}require(corners.size()==3,"OBJ must contain triangles");auto a=positions[corners[0][0]-1],bb=positions[corners[1][0]-1],c=positions[corners[2][0]-1];const auto ng=norm(cross(sub(bb,a),sub(c,a)));for(auto corner:corners)require(dot(ng,normals[corner[2]-1])>0,"OBJ loop hemisphere mismatch");++faces;}}
 require(objects==2&&faces==24,"OBJ object/triangle counts");const auto expected=world_point(scene,"leaf",scene["parts"][0]["vertices"][0].get<V3>());require(std::sqrt(dot(sub(positions[0],expected),sub(positions[0],expected)))<1e-12,"OBJ positions not in native world coordinates");
 require(mtl.find("Ni 1.52")!=std::string::npos,"glass IOR missing");require(mtl.find("\nd 1\n")!=std::string::npos&&mtl.find("\nTr ")==std::string::npos,"transmission must not be faked by alpha");
 std::istringstream materials(mtl);std::string material;size_t color_count=0,normal_count=0,rough_count=0,metal_count=0;
 while(std::getline(materials,line)){std::istringstream row(line);std::string key,value;row>>key>>value;if(key=="newmtl"){material=value;continue;}if(key!="map_Kd"&&key!="norm"&&key!="map_Pr"&&key!="map_Pm")continue;require(!value.empty()&&!std::filesystem::path(value).is_absolute()&&value.find("..") ==std::string::npos,"nonrelative texture reference");const auto image=root/value;require(std::filesystem::exists(image),"missing package image");size_t pi=material.find("glass.panel")!=std::string::npos?0:1;
 if(key=="map_Kd"){++color_count;require(read(image)==read(scene["parts"][pi]["textures"]["base_color"].get<std::string>()),"base color changed during copy");}
 else if(key=="norm"){++normal_count;require(read(image)==read(scene["parts"][pi]["textures"]["normal"].get<std::string>()),"normal map changed during copy");}
 else {key=="map_Pr"?++rough_count:++metal_count;const auto params=png_rows(scene["parts"][pi]["textures"]["material_params"].get<std::string>()),gray=png_rows(image);require(params.size()==gray.size(),"scalar map dimensions");auto bytes=read(image);size_t stride=size_t(be(bytes,16))*6+1;size_t channel=key=="map_Pr"?0:2;for(size_t y=0;y<be(bytes,20);++y){require(params[y*stride]==0&&gray[y*stride]==0,"independent PNG decoder must reconstruct complete scanlines");for(size_t x=1;x<stride;x+=6)for(size_t c=0;c<6;++c)require(gray[y*stride+x+c]==params[y*stride+x+channel+c%2],"rough/metal channel extraction or row orientation mismatch");}}
 }
 require(color_count==2&&normal_count==2&&rough_count>=1&&metal_count>=1,"required MTL maps missing");
 auto bad=scene;bad["metadata"].erase("native_material_bake");bool rejected=false;try{write_obj_mtl(bad,(root/"bad.obj").string());}catch(const std::exception&){rejected=true;}require(rejected,"OBJ accepted non-native images");
 std::cout<<"OBJ/MTL export passed: 2 parts, 24 triangles, native textures and exact scalar channels\n";
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}}
