#include "jerico/export.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>

namespace jerico {
namespace {
using Bytes = std::vector<unsigned char>;
constexpr uint32_t array_buffer = 34962, element_buffer = 34963;
void check(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error("GLB export: " + message);
}
V3 basis(V3 value) { return {value[0], value[2], -value[1]}; }
void finite(const Json& value) {
    if (value.is_number_float()) check(std::isfinite(value.get<double>()), "nonfinite JSON value");
    if (value.is_structured()) for (const auto& item : value) finite(item);
}
void le32(Bytes& data, uint32_t value) {
    for (unsigned i = 0; i != 4; ++i) data.push_back(static_cast<unsigned char>(value >> (8*i)));
}
uint32_t be32(const Bytes& data, size_t offset) {
    check(offset+4 <= data.size(), "truncated PNG header");
    return uint32_t(data[offset])<<24 | uint32_t(data[offset+1])<<16 |
           uint32_t(data[offset+2])<<8 | data[offset+3];
}
uint32_t checked_size(size_t value) {
    check(value <= std::numeric_limits<uint32_t>::max(), "asset exceeds GLB 4 GiB limit");
    return static_cast<uint32_t>(value);
}
void align(Bytes& data) { while (data.size()%4) data.push_back(0); }
float f32(double value) {
    const float converted = static_cast<float>(value);
    check(std::isfinite(value) && std::isfinite(converted), "nonfinite or out-of-range geometry");
    return converted;
}
struct Output {
    Json doc = {{"asset", {{"version", "2.0"}, {"generator", "JericoProceduralCppLab native C++ GLB2"}}},
                {"scene",0}, {"scenes",Json::array({{{"nodes",Json::array()}}})},
                {"nodes",Json::array()}, {"meshes",Json::array()}, {"materials",Json::array()},
                {"accessors",Json::array()}, {"bufferViews",Json::array()},
                {"images",Json::array()}, {"textures",Json::array()},
                {"samplers",Json::array({{{"magFilter",9729},{"minFilter",9987},{"wrapS",33071},{"wrapT",33071}}})}};
    Bytes bin;
    std::set<std::string> extensions;
    std::map<std::string,size_t> shared_textures,shared_materials;
    size_t view(const Bytes& data, uint32_t target=0) {
        check(!data.empty(), "empty buffer view");
        align(bin);
        const size_t id=doc["bufferViews"].size();
        Json value={{"buffer",0},{"byteOffset",checked_size(bin.size())},{"byteLength",checked_size(data.size())}};
        if (target) value["target"]=target;
        doc["bufferViews"].push_back(value);
        bin.insert(bin.end(),data.begin(),data.end());
        return id;
    }
    size_t floats(const std::vector<float>& values, size_t components, const char* type, bool bounds=false) {
        check(!values.empty() && values.size()%components==0, "invalid float attribute");
        Bytes bytes;
        bytes.reserve(values.size()*4);
        for (float value:values) {
            check(std::isfinite(value), "nonfinite float attribute");
            uint32_t bits;
            static_assert(sizeof(float)==4,"GLB requires IEEE float32");
            std::memcpy(&bits,&value,4);
            le32(bytes,bits);
        }
        Json accessor={{"bufferView",view(bytes,array_buffer)},{"byteOffset",0},
                       {"componentType",5126},{"count",values.size()/components},{"type",type}};
        if (bounds) {
            std::vector<float> lo(components,std::numeric_limits<float>::max());
            std::vector<float> hi(components,std::numeric_limits<float>::lowest());
            for (size_t i=0;i<values.size();++i) {
                lo[i%components]=std::min(lo[i%components],values[i]);
                hi[i%components]=std::max(hi[i%components],values[i]);
            }
            accessor["min"]=lo; accessor["max"]=hi;
        }
        const size_t id=doc["accessors"].size(); doc["accessors"].push_back(accessor); return id;
    }
    size_t indices(size_t count) {
        check(count>0 && count%3==0,"invalid triangle index count");
        Bytes bytes; bytes.reserve(count*4);
        for (size_t i=0;i<count;++i) le32(bytes,checked_size(i));
        const size_t id=doc["accessors"].size();
        doc["accessors"].push_back({{"bufferView",view(bytes,element_buffer)},{"byteOffset",0},
                                   {"componentType",5125},{"count",count},{"type","SCALAR"}});
        return id;
    }
    size_t texture(const Json& textures, const std::string& key, const std::string& name) {
        check(textures.contains(key) && textures[key].is_string(), "missing native PNG: "+name+"."+key);
        const std::filesystem::path path=textures[key].get<std::string>();
        const std::string cache_key=std::filesystem::weakly_canonical(path).string();
        if(shared_textures.count(cache_key))return shared_textures.at(cache_key);
        check(path.extension()==".png", "PNG16 native bake required: "+path.string());
        std::ifstream file(path,std::ios::binary);
        check(bool(file),"cannot open native PNG "+path.string());
        Bytes bytes{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
        const unsigned char signature[]={137,80,78,71,13,10,26,10};
        check(bytes.size()>=33 && std::memcmp(bytes.data(),signature,8)==0 &&
              be32(bytes,8)==13 && std::memcmp(bytes.data()+12,"IHDR",4)==0,"invalid PNG "+path.string());
        check(be32(bytes,16)>0 && be32(bytes,20)>0 && bytes[24]==16 && (bytes[25]==2 || bytes[25]==6) &&
              bytes[26]==0 && bytes[27]==0 && bytes[28]==0,"native RGB/RGBA16 noninterlaced PNG required");
        const size_t image=doc["images"].size();
        doc["images"].push_back({{"name",name+"."+key},{"bufferView",view(bytes)},{"mimeType","image/png"}});
        const size_t id=doc["textures"].size();
        doc["textures"].push_back({{"name",name+"."+key},{"sampler",0},{"source",image}});
        shared_textures[cache_key]=id;
        return id;
    }
    size_t material(const Json& part) {
        const std::string id=part.at("id"); const auto& textures=part.at("textures");
        const Json constants=part.value("material_constants",Json::object());
        const std::string cache_key=part.value("material_shared_id",id)+textures.dump()+constants.dump();
        if(shared_materials.count(cache_key))return shared_materials.at(cache_key);
        check(constants.contains("ior") && constants["ior"].is_number(),"exact uniform material_constants.ior required: "+id);
        const double ior=constants["ior"];
        check(std::isfinite(ior) && ior>=1,"invalid IOR: "+id);
        const size_t color=texture(textures,"base_color",id), normal=texture(textures,"normal",id),
                     orm=texture(textures,"orm",id), transmission=texture(textures,"transmission",id),
                     surface=texture(textures,"surface",id), specular=texture(textures,"specular_weight",id);
        const auto info=[](size_t index){return Json{{"index",index},{"texCoord",0}};};
        Json mat={{"name",id+".material"},
                  {"pbrMetallicRoughness",{{"baseColorFactor",{1,1,1,1}},
                    {"baseColorTexture",info(color)},{"roughnessFactor",1},{"metallicFactor",1},
                    {"metallicRoughnessTexture",info(orm)}}},
                  {"normalTexture",{{"index",normal},{"texCoord",0},{"scale",1}}},
                  {"occlusionTexture",{{"index",orm},{"texCoord",0},{"strength",1}}},
                  {"alphaMode","OPAQUE"},{"doubleSided",part.value("double_sided",false)},
                  {"extensions",{{"KHR_materials_transmission",{{"transmissionFactor",1},{"transmissionTexture",info(transmission)}}},
                                 {"KHR_materials_ior",{{"ior",ior}}},
                                 {"KHR_materials_clearcoat",{{"clearcoatFactor",1},{"clearcoatTexture",info(surface)},
                                                             {"clearcoatRoughnessFactor",constants.value("coat_roughness",.03)}}}}},
                  {"extras",{{"source_part_id",id},{"source_material",part.value("material","")},
                             {"material_constants",constants},{"orm_channels",{"occlusion","roughness","metallic"}},
                             {"normal_convention","OpenGL +Y normal PNG; tangent W converted for top-left glTF UV"}}}};
        extensions.insert("KHR_materials_transmission"); extensions.insert("KHR_materials_ior"); extensions.insert("KHR_materials_clearcoat");
        // The native bake stores twice Principled's Specular IOR Level in A.
        // A texture preserves face-group variation within a shared part atlas.
        mat["extensions"]["KHR_materials_specular"]={{"specularFactor",1},{"specularTexture",info(specular)}};
        extensions.insert("KHR_materials_specular");
        const size_t index=doc["materials"].size(); doc["materials"].push_back(mat);shared_materials[cache_key]=index; return index;
    }
};
std::vector<std::array<size_t,3>> triangles(const Json& polygon) {
    check(polygon.size()>=3,"face has fewer than three UV loops");
    std::vector<V2> uv=polygon.get<std::vector<V2>>();
    V2 lo=uv[0],hi=uv[0];
    for (auto p:uv) for (int c=0;c<2;++c) {lo[c]=std::min(lo[c],p[c]);hi[c]=std::max(hi[c],p[c]);}
    const double extent=std::max(hi[0]-lo[0],hi[1]-lo[1]);
    check(std::isfinite(extent) && extent>0,"degenerate UV polygon");
    for(auto& p:uv) for(int c=0;c<2;++c) p[c]=(p[c]-lo[c])/extent;
    double area=0;
    for(size_t i=0;i<uv.size();++i) area+=uv[i][0]*uv[(i+1)%uv.size()][1]-uv[(i+1)%uv.size()][0]*uv[i][1];
    check(std::abs(area)>1e-18,"zero-area UV polygon");
    const bool reverse=area<0;
    if(reverse) std::reverse(uv.begin(),uv.end());
    auto result=triangulate(uv);
    if(reverse) for(auto& t:result) {for(auto& i:t) i=uv.size()-1-i;std::swap(t[1],t[2]);}
    return result;
}
Json part_extras(const Json& part) {
    Json extras=part;
    for(const char* key:{"vertices","faces","normals","uvs","texture_uvs","textures"}) extras.erase(key);
    extras["coordinate_space"]="source Z-up, +X right, -Y front; exported positions are (x,z,-y)";
    return extras;
}
size_t mesh(Output& out,const Json& part) {
    const std::string id=part.at("id");
    const auto& faces=part.at("faces");
    check(!faces.empty(),"empty mesh: "+id);
    check(part.contains("normals") && part.contains("texture_uvs") && part.contains("uvs"),"evaluated loop normals and baked UVs required: "+id);
    check(faces.size()==part["normals"].size() && faces.size()==part["texture_uvs"].size() && faces.size()==part["uvs"].size(),"loop layer face count: "+id);
    std::vector<float> positions,normals,tangents,atlas,metric;
    Json source_loops=Json::array();
    for(size_t fi=0;fi<faces.size();++fi) {
        const auto& face=faces[fi]; const auto& ns=part["normals"][fi];
        const auto& uv=part["texture_uvs"][fi]; const auto& metres=part["uvs"][fi];
        check(face.size()==ns.size() && face.size()==uv.size() && face.size()==metres.size(),"loop layer corner count: "+id);
        for(const auto& tri:triangles(metres)) {
            V3 p[3],n[3];V2 tex[3];
            for(int k=0;k<3;++k) {
                const size_t loop=tri[k];
                check(face[loop].is_number_integer() && face[loop].get<int64_t>()>=0 && face[loop].get<size_t>()<part["vertices"].size(),"vertex index: "+id);
                p[k]=basis(part["vertices"][face[loop].get<size_t>()].get<V3>());
                n[k]=norm(basis(ns[loop].get<V3>()));
                // Derive tangent frames from the values the GLB actually serializes.
                tex[k]={f32(uv[loop][0].get<double>()),f32(1-uv[loop][1].get<double>())};
                for(int c=0;c<3;++c) p[k][c]=f32(p[k][c]);
                check(tex[k][0]>=0 && tex[k][0]<=1 && tex[k][1]>=0 && tex[k][1]<=1,"atlas UV outside [0,1]: "+id);
            }
            const V3 e1=sub(p[1],p[0]),e2=sub(p[2],p[0]);
            const V3 geometric=cross(e1,e2);
            check(dot(geometric,geometric)>0,"float32-degenerate triangle: "+id);
            const V3 geometric_normal=norm(geometric);
            for(int k=0;k<3;++k) {
                // A shading normal crossing the geometric tangent plane makes a
                // continuous triangle tangent frame impossible. Correct it in
                // evaluation, then rebake; never flip normals or W on export.
                const V3 serialized_normal={f32(n[k][0]),f32(n[k][1]),f32(n[k][2])};
                const double alignment=dot(serialized_normal,geometric_normal);
                check(alignment>0,"loop normal crosses geometric hemisphere: "+id+
                      " face "+std::to_string(fi)+" loop "+std::to_string(tri[k])+
                      " dot(Ns,Ng)="+std::to_string(alignment));
            }
            const double du1=tex[1][0]-tex[0][0],dv1=tex[1][1]-tex[0][1],du2=tex[2][0]-tex[0][0],dv2=tex[2][1]-tex[0][1];
            const double determinant=du1*dv2-du2*dv1;
            check(std::isfinite(determinant) && std::abs(determinant)>1e-30,"degenerate texture triangle: "+id);
            const V3 t=mul(sub(mul(e1,dv2),mul(e2,dv1)),1/determinant);
            const V3 b=mul(sub(mul(e2,du1),mul(e1,du2)),1/determinant);
            for(int k=0;k<3;++k) {
                const V3 tangent=norm(sub(t,mul(n[k],dot(t,n[k]))));
                // glTF's top-left texture coordinates do not change the baked
                // OpenGL +Y normal-map frame. Convert raw UV-derivative W,
                // matching Blender and the MikkTSpace glTF convention.
                // See docs/glb-tangent-convention.md for the paired-frame proof.
                const double handedness=dot(cross(n[k],tangent),b)<0?1:-1;
                for(int c=0;c<3;++c) {positions.push_back(f32(p[k][c]));normals.push_back(f32(n[k][c]));tangents.push_back(f32(tangent[c]));}
                tangents.push_back(float(handedness));
                for(int c=0;c<2;++c) {atlas.push_back(f32(tex[k][c]));metric.push_back(f32(metres[tri[k]][c].get<double>()));}
                source_loops.push_back({fi,tri[k]});
            }
        }
    }
    Json attributes={{"POSITION",out.floats(positions,3,"VEC3",true)},
                     {"NORMAL",out.floats(normals,3,"VEC3")},
                     {"TANGENT",out.floats(tangents,4,"VEC4")},
                     {"TEXCOORD_0",out.floats(atlas,2,"VEC2")},
                     {"TEXCOORD_1",out.floats(metric,2,"VEC2")}};
    Json primitive={{"attributes",attributes},{"indices",out.indices(positions.size()/3)},
                    {"mode",4},{"material",out.material(part)},
                    {"extras",{{"source_part_id",id},{"source_loops",source_loops},
                               {"uv_layers",{{"TEXCOORD_0","NativeTextureUV (V flipped)"},{"TEXCOORD_1","metricUV"}}}}}};
    const size_t index=out.doc["meshes"].size();
    out.doc["meshes"].push_back({{"name",id},{"primitives",Json::array({primitive})},{"extras",part_extras(part)}});
    return index;
}
}
void write_glb(const Json& scene,const std::string& path) {
    finite(scene);
    const Json metadata=scene.value("metadata",Json::object());
    check(metadata.contains("native_evaluation") && metadata["native_evaluation"].value("generator","")=="C++17", "requires native evaluated geometry (--evaluate 1)");
    check(metadata.contains("native_material_bake") && metadata["native_material_bake"].value("generator","")=="C++" &&
          metadata["native_material_bake"].value("native_images",false),"requires current native material bake (--bake N)");
    check(!scene.at("parts").empty(),"scene contains no parts");
    std::set<std::string> part_ids;
    for (const auto& part : scene.at("parts")) {
        const std::string id=part.at("id");
        check(part_ids.insert(id).second,"duplicate part ID: "+id);
        check(!part.at("faces").empty(),"empty part cannot be silently discarded: "+id);
    }
    Output out;
    out.doc["extras"]={{"schema_version",scene.value("schema_version",1)},{"units",scene.value("units","m")},
                       {"source_basis",scene.value("basis",Json::object())},
                       {"source_to_gltf","(x, y, z) -> (x, z, -y)"},
                       {"metadata_coordinate_space","source Z-up; sockets, grips and source extras retain original coordinates"},
                       {"recipe",scene.value("recipe",Json::object())},{"metadata",metadata},
                       {"sockets",scene.value("sockets",Json::array())},{"grips",scene.value("grips",Json::array())}};
    std::map<std::string,size_t> nodes;
    for(const auto& node:scene.at("nodes")) {
        const std::string id=node.at("id");
        check(nodes.emplace(id,out.doc["nodes"].size()).second,"duplicate node ID: "+id);
        const double angle=node.value("rotation_z",0.0);
        out.doc["nodes"].push_back({{"name",id},{"translation",basis(node.at("translation").get<V3>())},
                                    {"rotation",{0,std::sin(angle/2),0,std::cos(angle/2)}},{"extras",node}});
    }
    for(const auto& node:scene.at("nodes")) {
        const std::string id=node.at("id");
        // Also validates the full ancestry and catches cycles before writing a file.
        world_point(scene,id,{0,0,0});
        if(node.at("parent").is_null()) out.doc["scenes"][0]["nodes"].push_back(nodes.at(id));
        else {
            const std::string parent=node["parent"];
            check(nodes.count(parent)>0,"missing parent: "+parent);
            auto& children=out.doc["nodes"][nodes.at(parent)]["children"];
            if(children.is_null()) children=Json::array();
            children.push_back(nodes.at(id));
        }
    }
    for(const auto& part:scene.at("parts")) {
        const std::string owner=part.at("node"),id=part.at("id");
        check(nodes.count(owner)>0,"part references missing node: "+owner);
        const size_t index=out.doc["nodes"].size();
        out.doc["nodes"].push_back({{"name",id},{"mesh",mesh(out,part)},{"extras",part_extras(part)}});
        auto& children=out.doc["nodes"][nodes.at(owner)]["children"];
        if(children.is_null()) children=Json::array();
        children.push_back(index);
    }
    check(!out.doc["meshes"].empty(),"no nonempty meshes");
    out.doc["extensionsUsed"]=out.extensions;
    out.doc["buffers"]=Json::array({{{"byteLength",checked_size(out.bin.size())}}});
    std::string json=out.doc.dump();
    while(json.size()%4) json.push_back(' ');
    align(out.bin);
    Bytes header;
    le32(header,0x46546c67);le32(header,2);le32(header,checked_size(28+json.size()+out.bin.size()));
    le32(header,checked_size(json.size()));le32(header,0x4e4f534a);
    const std::filesystem::path target(path);
    if(!target.parent_path().empty()) std::filesystem::create_directories(target.parent_path());
    std::ofstream file(target,std::ios::binary);
    check(bool(file),"cannot create "+path);
    file.write(reinterpret_cast<const char*>(header.data()),static_cast<std::streamsize>(header.size()));
    file.write(json.data(),static_cast<std::streamsize>(json.size()));
    Bytes chunk;le32(chunk,checked_size(out.bin.size()));le32(chunk,0x004e4942);
    file.write(reinterpret_cast<const char*>(chunk.data()),static_cast<std::streamsize>(chunk.size()));
    file.write(reinterpret_cast<const char*>(out.bin.data()),static_cast<std::streamsize>(out.bin.size()));
    file.close();check(bool(file),"cannot finish "+path);
}
}
