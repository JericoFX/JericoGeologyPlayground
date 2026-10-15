#include "AnimationCore.hpp"
#include "AnimationInteraction.hpp"
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
Json documentJson(const Document& document) {
    auto tracks=Json::array();
    for(const auto& t:document.tracks) {
        auto keys=Json::array();for(const auto& k:t.keys)keys.push_back({{"time",k.time},{"local",transformJson(k.local)},{"velocity",vectorJson(k.position_velocity)}});
        tracks.push_back({{"bone",t.bone_id},{"keys",keys}});
    }
    return {{"bones",bonesJson(document.bones)},{"tracks",tracks}};
}
Document readDocument(const Json& j) {
    Document d;d.bones=readBones(j.at("bones"));
    for(const auto& t:j.at("tracks")){Examples::BoneTrack track{t.at("bone").get<std::string>(),{}};for(const auto& k:t.at("keys"))track.keys.push_back({k.at("time").get<double>(),readTransform(k.at("local")),readVector(k.at("velocity"))});d.tracks.push_back(std::move(track));}
    Examples::Validate(d);return d;
}
Json interactionJson(const InteractionState& state) {
    auto objects=Json::array(),points=Json::array(),limbs=Json::array(),limits=Json::array(),overrides=Json::array(),takes=Json::array();
    for(const auto& [id,o]:state.objects)objects.push_back({{"id",id},{"world",transformJson(o.world)},{"authority",static_cast<int>(o.authority)}});
    for(const auto& [id,p]:state.grabPoints)points.push_back({{"id",id},{"object_id",p.objectId},{"local",transformJson(p.local)},{"hands",static_cast<int>(p.permittedHands)},{"type",static_cast<int>(p.type)},{"status",static_cast<int>(p.status)},{"occupant",p.occupant},
        {"translation_min",vectorJson(p.limits.translationMin)},{"translation_max",vectorJson(p.limits.translationMax)},{"rotation_min",vectorJson(p.limits.rotationMin)},{"rotation_max",vectorJson(p.limits.rotationMax)}});
    for(const auto& [id,l]:state.limbs)limbs.push_back({{"id",id},{"bones",l.chain.bones},{"tip",vectorJson(l.chain.tipOffset)},{"shared",l.sharedJoints},{"hand",static_cast<int>(l.hand)},{"mode",static_cast<int>(l.mode)},{"enabled",l.enabled},
        {"weight",l.weight},{"weight_from",l.weightFrom},{"weight_start",l.weightStart},{"weight_duration",l.weightDuration},{"target",transformJson(l.target)},{"target_from",transformJson(l.targetFrom)},{"target_start",l.targetStart},{"target_duration",l.targetDuration},
        {"pole",l.pole?vectorJson(*l.pole):Json(nullptr)},{"grab_point",l.grabPointId},{"grab_offset",transformJson(l.grabOffset)},{"motor_target_enabled",l.motorTargetEnabled}});
    for(const auto& [id,l]:state.jointLimits)limits.push_back({{"id",id},{"axis",vectorJson(l.twistAxis)},{"swing",l.swingRadians},{"twist_min",l.twistMin},{"twist_max",l.twistMax}});
    for(const auto& [id,o]:state.overrides)overrides.push_back({{"id",id},{"delta",transformJson(o.delta)},{"start",o.startTime},{"duration",o.fadeDuration},{"fade",o.fadeToBase}});
    for(const auto& [id,t]:state.takes)takes.push_back({{"id",id},{"start",t.start},{"end",t.end},{"document",documentJson(t.document)}});
    const auto& s=state.settings;
    return {{"schema","je-animation-interaction-v1"},{"scene_id",state.sceneId},{"objects",objects},{"grab_points",points},{"limbs",limbs},{"limits",limits},{"overrides",overrides},{"takes",takes},
        {"settings",{{"iterations",s.maxIterations},{"position_tolerance",s.positionTolerance},{"angular_tolerance",s.angularTolerance},{"angular_scale",s.angularLengthScale},{"damping",s.damping},{"max_step",s.maxJointStep},{"pole_weight",s.poleWeight}}}};
}
InteractionState readInteraction(const Json& j) {
    if(j.at("schema")!="je-animation-interaction-v1")throw std::invalid_argument("Unsupported interaction schema");
    InteractionState state;state.sceneId=j.at("scene_id").get<std::string>();
    for(const auto& o:j.at("objects")){SceneObject object{o.at("id").get<std::string>(),readTransform(o.at("world")),static_cast<TransformAuthority>(o.at("authority").get<int>())};if(!state.objects.emplace(object.id,object).second)throw std::invalid_argument("Duplicate object ID");}
    for(const auto& p:j.at("grab_points")) {
        GrabPoint point;point.id=p.at("id").get<std::string>();point.objectId=p.at("object_id").get<std::string>();point.local=readTransform(p.at("local"));point.permittedHands=static_cast<Hand>(p.at("hands").get<int>());point.type=static_cast<GrabType>(p.at("type").get<int>());point.status=static_cast<GrabStatus>(p.at("status").get<int>());point.occupant=p.at("occupant").get<std::string>();
        point.limits={readVector(p.at("translation_min")),readVector(p.at("translation_max")),readVector(p.at("rotation_min")),readVector(p.at("rotation_max"))};
        if(!state.grabPoints.emplace(point.id,point).second)throw std::invalid_argument("Duplicate GrabPoint ID");
    }
    for(const auto& l:j.at("limbs")) {
        const auto id=l.at("id").get<std::string>();LimbControl limb;limb.chain={id,l.at("bones").get<std::vector<std::string>>(),readVector(l.at("tip"))};limb.sharedJoints=l.at("shared").get<std::vector<std::string>>();limb.hand=static_cast<Hand>(l.at("hand").get<int>());limb.mode=static_cast<LimbMode>(l.at("mode").get<int>());limb.enabled=l.at("enabled").get<bool>();
        limb.weight=l.at("weight").get<double>();limb.weightFrom=l.at("weight_from").get<double>();limb.weightStart=l.at("weight_start").get<double>();limb.weightDuration=l.at("weight_duration").get<double>();limb.target=readTransform(l.at("target"));limb.targetFrom=readTransform(l.at("target_from"));limb.targetStart=l.at("target_start").get<double>();limb.targetDuration=l.at("target_duration").get<double>();
        if(!l.at("pole").is_null())limb.pole=readVector(l.at("pole"));limb.grabPointId=l.at("grab_point").get<std::string>();limb.grabOffset=readTransform(l.at("grab_offset"));
        limb.motorTargetEnabled=l.value("motor_target_enabled",false);
        if(!state.limbs.emplace(id,limb).second)throw std::invalid_argument("Duplicate limb ID");
    }
    for(const auto& l:j.at("limits")){const auto id=l.at("id").get<std::string>();if(!state.jointLimits.emplace(id,JointLimit{id,readVector(l.at("axis")),l.at("swing").get<double>(),l.at("twist_min").get<double>(),l.at("twist_max").get<double>()}).second)throw std::invalid_argument("Duplicate joint limit ID");}
    for(const auto& o:j.at("overrides")){const auto id=o.at("id").get<std::string>();if(!state.overrides.emplace(id,PoseOverride{readTransform(o.at("delta")),o.at("start").get<double>(),o.at("duration").get<double>(),o.at("fade").get<bool>()}).second)throw std::invalid_argument("Duplicate override ID");}
    for(const auto& t:j.at("takes")){const auto id=t.at("id").get<std::string>();if(!state.takes.emplace(id,BakedTake{id,readDocument(t.at("document")),t.at("start").get<double>(),t.at("end").get<double>()}).second)throw std::invalid_argument("Duplicate take ID");}
    const auto& s=j.at("settings");state.settings={s.at("iterations").get<std::size_t>(),s.at("position_tolerance").get<double>(),s.at("angular_tolerance").get<double>(),s.at("angular_scale").get<double>(),s.at("damping").get<double>(),s.at("max_step").get<double>(),s.at("pole_weight").get<double>()};return state;
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
        {"locks",state.locks},{"time",state.time},{"interaction_time",state.interactionTime},{"interaction",state.interaction?interactionJson(*state.interaction):Json(nullptr)},{"chains",chains},
        {"mesh",{{"positions",positions},{"triangles",state.asset.mesh.triangles},{"influences",influences},
        {"joint_ids",state.asset.mesh.binding.joint_ids},{"inverse_bind",inverseBind}}}};
}
void validateState(const EditorState& state) {
    validateAsset(state.asset); Math::Finite(state.time);Math::Finite(state.interactionTime);
    if(state.time<0||state.interactionTime<0) throw std::invalid_argument("Negative time");
    if(state.interaction)validateInteraction(state.asset.document,*state.interaction);
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
    s.interactionTime=j.value("interaction_time",0.0);
    if(j.contains("interaction")&&!j.at("interaction").is_null())s.interaction=std::make_shared<const InteractionState>(readInteraction(j.at("interaction")));
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
std::shared_ptr<const InteractionState> runtimeFromAuthored(
    const std::shared_ptr<const InteractionState>& authored,
    const std::shared_ptr<const InteractionState>& previous) {
    if(!authored||!previous)return {};
    auto live=*authored;
    for(auto& [id,object]:live.objects) {
        const auto old=previous->objects.find(id);
        if(object.authority==TransformAuthority::PhysicsBody&&old!=previous->objects.end()&&old->second.authority==TransformAuthority::PhysicsBody)object.world=old->second.world;
    }
    return std::make_shared<const InteractionState>(std::move(live));
}

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
SkinPalette prepareSkinPalette(const SkinBinding& binding,const Pose& pose) {
    if(binding.joint_ids.size()!=binding.inverse_bind.size())throw std::invalid_argument("Skin binding cardinality mismatch");
    const auto world=Math::ForwardKinematics(pose);SkinPalette result;result.reserve(binding.joint_ids.size());std::set<std::string> ids;
    for(std::size_t i=0;i<binding.joint_ids.size();++i) {
        const auto& id=binding.joint_ids[i];
        if(id.empty()||!ids.insert(id).second||!world.contains(id))throw std::invalid_argument("Invalid posed binding joint ID");
        const auto transform=Math::Compose(world.at(id),binding.inverse_bind[i]);const auto q=Math::Unit(transform.rotation);
        Math::Finite(transform.position);
        const double xx=q.x*q.x,yy=q.y*q.y,zz=q.z*q.z,xy=q.x*q.y,xz=q.x*q.z,yz=q.y*q.z,wx=q.w*q.x,wy=q.w*q.y,wz=q.w*q.z;
        result.push_back({{1-2*(yy+zz),2*(xy-wz),2*(xz+wy),2*(xy+wz),1-2*(xx+zz),2*(yz-wx),2*(xz-wy),2*(yz+wx),1-2*(xx+yy)},transform.position});
    }return result;
}
std::vector<Math::Vec3> skinPositions(const SkinMesh& mesh,const SkinPalette& palette) {
    if(mesh.positions.size()!=mesh.influences.size()||palette.size()!=mesh.binding.joint_ids.size()||mesh.binding.joint_ids.size()!=mesh.binding.inverse_bind.size())throw std::invalid_argument("Skin mesh/palette cardinality mismatch");
    for(const auto& entry:palette) {
        Math::Finite(entry.translation);for(const auto coefficient:entry.rotation)Math::Finite(coefficient);
        const Math::Vec3 a{entry.rotation[0],entry.rotation[1],entry.rotation[2]},b{entry.rotation[3],entry.rotation[4],entry.rotation[5]},c{entry.rotation[6],entry.rotation[7],entry.rotation[8]};
        if(std::abs(Math::Dot(a,a)-1)>1e-8||std::abs(Math::Dot(b,b)-1)>1e-8||std::abs(Math::Dot(c,c)-1)>1e-8||std::abs(Math::Dot(a,b))>1e-8||std::abs(Math::Dot(a,c))>1e-8||std::abs(Math::Dot(b,c))>1e-8||std::abs(Math::Dot(a,Math::Cross(b,c))-1)>1e-8)throw std::invalid_argument("Skin palette must contain rigid rotations");
    }
    std::vector<Math::Vec3> result;result.reserve(mesh.positions.size());
    for(std::size_t vertex=0;vertex<mesh.positions.size();++vertex) {
        Math::Finite(mesh.positions[vertex]);const auto& row=mesh.influences[vertex];Math::Vec3 position{};double total{};
        for(std::size_t i=0;i<row.size();++i) {
            const auto& influence=row[i];Math::Finite(influence.weight);
            if(influence.weight<0||influence.joint>=palette.size())throw std::invalid_argument("Invalid skin influence");
            for(std::size_t previous=0;previous<i;++previous)if(row[previous].joint==influence.joint)throw std::invalid_argument("Duplicate skin influence");
            total+=influence.weight;position=position+applySkinPoint(palette[influence.joint],mesh.positions[vertex])*influence.weight;
        }
        if(!std::isfinite(total)||std::abs(total-1)>1e-8)throw std::invalid_argument("Skin weights must sum to one");
        Math::Finite(position);result.push_back(position);
    }return result;
}
std::vector<Math::Vec3> skinPositions(const SkinMesh& mesh,const Pose& pose) {return skinPositions(mesh,prepareSkinPalette(mesh.binding,pose));}
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
void Editor::beginEdit() {if(transaction_)throw std::logic_error("Edit already active");transaction_=state_;transactionAuthoredChanged_=false;}
bool Editor::commitEdit() {
    if(!transaction_)throw std::logic_error("No edit active");
    if(!transactionAuthoredChanged_||same(*transaction_,state_)) {transaction_.reset();transactionAuthoredChanged_=false;return false;}
    undo_.push_back(*transaction_);transaction_.reset();transactionAuthoredChanged_=false;redo_.clear();++revision_;return true;
}
void Editor::cancelEdit() {
    if(!transaction_)throw std::logic_error("No edit active");
    auto restored=*transaction_;
    auto restoredRuntime=runtimeFromAuthored(restored.interaction,runtimeInteraction_);
    if(restored.time!=state_.time||restored.interactionTime!=state_.interactionTime) {
        restored.time=state_.time;restored.interactionTime=state_.interactionTime;
        if(restored.interaction)restored.pose=sampleInteraction(restored.asset.document,restoredRuntime?*restoredRuntime:*restored.interaction,restored.time,restored.interactionTime,restored.locks).locals;
        else restored.pose=Examples::EvaluateLocals(restored.asset.document,restored.time);
    }
    std::swap(state_,restored);transaction_.reset();transactionAuthoredChanged_=false;runtimeInteraction_=std::move(restoredRuntime);
}
void Editor::publish(EditorState next) {
    validateState(next);
    if(same(state_,next))return;
    if(!transaction_) {undo_.push_back(state_);redo_.clear();++revision_;}
    else transactionAuthoredChanged_=true;
    std::swap(state_,next);runtimeInteraction_.reset();
}
const InteractionState& Editor::interaction() const {static const InteractionState empty;return state_.interaction?*state_.interaction:empty;}
const InteractionState& Editor::runtimeInteraction() const {return runtimeInteraction_?*runtimeInteraction_:interaction();}
void Editor::setInteraction(InteractionState candidate) {
    validateInteraction(document(),candidate);
    for(auto& [id,object]:candidate.objects) {
        const auto old=interaction().objects.find(id);
        if(object.authority==TransformAuthority::PhysicsBody&&old!=interaction().objects.end()&&old->second.authority==TransformAuthority::PhysicsBody)object.world=old->second.world;
    }
    if(interactionJson(candidate)==interactionJson(interaction()))return;
    auto next=state_;next.interaction=std::make_shared<const InteractionState>(std::move(candidate));
    auto live=runtimeFromAuthored(next.interaction,runtimeInteraction_);
    next.pose=sampleInteraction(next.asset.document,live?*live:*next.interaction,next.time,next.interactionTime,next.locks).locals;
    publish(std::move(next));runtimeInteraction_=std::move(live);
}
RuntimePose Editor::evaluateRuntime(double time,const InteractionState* transient) {
    Math::Finite(time);if(time<0)throw std::invalid_argument("Negative interaction time");
    auto candidate=transient?std::make_shared<const InteractionState>(*transient):runtimeInteraction_;
    const auto& evaluated=candidate?*candidate:interaction();
    auto result=sampleInteraction(document(),evaluated,state_.time,time,state_.locks);
    auto pose=result.locals;state_.pose.swap(pose);state_.interactionTime=time;runtimeInteraction_=std::move(candidate);return result;
}
void Editor::applyTake(const std::string& id) {
    if(transaction_)throw std::logic_error("Apply take during edit");const auto& take=interaction().takes.at(id);
    auto next=state_;next.asset.document=take.document;next.time=take.start;
    auto controls=interaction();controls.overrides.clear();
    for(auto& [_,limb]:controls.limbs){limb.mode=LimbMode::Fk;limb.grabPointId.clear();limb.grabOffset={};limb.motorTargetEnabled=false;}
    for(auto& [_,point]:controls.grabPoints){if(point.status==GrabStatus::Occupied)point.status=GrabStatus::Released;point.occupant.clear();}
    next.interaction=std::make_shared<const InteractionState>(std::move(controls));
    next.pose=Examples::EvaluateLocals(next.asset.document,next.time);publish(std::move(next));
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
    redo_.push_back(state_);std::swap(state_,undo_.back());undo_.pop_back();runtimeInteraction_.reset();++revision_;return true;
}
bool Editor::redo() {
    if(transaction_)throw std::logic_error("Redo during edit");if(redo_.empty())return false;
    undo_.push_back(state_);std::swap(state_,redo_.back());redo_.pop_back();runtimeInteraction_.reset();++revision_;return true;
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
