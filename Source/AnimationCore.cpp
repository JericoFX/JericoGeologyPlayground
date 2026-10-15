#include "AnimationCore.hpp"
#include "../External/AnimationNative/examples/coupled_system/products.hpp"
#include <nlohmann/json.hpp>
#include <chrono>
#include <fstream>
#include <functional>
#include <limits>
#include <numbers>
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#endif

namespace JE::AnimationTools {
namespace {
using Json = nlohmann::json;
auto findBone(Pose& pose, const std::string& id) {
    const auto found=std::find_if(pose.begin(),pose.end(),[&](const auto& b){return b.id==id;});
    if(found==pose.end()) throw std::invalid_argument("Unknown bone ID: "+id);
    return found;
}
Json vectorJson(Math::Vec3 p) { return Json::array({p.x,p.y,p.z}); }
Json transformJson(Math::RigidTransform t) {
    return {{"position",vectorJson(t.position)},{"rotation",Json::array({t.rotation.x,t.rotation.y,t.rotation.z,t.rotation.w})}};
}
Math::Vec3 readVector(const Json& j) {
    if(!j.is_array()||j.size()!=3) throw std::invalid_argument("Expected three coordinates");
    Math::Vec3 v{j.at(0).get<double>(),j.at(1).get<double>(),j.at(2).get<double>()}; Math::Finite(v); return v;
}
Math::RigidTransform readTransform(const Json& j) {
    const auto& q=j.at("rotation");
    if(!q.is_array()||q.size()!=4) throw std::invalid_argument("Expected quaternion xyzw");
    Math::Quat r{q.at(0).get<double>(),q.at(1).get<double>(),q.at(2).get<double>(),q.at(3).get<double>()};
    (void)Math::Unit(r); return {readVector(j.at("position")),r};
}
Json bonesJson(const Pose& pose) {
    auto result=Json::array();
    for(const auto& b:pose) result.push_back({{"id",b.id},{"parent",b.parent},{"local",transformJson(b.local)}});
    return result;
}
Pose readBones(const Json& j) {
    if(!j.is_array()) throw std::invalid_argument("Expected bones array");
    Pose result;
    for(const auto& b:j) result.push_back({b.at("id").get<std::string>(),b.at("parent").get<std::string>(),readTransform(b.at("local"))});
    return result;
}
Json stateJson(const EditorState& state) {
    auto tracks=Json::array();
    for(const auto& track:state.asset.document.tracks) {
        auto keys=Json::array();
        for(const auto& key:track.keys) keys.push_back({{"time",key.time},{"local",transformJson(key.local)},{"velocity",vectorJson(key.position_velocity)}});
        tracks.push_back({{"bone",track.bone_id},{"keys",keys}});
    }
    auto positions=Json::array(),influences=Json::array(),inverseBind=Json::array(),chains=Json::array();
    for(const auto p:state.asset.mesh.positions) positions.push_back(vectorJson(p));
    for(const auto& row:state.asset.mesh.influences) {
        auto values=Json::array(); for(const auto& i:row) values.push_back({{"joint",i.joint},{"weight",i.weight}}); influences.push_back(values);
    }
    for(const auto t:state.asset.mesh.binding.inverse_bind) inverseBind.push_back(transformJson(t));
    for(const auto& c:state.asset.chains) chains.push_back({{"id",c.id},{"bones",c.bones},{"tip",vectorJson(c.tipOffset)}});
    return {{"schema","je-animation-document-v1"},{"units","metres-seconds"},{"basis","right-handed-y-up"},
        {"bones",bonesJson(state.asset.document.bones)},{"tracks",tracks},{"pose",bonesJson(state.pose)},
        {"locks",state.locks},{"time",state.time},{"chains",chains},
        {"mesh",{{"positions",positions},{"triangles",state.asset.mesh.triangles},{"influences",influences},
        {"joint_ids",state.asset.mesh.binding.joint_ids},{"inverse_bind",inverseBind}}}};
}
void validateState(const EditorState& state) {
    validateAsset(state.asset); Math::Finite(state.time);
    if(state.time<0) throw std::invalid_argument("Negative time");
    const auto world=Math::ForwardKinematics(state.pose);
    if(state.pose.size()!=state.asset.document.bones.size()) throw std::invalid_argument("Pose skeleton mismatch");
    std::map<std::string,std::string> parents;
    for(const auto& b:state.asset.document.bones) parents.emplace(b.id,b.parent);
    for(const auto& b:state.pose) if(!parents.contains(b.id)||parents.at(b.id)!=b.parent) throw std::invalid_argument("Pose IDs/parents mismatch");
    for(const auto& id:state.locks) if(!world.contains(id)) throw std::invalid_argument("Unknown lock ID");
}
EditorState readState(const Json& j) {
    if(j.at("schema")!="je-animation-document-v1"||j.at("units")!="metres-seconds"||j.at("basis")!="right-handed-y-up")
        throw std::invalid_argument("Unsupported animation schema, units or coordinates");
    EditorState s; s.asset.document.bones=readBones(j.at("bones")); s.pose=readBones(j.at("pose"));
    s.time=j.at("time").get<double>(); s.locks=j.at("locks").get<std::set<std::string>>();
    for(const auto& t:j.at("tracks")) {
        Examples::BoneTrack track{t.at("bone").get<std::string>(),{}};
        for(const auto& k:t.at("keys")) track.keys.push_back({k.at("time").get<double>(),readTransform(k.at("local")),readVector(k.at("velocity"))});
        s.asset.document.tracks.push_back(std::move(track));
    }
    for(const auto& c:j.at("chains")) s.asset.chains.push_back({c.at("id").get<std::string>(),c.at("bones").get<std::vector<std::string>>(),readVector(c.at("tip"))});
    const auto& m=j.at("mesh");
    for(const auto& p:m.at("positions")) s.asset.mesh.positions.push_back(readVector(p));
    s.asset.mesh.triangles=m.at("triangles").get<std::vector<std::array<std::uint32_t,3>>>();
    for(const auto& row:m.at("influences")) {
        std::vector<Math::Influence> values;
        for(const auto& i:row) values.push_back({i.at("joint").get<std::size_t>(),i.at("weight").get<double>()});
        s.asset.mesh.influences.push_back(std::move(values));
    }
    s.asset.mesh.binding.joint_ids=m.at("joint_ids").get<std::vector<std::string>>();
    for(const auto& t:m.at("inverse_bind")) s.asset.mesh.binding.inverse_bind.push_back(readTransform(t));
    validateState(s); return s;
}
bool same(const EditorState& a,const EditorState& b) { return stateJson(a)==stateJson(b); }

// Original CCD adaptation: unavailable rotational DOFs are skipped, including
// ancestors whose rotation would move a hard world-transform lock.
Examples::CcdResult lockedCcd(const Pose& pose,const Examples::CcdRequest& request,const std::set<std::string>& locks) {
    (void)Examples::SolveCcd(pose,{request.chain,request.tip_offset,request.target,1,request.tolerance}); // Validate request/topology.
    auto world=Math::ForwardKinematics(pose);
    std::map<std::string,std::string> parent;
    for(const auto& b:pose) parent.emplace(b.id,b.parent);
    auto forbidden=locks;
    for(const auto& id:locks) { auto cursor=parent.at(id); while(!cursor.empty()) { forbidden.insert(cursor); cursor=parent.at(cursor); } }
    auto result=Examples::CcdResult{pose};
    auto tip=[&] {const auto& end=world.at(request.chain.back()); return end.position+Math::Rotate(end.rotation,request.tip_offset);};
    result.residual=Examples::RiggingDetail::Residual(tip(),request.target);
    result.converged=result.residual<=request.tolerance;
    for(std::size_t sweep=0;!result.converged&&sweep<request.max_iterations;++sweep) {
        result.iterations=sweep+1;
        for(auto it=request.chain.rbegin();it!=request.chain.rend();++it) {
            if(forbidden.contains(*it)) continue;
            const auto from=tip()-world.at(*it).position,to=request.target-world.at(*it).position;
            if(Math::Length(from)<=1e-12||Math::Length(to)<=1e-12) continue;
            const auto delta=Examples::RiggingDetail::RotationBetween(from,to);
            auto bone=findBone(result.bones,*it);
            const auto q=bone->parent.empty()?Math::Quat{}:world.at(bone->parent).rotation;
            const auto localDelta=Math::Multiply(Math::Multiply(Examples::RiggingDetail::Conjugate(q),delta),q);
            bone->local.rotation=Math::Multiply(localDelta,bone->local.rotation);
            world=Math::ForwardKinematics(result.bones);
            result.residual=Examples::RiggingDetail::Residual(tip(),request.target);
            if(result.residual<=request.tolerance) {result.converged=true;break;}
        }
    }
    return result;
}
void appendBox(SkinMesh& mesh,Math::Vec3 a,Math::Vec3 b,double radius,std::size_t joint) {
    const auto axis=Math::Unit(b-a);
    const auto side=Math::Unit(Math::Cross(axis,std::abs(axis.y)<.9?Math::Vec3{0,1,0}:Math::Vec3{0,0,1}))*radius;
    const auto up=Math::Cross(axis,side);
    const auto base=static_cast<std::uint32_t>(mesh.positions.size());
    for(const auto p:{a,b}) for(const auto signs:std::array<std::array<int,2>,4>{{{{-1,-1}},{{1,-1}},{{1,1}},{{-1,1}}}}) {
        mesh.positions.push_back(p+side*signs[0]+up*signs[1]); mesh.influences.push_back({{joint,1}});
    }
    constexpr std::array<std::array<std::uint32_t,3>,12> faces{{{{0,2,1}},{{0,3,2}},{{4,5,6}},{{4,6,7}},{{0,1,5}},{{0,5,4}},{{1,2,6}},{{1,6,5}},{{2,3,7}},{{2,7,6}},{{3,0,4}},{{3,4,7}}}};
    for(auto face:faces) {for(auto& v:face) v+=base;mesh.triangles.push_back(face);}
}
}

void validateAsset(const AnimationAsset& asset) {
    Examples::Validate(asset.document);
    if(asset.document.bones.empty()) throw std::invalid_argument("Animation needs bones");
    for(const auto& track:asset.document.tracks) for(const auto& key:track.keys) if(key.time<0) throw std::invalid_argument("Negative key time");
    const auto world=Math::ForwardKinematics(asset.document.bones);
    std::set<std::string> ids;
    for(const auto& chain:asset.chains) {
        if(chain.id.empty()||!ids.insert(chain.id).second||chain.bones.empty()) throw std::invalid_argument("Invalid chain ID/empty chain");
        Math::Finite(chain.tipOffset);
        std::string previous; std::set<std::string> seen;
        for(const auto& id:chain.bones) {
            if(!world.contains(id)||!seen.insert(id).second) throw std::invalid_argument("Invalid chain bone ID");
            const auto b=std::find_if(asset.document.bones.begin(),asset.document.bones.end(),[&](const auto& value){return value.id==id;});
            if(!previous.empty()&&b->parent!=previous) throw std::invalid_argument("Chain must follow direct parents"); previous=id;
        }
    }
    for(const auto& t:asset.mesh.triangles) for(const auto v:t) if(v>=asset.mesh.positions.size()) throw std::invalid_argument("Invalid triangle vertex");
    (void)skinPositions(asset.mesh,asset.document.bones);
}
std::vector<Math::Vec3> skinPositions(const SkinMesh& mesh,const Pose& pose) {
    return Examples::SkinMeshPositions(mesh.positions,mesh.influences,mesh.binding,pose);
}
AnimationAsset ownSpiderFixture() {
    AnimationAsset a;
    a.document.bones.push_back({"spider/body","",{{0,1.3,0},{}}});
    for(int leg=0;leg<8;++leg) {
        const auto theta=2*std::numbers::pi*leg/8;
        const auto prefix="spider/leg/"+std::to_string(leg);
        const auto hip=Math::Multiply(Math::AxisAngle({0,1,0},-theta),Math::AxisAngle({0,0,1},-.15));
        a.document.bones.push_back({prefix+"/hip","spider/body",{{.55*std::cos(theta),0,.55*std::sin(theta)},hip}});
        a.document.bones.push_back({prefix+"/knee",prefix+"/hip",{{1,0,0},Math::AxisAngle({0,0,1},-1.25)}});
        a.document.bones.push_back({prefix+"/foot",prefix+"/knee",{{1,0,0},{}}});
        a.chains.push_back({prefix,{prefix+"/hip",prefix+"/knee",prefix+"/foot"},{}});
        const auto& h=a.document.bones[a.document.bones.size()-3]; const auto& k=a.document.bones[a.document.bones.size()-2];
        auto hipMiddle=h.local;hipMiddle.rotation=Math::Multiply(Math::AxisAngle({0,1,0},-theta),Math::AxisAngle({0,0,1},-.35));
        auto kneeMiddle=k.local;kneeMiddle.rotation=Math::AxisAngle({0,0,1},-.9);
        a.document.tracks.push_back({h.id,{{0,h.local,{}},{1,hipMiddle,{}},{2,h.local,{}}}});
        a.document.tracks.push_back({k.id,{{0,k.local,{}},{1,kneeMiddle,{}},{2,k.local,{}}}});
    }
    std::vector<std::string> jointIds; for(const auto& b:a.document.bones) jointIds.push_back(b.id);
    a.mesh.binding=Examples::BuildSkinBinding(a.document.bones,jointIds);
    const auto world=Math::ForwardKinematics(a.document.bones);
    appendBox(a.mesh,{-.55,1.3,0},{.55,1.3,0},.32,0);
    for(std::size_t leg=0;leg<8;++leg) {
        const auto& c=a.chains[leg];
        appendBox(a.mesh,world.at(c.bones[0]).position,world.at(c.bones[1]).position,.09,1+leg*3);
        appendBox(a.mesh,world.at(c.bones[1]).position,world.at(c.bones[2]).position,.065,2+leg*3);
        const auto foot=world.at(c.bones[2]).position;
        appendBox(a.mesh,foot-Math::Vec3{.07,0,0},foot+Math::Vec3{.07,0,0},.07,3+leg*3);
    }
    validateAsset(a);return a;
}
Editor::Editor(AnimationAsset asset) { state_.asset=std::move(asset);state_.pose=Examples::EvaluateLocals(state_.asset.document,0);validateState(state_); }
bool Editor::locked(const std::string& id) const { if(!worldPose().contains(id)) throw std::invalid_argument("Unknown bone ID");return state_.locks.contains(id); }
Math::WorldPose Editor::worldPose() const {return Math::ForwardKinematics(state_.pose);}
void Editor::beginEdit() {if(transaction_)throw std::logic_error("Edit already active");transaction_=state_;}
bool Editor::commitEdit() {
    if(!transaction_)throw std::logic_error("No edit active");
    if(same(*transaction_,state_)) {transaction_.reset();return false;}
    undo_.push_back(*transaction_);transaction_.reset();redo_.clear();++revision_;return true;
}
void Editor::cancelEdit() {if(!transaction_)throw std::logic_error("No edit active");std::swap(state_,*transaction_);transaction_.reset();}
void Editor::publish(EditorState next) {
    validateState(next);
    if(same(state_,next))return;
    if(!transaction_) {undo_.push_back(state_);redo_.clear();++revision_;}
    std::swap(state_,next);
}
void Editor::preserveLocks(Pose& next) const {
    const auto lockedWorld=worldPose();
    std::map<std::string,Math::Bone*> byId;for(auto& b:next)byId.emplace(b.id,&b);
    Math::WorldPose resolved;
    std::function<void(const std::string&)> visit=[&](const auto& id) {
        if(resolved.contains(id))return;
        auto& b=*byId.at(id);if(!b.parent.empty())visit(b.parent);
        if(state_.locks.contains(id)) {
            b.local=b.parent.empty()?lockedWorld.at(id):Math::Compose(Math::Inverse(resolved.at(b.parent)),lockedWorld.at(id));
            resolved.emplace(id,lockedWorld.at(id));
        } else resolved.emplace(id,b.parent.empty()?b.local:Math::Compose(resolved.at(b.parent),b.local));
    };
    for(const auto& b:next)visit(b.id);
}
void Editor::setLocal(const std::string& id,Math::RigidTransform local) {
    Math::Finite(local.position);local.rotation=Math::Unit(local.rotation);
    auto next=state_;auto bone=findBone(next.pose,id);if(state_.locks.contains(id))return;
    bone->local=local;preserveLocks(next.pose);publish(std::move(next));
}
void Editor::moveWorld(const std::string& id,Math::Vec3 position) {
    Math::Finite(position);const auto world=worldPose();auto target=world.at(id);target.position=position;
    auto pose=state_.pose;const auto bone=findBone(pose,id);
    setLocal(id,bone->parent.empty()?target:Math::Compose(Math::Inverse(world.at(bone->parent)),target));
}
void Editor::setLocked(const std::string& id,bool value) { (void)locked(id);auto next=state_;if(value)next.locks.insert(id);else next.locks.erase(id);publish(std::move(next)); }
Examples::CcdResult Editor::solveIk(const RigChain& chain,Math::Vec3 target,std::size_t maxIterations,double tolerance) {
    if(maxIterations==0)throw std::invalid_argument("Zero CCD iteration budget");
    auto result=lockedCcd(state_.pose,{chain.bones,chain.tipOffset,target,maxIterations,tolerance},state_.locks);
    auto next=state_;next.pose=result.bones;publish(std::move(next));return result;
}
void Editor::setKey(const std::string& id,Math::Vec3 velocity) {
    Math::Finite(velocity);auto next=state_;auto bone=findBone(next.pose,id);
    auto& tracks=next.asset.document.tracks;
    auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto& t){return t.bone_id==id;});
    const Keyframe key{state_.time,bone->local,velocity};
    if(track==tracks.end())tracks.push_back({id,{key}});
    else {auto at=std::lower_bound(track->keys.begin(),track->keys.end(),key.time,[](const auto& k,double t){return k.time<t;});if(at!=track->keys.end()&&at->time==key.time)*at=key;else track->keys.insert(at,key);}
    publish(std::move(next));
}
void Editor::removeKey(const std::string& id,double time) {
    Math::Finite(time);auto next=state_;(void)findBone(next.pose,id);
    auto& tracks=next.asset.document.tracks;auto track=std::find_if(tracks.begin(),tracks.end(),[&](const auto& t){return t.bone_id==id;});
    if(track==tracks.end())throw std::invalid_argument("Bone has no keys");
    const auto at=std::find_if(track->keys.begin(),track->keys.end(),[&](const auto& k){return k.time==time;});
    if(at==track->keys.end())throw std::invalid_argument("Key does not exist");
    track->keys.erase(at);if(track->keys.empty())tracks.erase(track);publish(std::move(next));
}
void Editor::seek(double seconds) {
    Math::Finite(seconds);if(seconds<0)throw std::invalid_argument("Negative time");
    auto pose=Examples::EvaluateLocals(document(),seconds);state_.pose.swap(pose);state_.time=seconds;
}
void Editor::advance(double delta,double start,double end,bool loop) {
    Math::Finite(delta);Math::Finite(start);Math::Finite(end);
    if(delta<0||start<0||end<=start)throw std::invalid_argument("Invalid playback interval");
    auto time=state_.time+delta;Math::Finite(time);
    if(loop&&time>end)time=start+std::fmod(time-start,end-start);
    else time=std::clamp(time,start,end);
    seek(time);
}
bool Editor::undo() {
    if(transaction_)throw std::logic_error("Undo during edit");if(undo_.empty())return false;
    redo_.push_back(state_);std::swap(state_,undo_.back());undo_.pop_back();++revision_;return true;
}
bool Editor::redo() {
    if(transaction_)throw std::logic_error("Redo during edit");if(redo_.empty())return false;
    undo_.push_back(state_);std::swap(state_,redo_.back());redo_.pop_back();++revision_;return true;
}
std::string Editor::serialize() const {return stateJson(state_).dump(2);}
void Editor::loadText(const std::string& text) {if(transaction_)throw std::logic_error("Load during edit");publish(readState(Json::parse(text)));}
void Editor::save(const std::filesystem::path& path) const {
    const auto text=serialize();auto temp=path;
    temp+=std::filesystem::path(".animation-tmp-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    try {
        {std::ofstream stream(temp,std::ios::binary|std::ios::trunc);if(!stream)throw std::runtime_error("Cannot open animation output");stream.write(text.data(),static_cast<std::streamsize>(text.size()));stream.flush();if(!stream)throw std::runtime_error("Cannot write animation output");}
#ifdef _WIN32
        if(!MoveFileExW(temp.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH))throw std::runtime_error("Cannot publish animation output");
#else
        std::filesystem::rename(temp,path);
#endif
    } catch(...) {std::error_code error;std::filesystem::remove(temp,error);throw;}
}
void Editor::load(const std::filesystem::path& path) {
    std::ifstream stream(path,std::ios::binary);if(!stream)throw std::runtime_error("Cannot open animation document");
    const std::string text{std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>()};
    if(stream.bad())throw std::runtime_error("Cannot read animation document");loadText(text);
}

CorrectionResult Editor::correctPositions(double groundHeight,std::size_t iterations,double tolerance) {
    Math::Finite(groundHeight);Math::Finite(tolerance);if(tolerance<0)throw std::invalid_argument("Negative correction tolerance");
    const auto original=worldPose();std::map<std::string,Math::Vec3> positions;
    for(const auto& [id,t]:original)positions.emplace(id,t.position);
    const auto rest=Math::ForwardKinematics(document().bones);
    auto residual=[&](const auto& p) {
        double maximum{};
        for(const auto& b:state_.pose) {
            maximum=std::max(maximum,std::max(0.0,groundHeight-p.at(b.id).y));
            if(!b.parent.empty())maximum=std::max(maximum,std::abs(Math::Length(p.at(b.id)-p.at(b.parent))-Math::Length(rest.at(b.id).position-rest.at(b.parent).position)));
        } return maximum;
    };
    CorrectionResult result;result.residualBefore=residual(positions);result.residualAfter=result.residualBefore;
    // Every row is an original coupled J W J^T solve (six reduced rows,
    // twelve coordinate columns); unused rows receive explicit regularization.
    auto solveRow=[&](Math::Vec3 gradientA,Math::Vec3 gradientB,double error,bool lockA,bool lockB) {
        original_coupled::JMatrix j{};original_coupled::WMatrix w{};original_spd::Vector r{};
        j[0][0]=gradientA.x;j[0][1]=gradientA.y;j[0][2]=gradientA.z;
        j[0][3]=gradientB.x;j[0][4]=gradientB.y;j[0][5]=gradientB.z;r[0]=error;
        for(std::size_t axis=0;axis<3;++axis) {w[axis][axis]=lockA?0:1;w[axis+3][axis+3]=lockB?0:1;}
        const auto delta=original_coupled::solve(j,w,r,1e-8);
        if(!delta)throw std::runtime_error("Original coupled SPD correction rejected inputs");
        return std::pair{Math::Vec3{(*delta)[0],(*delta)[1],(*delta)[2]},Math::Vec3{(*delta)[3],(*delta)[4],(*delta)[5]}};
    };
    for(std::size_t iteration=0;iteration<iterations&&result.residualAfter>tolerance;++iteration) {
        const auto before=positions;
        // Iterate in persistent-ID order so storage reordering cannot change the solve.
        std::map<std::string,const Math::Bone*> ordered;for(const auto& b:state_.pose)ordered.emplace(b.id,&b);
        for(const auto& [id,b]:ordered) {
            if(!b->parent.empty()) {
                const auto difference=positions.at(id)-positions.at(b->parent);const auto length=Math::Length(difference);
                const auto target=Math::Length(rest.at(id).position-rest.at(b->parent).position);
                if(length>1e-12&&std::abs(length-target)>tolerance) {
                    const auto normal=difference*(1/length);
                    const auto delta=solveRow(normal*-1,normal,length-target,state_.locks.contains(b->parent),state_.locks.contains(id));
                    if(!state_.locks.contains(b->parent))positions.at(b->parent)=positions.at(b->parent)+delta.first;
                    if(!state_.locks.contains(id))positions.at(id)=positions.at(id)+delta.second;
                }
            }
            if(positions.at(id).y<groundHeight&&!state_.locks.contains(id)) {
                const auto delta=solveRow({0,1,0},{},positions.at(id).y-groundHeight,false,true);
                positions.at(id)=positions.at(id)+delta.first;
            }
        }
        const auto after=residual(positions);Math::Finite(after);result.iterations=iteration+1;
        if(after>result.residualAfter) {positions=before;break;} // Never publish a worse maximum residual.
        if(after==result.residualAfter)break;
        result.residualAfter=after;
    }
    result.converged=result.residualAfter<=tolerance;
    if(result.residualAfter==result.residualBefore)return result;
    auto next=state_;
    for(auto& b:next.pose) {
        auto world=original.at(b.id);world.position=positions.at(b.id);
        if(b.parent.empty())b.local=world;
        else {auto parent=original.at(b.parent);parent.position=positions.at(b.parent);b.local=Math::Compose(Math::Inverse(parent),world);}
    }
    preserveLocks(next.pose);publish(std::move(next));return result;
}
} // namespace JE::AnimationTools
