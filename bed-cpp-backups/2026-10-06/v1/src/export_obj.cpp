#include "jerico/export.hpp"
#include "jerico/png.hpp"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <locale>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <zlib.h>

namespace jerico {
namespace {
using Bytes=std::vector<unsigned char>;
void need(bool value,const std::string&message){if(!value)throw std::runtime_error("OBJ export: "+message);}
void finite_json(const Json&j){if(j.is_number_float())need(std::isfinite(j.get<double>()),"nonfinite source value");if(j.is_structured())for(const auto&v:j)finite_json(v);}
uint32_t big32(const Bytes&b,size_t i){need(i+4<=b.size(),"truncated PNG");return uint32_t(b[i])<<24|uint32_t(b[i+1])<<16|uint32_t(b[i+2])<<8|b[i+3];}
Bytes read_bytes(const std::filesystem::path&p){std::ifstream f(p,std::ios::binary);need(bool(f),"cannot read native PNG "+p.string());return {std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};}
struct Png {int width=0,height=0,channels=0;Bytes rows;};
int paeth(int a,int b,int c){int p=a+b-c,aa=std::abs(p-a),bb=std::abs(p-b),cc=std::abs(p-c);return aa<=bb&&aa<=cc?a:bb<=cc?b:c;}
Png read_png16(const std::filesystem::path&path){
 need(path.extension()==".png","native PNG16 input required");const Bytes file=read_bytes(path);const unsigned char signature[]={137,80,78,71,13,10,26,10};
 need(file.size()>=33&&std::memcmp(file.data(),signature,8)==0,"invalid PNG signature");
 Png image;Bytes compressed;bool header=false,end=false;size_t at=8;
 while(at<file.size()){
  need(file.size()-at>=12,"truncated PNG chunk");const uint32_t n=big32(file,at);need(size_t(n)<=file.size()-at-12,"PNG chunk exceeds input");
  const auto*type=file.data()+at+4;const auto*data=file.data()+at+8;
  need(uint32_t(::crc32(0,type,static_cast<uInt>(size_t(n)+4)))==big32(file,at+8+n),"PNG CRC mismatch");
  if(std::memcmp(type,"IHDR",4)==0){
   need(!header&&at==8&&n==13,"invalid PNG IHDR");header=true;const auto w=big32(file,at+8),h=big32(file,at+12);
   need(w>0&&h>0&&w<=16384&&h<=16384,"invalid PNG dimensions");need(data[8]==16&&(data[9]==2||data[9]==6)&&data[10]==0&&data[11]==0&&data[12]==0,"native noninterlaced RGB/RGBA16 PNG required");
   image.width=int(w);image.height=int(h);image.channels=data[9]==2?3:4;
  }else if(std::memcmp(type,"IDAT",4)==0){need(header&&!end,"PNG IDAT before IHDR");compressed.insert(compressed.end(),data,data+n);}
  else if(std::memcmp(type,"IEND",4)==0){need(n==0,"invalid PNG IEND");end=true;at+=12;break;}
  at+=size_t(n)+12;
 }
 need(header&&end&&at==file.size()&&!compressed.empty(),"incomplete native PNG");
 const size_t pixel_bytes=size_t(image.channels)*2,stride=size_t(image.width)*pixel_bytes;
 Bytes raw((stride+1)*size_t(image.height));uLongf decoded=static_cast<uLongf>(raw.size());
 need(uncompress(raw.data(),&decoded,compressed.data(),static_cast<uLong>(compressed.size()))==Z_OK&&decoded==raw.size(),"PNG decompression length mismatch");
 image.rows.resize(stride*size_t(image.height));
 for(size_t y=0;y<size_t(image.height);++y){
  const unsigned filter=raw[y*(stride+1)];need(filter<=4,"unsupported PNG predictor");
  for(size_t x=0;x<stride;++x){
   const int a=x>=pixel_bytes?image.rows[y*stride+x-pixel_bytes]:0,b=y?image.rows[(y-1)*stride+x]:0,c=y&&x>=pixel_bytes?image.rows[(y-1)*stride+x-pixel_bytes]:0;
   const int predictor=filter==0?0:filter==1?a:filter==2?b:filter==3?(a+b)/2:paeth(a,b,c);
   image.rows[y*stride+x]=static_cast<unsigned char>(unsigned(raw[y*(stride+1)+1+x])+unsigned(predictor));
  }
 }
 return image;
}
void scalar_png(const Png&image,int channel,const std::filesystem::path&path){
 need(channel>=0&&channel<image.channels,"invalid packed texture channel");
 std::vector<V3> pixels(size_t(image.width)*image.height);
 for(int y=0;y<image.height;++y)for(int x=0;x<image.width;++x){const size_t at=(size_t(y)*image.width+x)*image.channels*2+channel*2;const double value=(unsigned(image.rows[at])*256+image.rows[at+1])/65535.;pixels[size_t(image.height-1-y)*image.width+x]={value,value,value};}
 write_png16(path.string(),image.width,image.height,pixels,false);
}
std::string safe(std::string name){for(char&c:name)if(!std::isalnum(static_cast<unsigned char>(c))&&c!='.'&&c!='_'&&c!='-')c='_';if(name.empty()||name=="."||name=="..")name="asset";return name;}
void copy_png(const std::filesystem::path&source,const std::filesystem::path&target,const Png&params){
 const Png image=read_png16(source);need(image.width==params.width&&image.height==params.height,"native atlas image dimensions differ");
 if(std::filesystem::weakly_canonical(source)!=std::filesystem::weakly_canonical(target))std::filesystem::copy_file(source,target,std::filesystem::copy_options::overwrite_existing);
}
V3 world_normal(const Json&scene,const std::string&node,V3 n){
 Json id=node;std::set<std::string>seen;
 while(!id.is_null()){
  const std::string key=id;need(seen.insert(key).second,"cyclic node hierarchy");const Json*found=nullptr;for(const auto&candidate:scene.at("nodes"))if(candidate.at("id")==id){found=&candidate;break;}
  need(found!=nullptr,"missing normal owner node "+key);const double angle=found->value("rotation_z",0.);const double c=std::cos(angle),s=std::sin(angle);n={c*n[0]-s*n[1],s*n[0]+c*n[1],n[2]};id=found->at("parent");
 }
 return norm(n);
}
std::vector<std::array<size_t,3>> polygon_triangles(const Json&metric){
 std::vector<V2> uv=metric.get<std::vector<V2>>();need(uv.size()>=3,"face needs three loops");V2 lo=uv[0],hi=uv[0];
 for(const auto&p:uv)for(int c=0;c<2;++c){lo[c]=std::min(lo[c],p[c]);hi[c]=std::max(hi[c],p[c]);}
 const double extent=std::max(hi[0]-lo[0],hi[1]-lo[1]);need(extent>0&&std::isfinite(extent),"degenerate metric UV polygon");for(auto&p:uv)for(int c=0;c<2;++c)p[c]=(p[c]-lo[c])/extent;
 double area=0;for(size_t i=0;i<uv.size();++i)area+=uv[i][0]*uv[(i+1)%uv.size()][1]-uv[(i+1)%uv.size()][0]*uv[i][1];need(std::abs(area)>1e-18,"zero-area metric UV polygon");const bool reversed=area<0;if(reversed)std::reverse(uv.begin(),uv.end());
 auto result=triangulate(uv);if(reversed)for(auto&t:result){for(auto&i:t)i=uv.size()-1-i;std::swap(t[1],t[2]);}return result;
}
void save(const std::filesystem::path&path,const std::string&text){std::ofstream f(path);f.imbue(std::locale::classic());need(bool(f),"cannot create "+path.string());f<<text;f.close();need(bool(f),"cannot finish "+path.string());}
}
void write_obj_mtl(const Json&scene,const std::string&obj_path){
 finite_json(scene);const auto metadata=scene.value("metadata",Json::object());
 need(metadata.contains("native_evaluation")&&metadata["native_evaluation"].value("generator","")=="C++17","native evaluated scene required");
 need(metadata.contains("native_material_bake")&&metadata["native_material_bake"].value("generator","")=="C++"&&metadata["native_material_bake"].value("native_images",false),"native C++ baked PNGs required");
 need(!scene.at("parts").empty(),"no parts to export");const std::filesystem::path target(obj_path);need(!target.filename().empty(),"OBJ filename required");
 const auto directory=target.parent_path().empty()?std::filesystem::path("."):target.parent_path();const std::string stem=safe(target.stem().string()),mtl_name=stem+".mtl",texture_dir=stem+"_textures";
 std::ostringstream obj,mtl;obj.imbue(std::locale::classic());mtl.imbue(std::locale::classic());obj<<std::setprecision(17);mtl<<std::setprecision(17);
 obj<<"# Native C++ evaluated OBJ; metres, right-handed Z-up, front -Y\n# Geometry is flattened at the recipe pose; explicit per-corner normals\nmtllib "<<mtl_name<<"\n";
 mtl<<"# Native C++ baked PBR material approximation for OBJ/MTL\n# LIMITATION: glass transmission/refraction is not preserved; d 1 is an opaque fallback.\n# map_Pr, map_Pm and norm are importer-dependent PBR extensions.\n# norm is an OpenGL +Y tangent-space normal map, never a height/bump map.\n# Ks is a legacy-Phong hint from source Specular IOR Level, not a physical PBR conversion.\n";
 size_t vertex_base=1,loop_base=1,index=0;std::set<std::string>ids,names;
 // Validate and serialize all geometry before creating any output files.
 for(const auto&part:scene.at("parts")){
  const std::string id=part.at("id"),node=part.at("node");need(ids.insert(id).second,"duplicate part ID "+id);need(!part.at("faces").empty(),"empty part "+id);
  std::string name=safe(id);if(!names.insert(name).second){name="part_"+std::to_string(index)+"_"+name;need(names.insert(name).second,"duplicate escaped object name");}
  const std::string material="m"+std::to_string(index)+"_"+safe(id);
  need(part.contains("normals")&&part.contains("texture_uvs")&&part.contains("textures"),"evaluated normals and atlas required for "+id);
  need(part["faces"].size()==part["normals"].size()&&part["faces"].size()==part["uvs"].size()&&part["faces"].size()==part["texture_uvs"].size(),"loop layer face count mismatch");
  std::vector<V3> world;for(const auto&v:part.at("vertices"))world.push_back(world_point(scene,node,v.get<V3>()));
  obj<<"\n# jerico_part_id "<<Json(id).dump()<<"\n# jerico_node_id "<<Json(node).dump()<<"\no "<<name<<"\ng "<<name<<"\nusemtl "<<material<<"\ns 1\n";
  for(const auto&v:world)obj<<"v "<<v[0]<<' '<<v[1]<<' '<<v[2]<<'\n';
  std::vector<size_t>face_offsets;std::vector<std::vector<V3>>normals;size_t count=0;
  for(size_t fi=0;fi<part["faces"].size();++fi){
   const auto&face=part["faces"][fi];const auto&uv=part["texture_uvs"][fi];const auto&ns=part["normals"][fi];need(face.size()>=3&&face.size()==uv.size()&&face.size()==ns.size()&&face.size()==part["uvs"][fi].size(),"loop cardinality mismatch");
   face_offsets.push_back(count);normals.emplace_back();
   for(size_t loop=0;loop<face.size();++loop){
    need(face[loop].is_number_integer()&&face[loop].get<int64_t>()>=0&&face[loop].get<size_t>()<world.size(),"source vertex index out of range");const V2 q=uv[loop].get<V2>();need(q[0]>=0&&q[0]<=1&&q[1]>=0&&q[1]<=1,"atlas UV outside [0,1]");const auto n=world_normal(scene,node,ns[loop].get<V3>());normals.back().push_back(n);
    obj<<"vt "<<q[0]<<' '<<q[1]<<"\nvn "<<n[0]<<' '<<n[1]<<' '<<n[2]<<'\n';++count;
   }
  }
  for(size_t fi=0;fi<part["faces"].size();++fi){const auto&face=part["faces"][fi];for(const auto&tri:polygon_triangles(part["uvs"][fi])){
   const V3 a=world[face[tri[0]].get<size_t>()],b=world[face[tri[1]].get<size_t>()],c=world[face[tri[2]].get<size_t>()];const V3 ng=norm(cross(sub(b,a),sub(c,a)));
   for(size_t loop:tri)need(dot(ng,normals[fi][loop])>0,"loop normal crosses geometric hemisphere: "+id+" face "+std::to_string(fi));
   obj<<"f";for(size_t loop:tri){const size_t li=loop_base+face_offsets[fi]+loop;obj<<' '<<vertex_base+face[loop].get<size_t>()<<'/'<<li<<'/'<<li;}obj<<'\n';
  }}
  vertex_base+=world.size();loop_base+=count;++index;
 }
 std::filesystem::create_directories(directory/texture_dir);index=0;
 std::map<std::string,Png> shared_params;std::set<std::string> emitted;
 for(const auto&part:scene.at("parts")){
  const auto&textures=part.at("textures");const auto constants=part.value("material_constants",Json::object());const std::string id=part.at("id"),prefix=part.contains("material_shared_id")?safe(part["material_shared_id"]):std::to_string(index)+"."+safe(id),material="m"+std::to_string(index)+"_"+safe(id);++index;
  need(constants.contains("ior")&&constants["ior"].is_number(),"exact uniform IOR missing: "+id);const double ior=constants["ior"];need(ior>=1&&std::isfinite(ior),"invalid IOR");
  const std::string param_path=textures.at("material_params");if(!shared_params.count(param_path))shared_params[param_path]=read_png16(param_path);const Png&params=shared_params.at(param_path);const bool fresh=emitted.insert(prefix).second;
  const auto relative=[&](const std::string&suffix){return (std::filesystem::path(texture_dir)/(prefix+suffix+".png")).generic_string();};
  const auto color=relative(".base_color"),normal=relative(".normal");if(fresh){copy_png(textures.at("base_color").get<std::string>(),directory/color,params);copy_png(textures.at("normal").get<std::string>(),directory/normal,params);}
  mtl<<"\n# jerico_part_id "<<Json(id).dump()<<"\nnewmtl "<<material<<"\nKd 1 1 1\nNi "<<ior<<"\nd 1\nillum 2\n";
  if(constants.contains("specular")){const double specular=constants["specular"];mtl<<"Ks "<<specular<<' '<<specular<<' '<<specular<<'\n';}
  mtl<<"map_Kd "<<color<<"\nnorm "<<normal<<'\n';
  for(const auto&property:std::array<std::pair<const char*,const char*>,2>{{{"roughness","Pr"},{"metallic","Pm"}}}){
   if(constants.contains(property.first)){const double value=constants[property.first];need(value>=0&&value<=1,"invalid material scalar");mtl<<property.second<<' '<<value<<'\n';}
   else{const int channel=std::string(property.first)=="roughness"?0:1;const auto file=relative(std::string(".")+property.first);if(fresh)scalar_png(params,channel,directory/file);mtl<<property.second<<" 1\nmap_"<<property.second<<' '<<file<<'\n';}
  }
  bool transmission=false;for(size_t at=4;at<params.rows.size();at+=size_t(params.channels)*2)if(params.rows[at]||params.rows[at+1]){transmission=true;break;}
  if(transmission)mtl<<"# LIMITATION: this native material has transmission; OBJ/MTL exports it opaque. Use GLB KHR_materials_transmission for glass.\n";
 }
 save(directory/mtl_name,mtl.str());save(target,obj.str());
 save(directory/(stem+".README.txt"),
  "Native C++ OBJ/MTL package\n\n"
  "Coordinates: metres, right-handed Z-up, front -Y, flattened into world space at the recipe pose.\n"
  "OBJ contains triangles, original packed atlas UVs, explicit rotated per-corner normals and stable part IDs/groups.\n"
  "Native PNG16 base-color and OpenGL +Y normal maps are copied unchanged. Variable roughness/metallic maps are\n"
  "losslessly derived in C++ from the native material_params R/G channels into linear RGB16 grayscale PNGs.\n"
  "Pr/Pm, map_Pr/map_Pm and norm are PBR extensions: support depends on the importing application.\n"
  "Ks, when present, copies source Specular IOR Level only as a legacy-Phong hint; it is not a physical PBR conversion.\n"
  "Read-only Blender 4.3.2 testing preserves geometry, custom vertex normals and roughness/metallic maps, but its OBJ importer ignores norm.\n"
  "The normal PNG is retained for consumers that support norm; no bump/height substitution or importer repair is performed.\n"
  "For literal coordinate identity in Blender 4.3.2 use OBJ importer forward_axis='Y', up_axis='Z'; the declared asset front is still -Y.\n"
  "Physical glass transmission/refraction is not preserved. d 1 is an explicitly opaque fallback, not a transmission approximation.\n"
  "Ni preserves uniform IOR, but does not by itself make an OBJ material physically transmissive. Use GLB for that.\n"
  "OBJ has no live node hierarchy, articulation pivots, sockets or grips; retain the original Scene JSON for those.\n"
  "Clearcoat, transmission textures and nonuniform specular-weight textures are not represented by this MTL.\n"
  "Finite atlas resolution and consumer material models prevent a universal pixel-identical rendering guarantee.\n"
  "No Python, Blender, external image converter or post-import geometry repair participates in this export.\n");
}
}
