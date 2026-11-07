#include "AnimationImport.hpp"
#include "GeometryArchetypes.hpp"
#include "AnimationRigFit.hpp"
#include "AnimationMouthFit.hpp"
#include "../External/AnimationImport/ufbx.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cmath>
#include <cstring>
#include <fstream>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <numeric>
#include <set>
#include <stdexcept>
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include <stb_image.h>

namespace Playground::AnimationImport {
namespace {
namespace Math = Animation::Math;
using Json = nlohmann::json;
constexpr Matrix identity{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
void require(bool value, const char* message) { if (!value) throw std::invalid_argument(message); }
Math::Vec3 vector(Vec3d p) { return {p.x,p.y,p.z}; }
Vec3d vector(Math::Vec3 p) { return {p.x,p.y,p.z}; }
Matrix multiply(const Matrix& a,const Matrix& b) {
    Matrix result{};
    for (int c=0;c<4;++c) for (int r=0;r<4;++r) for (int k=0;k<4;++k)
        result[c*4+r]+=a[k*4+r]*b[c*4+k];
    return result;
}
Math::Vec3 point(const Matrix& m,Math::Vec3 p) {
    const Math::Vec3 result{m[0]*p.x+m[4]*p.y+m[8]*p.z+m[12],
        m[1]*p.x+m[5]*p.y+m[9]*p.z+m[13],m[2]*p.x+m[6]*p.y+m[10]*p.z+m[14]};
    Math::Finite(result); return result;
}
Matrix matrix(const ufbx_matrix& m) {
    return {m.m00,m.m10,m.m20,0,m.m01,m.m11,m.m21,0,m.m02,m.m12,m.m22,0,m.m03,m.m13,m.m23,1};
}
Matrix matrix(Math::RigidTransform transform) {
    const auto x=Math::Rotate(transform.rotation,{1,0,0}),y=Math::Rotate(transform.rotation,{0,1,0}),z=Math::Rotate(transform.rotation,{0,0,1});
    return {x.x,x.y,x.z,0,y.x,y.y,y.z,0,z.x,z.y,z.z,0,transform.position.x,transform.position.y,transform.position.z,1};
}
// Static affine scale is retained in the mesh and source matrices. The editable
// skeleton uses world translations/orientations in the core's rigid contract.
Math::RigidTransform rigid(const Matrix& m) {
    for (const auto value:m) Math::Finite(value);
    require(m[3]==0&&m[7]==0&&m[11]==0&&m[15]==1,"Non-affine animation transform");
    const auto x=Math::Unit(Math::Vec3{m[0],m[1],m[2]});
    const auto rawY=Math::Vec3{m[4],m[5],m[6]},rawZ=Math::Vec3{m[8],m[9],m[10]};
    const auto y=Math::Unit(rawY-x*Math::Dot(rawY,x)),z=Math::Cross(x,y);
    require(Math::Dot(z,rawZ)>1e-12,"Reflected/singular skeleton transforms cannot enter the rigid core");
    // Gram-Schmidt preserves orientation for constant inherited affine scale.
    const ufbx_matrix rotation{x.x,x.y,x.z,y.x,y.y,y.z,z.x,z.y,z.z,0,0,0};
    const auto transform=ufbx_matrix_to_transform(&rotation);
    return {{m[12],m[13],m[14]},Math::Unit(Math::Quat{transform.rotation.x,transform.rotation.y,transform.rotation.z,transform.rotation.w})};
}
std::string text(ufbx_string s) { return std::string(s.data,s.length); }
std::string nodeId(const ufbx_node* node) { return "fbx:"+std::to_string(node->element_id); }
void geometryToSkin(Asset& asset) {
    asset.skin.positions.clear(); asset.skin.triangles.clear();
    for (const auto p:asset.renderMesh.mesh.mesh.positions) asset.skin.positions.push_back(vector(p));
    const auto& indices=asset.renderMesh.mesh.mesh.indices;
    require(indices.size()%3==0,"Animation mesh requires triangle indices");
    for (std::size_t i=0;i<indices.size();i+=3) asset.skin.triangles.push_back({indices[i],indices[i+1],indices[i+2]});
}
void initializeBinding(Asset& asset) {
    std::vector<std::string> ids;
    for (const auto& bone:asset.document.bones) ids.push_back(bone.id);
    asset.skin.binding=Animation::Examples::BuildSkinBinding(asset.document.bones,ids);
}
Math::Vec3 normal(const Matrix& transform,Math::Vec3 n) {
    const auto x=Math::Vec3{transform[0],transform[1],transform[2]},y=Math::Vec3{transform[4],transform[5],transform[6]},z=Math::Vec3{transform[8],transform[9],transform[10]};
    const auto nx=Math::Cross(y,z),ny=Math::Cross(z,x),nz=Math::Cross(x,y);
    const auto determinant=Math::Dot(x,nx);
    require(std::isfinite(determinant)&&std::abs(determinant)>1e-15,"Singular skin normal transform");
    return Math::Unit((nx*n.x+ny*n.y+nz*n.z)*(1/determinant));
}

struct Glb {
    std::vector<std::uint8_t> bytes;
    Json json;
    std::size_t binStart{},binSize{};
    template<class T> T read(std::size_t offset) const {
        require(offset<=bytes.size()&&sizeof(T)<=bytes.size()-offset,"Truncated animation GLB");
        T result; std::memcpy(&result,bytes.data()+offset,sizeof(T)); return result;
    }
    explicit Glb(const std::filesystem::path& path) {
        const auto length=std::filesystem::file_size(path);
        require(length>=20&&length<=512ull*1024*1024,"Animation GLB size exceeds budget");
        bytes.resize(static_cast<std::size_t>(length));
        std::ifstream input(path,std::ios::binary); input.read(reinterpret_cast<char*>(bytes.data()),std::streamsize(bytes.size()));
        require(bool(input),"Cannot read animation GLB");
        require(read<std::uint32_t>(0)==0x46546c67&&read<std::uint32_t>(4)==2&&read<std::uint32_t>(8)==length,"Expected GLB 2.0");
        for (std::size_t offset=12;offset<bytes.size();) {
            const auto size=read<std::uint32_t>(offset),type=read<std::uint32_t>(offset+4); offset+=8;
            require(size%4==0&&size<=bytes.size()-offset,"Invalid animation GLB chunk");
            if (type==0x4e4f534a) { require(json.is_null(),"Duplicate GLB JSON"); json=Json::parse(bytes.begin()+offset,bytes.begin()+offset+size); }
            if (type==0x004e4942) { require(!binStart,"Duplicate GLB BIN"); binStart=offset;binSize=size; }
            offset+=size;
        }
        require(!json.is_null()&&binStart,"GLB requires JSON and BIN");
        require(json.at("buffers").size()==1&&!json.at("buffers")[0].contains("uri"),"Expected one embedded GLB buffer");
        const auto declared=json.at("buffers")[0].at("byteLength").get<std::size_t>();
        require(declared<=binSize,"GLB buffer exceeds chunk"); binSize=declared;
    }
    struct Accessor { std::size_t offset{},count{},stride{},components{},bytes{}; int type{}; bool normalized{}; };
    Accessor accessor(std::size_t id,std::size_t components) const {
        const auto& a=json.at("accessors").at(id);
        require(!a.contains("sparse"),"Sparse animation GLB accessor unsupported");
        const std::string expected=components==1?"SCALAR":components==2?"VEC2":components==3?"VEC3":components==4?"VEC4":"MAT4";
        require(a.at("type")==expected,"Animation GLB accessor dimensions mismatch");
        Accessor result;result.count=a.at("count");result.components=components;result.type=a.at("componentType");result.normalized=a.value("normalized",false);
        result.bytes=result.type==5121?1:result.type==5123?2:result.type==5125||result.type==5126?4:0;
        require(result.bytes&&result.count>0&&result.count<=6000000,"Invalid animation GLB accessor type/count");
        const auto& view=json.at("bufferViews").at(a.at("bufferView").get<std::size_t>());
        const auto viewOffset=view.value("byteOffset",std::size_t{}),viewSize=view.at("byteLength").get<std::size_t>(),local=a.value("byteOffset",std::size_t{});
        result.stride=view.value("byteStride",result.bytes*components);
        require(view.at("buffer")==0&&viewOffset<=binSize&&viewSize<=binSize-viewOffset&&local<=viewSize&&result.stride>=result.bytes*components&&result.stride<=252&&
            (result.count-1)*result.stride+result.bytes*components<=viewSize-local,"Animation GLB accessor exceeds buffer");
        result.offset=binStart+viewOffset+local; return result;
    }
    double value(const Accessor& a,std::size_t row,std::size_t component) const {
        require(row<a.count&&component<a.components,"Animation accessor index out of bounds");
        const auto offset=a.offset+row*a.stride+component*a.bytes;
        double result=a.type==5126?read<float>(offset):a.type==5125?read<std::uint32_t>(offset):a.type==5123?read<std::uint16_t>(offset):read<std::uint8_t>(offset);
        if (a.normalized) { require(a.type==5121||a.type==5123,"Unsupported normalized animation accessor");result/=a.type==5121?255.0:65535.0; }
        Math::Finite(result);return result;
    }
};
Matrix nodeMatrix(const Json& node) {
    if (node.contains("matrix")) return node.at("matrix").get<Matrix>();
    const auto p=node.value("translation",std::array<double,3>{}),s=node.value("scale",std::array<double,3>{1,1,1});
    const auto q=node.value("rotation",std::array<double,4>{0,0,0,1});
    auto result=matrix({{p[0],p[1],p[2]},Math::Unit(Math::Quat{q[0],q[1],q[2],q[3]})});
    for (int c=0;c<3;++c) for (int r=0;r<3;++r) result[c*4+r]*=s[c];
    return result;
}
// Reuse the established material/UV importer on an in-memory-derived static
// projection. The original file is never modified; the temporary is RAII-owned.
Pipes::ImportedMesh staticProjection(const Glb& glb) {
    auto json=glb.json;json.erase("skins");json.erase("animations");
    for (auto& node:json.at("nodes")) node.erase("skin");
    auto encoded=json.dump();while (encoded.size()%4) encoded+=' ';
    std::vector<std::uint8_t> bytes;
    auto append=[&](std::uint32_t value) { const auto start=bytes.size();bytes.resize(start+4);std::memcpy(bytes.data()+start,&value,4); };
    const auto binLength=(glb.binSize+3)&~std::size_t(3);
    append(0x46546c67);append(2);append(static_cast<std::uint32_t>(28+encoded.size()+binLength));append(static_cast<std::uint32_t>(encoded.size()));append(0x4e4f534a);
    bytes.insert(bytes.end(),encoded.begin(),encoded.end());append(static_cast<std::uint32_t>(binLength));append(0x004e4942);
    bytes.insert(bytes.end(),glb.bytes.begin()+glb.binStart,glb.bytes.begin()+glb.binStart+glb.binSize);bytes.resize(28+encoded.size()+binLength);
    static std::atomic<std::uint64_t> sequence{};
    const auto stamp=std::chrono::steady_clock::now().time_since_epoch().count();
    struct Temporary { std::filesystem::path path;~Temporary(){std::error_code error;std::filesystem::remove(path,error);} };
    Temporary temporary{std::filesystem::temp_directory_path()/("jerico-animation-"+std::to_string(stamp)+"-"+std::to_string(sequence++)+".glb")};
    { std::ofstream output(temporary.path,std::ios::binary);output.write(reinterpret_cast<const char*>(bytes.data()),std::streamsize(bytes.size()));require(bool(output),"Cannot write temporary GLB material projection"); }
    return Pipes::loadGlb(temporary.path);
}

Asset loadGlb(const std::filesystem::path& path) {
    Glb glb(path);Asset asset;asset.source=path;asset.label=path.filename().string();
    const auto& nodes=glb.json.at("nodes");
    const bool skinned=!glb.json.value("skins",Json::array()).empty();
    const bool animated=!glb.json.value("animations",Json::array()).empty();
    asset.renderMesh=(skinned||animated)?staticProjection(glb):Pipes::loadGlb(path);
    geometryToSkin(asset);
    asset.statistics.nodes=nodes.size();asset.statistics.meshes=glb.json.at("meshes").size();asset.statistics.skins=glb.json.value("skins",Json::array()).size();asset.statistics.animations=glb.json.value("animations",Json::array()).size();
    asset.statistics.vertices=asset.skin.positions.size();asset.statistics.triangles=asset.skin.triangles.size();
    std::set<std::size_t> authoredJoints;for(const auto& skin:glb.json.value("skins",Json::array()))for(const auto& joint:skin.at("joints"))authoredJoints.insert(joint.get<std::size_t>());asset.statistics.bones=authoredJoints.size();
    if (!skinned&&!animated) {asset.rigDescription="Static source mesh: no imported rig or animation";return asset;}
    std::vector<int> parent(nodes.size(),-1);std::vector<Matrix> world(nodes.size());std::vector<int> visited(nodes.size());
    for (std::size_t id=0;id<nodes.size();++id) for (const auto& child:nodes[id].value("children",Json::array())) {
        const auto c=child.get<std::size_t>();require(c<nodes.size()&&parent[c]==-1,"GLB node has invalid/multiple parents");parent[c]=static_cast<int>(id);
    }
    std::function<void(std::size_t)> resolve=[&](std::size_t id) {
        require(visited[id]!=1,"Cyclic animation GLB hierarchy");if (visited[id]==2)return;visited[id]=1;
        if(parent[id]>=0)resolve(static_cast<std::size_t>(parent[id]));
        world[id]=multiply(parent[id]<0?identity:world[parent[id]],nodeMatrix(nodes[id]));visited[id]=2;
    };
    std::map<std::string,Math::RigidTransform> rigidWorld;
    for (std::size_t id=0;id<nodes.size();++id) {resolve(id);rigidWorld["gltf:"+std::to_string(id)]=rigid(world[id]);}
    for (std::size_t id=0;id<nodes.size();++id) {
        const auto boneId="gltf:"+std::to_string(id),parentId=parent[id]<0?std::string{}:"gltf:"+std::to_string(parent[id]);
        const auto local=parentId.empty()?rigidWorld.at(boneId):Math::Compose(Math::Inverse(rigidWorld.at(parentId)),rigidWorld.at(boneId));
        asset.document.bones.push_back({boneId,parentId,local});
        asset.sourceJoints.push_back({boneId,nodes[id].value("name",boneId),parentId,world[id],identity});
    }
    initializeBinding(asset);asset.skin.influences.resize(asset.skin.positions.size());
    std::map<std::string,std::size_t> bindingIndex;
    for(std::size_t i=0;i<asset.skin.binding.joint_ids.size();++i)bindingIndex[asset.skin.binding.joint_ids[i]]=i;
    std::size_t vertexBase=0;
    std::function<void(std::size_t)> visitMesh=[&](std::size_t id) {
        const auto& node=nodes[id];
        if(node.contains("mesh")) for(const auto& primitive:glb.json.at("meshes").at(node.at("mesh").get<std::size_t>()).at("primitives")) {
            const auto& attributes=primitive.at("attributes");const auto positions=glb.accessor(attributes.at("POSITION"),3);
            require(vertexBase+positions.count<=asset.skin.positions.size(),"GLB skin/render vertex order mismatch");
            if(node.contains("skin")) {
                const auto& skin=glb.json.at("skins").at(node.at("skin").get<std::size_t>());
                const auto joints=skin.at("joints").get<std::vector<std::size_t>>();
                const auto jointValues=glb.accessor(attributes.at("JOINTS_0"),4),weights=glb.accessor(attributes.at("WEIGHTS_0"),4);
                require((jointValues.type==5121||jointValues.type==5123)&&!jointValues.normalized&&jointValues.count==positions.count&&weights.count==positions.count,"Invalid GLB skin attribute types/counts");
                require(!attributes.contains("JOINTS_1")&&!attributes.contains("WEIGHTS_1"),"GLB skins above four authored influences are unsupported");
                std::vector<Matrix> inverses(joints.size(),identity);
                if(skin.contains("inverseBindMatrices")) {
                    const auto inverse=glb.accessor(skin.at("inverseBindMatrices"),16);require(inverse.type==5126&&!inverse.normalized&&inverse.count==joints.size(),"Invalid GLB inverse bind matrices");
                    for(std::size_t j=0;j<joints.size();++j)for(std::size_t c=0;c<16;++c)inverses[j][c]=glb.value(inverse,j,c);
                }
                for(std::size_t j=0;j<joints.size();++j) {require(joints[j]<nodes.size(),"GLB skin joint index out of bounds");asset.sourceJoints[joints[j]].inverseBind=inverses[j];}
                for(std::size_t v=0;v<positions.count;++v) {
                    const Math::Vec3 original{glb.value(positions,v,0),glb.value(positions,v,1),glb.value(positions,v,2)};
                    std::vector<Math::Influence> influences;Math::Vec3 actual{};
                    for(std::size_t c=0;c<4;++c) {
                        const auto j=static_cast<std::size_t>(glb.value(jointValues,v,c));const auto weight=glb.value(weights,v,c);
                        require(j<joints.size()&&weight>=0,"Invalid GLB joint index/weight");if(weight==0)continue;
                        influences.push_back({bindingIndex.at("gltf:"+std::to_string(joints[j])),weight});
                        actual=actual+point(multiply(world[joints[j]],inverses[j]),original)*weight;
                    }
                    const auto normalized=Animation::Examples::NormalizeInfluences(influences,bindingIndex.size());
                    double sum=0;for(const auto influence:influences)sum+=influence.weight;actual=actual*(1/sum);
                    require(Math::Length(actual-asset.skin.positions[vertexBase+v])<=1e-5*(1+Math::Length(actual)),"GLB bind matrices do not reproduce the visual rest mesh");
                    asset.skin.influences[vertexBase+v]=normalized;
                }
            } else for(std::size_t v=0;v<positions.count;++v)asset.skin.influences[vertexBase+v]={{bindingIndex.at("gltf:"+std::to_string(id)),1}};
            vertexBase+=positions.count;
        }
        for(const auto& child:node.value("children",Json::array()))visitMesh(child.get<std::size_t>());
    };
    for(const auto& root:glb.json.at("scenes").at(glb.json.value("scene",0)).at("nodes"))visitMesh(root.get<std::size_t>());
    require(vertexBase==asset.skin.positions.size(),"GLB skin/render vertex coverage mismatch");
    for(const auto& animation:glb.json.value("animations",Json::array())) {
        Clip clip;clip.name=animation.value("name","Clip "+std::to_string(asset.clips.size()+1));clip.document=asset.document;
        struct Channel { std::size_t node;std::string path;Glb::Accessor times,values; };
        std::vector<Channel> channels;std::set<double> times;
        for(const auto& channel:animation.at("channels")) {
            const auto& sampler=animation.at("samplers").at(channel.at("sampler").get<std::size_t>());
            const auto node=channel.at("target").at("node").get<std::size_t>();require(node<nodes.size(),"Animation targets unknown GLB node");
            const auto property=channel.at("target").at("path").get<std::string>();
            require(property=="translation"||property=="rotation"||property=="scale","GLB morph animation unsupported");
            require(sampler.value("interpolation",std::string("LINEAR"))=="LINEAR","Only LINEAR GLB animation channels enter the rigid preview");
            const auto input=glb.accessor(sampler.at("input"),1),output=glb.accessor(sampler.at("output"),property=="rotation"?4:3);
            require(input.type==5126&&output.type==5126&&input.count==output.count,"GLB animation sampler type/count mismatch");
            double previous=-std::numeric_limits<double>::infinity();
            for(std::size_t k=0;k<input.count;++k) {const auto time=glb.value(input,k,0);require(time>=0&&time>previous,"GLB key times must increase and be nonnegative");previous=time;times.insert(time);}
            channels.push_back({node,property,input,output});
        }
        require(!times.empty(),"GLB clip has no keys");clip.start=*times.begin();clip.end=*times.rbegin();
        for(const auto& bone:asset.document.bones)clip.document.tracks.push_back({bone.id,{}});
        for(const auto time:times) {
            auto locals=nodes;
            for(const auto& channel:channels) {
                require(!locals[channel.node].contains("matrix"),"GLB animation of matrix-authored nodes unsupported");
                std::size_t upper=0;while(upper+1<channel.times.count&&glb.value(channel.times,upper,0)<time)++upper;
                const auto lower=upper>0?upper-1:upper;const auto a=glb.value(channel.times,lower,0),b=glb.value(channel.times,upper,0);
                const auto u=b>a?std::clamp((time-a)/(b-a),0.0,1.0):0.0;
                Json value=Json::array();
                if(channel.path=="rotation") {
                    const auto& v=channel.values;
                    const auto q=Math::Slerp({glb.value(v,lower,0),glb.value(v,lower,1),glb.value(v,lower,2),glb.value(v,lower,3)},
                        {glb.value(v,upper,0),glb.value(v,upper,1),glb.value(v,upper,2),glb.value(v,upper,3)},u);
                    value={q.x,q.y,q.z,q.w};
                } else for(std::size_t c=0;c<3;++c)value.push_back(glb.value(channel.values,lower,c)*(1-u)+glb.value(channel.values,upper,c)*u);
                if(channel.path=="scale") {
                    const auto rest=nodes[channel.node].value("scale",std::array<double,3>{1,1,1});
                    for(std::size_t c=0;c<3;++c)require(std::abs(value[c].get<double>()-rest[c])<1e-8,"Animated GLB scale is not representable by the rigid core");
                }
                locals[channel.node][channel.path]=value;
            }
            std::vector<Matrix> frameWorld(nodes.size());std::vector<bool> done(nodes.size());
            std::function<void(std::size_t)> frameResolve=[&](std::size_t id){if(done[id])return;if(parent[id]>=0)frameResolve(parent[id]);frameWorld[id]=multiply(parent[id]<0?identity:frameWorld[parent[id]],nodeMatrix(locals[id]));done[id]=true;};
            for(std::size_t id=0;id<nodes.size();++id)frameResolve(id);
            for(std::size_t id=0;id<nodes.size();++id) {
                const auto w=rigid(frameWorld[id]),local=parent[id]<0?w:Math::Compose(Math::Inverse(rigid(frameWorld[parent[id]])),w);
                clip.document.tracks[id].keys.push_back({time,local,{}});
            }
        }
        // The original rigid evaluator uses Hermite translation. Source values
        // and times remain exact; between-key translation is a preview fit.
        for(auto& track:clip.document.tracks)for(std::size_t k=0;k<track.keys.size();++k) {
            const auto a=k? k-1:k,b=k+1<track.keys.size()?k+1:k;
            if(a!=b)track.keys[k].position_velocity=(track.keys[b].local.position-track.keys[a].local.position)*(1/(track.keys[b].time-track.keys[a].time));
        }
        Animation::Examples::Validate(clip.document);asset.clips.push_back(std::move(clip));
    }
    if(!asset.clips.empty())asset.document=asset.clips.front().document;
    asset.importedRig=true;asset.rigDescription=skinned?"Imported GLB joints and skin; constant affine scale baked into the rigid rest mesh":"Imported GLB node hierarchy and animation; rigid mesh-part bindings, no source skin";
    if(animated)asset.warnings.push_back("GLB source key times/values retained; rigid Hermite translation between keys is a preview fit");
    Animation::validateAsset(animationAsset(asset));return asset;
}

Asset loadFbx(const std::filesystem::path& path) {
    ufbx_load_opts options{};options.target_axes=ufbx_axes_right_handed_y_up;options.target_unit_meters=1;
    options.generate_missing_normals=true;options.node_depth_limit=128;options.temp_allocator.memory_limit=256ull*1024*1024;options.result_allocator.memory_limit=256ull*1024*1024;
    options.use_blender_pbr_material=true;
    ufbx_error error{};using Scene=std::unique_ptr<ufbx_scene,decltype(&ufbx_free_scene)>;
    Scene scene(ufbx_load_file(path.string().c_str(),&options,&error),&ufbx_free_scene);
    if(!scene)throw std::invalid_argument("Cannot load FBX: "+text(error.description));
    Asset asset;asset.source=path;asset.label=path.filename().string();
    asset.statistics.nodes=scene->nodes.count;asset.statistics.meshes=scene->meshes.count;
    asset.statistics.bones=scene->bones.count;asset.statistics.skins=scene->skin_deformers.count;asset.statistics.animations=scene->anim_stacks.count;
    std::set<const ufbx_node*> skeleton;
    for(const auto* node:scene->nodes)if(node->bone)for(auto* ancestor=node;ancestor&&!ancestor->is_root;ancestor=ancestor->parent)skeleton.insert(ancestor);
    for(const auto* deformer:scene->skin_deformers)for(const auto* cluster:deformer->clusters)for(auto* ancestor=cluster->bone_node;ancestor&&!ancestor->is_root;ancestor=ancestor->parent)skeleton.insert(ancestor);
    std::map<std::string,Math::RigidTransform> rest;
    for(const auto* node:skeleton) {
        try {rest[nodeId(node)]=rigid(matrix(node->node_to_world));}
        catch(const std::exception& cause) {
            const auto& m=node->node_to_world;
            throw std::invalid_argument("FBX rest skeleton "+nodeId(node)+" '"+text(node->name)+"': "+cause.what()+
                "; basis lengths="+std::to_string(Math::Length({m.cols[0].x,m.cols[0].y,m.cols[0].z}))+","+
                std::to_string(Math::Length({m.cols[1].x,m.cols[1].y,m.cols[1].z}))+","+std::to_string(Math::Length({m.cols[2].x,m.cols[2].y,m.cols[2].z})));
        }
    }
    for(const auto* node:scene->nodes)if(skeleton.contains(node)) {
        const auto id=nodeId(node),parent=node->parent&&skeleton.contains(node->parent)?nodeId(node->parent):std::string{};
        asset.document.bones.push_back({id,parent,parent.empty()?rest.at(id):Math::Compose(Math::Inverse(rest.at(parent)),rest.at(id))});
        asset.sourceJoints.push_back({id,text(node->name),parent,matrix(node->node_to_world),identity});
    }
    if(!asset.document.bones.empty())initializeBinding(asset);
    std::map<std::string,std::size_t> jointIndex;
    for(std::size_t i=0;i<asset.skin.binding.joint_ids.size();++i)jointIndex[asset.skin.binding.joint_ids[i]]=i;
    std::map<const ufbx_material*,std::shared_ptr<const Pipes::PipeMaterial>> materials;
    std::map<const ufbx_texture*,std::shared_ptr<const Pipes::TextureImage>> textures;
    auto texture=[&](const ufbx_texture* source)->std::shared_ptr<const Pipes::TextureImage> {
        if(!source)return {};
        if(textures.contains(source))return textures.at(source);
        require(source->type==UFBX_TEXTURE_FILE,"FBX layered/procedural textures unsupported");
        auto image=std::make_shared<Pipes::TextureImage>();image->name=text(source->name);
        if(source->content.size) {
            require(source->content.size<=64ull*1024*1024,"FBX texture exceeds image budget");
            const auto* begin=static_cast<const std::uint8_t*>(source->content.data);image->encoded.assign(begin,begin+source->content.size);
        } else {
            auto relative=std::filesystem::path(text(source->relative_filename));
            // Asset-local files only: preserve missing-resource evidence rather
            // than following stale author-machine absolute references.
            if(relative.is_absolute()||relative.empty())relative=std::filesystem::path(text(source->filename)).filename();
            const auto imagePath=(path.parent_path()/relative).lexically_normal();
            const auto local=imagePath.lexically_relative(path.parent_path());
            if(local.empty()||*local.begin()==".."||!std::filesystem::is_regular_file(imagePath)) {
                asset.warnings.push_back("FBX texture unavailable beside asset: "+text(source->relative_filename));textures[source]={};return {};
            }
            const auto size=std::filesystem::file_size(imagePath);require(size>0&&size<=64ull*1024*1024,"FBX texture exceeds image budget");
            image->encoded.resize(static_cast<std::size_t>(size));std::ifstream input(imagePath,std::ios::binary);input.read(reinterpret_cast<char*>(image->encoded.data()),std::streamsize(size));require(bool(input),"Cannot read FBX texture");
        }
        int width=0,height=0,channels=0;
        require(stbi_info_from_memory(image->encoded.data(),static_cast<int>(image->encoded.size()),&width,&height,&channels)&&width>0&&height>0&&width<=8192&&height<=8192,"FBX texture requires valid PNG/JPEG within 8192 dimensions");
        image->width=static_cast<unsigned>(width);image->height=static_cast<unsigned>(height);image->bitsPerChannel=stbi_is_16_bit_from_memory(image->encoded.data(),static_cast<int>(image->encoded.size()))?16:8;
        textures[source]=image;return image;
    };
    auto material=[&](const ufbx_material* source)->std::shared_ptr<const Pipes::PipeMaterial> {
        if(materials.contains(source))return materials.at(source);
        auto value=std::make_shared<Pipes::PipeMaterial>();value->name=source?text(source->name):"FBX default";value->metallicFactor=0;value->roughnessFactor=.6f;
        if(source) {
            const auto& color=source->pbr.base_color.has_value?source->pbr.base_color:source->fbx.diffuse_color;
            if(color.has_value) {const auto c=color.value_vec3;value->baseColorFactor={float(c.x),float(c.y),float(c.z),1};}
            value->baseColor=texture(color.texture);
            value->normal=texture(source->pbr.normal_map.texture?source->pbr.normal_map.texture:source->fbx.normal_map.texture);
            for(auto& c:value->baseColorFactor) {require(std::isfinite(c),"Nonfinite FBX material");c=std::clamp(c,0.f,1.f);}
            if(source->pbr.roughness.has_value)value->roughnessFactor=float(std::clamp(source->pbr.roughness.value_real,0.0,1.0));
            if(source->pbr.metalness.has_value)value->metallicFactor=float(std::clamp(source->pbr.metalness.value_real,0.0,1.0));
        }
        materials[source]=value;return value;
    };
    for(const auto* node:scene->nodes)if(node->mesh) {
        const auto* mesh=node->mesh;
        require(mesh->num_vertices<=6000000&&mesh->num_triangles<=4000000&&mesh->skin_deformers.count<=1&&mesh->blend_deformers.count==0,"Unsupported/oversized FBX mesh deformation");
        const auto* skin=mesh->skin_deformers.count?mesh->skin_deformers[0]:nullptr;
        if(skin)require(skin->skinning_method==UFBX_SKINNING_METHOD_LINEAR,"FBX dual-quaternion skinning is not representable by rigid LBS");
        const auto base=static_cast<std::uint32_t>(asset.renderMesh.mesh.mesh.positions.size());
        require(base+mesh->num_vertices<=6000000,"FBX scene exceeds vertex budget");
        asset.statistics.vertices+=mesh->num_vertices;asset.statistics.triangles+=mesh->num_triangles;
        for(std::size_t v=0;v<mesh->num_vertices;++v) {
            const auto transform=skin?ufbx_get_skin_vertex_matrix(skin,v,&node->geometry_to_world):node->geometry_to_world;
            const auto p=ufbx_transform_position(&transform,mesh->vertices[v]);Math::Finite(Math::Vec3{p.x,p.y,p.z});
            asset.renderMesh.mesh.mesh.positions.push_back({p.x,p.y,p.z});
            std::vector<Math::Influence> weights;
            if(skin) {
                require(v<skin->vertices.count,"FBX skin vertex coverage mismatch");const auto info=skin->vertices[v];
                require(info.weight_begin<=skin->weights.count&&info.num_weights<=skin->weights.count-info.weight_begin,"FBX skin weights out of range");
                for(std::size_t w=0;w<info.num_weights;++w) {
                    const auto weight=skin->weights[info.weight_begin+w];require(weight.cluster_index<skin->clusters.count,"FBX skin cluster index out of range");
                    const auto* cluster=skin->clusters[weight.cluster_index];require(cluster->bone_node,"FBX cluster requires a bone");
                    weights.push_back({jointIndex.at(nodeId(cluster->bone_node)),weight.weight});
                    auto source=std::find_if(asset.sourceJoints.begin(),asset.sourceJoints.end(),[&](const auto& joint){return joint.id==nodeId(cluster->bone_node);});
                    source->inverseBind=matrix(cluster->geometry_to_bone);
                }
                weights=Animation::Examples::NormalizeInfluences(weights,jointIndex.size());
            } else if(!jointIndex.empty()) {
                auto* ancestor=node;while(ancestor&&!skeleton.contains(ancestor))ancestor=ancestor->parent;
                weights={{ancestor?jointIndex.at(nodeId(ancestor)):0,1}};
            }
            asset.skin.influences.push_back(std::move(weights));
        }
        std::vector<std::uint32_t> triangle(mesh->max_face_triangles*3);
        const auto partStart=static_cast<std::uint32_t>(asset.renderMesh.mesh.mesh.indices.size());
        for(std::size_t f=0;f<mesh->faces.count;++f) {
            const auto face=mesh->faces[f];if(face.num_indices<3)continue;
            const auto count=ufbx_triangulate_face(triangle.data(),triangle.size(),mesh,face);
            const auto first=static_cast<std::uint32_t>(asset.renderMesh.mesh.mesh.indices.size());
            const auto materialId=mesh->face_material.count?mesh->face_material[f]:0;
            const auto surface=material(materialId<node->materials.count?node->materials[materialId]:nullptr);
            for(std::size_t t=0;t<count;++t) {
                std::array<std::uint32_t,3> vertices{};
                for(std::size_t k=0;k<3;++k) {
                    const auto corner=triangle[t*3+k];require(corner<mesh->num_indices,"FBX corner index out of bounds");
                    vertices[k]=mesh->vertex_indices[corner];require(vertices[k]<mesh->num_vertices,"FBX vertex index out of bounds");
                }
                const auto a=vector(asset.renderMesh.mesh.mesh.positions[base+vertices[0]]),b=vector(asset.renderMesh.mesh.mesh.positions[base+vertices[1]]),d=vector(asset.renderMesh.mesh.mesh.positions[base+vertices[2]]);
                const auto cross=Math::Cross(b-a,d-a);const auto doubleArea=Math::Length(cross);Math::Finite(doubleArea);
                // A zero-area triangle has no representable oriented surface.
                // Keep source vertices/weights; omit only its render corners.
                if(doubleArea==0) {++asset.statistics.omittedDegenerateTriangles;continue;}
                const auto geometricNormal=cross*(1/doubleArea);Math::Finite(geometricNormal);
                for(std::size_t k=0;k<3;++k) {
                const auto c=t*3+k;
                const auto index=triangle[c];require(index<mesh->num_indices,"FBX corner index out of bounds");
                const auto vertex=mesh->vertex_indices[index];require(vertex<mesh->num_vertices,"FBX vertex index out of bounds");
                asset.renderMesh.mesh.mesh.indices.push_back(base+vertex);
                const auto n=ufbx_get_vertex_vec3(&mesh->vertex_normal,index);
                const auto transform=skin?ufbx_get_skin_vertex_matrix(skin,vertex,&node->geometry_to_world):node->geometry_to_world;
                try {
                    Math::Finite(Math::Vec3{n.x,n.y,n.z});
                    if(n.x==0&&n.y==0&&n.z==0) {asset.renderMesh.mesh.cornerNormals.push_back(vector(geometricNormal));++asset.statistics.reconstructedNormals;}
                    else asset.renderMesh.mesh.cornerNormals.push_back(vector(normal(matrix(transform),{n.x,n.y,n.z})));
                }
                catch(const std::exception& cause) {
                    throw std::invalid_argument("FBX corner normal "+nodeId(node)+" node='"+text(node->name)+"' mesh='"+text(mesh->name)+
                        "' face="+std::to_string(f)+" corner="+std::to_string(index)+" vertex="+std::to_string(vertex)+": "+cause.what()+
                        "; source normal="+std::to_string(n.x)+","+std::to_string(n.y)+","+std::to_string(n.z)+
                        "; triangle double-area="+std::to_string(doubleArea));
                }
                const auto uv=mesh->vertex_uv.exists?ufbx_get_vertex_vec2(&mesh->vertex_uv,index):ufbx_vec2{};
                require(std::isfinite(uv.x)&&std::isfinite(uv.y),"Nonfinite FBX UV");asset.renderMesh.cornerUV.push_back({float(uv.x),float(uv.y)});
                }
            }
            const auto indices=static_cast<std::uint32_t>(asset.renderMesh.mesh.mesh.indices.size()-first);if(indices==0)continue;
            if(!asset.renderMesh.materials.empty()&&asset.renderMesh.materials.back().material==surface&&asset.renderMesh.materials.back().firstIndex+asset.renderMesh.materials.back().indexCount==first)asset.renderMesh.materials.back().indexCount+=indices;
            else asset.renderMesh.materials.push_back({first,indices,surface});
        }
        asset.renderMesh.parts.push_back({text(mesh->name),nodeId(node),partStart,static_cast<std::uint32_t>(asset.renderMesh.mesh.mesh.indices.size()-partStart)});
    }
    require(!asset.renderMesh.mesh.mesh.indices.empty(),"FBX scene has no renderable triangles");
    if(asset.statistics.omittedDegenerateTriangles)asset.warnings.push_back("FBX omitted "+std::to_string(asset.statistics.omittedDegenerateTriangles)+" zero-area triangles; original vertices and weights retained");
    if(asset.statistics.reconstructedNormals)asset.warnings.push_back("FBX reconstructed "+std::to_string(asset.statistics.reconstructedNormals)+" zero corner normals from oriented nondegenerate triangles");
    asset.renderMesh.mesh.normals.resize(asset.renderMesh.mesh.mesh.positions.size(),{0,1,0});asset.renderMesh.mesh.field.resize(asset.renderMesh.mesh.mesh.positions.size());asset.renderMesh.mesh.closedSolid=false;
    geometryToSkin(asset);
    // Explicit 30 Hz preview sampling preserves the original stack ranges/names.
    // Reject animated scale: the portable authoring core is rigid TR only.
    for(const auto* stack:scene->anim_stacks)if(stack->time_end>stack->time_begin&&!skeleton.empty()) {
        require(std::isfinite(stack->time_begin)&&std::isfinite(stack->time_end)&&stack->time_end-stack->time_begin<=600,"FBX clip exceeds preview time budget");
        Clip clip{text(stack->name),asset.document,stack->time_begin,stack->time_end};
        for(const auto& bone:asset.document.bones)clip.document.tracks.push_back({bone.id,{}});
        const auto frames=static_cast<std::size_t>(std::ceil((clip.end-clip.start)*30));
        for(std::size_t f=0;f<=frames;++f) {
            const auto time=std::min(clip.end,clip.start+double(f)/30);ufbx_evaluate_opts evaluate{};
            Scene evaluated(ufbx_evaluate_scene(scene.get(),stack->anim,time,&evaluate,&error),&ufbx_free_scene);
            require(bool(evaluated),"Cannot evaluate FBX animation");
            std::map<std::string,Math::RigidTransform> frame;
            for(const auto* node:skeleton) {
                const auto* posed=evaluated->nodes[node->typed_id];
                const auto originalScale=node->local_transform.scale,scale=posed->local_transform.scale;
                require(std::abs(scale.x-originalScale.x)<1e-7&&std::abs(scale.y-originalScale.y)<1e-7&&std::abs(scale.z-originalScale.z)<1e-7,"Animated FBX scale is not representable by the rigid core");
                frame[nodeId(node)]=rigid(matrix(posed->node_to_world));
            }
            for(std::size_t b=0;b<asset.document.bones.size();++b) {
                const auto& bone=asset.document.bones[b];const auto world=frame.at(bone.id);
                const auto local=bone.parent.empty()?world:Math::Compose(Math::Inverse(frame.at(bone.parent)),world);
                clip.document.tracks[b].keys.push_back({time,local,{}});
            }
        }
        Animation::Examples::Validate(clip.document);asset.clips.push_back(std::move(clip));
    }
    if(!asset.clips.empty())asset.document=asset.clips.front().document;
    asset.importedRig=!skeleton.empty();asset.rigDescription=asset.importedRig?"Imported FBX joints/weights; static affine rest baked to metres, rigid TR editing; clips sampled at 30 Hz":"Static FBX source: no imported rig";
    for(const auto* node:scene->nodes)if(skeleton.contains(node)&&node->children.count==0) {
        std::vector<std::string> chain;
        for(auto* cursor=node;cursor&&skeleton.contains(cursor)&&chain.size()<3;cursor=cursor->parent)chain.push_back(nodeId(cursor));
        if(chain.size()>=2) {std::reverse(chain.begin(),chain.end());asset.chains.push_back({text(node->name),std::move(chain),{}});}
    }
    if(asset.importedRig)Animation::validateAsset(animationAsset(asset));return asset;
}
} // namespace

Asset loadAsset(const std::filesystem::path& path) {
    auto extension=path.extension().string();std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return char(std::tolower(c));});
    if(extension==".glb")return loadGlb(path);
    if(extension==".fbx")return loadFbx(path);
    throw std::invalid_argument("Animation import supports GLB 2.0 and FBX");
}
Animation::AnimationAsset animationAsset(const Asset& asset) { return {asset.document,asset.skin,asset.chains}; }

void bindRig(Asset& asset,const Animation::Document& document,const std::vector<Animation::RigChain>& chains) {
    Animation::Examples::Validate(document);require(!document.bones.empty(),"Binding requires an authored rig");
    Asset next=asset;next.document=document;next.chains=chains;next.clips.clear();geometryToSkin(next);initializeBinding(next);
    const auto world=Math::ForwardKinematics(document.bones);
    const auto& positions=next.skin.positions;require(!positions.empty(),"Cannot bind an empty mesh");
    Math::Vec3 minimum=positions.front(),maximum=minimum;
    for(const auto p:positions) {Math::Finite(p);minimum={std::min(minimum.x,p.x),std::min(minimum.y,p.y),std::min(minimum.z,p.z)};maximum={std::max(maximum.x,p.x),std::max(maximum.y,p.y),std::max(maximum.z,p.z)};}
    const auto radius=std::max(1e-8,Math::Length(maximum-minimum)*.015);
    std::vector<std::vector<Math::Vec3>> endpoints(document.bones.size());
    for(std::size_t j=0;j<document.bones.size();++j) {
        for(const auto& child:document.bones)if(child.parent==document.bones[j].id)
            endpoints[j].push_back(world.at(child.id).position);
        if(endpoints[j].empty())endpoints[j].push_back(world.at(document.bones[j].id).position);
    }
    next.skin.influences.clear();next.skin.influences.reserve(positions.size());
    for(const auto p:positions) {
        std::vector<std::pair<double,std::size_t>> distances;distances.reserve(document.bones.size());
        for(std::size_t j=0;j<document.bones.size();++j) {
            const auto a=world.at(document.bones[j].id).position;
            double nearest=std::numeric_limits<double>::infinity();
            // A joint owns its outgoing segments. Rotating an elbow must not move
            // the upper arm merely because that segment ends at the elbow.
            for(const auto b:endpoints[j]) {
                const auto edge=b-a;const auto length2=Math::Dot(edge,edge);
                const auto u=length2>1e-20?std::clamp(Math::Dot(p-a,edge)/length2,0.0,1.0):0.0;
                const auto delta=p-(a+edge*u);nearest=std::min(nearest,Math::Dot(delta,delta));
            }
            distances.push_back({nearest,j});
        }
        const auto count=std::min<std::size_t>(4,distances.size());std::partial_sort(distances.begin(),distances.begin()+count,distances.end());
        std::vector<Math::Influence> influences;influences.reserve(count);
        for(std::size_t j=0;j<count;++j) {const auto denominator=distances[j].first+radius*radius;influences.push_back({distances[j].second,1/(denominator*denominator)});}
        next.skin.influences.push_back(Animation::Examples::NormalizeInfluences(influences,document.bones.size()));
    }
    next.importedRig=false;next.automaticBinding=true;next.sourceJoints.clear();next.rigDescription="User-created rig with automatic distance-to-bone binding";
    Animation::validateAsset(animationAsset(next));asset=std::move(next);
}

void createArchetypeRig(Asset& asset,std::size_t jointsPerLimb) {
    require(jointsPerLimb>=1&&jointsPerLimb<=16,"Archetype rig requires 1..16 joints per limb");
    const auto& mesh=asset.renderMesh.mesh.mesh;
    require(!mesh.positions.empty(),"Archetype rig requires a mesh");
    std::vector<JE::GeometryArchetypes::Vec3> points;points.reserve(mesh.positions.size());
    for(const auto& p:mesh.positions)points.push_back({p.x,p.y,p.z});
    const auto report=JE::GeometryArchetypes::classify(points,mesh.indices);
    const auto skeleton=JE::GeometryArchetypes::skeleton(points,mesh.indices,report,jointsPerLimb);
    require(!skeleton.chains.empty(),"Archetype rig found no limb chains for this mesh");
    const auto archetype=std::string(JE::GeometryArchetypes::name(report.ranked.front().type));
    require(archetype=="Biped"||archetype=="Quadruped"||archetype=="Arthropod"||archetype=="Axial",
        ("Archetype rig only builds creature families, not "+archetype).c_str());

    Animation::Document document;
    std::vector<Animation::RigChain> chains;
    std::map<std::string,Math::Vec3> world;
    auto add=[&](const std::string& id,const std::string& parent,Math::Vec3 p) {
        world[id]=p;
        Math::RigidTransform local{ parent.empty()?p:p-world.at(parent),{0,0,0,1} };
        document.bones.push_back({id,parent,local});
    };
    add("body","",{skeleton.root.x,skeleton.root.y,skeleton.root.z});
    for(std::size_t index=0;index<skeleton.chains.size();++index) {
        Animation::RigChain chain{"limb_"+std::to_string(index+1),{"body"},{}};
        std::string parent="body";
        for(std::size_t bone=0;bone<skeleton.chains[index].bones.size();++bone) {
            const auto id=chain.id+"_"+std::to_string(bone+1);
            const auto tail=skeleton.bones[skeleton.chains[index].bones[bone]].tail;
            add(id,parent,{tail.x,tail.y,tail.z});
            chain.bones.push_back(id);
            parent=id;
        }
        chains.push_back(std::move(chain));
    }
    auto next=asset;bindRig(next,document,chains);
    next.importedRig=false;next.automaticBinding=true;next.sourceJoints.clear();
    next.rigDescription="Archetype "+archetype+" ("+std::to_string(report.evidence.groundedSupportCount)
        +" grounded contacts, boxiness "+std::to_string(report.evidence.boxiness)
        +"); "+std::to_string(chains.size())+" geodesic limb chains with "+std::to_string(jointsPerLimb)
        +" bones each; automatic distance-to-bone binding, editable approximation";
    asset=std::move(next);
}

void createPresetRig(Asset& asset,RigPreset preset,std::size_t jointsPerLeg) {
    if(preset!=RigPreset::Quadruped)require(jointsPerLeg>=3&&jointsPerLeg<=64,"Anatomical generation requires 3..64 joints per leg");
    const auto& positions=asset.renderMesh.mesh.mesh.positions;require(!positions.empty(),"Rig preset requires a mesh");
    auto low=vector(positions.front()),high=low;
    for(const auto point:positions) {const auto p=vector(point);Math::Finite(p);low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
    const auto extent=high-low,center=(low+high)*.5;require(Math::Length(extent)>1e-8,"Rig preset requires nonzero mesh bounds");
    Animation::Document document;std::vector<Animation::RigChain> chains;std::map<std::string,Math::Vec3> world;bool axialRig=false;std::optional<MouthFit> mouth;
    auto add=[&](std::string id,std::string parent,Math::Vec3 p){world[id]=p;document.bones.push_back({id,parent,{parent.empty()?p:p-world.at(parent),{}}});};
    if(preset!=RigPreset::Quadruped) {
        WalkingRigFit fitted;std::vector<Math::Vec3> axis;
        try {fitted=fitWalkingRig(asset.renderMesh.mesh.mesh,preset==RigPreset::Automatic?0:8,jointsPerLeg);}
        catch(const std::invalid_argument&) {
            if(preset!=RigPreset::Automatic)throw;
            axis=fitAxialRig(asset.renderMesh.mesh.mesh,std::max(std::size_t(17),jointsPerLeg));
        }
        if(!axis.empty()) {
            mouth=fitTerminalMouth(asset.renderMesh.mesh.mesh,axis);
            axialRig=true;const auto middle=int(axis.size()/2);add("body","",axis[std::size_t(middle)]);
            for(const auto direction:{-1,1}) {
                const std::string id=direction<0?"axial_a":"axial_b";Animation::RigChain chain{id,{"body"},{}};
                for(int i=middle+direction;i>=0&&i<int(axis.size());i+=direction) {
                    const auto name=id+"_"+std::to_string(std::abs(i-middle));add(name,chain.bones.back(),axis[std::size_t(i)]);chain.bones.push_back(name);
                }
                chains.push_back(std::move(chain));
            }
        } else {
        add("body","",fitted.body);add("head","body",fitted.head);
        for(std::size_t leg=0;leg<fitted.legs.size();++leg) {
            const auto id="leg_"+std::to_string(leg+1),hip=id+"_hip",knee=id+"_knee",foot=id+"_foot";
            add(hip,"body",fitted.legs[leg].front());Animation::RigChain chain{id,{hip},{}};
            const auto upperSegments=(jointsPerLeg-1)/2;
            for(std::size_t joint=1;joint<jointsPerLeg;++joint) {
                const auto name=joint==jointsPerLeg-1?foot:joint==upperSegments?knee:
                    joint<upperSegments?id+"_upper_"+std::to_string(joint):id+"_lower_"+std::to_string(joint-upperSegments);
                add(name,chain.bones.back(),fitted.legs[leg][joint]);chain.bones.push_back(name);
            }
            chains.push_back(std::move(chain));
        }
        }
    } else {
        const auto body=Math::Vec3{center.x,low.y+extent.y*.63,center.z};add("body","",body);
        const bool alongZ=extent.z>=extent.x;
        auto coordinate=[&](double across,double height,double forward){return Math::Vec3{center.x+(alongZ?across*extent.x:forward*extent.x),low.y+height*extent.y,center.z+(alongZ?forward*extent.z:across*extent.z)};};
        add("neck","body",coordinate(0,.76,-.27));add("head","neck",coordinate(0,.80,-.42));add("tail","body",coordinate(0,.66,.41));
        for(int leg=0;leg<4;++leg) {
            const auto across=leg%2?1.0:-1.0,forward=leg<2?-.27:.27;
            const auto id="leg_"+std::to_string(leg+1),hip=id+"_hip",knee=id+"_knee",foot=id+"_foot";
            add(hip,"body",coordinate(across*.19,.60,forward));add(knee,hip,coordinate(across*.28,.30,forward+.035));add(foot,knee,coordinate(across*.30,.055,forward-.025));chains.push_back({id,{hip,knee,foot},{}});
        }
    }
    auto next=asset;bindRig(next,document,chains);
    if(mouth)appendMouthRig(next,*mouth);
    next.rigDescription=(mouth?"Elongated body; inferred circular mouth, "+std::to_string(mouth->teeth.size())+" rigid tooth groups; generated editable open-close clip":
        axialRig?"Geometry suggests an elongated body; curved axial rig; endpoint identities and mouth detail unresolved":
        preset==RigPreset::Automatic?"Geometry suggests "+std::string(chains.size()==8?"an arachnid":"a quadruped")+"; "+std::to_string(chains.size())+" supports detected; face/body detail unresolved":
        preset==RigPreset::Spider?"Spider anatomy fitted to mesh; "+std::to_string(jointsPerLeg)+" joints per leg":"Quadruped preset created here")+
        std::string("; automatic distance-to-bone binding, editable approximation");
    asset=std::move(next);
}

Pipes::ImportedMesh deform(const Asset& asset,const Animation::Pose& pose) {
    require(!asset.document.bones.empty(),"Static mesh needs an explicitly created rig before deformation");
    const auto palette=Animation::prepareSkinPalette(asset.skin.binding,pose);
    auto result=asset.renderMesh;const auto positions=Animation::skinPositions(asset.skin,palette);
    require(positions.size()==result.mesh.mesh.positions.size(),"Skin/render cardinality mismatch");
    for(std::size_t v=0;v<positions.size();++v)result.mesh.mesh.positions[v]=vector(positions[v]);
    auto skinNormal=[&](Math::Vec3 n,std::size_t vertex) {
        Math::Vec3 value{};for(const auto influence:asset.skin.influences.at(vertex))value=value+Animation::applySkinVector(palette.at(influence.joint),n)*influence.weight;
        if(Math::Length(value)<1e-12)return n;return Math::Unit(value);
    };
    require(result.mesh.cornerNormals.empty()||result.mesh.cornerNormals.size()==result.mesh.mesh.indices.size(),"Corner normal coverage mismatch");
    for(std::size_t c=0;c<result.mesh.cornerNormals.size();++c)result.mesh.cornerNormals[c]=vector(skinNormal(vector(asset.renderMesh.mesh.cornerNormals[c]),result.mesh.mesh.indices[c]));
    for(std::size_t v=0;v<result.mesh.normals.size();++v)result.mesh.normals[v]=vector(skinNormal(vector(asset.renderMesh.mesh.normals[v]),v));
    for(std::size_t c=0;c<result.cornerTangents.size();++c) {
        const auto t=asset.renderMesh.cornerTangents[c];const auto value=skinNormal({t[0],t[1],t[2]},result.mesh.mesh.indices[c]);result.cornerTangents[c]={float(value.x),float(value.y),float(value.z),t[3]};
    }
    return result;
}
} // namespace Playground::AnimationImport
