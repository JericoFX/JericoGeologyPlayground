#include "jerico/materials.hpp"
#include <cmath>
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include <cstring>
#include <array>
#include <zlib.h>
using namespace jerico;
void close(double a,double b,double e,const char* name){if(std::abs(a-b)>e)throw std::runtime_error(std::string(name)+": expected "+std::to_string(b)+", got "+std::to_string(a));}
V3 read_pfm(const std::string&path,int x,int y){std::ifstream f(path,std::ios::binary);std::string format;int w,h;float scale;f>>format>>w>>h>>scale;f.get();if(format!="PF"||scale!=-1||x>=w||y>=h)throw std::runtime_error("invalid float map");f.seekg((static_cast<std::streamoff>(y)*w+x)*12,std::ios::cur);float p[3];f.read(reinterpret_cast<char*>(p),12);if(!f)throw std::runtime_error("truncated float map");return {p[0],p[1],p[2]};}
V3 read_png16(const std::string&path,int x,int y,bool color){std::ifstream f(path,std::ios::binary);std::vector<unsigned char>b((std::istreambuf_iterator<char>(f)),{});auto be=[&](size_t i){return uint32_t(b.at(i))<<24|uint32_t(b.at(i+1))<<16|uint32_t(b.at(i+2))<<8|b.at(i+3);};if(b.size()<30||b[0]!=137||b[24]!=16)throw std::runtime_error("not native PNG16");int width=be(16),height=be(20),channels=b[25]==6?4:3;std::vector<unsigned char>z,raw;for(size_t i=8;i<b.size();){size_t n=be(i);if(b[i+4]=='I'&&b[i+5]=='D'&&b[i+6]=='A'&&b[i+7]=='T')z.insert(z.end(),b.begin()+i+8,b.begin()+i+8+n);i+=n+12;}uLongf decoded=size_t(height)*(width*channels*2+1);raw.resize(decoded);if(uncompress(raw.data(),&decoded,z.data(),z.size())!=Z_OK||decoded!=raw.size())throw std::runtime_error("PNG16 independent zlib decode failed");size_t i=(size_t(height-1-y)*(width*channels*2+1))+1+x*channels*2;V3 p;for(int c=0;c<3;c++){double v=(raw.at(i+2*c)*256+raw.at(i+2*c+1))/65535.;p[c]=color?(v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4)):v;}return p;}
V3 read_tga(const std::string&path,int x,int y,bool color){if(path.size()>4&&path.substr(path.size()-4)==".png")return read_png16(path,x,y,color);std::ifstream f(path,std::ios::binary);unsigned char h[18];f.read(reinterpret_cast<char*>(h),18);int width=h[12]+256*h[13];if(h[2]!=2||h[16]!=24||h[17]!=0)throw std::runtime_error("unexpected native TGA encoding");f.seekg((static_cast<std::streamoff>(y)*width+x)*3,std::ios::cur);unsigned char b[3];f.read(reinterpret_cast<char*>(b),3);V3 p;for(int i=0;i<3;i++){double v=b[2-i]/255.;p[i]=color?(v<=.04045?v/12.92:std::pow((v+.055)/1.055,2.4)):v;}return p;}
int main(int argc,char**argv){try{
 Json p={{"id","glass"},{"material","glass"},{"glass_study",true},{"glass_bounds",{-1.,1.,0.,2.}}};
 auto edge=sample_material(p,"glass_edge",{0,0,1});
 close(edge.roughness,.16,1e-6,"approved glass edge roughness");
 close(edge.transmission,.58,1e-6,"approved glass edge transmission");
 close(edge.base_color[0],.48,1e-6,"approved glass edge linear color");
 auto clean=sample_material(p,"glass",{0,0,1});
 close(clean.roughness,.095,1e-6,"clean glass roughness");
 close(clean.transmission,.96,1e-6,"clean glass transmission");
 close(clean.height,0,1e-6,"clean glass flat normal");
 if(argc>1){std::ifstream file(argv[1]);Json oracle;file>>oracle;double max_error=0;size_t n=0;
  for(auto& test:oracle["samples"]){auto part=test["part"];part["_material_family"]=test["family"];auto s=sample_material(part,test["material"],test["position"].get<V3>());auto expected=test["expected"];
   auto check=[&](const char*key,double val){double error=std::abs(val-expected[key].get<double>());max_error=std::max(max_error,error);close(val,expected[key],3e-5,(test["id"].get<std::string>()+" "+key).c_str());};
   for(int i=0;i<3;i++){double e=std::abs(s.base_color[i]-expected["base_color"][i].get<double>());max_error=std::max(max_error,e);close(s.base_color[i],expected["base_color"][i],3e-5,(test["id"].get<std::string>()+" oracle base_color").c_str());}
   check("roughness",s.roughness);check("metallic",s.metallic);check("transmission",s.transmission);check("ior",s.ior);check("coat",s.coat);check("height",s.height);check("bump_strength",s.bump_strength);check("bump_distance",s.bump_distance);check("specular",s.specular);++n;
   if(test.contains("textures")){int x=test["pixel"][0],y=test["pixel"][1];auto maps=test["textures"];auto linear=read_pfm(maps["base_color_linear"],x,y),rawheight=read_pfm(maps["height"],x,y),color=read_tga(maps["base_color"],x,y,true),packed=read_tga(maps["material_params"],x,y,false);for(int i=0;i<3;i++){close(linear[i],expected["base_color"][i],3e-5,"native float atlas vs original Blender graph");close(color[i],expected["base_color"][i],.004,"native color texture quantization");}close(rawheight[2],expected["height"],3e-5,"native height atlas vs original Blender graph");close(packed[0],expected["roughness"],.5/255+3e-5,"native roughness texture");close(packed[1],expected["metallic"],.5/255+3e-5,"native metallic texture");close(packed[2],expected["transmission"],.5/255+3e-5,"native transmission texture");}
  }std::cout<<"Blender oracle: "<<n<<" samples; max absolute error "<<max_error<<"\n";
 }

 Json part={{"id","atlas.test"},{"node","frame"},{"material","painted_steel"},{"vertices",{{-1.,-.03,0.},{1.,-.03,0.},{1.,-.03,2.},{-1.,-.03,2.}}},{"faces",{{0,1,2,3}}},{"uvs",{{{0.,0.},{2.,0.},{2.,2.},{0.,2.}}}}};
 Json scene={{"parts",Json::array({part})},{"metadata",Json::object()}};
 auto report=bake_material_textures(scene,"/tmp/jerico-material-test-atlas",64);
 if(!scene["parts"][0].contains("texture_uvs"))throw std::runtime_error("native atlas missing texture_uvs");
 if(scene["parts"][0]["uvs"]!=part["uvs"])throw std::runtime_error("metric UV changed");
 if(!std::filesystem::exists(scene["parts"][0]["textures"]["base_color"].get<std::string>()))throw std::runtime_error("native atlas texture missing");
 if(report["baked_parts"]!=1)throw std::runtime_error("native atlas no parts baked");
 std::ofstream("/tmp/jerico-material-test-atlas/report.json")<<report.dump(2); 

 auto thin=part;thin["id"]="thin.bevel.strip";thin["vertices"]={{-1.,-.03,0.},{1.,-.03,0.},{1.,-.03,.0001},{-1.,-.03,.0001}};thin["uvs"]={{{0.,0.},{2.,0.},{2.,.0001},{0.,.0001}}};
 Json narrowScene={{"parts",Json::array({thin})},{"metadata",Json::object()}};
 auto narrow=bake_material_textures(narrowScene,"/tmp/jerico-material-test-thin",64);
 if(narrow["parts"][0]["covered_pixels"]==0)throw std::runtime_error("thin bevel strip received no material texels");

 auto smoothpart=part;smoothpart["normals"]={{{.2,-std::sqrt(.95),.1},{.2,-std::sqrt(.95),.1},{.2,-std::sqrt(.95),.1},{.2,-std::sqrt(.95),.1}}};
 Json smoothscene={{"parts",Json::array({smoothpart})},{"metadata",Json::object()}};auto sr=bake_material_textures(smoothscene,"/tmp/jerico-material-test-smooth",64);
 auto mirrored=smoothpart;mirrored["uvs"]={{{2.,0.},{0.,0.},{0.,2.},{2.,2.}}};Json mirrorscene={{"parts",Json::array({mirrored})},{"metadata",Json::object()}};auto mr=bake_material_textures(mirrorscene,"/tmp/jerico-material-test-mirror",64);
 auto originalUV=smoothscene["parts"][0]["texture_uvs"][0].get<std::vector<V2>>(),mirrorUV=mirrorscene["parts"][0]["texture_uvs"][0].get<std::vector<V2>>();
 auto tangentBasis=[](const std::vector<V2>&uv){V2 a={uv[1][0]-uv[0][0],uv[1][1]-uv[0][1]},b={uv[3][0]-uv[0][0],uv[3][1]-uv[0][1]};double det=a[0]*b[1]-a[1]*b[0];V3 N={.2,-std::sqrt(.95),.1},e1={2,0,0},e2={0,0,2};auto rawT=mul(sub(mul(e1,b[1]),mul(e2,a[1])),1/det),rawB=mul(sub(mul(e2,a[0]),mul(e1,b[0])),1/det);auto T=norm(sub(rawT,mul(N,dot(rawT,N)))),B=norm(cross(N,T));if(dot(B,rawB)<0)B=mul(B,-1);return std::array<V3,3>{T,B,N};};
 auto originalBasis=tangentBasis(originalUV),mirrorBasis=tangentBasis(mirrorUV);int originalSize=smoothscene["parts"][0]["textures"]["resolution"],mirrorSize=mirrorscene["parts"][0]["textures"]["resolution"];
 for(int y=8;y<56;y+=7)for(int x=8;x<56;x+=7){
  V2 delta={(x+.5)/originalSize-originalUV[0][0],(y+.5)/originalSize-originalUV[0][1]},a={originalUV[1][0]-originalUV[0][0],originalUV[1][1]-originalUV[0][1]},b={originalUV[3][0]-originalUV[0][0],originalUV[3][1]-originalUV[0][1]};double det=a[0]*b[1]-a[1]*b[0],u=(delta[0]*b[1]-delta[1]*b[0])/det,v=(a[0]*delta[1]-a[1]*delta[0])/det;V2 mu={mirrorUV[0][0]+u*(mirrorUV[1][0]-mirrorUV[0][0])+v*(mirrorUV[3][0]-mirrorUV[0][0]),mirrorUV[0][1]+u*(mirrorUV[1][1]-mirrorUV[0][1])+v*(mirrorUV[3][1]-mirrorUV[0][1])};
  auto ca=read_tga(smoothscene["parts"][0]["textures"]["normal"],x,y,false),cb=read_tga(mirrorscene["parts"][0]["textures"]["normal"],int(std::floor(mu[0]*mirrorSize)),int(std::floor(mu[1]*mirrorSize)),false);V3 wa={},wb={};double length=0;for(int i=0;i<3;i++){wa=add(wa,mul(originalBasis[i],ca[i]*2-1));wb=add(wb,mul(mirrorBasis[i],cb[i]*2-1));length+=(ca[i]*2-1)*(ca[i]*2-1);}for(int i=0;i<3;i++)close(wa[i],wb[i],.0005,"oriented/mirrored chart preserves world normal");close(std::sqrt(length),1,.00004,"PNG16 normal unit length");
 }

 std::cout<<"material tests passed\n";return 0;
 }catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<"\n";return 1;}}
