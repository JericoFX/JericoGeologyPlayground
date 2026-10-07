#include "jerico/export.hpp"
#include "jerico/evaluate.hpp"
#include "jerico/materials.hpp"
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <vector>
using namespace jerico;
namespace {
void require(bool value,const std::string& message){if(!value)throw std::runtime_error(message);}
uint32_t u32(const std::vector<unsigned char>& b,size_t i){require(i+4<=b.size(),"GLB u32 outside file");return uint32_t(b[i])|uint32_t(b[i+1])<<8|uint32_t(b[i+2])<<16|uint32_t(b[i+3])<<24;}
struct File{Json json;std::vector<unsigned char> bytes;size_t bin=0;explicit File(const std::string& path){std::ifstream f(path,std::ios::binary);bytes={std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};require(u32(bytes,0)==0x46546c67,"GLB magic");require(u32(bytes,4)==2,"GLB version");require(u32(bytes,8)==bytes.size(),"GLB length");require(u32(bytes,16)==0x4e4f534a,"GLB JSON chunk");size_t n=u32(bytes,12);require(n%4==0,"GLB JSON alignment");json=Json::parse(bytes.begin()+20,bytes.begin()+20+n);require(u32(bytes,24+n)==0x004e4942,"GLB BIN chunk");bin=28+n;require(bin+u32(bytes,20+n)==bytes.size(),"GLB BIN length");}
std::vector<double> values(size_t ai){const auto&a=json["accessors"][ai];const auto&v=json["bufferViews"][a["bufferView"].get<size_t>()];size_t n=a["count"],c=a["type"]=="VEC2"?2:a["type"]=="VEC3"?3:a["type"]=="VEC4"?4:1;size_t pos=bin+v.value("byteOffset",size_t(0))+a.value("byteOffset",size_t(0));std::vector<double> out;for(size_t i=0;i<n*c;i++){uint32_t bits=u32(bytes,pos+4*i);if(a["componentType"]==5126){float value;std::memcpy(&value,&bits,4);require(std::isfinite(value),"nonfinite accessor");out.push_back(value);}else{require(a["componentType"]==5125,"unexpected component type");out.push_back(bits);}}return out;}
};
// glTF uses top-left image coordinates but conventional normal maps retain an
// upward-V tangent frame. Blender/Mikk glTF exports therefore negate the raw
// derivative handedness, while keeping the original normal texture green.
void check_canonical_normal_frame(File&g){
 for(const auto&mesh:g.json["meshes"]){
  const auto&p=mesh["primitives"][0];const auto&a=p["attributes"];
  const auto pos=g.values(a["POSITION"]),ns=g.values(a["NORMAL"]),ts=g.values(a["TANGENT"]),uv=g.values(a["TEXCOORD_0"]),indices=g.values(p["indices"]);
  for(size_t k=0;k<indices.size();k+=3){
   const size_t ia=size_t(indices[k]),ib=size_t(indices[k+1]),ic=size_t(indices[k+2]);
   V3 pa={pos[3*ia],pos[3*ia+1],pos[3*ia+2]},pb={pos[3*ib],pos[3*ib+1],pos[3*ib+2]},pc={pos[3*ic],pos[3*ic+1],pos[3*ic+2]};
   const double du1=uv[2*ib]-uv[2*ia],dv1=uv[2*ib+1]-uv[2*ia+1],du2=uv[2*ic]-uv[2*ia],dv2=uv[2*ic+1]-uv[2*ia+1];
   const V3 raw_b=mul(sub(mul(sub(pc,pa),du1),mul(sub(pb,pa),du2)),1/(du1*dv2-du2*dv1));
   for(size_t v:{ia,ib,ic}){
    const V3 n={ns[3*v],ns[3*v+1],ns[3*v+2]},t={ts[4*v],ts[4*v+1],ts[4*v+2]};
    require(dot(cross(n,t),raw_b)*ts[4*v+3]<0,"glTF tangent W must retain the upward-V normal-map convention");
   }
  }
  const auto&m=g.json["materials"][p["material"].get<size_t>()];const auto&texture=g.json["textures"][m["normalTexture"]["index"].get<size_t>()];
  require(g.json["images"][texture["source"].get<size_t>()]["name"]==mesh["name"].get<std::string>()+".normal","GLB must embed the original normal PNG without green inversion");
 }
}
void throws(const Json&scene,const std::string&path,const std::string&label){bool failed=false;try{write_glb(scene,path);}catch(const std::exception&){failed=true;}require(failed,label);}
// Exact three-loop reductions of the source bevel failures found by the GLB gate.
// Changing the exporter to accept an incompatible shading hemisphere must fail.
void reject_inconsistent_bevel_normals(const Json& valid,const std::string& directory) {
    const auto cases=Json::parse(R"fixture([{"id":"door.plank.0.source_face_197","vertices":[[0.05183753150330956,0.02401212790655348,0.04697063169275854],[0.051829087009585176,0.02404526196344703,0.046808582119977724],[0.0518288246911264,0.024052986160841918,0.04682161552128927]],"faces":[[0,1,2]],"normals":[[[0.48348918824044,-0.8660254037844387,-0.12742921507488106],[0.8374278389426754,-0.5000000000000001,-0.22071387487831598],[0.0,-0.8909375310175959,-0.45412588103329926]]],"uvs":[[[0.019383171066474116,0.048683942259866046],[0.019416440750618227,0.048521700565607506],[0.019424196565380667,0.048534717820444356]]],"texture_uvs":[[[0.16141469750698995,0.7523653566229983],[0.1625064005637585,0.7523653566229983],[0.1624326137196445,0.7534570596797668]]],"source_case":"wood_reinforced","source_face":197},{"id":"post.1.-1.source_face_528","vertices":[[0.3567039361966128,-0.2584590748895283,0.7792750106223562],[0.35718387655384776,-0.25816833870614536,0.7210497293140216],[0.35778125422301543,-0.2583284618098475,0.6669250493750534]],"faces":[[0,1,2]],"normals":[[[0.26014109894369636,-0.9655641712080961,-0.0035270270184169536],[0.5023654831760381,-0.864655097234262,0.000695801778597516],[1.5077176139223295e-07,-0.9999999990775367,-4.29523434020936e-05]]],"uvs":[[[-0.36467235164383804,-0.7756701095456365],[-0.364364188954056,-0.7174429398611484],[-0.3645339097151905,-0.6633149926171698]]],"texture_uvs":[[[0.3561795169276823,0.9589776632302405],[0.31276983212401677,0.9602663230240549],[0.2724166303297442,0.9589776632302405]]],"source_case":"cabinet_open","source_face":528}])fixture");
    for (const auto& fixture:cases) {
        Json scene=valid,part=valid["parts"][0];
        for (const auto& item:fixture.items()) part[item.key()]=item.value();
        scene["parts"]=Json::array({part});
        bool rejected=false;
        try { write_glb(scene,directory+"/invalid-bevel.glb"); }
        catch (const std::exception& e) {
            rejected=std::string(e.what()).find("loop normal crosses geometric hemisphere")!=std::string::npos;
        }
        require(rejected,fixture["id"].get<std::string>()+": incompatible bevel normal was not rejected precisely");
    }
}

}
int main(int argc,char**argv){try{
 std::string directory=argc>1?argv[1]:"/tmp/jerico-export-test";std::filesystem::create_directories(directory);
 Builder b(normalize_recipe(Json::object()));b.node("frame",{1,2,3},"Frame",nullptr);b.node("hinge",{.2,.3,.4},"Hinge","frame",.3);b.box("glass.panel","hinge",-1,1,-.03,.03,0,2,"glass",0,"Glass");b.box("metal.panel","frame",-1,1,-.1,.1,0,.4,"iron",0,"Structure");b.socket("glass.socket","hinge","HandleFront","mount.test",{.2,-.03,1},{{"support_part_ids",{"glass.panel"}}});b.scene["grips"].push_back({{"id","glass.grip"},{"node","hinge"},{"socket_id","glass.socket"},{"position",{.1,-.03,1}},{"approach",{0,1,0}},{"enabled",true}});
 auto scene=evaluate_scene(b.scene);for(auto& face:scene["parts"][1]["uvs"])for(auto& uv:face)uv[0]=-uv[0].get<double>();
 bake_material_textures(scene,directory,32);std::ofstream(directory+"/scene.json")<<scene.dump(2);
 reject_inconsistent_bevel_normals(scene,directory);
 const std::string path=directory+"/native.glb";write_glb(scene,path);File g(path);check_canonical_normal_frame(g);
 require(g.json["asset"]["version"]=="2.0","glTF version");require(g.json["meshes"].size()==2,"one mesh per part");require(g.json["materials"].size()==2,"one material per part");require(g.json["nodes"][0]["translation"]==Json({1,3,-2}),"node coordinate conversion");require(g.json["nodes"][1]["extras"]["id"]=="hinge","pivot ID missing");require(g.json["extras"]["sockets"]==scene["sockets"],"socket metadata changed");require(g.json["extras"]["grips"]==scene["grips"],"grip metadata changed");
 size_t vertices=0,triangles=0;bool mirrored=false,unmirrored=false;
 for(const auto& mesh:g.json["meshes"]){const auto&p=mesh["primitives"][0];require(p["mode"]==4,"primitive must be triangles");const auto&a=p["attributes"];auto pos=g.values(a["POSITION"]),ns=g.values(a["NORMAL"]),ts=g.values(a["TANGENT"]),uv=g.values(a["TEXCOORD_0"]),metric=g.values(a["TEXCOORD_1"]),indices=g.values(p["indices"]);require(indices.size()%3==0,"triangle index count");require(ns.size()==pos.size()&&ts.size()/4==pos.size()/3&&uv.size()/2==pos.size()/3&&metric.size()==uv.size(),"attribute lengths");for(size_t i=0;i<pos.size()/3;i++){double n=0,t=0,d=0;for(size_t c=0;c<3;c++){n+=ns[i*3+c]*ns[i*3+c];t+=ts[i*4+c]*ts[i*4+c];d+=ns[i*3+c]*ts[i*4+c];}require(std::abs(n-1)<1e-5,"unit normal");require(std::abs(t-1)<1e-5,"unit tangent");require(std::abs(d)<1e-5,"orthogonal tangent");require(std::abs(ts[i*4+3])==1,"tangent handedness");mirrored|=ts[i*4+3]<0;unmirrored|=ts[i*4+3]>0;require(uv[i*2]>=0&&uv[i*2]<=1&&uv[i*2+1]>=0&&uv[i*2+1]<=1,"atlas UV bounds");}for(double i:indices)require(i<pos.size()/3,"index bounds");vertices+=pos.size()/3;triangles+=indices.size()/3;}
 require(vertices>=48&&triangles==24,"cube mesh counts");require(mirrored&&unmirrored,"mirrored UV handedness missing");
 auto&m=g.json["materials"][0];require(m["extensions"]["KHR_materials_specular"].contains("specularTexture"),"per-texel specular weight missing");require(m["pbrMetallicRoughness"]["metallicFactor"]==1,"metallic texture must not be multiplied by zero");require(m["extensions"]["KHR_materials_transmission"]["transmissionFactor"]==1,"transmission texture multiplier");require(std::abs(m["extensions"]["KHR_materials_ior"]["ior"].get<double>()-1.52)<1e-10,"exact glass IOR");
 for(const auto& image:g.json["images"]){require(image["mimeType"]=="image/png"&&!image.contains("uri"),"embedded PNG required");const auto&v=g.json["bufferViews"][image["bufferView"].get<size_t>()];size_t offset=g.bin+v["byteOffset"].get<size_t>();const unsigned char signature[]={137,80,78,71,13,10,26,10};require(std::memcmp(g.bytes.data()+offset,signature,8)==0,"invalid embedded PNG");}
 auto invalid=scene;invalid["parts"].push_back(invalid["parts"][0]);throws(invalid,directory+"/bad.glb","duplicate part ID accepted");invalid=scene;invalid["parts"][0]["faces"]=Json::array();throws(invalid,directory+"/bad.glb","empty part silently dropped");invalid=scene;for(auto&uv:invalid["parts"][0]["texture_uvs"][0])uv[0]=.5+uv[0].get<double>()*1e-10;throws(invalid,directory+"/bad.glb","UV island collapsed by float32 accepted");invalid=scene;invalid["metadata"].erase("native_evaluation");throws(invalid,directory+"/bad.glb","unevaluated scene accepted");invalid=scene;invalid["metadata"].erase("native_material_bake");throws(invalid,directory+"/bad.glb","non-native bake accepted");invalid=scene;invalid["parts"][0]["texture_uvs"][0][0][0]=std::numeric_limits<double>::quiet_NaN();throws(invalid,directory+"/bad.glb","nonfinite UV accepted");invalid=scene;invalid["nodes"][0]["parent"]="hinge";throws(invalid,directory+"/bad.glb","cyclic node hierarchy accepted");
 std::cout<<"native GLB export tests passed: "<<vertices<<" vertices, "<<triangles<<" triangles, "<<g.json["images"].size()<<" embedded PNG images\n";
}catch(const std::exception&e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;} }
