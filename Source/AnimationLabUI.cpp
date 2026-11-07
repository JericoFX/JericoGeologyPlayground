#include "AnimationLabUI.hpp"
#include "ViewportProjection.hpp"
#include <Windows.h>
#include <commdlg.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <functional>
#include <numbers>

namespace Playground {
namespace A = JE::AnimationTools;
namespace M = A::Math;
namespace {
Vec3d renderPoint(M::Vec3 p) { return {p.x,p.y,p.z}; }
M::Vec3 animationPoint(Vec3d p) { return {p.x,p.y,p.z}; }
void fieldLabel(const char* text) { ImGui::TextWrapped("%s",text);ImGui::SetNextItemWidth(-1); }
void panel(const char* title,float x,float y,float w,float h) {
    ImGui::SetNextWindowPos({x,y}); ImGui::SetNextWindowSize({w,h});
    ImGui::Begin(title,nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
    ImGui::SetWindowFontScale(1.25f);
}
std::filesystem::path chooseModel() {
    std::array<wchar_t,32768> path{}; OPENFILENAMEW dialog{};
    dialog.lStructSize=sizeof(dialog); dialog.lpstrFile=path.data(); dialog.nMaxFile=DWORD(path.size());
    dialog.lpstrFilter=L"Animation meshes (*.glb;*.fbx)\0*.glb;*.fbx\0\0";
    dialog.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR;
    return GetOpenFileNameW(&dialog)?std::filesystem::path(path.data()):std::filesystem::path{};
}
Pipes::ImportedMesh fixtureMesh(const A::SkinMesh& skin,const A::Pose& pose) {
    Pipes::ImportedMesh result; const auto positions=A::skinPositions(skin,pose);
    for(auto p:positions) result.mesh.mesh.positions.push_back(renderPoint(p));
    for(auto triangle:skin.triangles) for(auto index:triangle) result.mesh.mesh.indices.push_back(index);
    result.mesh.field.resize(positions.size());
    normals(result.mesh); return result;
}
double meshDelta(const std::vector<M::Vec3>& a,const std::vector<M::Vec3>& b) {
    if(a.size()!=b.size()) throw std::runtime_error("Deformation changed vertex identity");
    double delta=0; for(std::size_t i=0;i<a.size();++i) delta=std::max(delta,M::Length(a[i]-b[i])); return delta;
}
struct ChainSelection {std::size_t chain,joint;};
std::optional<ChainSelection> selectedChain(const A::AnimationAsset& asset,const std::string& bone) {
    for(std::size_t i=0;i<asset.chains.size();++i) {
        const auto& joints=asset.chains[i].bones;const auto selected=std::find(joints.begin(),joints.end(),bone);
        if(selected!=joints.end()&&selected!=joints.begin())return ChainSelection{i,std::size_t(selected-joints.begin())};
    }
    return {};
}
}
AnimationLabUI::AnimationLabUI():editor(std::make_unique<Editor>()) {
    selected=editor->pose().front().id;
    const auto path=(std::filesystem::path(PLAYGROUND_ROOT)/"Evidence/animation-tools/edited-rig.json").string();
    std::snprintf(documentPath.data(),documentPath.size(),"%s",path.c_str());
}
void AnimationLabUI::adopt(AnimationImport::Asset asset) {
    auto next=std::make_unique<Editor>(AnimationImport::animationAsset(asset));
    clearDocumentTransients();
    imported=std::move(asset); editor=std::move(next); selected=editor->pose().empty()?"":editor->pose().front().id;
    playing=false; dirty=fitPending=true; chainChoice=0; correction={};
    interactiveScene=false;sceneValve.reset();physics.reset();physicalGrabs.clear();recording=false;
    if(!imported->clips.empty()) {rangeStart=imported->clips.front().start;rangeEnd=std::max(rangeStart+.1,imported->clips.front().end);}
    else {rangeStart=0;rangeEnd=2;}
    status=imported->rigDescription;
}
void AnimationLabUI::clearDocumentTransients() {
    dragging=false;draggedObject.clear();draggedLimb.clear();playing=false;
    ikMode=false;ikTarget={};chainChoice=0;
    recording=false;recorded.clear();recordingClock=0;
    sourceClipPlaying=false;sourceClipTime=0;interactionClock=0;mechanismAngle=0;
    interactiveScene=selectInteractionTab=false;sceneValve.reset();
    physics.reset();physicsSnapshot={};physicalGrabs.clear();holdingTargets.clear();previousLimbModes.clear();
    runtimeState={};runtimePose={};
}
void AnimationLabUI::recoverEditError(const char* message) {
    error=message;playing=false;dragging=false;draggedObject.clear();draggedLimb.clear();
    if(editor&&editor->editing())editor->cancelEdit();
    dirty=true;
}
void AnimationLabUI::setIkMode(bool enabled) {
    ikMode=enabled;if(!enabled||!editor||interactiveScene||editor->asset().chains.empty())return;
    if(const auto match=selectedChain(editor->asset(),selected))chainChoice=int(match->chain);
    chainChoice=std::clamp(chainChoice,0,int(editor->asset().chains.size()-1));
    const auto& chain=editor->asset().chains[chainChoice];const auto tip=editor->worldPose().at(chain.bones.back());
    ikTarget=tip.position+M::Rotate(tip.rotation,chain.tipOffset);
}
void AnimationLabUI::dragBoneTo(M::Vec3 goal) {
    const auto& bone=*std::find_if(editor->pose().begin(),editor->pose().end(),[&](const auto& b){return b.id==selected;});
    if(!interactiveScene&&!bone.parent.empty()&&ikMode)if(const auto match=selectedChain(editor->asset(),selected)) {
        auto chain=editor->asset().chains[match->chain];chainChoice=int(match->chain);
        if(match->joint+1<chain.bones.size()){chain.bones.resize(match->joint+1);chain.tipOffset={};}
        ikTarget=goal;editor->solveIk(chain,goal);dirty=true;return;
    }
    if(interactiveScene) {
        auto worldTarget=editor->worldPose().at(selected);worldTarget.position=goal;
        const auto local=bone.parent.empty()?worldTarget:M::Compose(M::Inverse(editor->worldPose().at(bone.parent)),worldTarget);
        applyInteraction(A::withPoseOverride(editor->document(),editor->interaction(),selected,local,editor->currentTime(),interactionClock));
    }else editor->moveWorld(selected,goal);
    dirty=true;
}
void AnimationLabUI::loadDocument(const std::filesystem::path& path) {
    if(editor)editor->load(path);
    else {
        auto candidate=std::make_unique<Editor>();
        candidate->load(path);
        editor=std::move(candidate);
    }
    clearDocumentTransients();imported.reset();selected=editor->pose().front().id;dirty=fitPending=true;
    interactiveScene=!editor->interaction().sceneId.empty();interactionClock=editor->interactionTime();
    if(editor->interaction().sceneId=="valve")sceneValve=AnimationImport::loadAsset(std::filesystem::path(PLAYGROUND_ROOT)/"models/VALVE01_Gate_PBR_1K.glb");
    if(interactiveScene){resetPhysics();stepInteraction(0);}
    status="Document loaded";
}
void AnimationLabUI::loadModel(int index) {
    modelChoice=index;
    if(index==0) {
        auto next=std::make_unique<Editor>(A::ownSpiderFixture());
        clearDocumentTransients();
        editor=std::move(next); imported.reset(); selected=editor->pose().front().id;
        rangeStart=0;rangeEnd=2;playing=false;chainChoice=0;dirty=fitPending=true;correction={};
        interactiveScene=false;sceneValve.reset();physics.reset();physicalGrabs.clear();recording=false;
        status="Own eight-leg fixture / authored FK cycle"; return;
    }
    const char* names[]={"alien+creature+3d+model.glb","giant+spider+3d+model.glb","Philips.fbx","VALVE01_Gate_PBR_1K.glb"};
    auto asset=AnimationImport::loadAsset(std::filesystem::path(PLAYGROUND_ROOT)/"models"/names[index-1]);
    if(asset.document.bones.empty()) {
        clearDocumentTransients();
        imported=std::move(asset);editor.reset();selected.clear();playing=false;dirty=fitPending=true;
        status="Static mesh loaded. Create a rig to start animating.";
    } else adopt(std::move(asset));
}
void AnimationLabUI::applyInteraction(A::InteractionState candidate) {
    editor->setInteraction(std::move(candidate));
    stepInteraction(0);dirty=true;
}
void AnimationLabUI::loadScene(int index) {
    auto scene=AnimationScene::create(static_cast<AnimationScene::Preset>(index));
    auto next=std::make_unique<Editor>(std::move(scene.actor));
    next->setInteraction(std::move(scene.interaction));
    clearDocumentTransients();
    editor=std::move(next);imported.reset();sceneValve=std::move(scene.valve);
    sceneChoice=index;interactiveScene=selectInteractionTab=true;interactionClock=recordingClock=0;recording=false;recorded.clear();
    maximumTransitionPosition=maximumTransitionAngle=maximumPhysicsEnergy=0;transitionChecks=0;
    selected=editor->pose().front().id;limbChoice=grabChoice=objectChoice=chainChoice=0;
    rangeStart=0;rangeEnd=2;playing=index==3||index==4;dirty=fitPending=true;sourceClipPlaying=false;sourceClipTime=0;
    resetPhysics();stepInteraction(0);
    status=index==4?"Original eight-leg fixture: animated base and persistent IK supports":"Interactive scene: base animation, hand targets and object authority";
}
void AnimationLabUI::resetPhysics() {
    physicalGrabs.clear();holdingTargets.clear();previousLimbModes.clear();runtimeState=editor->interaction();
    auto description=AnimationScene::physicsDescription(runtimeState);
    if(sceneValve)for(auto& body:description.bodies)if(body.id=="valve:gltf:100") {
        const auto principal=2*std::atan2(body.world.rotation.y,body.world.rotation.w);
        const auto candidate=sourceClipTime>0?AnimationScene::valveSourceAngle(*sceneValve,sourceClipTime):principal;
        if(std::abs(std::remainder(candidate-principal,2*std::numbers::pi))<1e-5)body.hingeInitialUnwrappedAngle=candidate;
        for(const auto& previous:physicsSnapshot.bodies)if(previous.id==body.id&&std::abs(std::remainder(previous.hingeUnwrappedAngle-principal,2*std::numbers::pi))<1e-5&&!sourceClipPlaying)body.hingeInitialUnwrappedAngle=previous.hingeUnwrappedAngle;
    }
    if(description.bodies.empty()){physics.reset();physicsSnapshot={};return;}
    if(!physics)physics=std::make_unique<A::InteractionPhysics::World>();
    physics->reset(description);physicsSnapshot=physics->snapshot();
}
void AnimationLabUI::changeAuthority(const std::string& id,A::TransformAuthority authority) {
    if(id=="valve:gltf:105")throw std::runtime_error("The stem reads the solved wheel rotation; change wheel authority");
    auto next=editor->interaction();
    if(auto live=runtimeState.objects.find(id);live!=runtimeState.objects.end())next.objects.at(id).world=live->second.world;
    next.objects.at(id).authority=authority;
    if(authority==A::TransformAuthority::PhysicsBody)for(auto& [limbId,limb]:next.limbs)
        if(!limb.grabPointId.empty()&&next.grabPoints.at(limb.grabPointId).objectId==id)limb.target=runtimePose.limbs.at(limbId).actual;
    editor->setInteraction(std::move(next));resetPhysics();stepInteraction(0);dirty=true;
    sourceClipPlaying=false;
    status=authority==A::TransformAuthority::PhysicsBody?"Jolt owns this object's transform; hand targets drive compliant constraints":"Scene owns this object's transform; IK follows its GrabPoints";
}
void AnimationLabUI::setObjectTarget(const std::string& id,M::RigidTransform target) {
    if(id=="valve:gltf:105")throw std::runtime_error("The stem reads the solved wheel rotation; drive the wheel");
    if(id=="valve:gltf:100"){sourceClipPlaying=false;sourceClipTime=0;}
    const auto& current=editor->interaction();
    auto next=current;
    if(next.objects.at(id).authority==A::TransformAuthority::KinematicScene) {
        if(id=="door"||id=="valve:gltf:100") {
            const auto angle=2*std::atan2(target.rotation.y,target.rotation.w);
            const auto constrained=id=="door"?std::clamp(angle,0.,std::numbers::pi/2):angle;
            target.rotation=M::AxisAngle({0,1,0},constrained);
            target.position=id=="door"?M::Vec3{-.8,1.55,.55}+M::Rotate(target.rotation,{.4,0,0}):M::Vec3{0,1.8,.55};
        }
        next=A::withObjectTransform(next,id,target);
    }
    else {
        bool held=false;
        for(const auto& [limbId,limb]:current.limbs)if(!limb.grabPointId.empty()) {
            const auto& point=current.grabPoints.at(limb.grabPointId);if(point.objectId!=id)continue;
            // This is a controller request. The object transform remains owned
            // by Jolt and the character's IK continues to read the solved grip.
            next=A::withLimbTarget(next,limbId,M::Compose(target,M::Compose(point.local,limb.grabOffset)),interactionClock);held=true;
        }
        if(!held)throw std::runtime_error("Grab the physical object before driving it with hand targets");
    }
    applyInteraction(std::move(next));
}
void AnimationLabUI::stepInteraction(double seconds) {
    if(!interactiveScene||!editor)return;
    interactionClock+=seconds;runtimeState=editor->interaction();
    if(sourceClipPlaying&&sceneValve&&runtimeState.objects.at("valve:gltf:100").authority==A::TransformAuthority::KinematicScene) {
        sourceClipTime=std::fmod(sourceClipTime+seconds,4.);
        const auto sourcePose=A::Examples::EvaluateLocals(sceneValve->document,sourceClipTime);
        const auto sourceWorld=M::ForwardKinematics(sourcePose);
        auto wheel=sourceWorld.at("gltf:100");wheel.position=wheel.position+M::Vec3{0,1.3,.55};
        runtimeState=A::withObjectTransform(runtimeState,"valve:gltf:100",wheel);
        auto stem=sourceWorld.at("gltf:105");stem.position=stem.position+M::Vec3{0,1.3,.55};
        runtimeState.objects["valve:gltf:105"]={"valve:gltf:105",stem,A::TransformAuthority::KinematicScene};
    }
    if(physics) {
        for(const auto& body:physicsSnapshot.bodies)if(body.authority==A::TransformAuthority::PhysicsBody)
            runtimeState=A::withPhysicsSnapshot(runtimeState,body.id,body.world);
        for(const auto& [id,object]:runtimeState.objects)if(object.authority==A::TransformAuthority::KinematicScene&&id!="valve:gltf:105")physics->setKinematicTarget(id,object.world);
        runtimePose=editor->evaluateRuntime(interactionClock,&runtimeState);
        for(auto it=physicalGrabs.begin();it!=physicalGrabs.end();) {
            const auto limb=runtimeState.limbs.find(it->first);
            if(limb==runtimeState.limbs.end()||limb->second.grabPointId!=it->second) {physics->release(it->first);holdingTargets.erase(it->first);previousLimbModes.erase(it->first);it=physicalGrabs.erase(it);}else ++it;
        }
        for(const auto& [id,limb]:runtimeState.limbs)if(!limb.grabPointId.empty()) {
            const auto& point=runtimeState.grabPoints.at(limb.grabPointId);
            const auto actual=runtimePose.limbs.at(id).actual;
            const auto grip=A::grabPointWorld(runtimeState,point.id);
            const auto physical=runtimeState.objects.at(point.objectId).authority==A::TransformAuthority::PhysicsBody;
            if(!holdingTargets.contains(id))holdingTargets[id]=grip;
            if(previousLimbModes.contains(id)&&previousLimbModes.at(id)==A::LimbMode::Fk&&limb.mode==A::LimbMode::Ik)holdingTargets[id]=actual;
            previousLimbModes[id]=limb.mode;
            const auto requested=physical?(limb.mode==A::LimbMode::Fk||runtimePose.limbs.at(id).weight<1-1e-8?actual:limb.motorTargetEnabled?limb.target:holdingTargets.at(id)):grip;
            physics->setHandTarget(id,physicalGrabs.contains(id)?requested:grip);
            if(!physicalGrabs.contains(id)&&M::Length(actual.position-grip.position)<.03&&interactionClock>=limb.targetStart+limb.targetDuration) {
                const auto q=M::Multiply(M::Inverse(grip).rotation,actual.rotation);
                if(2*std::acos(std::clamp(std::abs(q.w),0.,1.))<.3) {
                    physics->attach(id,point.objectId,M::Compose(point.local,limb.grabOffset),physical?A::InteractionPhysics::GrabMode::Physical:A::InteractionPhysics::GrabMode::Kinematic);
                    physicalGrabs.emplace(id,point.id);
                }
            }
        }
        physics->advance(seconds);physicsSnapshot=physics->snapshot();
        for(const auto& body:physicsSnapshot.bodies)maximumPhysicsEnergy=std::max(maximumPhysicsEnergy,body.kineticEnergyJoules);
        for(const auto& body:physicsSnapshot.bodies)if(body.authority==A::TransformAuthority::PhysicsBody)runtimeState=A::withPhysicsSnapshot(runtimeState,body.id,body.world);
        for(const auto& derived:physicsSnapshot.derived) {
            if(!sourceClipPlaying)runtimeState.objects[derived.id]={derived.id,derived.world,A::TransformAuthority::PhysicsBody};
        }
    }
    runtimePose=editor->evaluateRuntime(interactionClock,&runtimeState);dirty=true;
    if(recording&&seconds>0){recordingClock+=seconds;recorded.push_back({recordingClock,editor->pose()});}
}
void AnimationLabUI::sceneHierarchy() {
    if(!interactiveScene||!editor)return;
    if(ImGui::TreeNodeEx("Objects / GrabPoints",ImGuiTreeNodeFlags_DefaultOpen)) {
        int index=0;
        for(const auto& [id,object]:editor->interaction().objects) {
            ImGui::PushID(id.c_str());
            if(ImGui::Selectable(id.c_str(),objectChoice==index))objectChoice=index;
            for(const auto& [pointId,point]:editor->interaction().grabPoints)if(point.objectId==id) {
                ImGui::Indent();ImGui::Text("%s: %s",pointId.c_str(),point.status==A::GrabStatus::Occupied?point.occupant.c_str():point.status==A::GrabStatus::Released?"released":"free");ImGui::Unindent();
            }
            ImGui::PopID();++index;
        }
        ImGui::TreePop();
    }
}
void AnimationLabUI::interactionInspector() {
    if(!interactiveScene||!editor)return;
    auto state=editor->interaction();
    if(state.limbs.empty())return;
    limbChoice=std::clamp(limbChoice,0,int(state.limbs.size()-1));
    auto chosen=std::next(state.limbs.begin(),limbChoice);const auto id=chosen->first;auto limb=chosen->second;
    fieldLabel("Limb");
    if(ImGui::BeginCombo("##limb",id.c_str())) {
        int i=0;for(const auto& [limbId,control]:state.limbs){if(ImGui::Selectable(limbId.c_str(),i==limbChoice)) {
            limbChoice=i;int pointIndex=0;
            for(const auto& [pointId,point]:state.grabPoints){if((static_cast<int>(point.permittedHands)&static_cast<int>(control.hand))!=0&&point.status!=A::GrabStatus::Occupied){grabChoice=pointIndex;break;}++pointIndex;}
        }++i;}ImGui::EndCombo();
    }
    ImGui::Text("Mode: %s",limb.mode==A::LimbMode::Ik?"IK":"FK");ImGui::SameLine();
    if(ImGui::Button("IK"))applyInteraction(A::withLimbMode(editor->document(),runtimeState,id,A::LimbMode::Ik,editor->currentTime(),interactionClock,editor->state().locks));
    ImGui::SameLine();if(ImGui::Button("FK"))applyInteraction(A::withLimbMode(editor->document(),runtimeState,id,A::LimbMode::Fk,editor->currentTime(),interactionClock,editor->state().locks));
    double blend=limb.weight,min=0,max=1;
    fieldLabel("FK / IK blend");
    if(ImGui::SliderScalar("##blend",ImGuiDataType_Double,&blend,&min,&max,"%.2f"))applyInteraction(A::withLimbBlend(editor->interaction(),id,blend,interactionClock));
    if(!state.grabPoints.empty()) {
        grabChoice=std::clamp(grabChoice,0,int(state.grabPoints.size()-1));auto grab=std::next(state.grabPoints.begin(),grabChoice);
        fieldLabel("GrabPoint");
        if(ImGui::BeginCombo("##grabPoint",grab->first.c_str())){int i=0;for(const auto& [pointId,point]:state.grabPoints){if(ImGui::Selectable(pointId.c_str(),i==grabChoice))grabChoice=i;++i;}ImGui::EndCombo();}
        ImGui::Text("%s / hand %s",grab->second.type==A::GrabType::Wheel?"Wheel":grab->second.type==A::GrabType::Handle?"Handle":"Fixed",grab->second.permittedHands==A::Hand::Left?"left":grab->second.permittedHands==A::Hand::Right?"right":"either");
        ImGui::BeginDisabled(limb.hand==A::Hand::None||!limb.grabPointId.empty()||grab->second.status==A::GrabStatus::Occupied||sourceClipPlaying);
        if(ImGui::Button("Grab"))applyInteraction(A::acquireGrab(editor->document(),runtimeState,id,grab->first,editor->currentTime(),interactionClock,.4,editor->state().locks));
        ImGui::EndDisabled();
        ImGui::SameLine();ImGui::BeginDisabled(limb.grabPointId.empty());if(ImGui::Button("Release"))applyInteraction(A::releaseGrab(editor->document(),runtimeState,id,editor->currentTime(),interactionClock,.4,editor->state().locks));ImGui::EndDisabled();
    }
    auto target=limb.target;
    auto transaction=[&](bool changed,auto command) {
        if(ImGui::IsItemActivated()&&!editor->editing())editor->beginEdit();
        if(changed)command();
        if(ImGui::IsItemDeactivated()&&editor->editing())editor->commitEdit();
    };
    fieldLabel("Hand / tip target (X, Y, Z)");
    bool changed=ImGui::DragScalarN("##handTarget",ImGuiDataType_Double,&target.position.x,3,.005f,nullptr,nullptr,"%.3f m");
    auto editTarget=[&]{applyInteraction(A::withLimbTarget(editor->interaction(),id,target,interactionClock));};
    transaction(changed,editTarget);
    auto angles=Pipes::eulerAngles({target.rotation.x,target.rotation.y,target.rotation.z,target.rotation.w});
    fieldLabel("Target orientation (X, Y, Z)");
    changed=ImGui::DragScalarN("##targetOrientation",ImGuiDataType_Double,&angles.x,3,.3f,nullptr,nullptr,"%.1f deg");
    if(changed){auto q=Pipes::quaternion(angles);target.rotation={q[0],q[1],q[2],q[3]};}
    transaction(changed,editTarget);
    if(ImGui::Button("Pin current tip")){auto actual=runtimePose.limbs.at(id).actual;
        auto next=A::withLimbMode(editor->document(),editor->interaction(),id,A::LimbMode::Ik,editor->currentTime(),interactionClock,editor->state().locks);
        applyInteraction(A::withLimbTarget(next,id,actual,interactionClock));}
    if(state.sceneId=="spider"&&ImGui::Button("Pin all eight feet")) {
        auto next=editor->interaction();auto runtime=runtimePose;
        for(const auto& [limbId,control]:state.limbs){next=A::withLimbMode(editor->document(),next,limbId,A::LimbMode::Ik,editor->currentTime(),interactionClock,editor->state().locks);next=A::withLimbTarget(next,limbId,runtime.limbs.at(limbId).actual,interactionClock);}
        applyInteraction(std::move(next));
    }
    if(ImGui::CollapsingHeader("Pole / joint limits")) {
        auto pole=limb.pole.value_or(M::Vec3{});
        fieldLabel("Pole (X, Y, Z)");
        changed=ImGui::DragScalarN("##pole",ImGuiDataType_Double,&pole.x,3,.01f,nullptr,nullptr,"%.3f");
        transaction(changed,[&]{auto next=editor->interaction();next.limbs.at(id).pole=pole;applyInteraction(std::move(next));});
        for(const auto& bone:limb.chain.bones) {
            ImGui::PushID(bone.c_str());auto found=state.jointLimits.find(bone);A::JointLimit limit=found==state.jointLimits.end()?A::JointLimit{bone}:found->second;
            ImGui::TextWrapped("%s",bone.c_str());double degrees=limit.swingRadians*180/std::numbers::pi;
            changed=ImGui::DragScalar("Swing max",ImGuiDataType_Double,&degrees,.5f,nullptr,nullptr,"%.1f deg");degrees=std::clamp(degrees,0.,180.);
            transaction(changed,[&]{auto next=editor->interaction();limit.swingRadians=degrees*std::numbers::pi/180;next.jointLimits[bone]=limit;applyInteraction(std::move(next));});
            ImGui::PopID();
        }
    }
    if(auto found=runtimePose.limbs.find(id);found!=runtimePose.limbs.end()) {
        const auto& diagnostic=found->second;
        ImGui::Text("IK %.5g m / %.4g rad",diagnostic.positionError,diagnostic.angularError);
        ImGui::TextWrapped("Contact %.5g m | limit %.4g rad",diagnostic.contactError,diagnostic.limitViolation);
        if(diagnostic.partialBlend)ImGui::TextWrapped("Partial blend: contact offset shown above");
        if(diagnostic.unreachable||diagnostic.singular||diagnostic.hardLockConflict)ImGui::TextColored({1,.63f,.28f,1},"%s%s%s",diagnostic.unreachable?"Unreachable ":"",diagnostic.singular?"Singular ":"",diagnostic.hardLockConflict?"Locked conflict":"");
    }
    ImGui::Text("Coupled solve: %zu unique joints",runtimePose.uniqueSolverJoints);
    ImGui::Separator();
    if(!state.objects.empty()) {
        objectChoice=std::clamp(objectChoice,0,int(state.objects.size()-1));auto object=std::next(state.objects.begin(),objectChoice);
        fieldLabel("Object");
        if(ImGui::BeginCombo("##object",object->first.c_str())){int i=0;for(const auto& [objectId,value]:state.objects){if(ImGui::Selectable(objectId.c_str(),i==objectChoice))objectChoice=i;++i;}ImGui::EndCombo();}
        const auto objectId=object->first;const bool physical=object->second.authority==A::TransformAuthority::PhysicsBody;
        ImGui::Text("Authority: %s",physical?"Jolt physics":"Kinematic scene");
        if(ImGui::Button(physical?"Use kinematic":"Use physical"))changeAuthority(objectId,physical?A::TransformAuthority::KinematicScene:A::TransformAuthority::PhysicsBody);
        auto transform=runtimeState.objects.at(objectId).world;
        fieldLabel(physical?"Drive via hands (X, Y, Z)":"Object position (X, Y, Z)");
        changed=ImGui::DragScalarN("##objectPosition",ImGuiDataType_Double,&transform.position.x,3,.005f,nullptr,nullptr,"%.3f m");
        transaction(changed,[&]{setObjectTarget(objectId,transform);});
        auto rotation=Pipes::eulerAngles({transform.rotation.x,transform.rotation.y,transform.rotation.z,transform.rotation.w});
        fieldLabel("Object orientation (X, Y, Z)");
        changed=ImGui::DragScalarN("##objectOrientation",ImGuiDataType_Double,&rotation.x,3,.3f,nullptr,nullptr,"%.1f deg");
        if(changed){auto q=Pipes::quaternion(rotation);transform.rotation={q[0],q[1],q[2],q[3]};}
        transaction(changed,[&]{setObjectTarget(objectId,transform);});
        if(objectId=="door"||objectId=="valve:gltf:100") {
            double lower=objectId=="door"?0:-.8,upper=objectId=="door"?std::numbers::pi/2:.8;
            fieldLabel("Mechanism angle");
            changed=ImGui::SliderScalar("##mechanismAngle",ImGuiDataType_Double,&mechanismAngle,&lower,&upper,"%.3f rad");
            transaction(changed,[&]{auto q=M::AxisAngle({0,1,0},mechanismAngle);auto desired=object->second.world;desired.rotation=q;
                if(objectId=="door")desired.position=M::Vec3{-.8,1.55,.55}+M::Rotate(q,{.4,0,0});setObjectTarget(objectId,desired);});
        }
    }
    if(sceneValve) {
        const bool occupied=std::any_of(state.grabPoints.begin(),state.grabPoints.end(),[](const auto& entry){return entry.second.status==A::GrabStatus::Occupied;});
        ImGui::BeginDisabled(occupied||state.objects.at("valve:gltf:100").authority==A::TransformAuthority::PhysicsBody);
        ImGui::Checkbox("Play original valve clip (4 s)",&sourceClipPlaying);
        ImGui::EndDisabled();
        ImGui::TextWrapped("The source wheel makes ~19.7 turns. Joint errors remain visible; the reachable hand exercise uses +/-0.8 rad.");
    }
    if(physics) {
        ImGui::Text("Physics: %llu fixed ticks / %.3f ms",static_cast<unsigned long long>(physicsSnapshot.fixedSteps),physicsSnapshot.stepCpuMilliseconds);
        for(const auto& grip:physicsSnapshot.grabs)ImGui::TextWrapped("%s contact %.4g m / %.4g rad",grip.handId.c_str(),grip.positionErrorMeters,grip.angularErrorRadians);
        if(physicsSnapshot.conflicting)ImGui::TextColored({1,.45f,.35f,1},"Physical constraints conflict");
        for(const auto& body:physicsSnapshot.bodies)ImGui::Text("%s: %.3g J / %zu contacts",body.id.c_str(),body.kineticEnergyJoules,body.contacts);
    }
}
void AnimationLabUI::fit() {
    if(displayed.mesh.mesh.positions.empty()) return;
    auto low=displayed.mesh.mesh.positions.front(),high=low;
    for(auto p:displayed.mesh.mesh.positions) {
        low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};
    }
    camera.target={float((low.x+high.x)*.5),float((low.y+high.y)*.5),float((low.z+high.z)*.5)};
    camera.distance=float(std::max(.2,std::hypot(high.x-low.x,high.y-low.y,high.z-low.z)*1.5));
    camera.yaw=.5f;camera.pitch=-.25f;fitPending=false;
}
void AnimationLabUI::rebuild(Renderer& renderer) {
    auto start=std::chrono::steady_clock::now();
    if(imported) displayed=editor?AnimationImport::deform(*imported,editor->pose()):imported->renderMesh;
    else if(editor) displayed=fixtureMesh(editor->asset().mesh,editor->pose());
    if(interactiveScene)AnimationScene::appendObjects(displayed,runtimeState,sceneValve?&*sceneValve:nullptr);
    skinMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    if(!displayed.materials.empty()) renderer.uploadPipes(displayed);
    else renderer.upload(displayed.mesh);
    dirty=false;if(fitPending)fit();
}
void AnimationLabUI::insertKeys(bool all) {
    if(!editor) return;
    editor->beginEdit();
    try {if(all) {std::vector<std::string> ids;for(const auto& bone:editor->pose())ids.push_back(bone.id);
            for(const auto& id:ids)if(!editor->locked(id))editor->setKey(id);}
        else if(!selected.empty()&&!editor->locked(selected))editor->setKey(selected);
        editor->commitEdit();status="Keyframe inserted";
    }catch(...){editor->cancelEdit();throw;}
}
void AnimationLabUI::hierarchy() {
    if(!editor) {ImGui::TextWrapped("This mesh has no skeleton. Choose a rig preset below.");return;}
    const bool nodeControls=imported&&imported->importedRig&&imported->statistics.skins==0&&imported->source.extension()==".glb";
    ImGui::Text("%zu %s / %zu chains",editor->pose().size(),nodeControls?"nodes":"bones",editor->asset().chains.size());
    ImGui::BeginChild("Bone hierarchy",{0,0},true);
    std::function<void(const std::string&)> children=[&](const std::string& parent) {
        for(const auto& bone:editor->pose()) if(bone.parent==parent) {
            const bool leaf=std::none_of(editor->pose().begin(),editor->pose().end(),[&](const auto& b){return b.parent==bone.id;});
            auto flags=ImGuiTreeNodeFlags_OpenOnArrow|ImGuiTreeNodeFlags_SpanAvailWidth;
            if(parent.empty())flags|=ImGuiTreeNodeFlags_DefaultOpen;
            if(leaf)flags|=ImGuiTreeNodeFlags_Leaf;
            if(selected==bone.id)flags|=ImGuiTreeNodeFlags_Selected;
            ImGui::PushID(bone.id.c_str());
            std::string name=bone.id;
            if(imported){auto source=std::find_if(imported->sourceJoints.begin(),imported->sourceJoints.end(),[&](const auto& joint){return joint.id==bone.id;});if(source!=imported->sourceJoints.end())name=source->name;}
            const auto label=(editor->locked(bone.id)?"[LOCK] ":"")+name;
            bool open=ImGui::TreeNodeEx(label.c_str(),flags);
            if(ImGui::IsItemClicked())selected=bone.id;
            if(open) {if(!leaf)children(bone.id);ImGui::TreePop();}
            ImGui::PopID();
        }
    };
    children("");ImGui::EndChild();
}
void AnimationLabUI::properties() {
    if(!editor||selected.empty())return;
    auto bone=std::find_if(editor->pose().begin(),editor->pose().end(),[&](const auto& b){return b.id==selected;});
    if(bone==editor->pose().end())return;
    auto local=bone->local;
    ImGui::TextWrapped("%s",selected.c_str());
    if(imported){auto source=std::find_if(imported->sourceJoints.begin(),imported->sourceJoints.end(),[&](const auto& joint){return joint.id==selected;});if(source!=imported->sourceJoints.end())ImGui::TextWrapped("%s",source->name.c_str());}
    bool locked=editor->locked(selected);
    if(ImGui::Checkbox("Lock edit / solve",&locked)) {editor->setLocked(selected,locked);dirty=true;}
    if(ImGui::IsItemHovered())ImGui::SetTooltip("Solves preserve this bone's evaluated base transform. Pin an IK target for a persistent world-space support.");
    ImGui::BeginDisabled(locked||(playing&&!interactiveScene));
    auto transaction=[&](bool changed) {
        if(ImGui::IsItemActivated()&&!editor->editing())editor->beginEdit();
        if(changed){if(interactiveScene)applyInteraction(A::withPoseOverride(editor->document(),editor->interaction(),selected,local,editor->currentTime(),interactionClock));
            else editor->setLocal(selected,local);dirty=true;}
        if(ImGui::IsItemDeactivated()&&editor->editing())editor->commitEdit();
    };
    ImGui::TextUnformatted("FK / local position (metres)");
    const char* axes[]={"X","Y","Z"};
    for(int axis=0;axis<3;++axis){double* value=axis==0?&local.position.x:axis==1?&local.position.y:&local.position.z;
        ImGui::PushID(axis);transaction(ImGui::DragScalar(axes[axis],ImGuiDataType_Double,value,.01f,nullptr,nullptr,"%.4f"));ImGui::PopID();}
    auto angles=Pipes::eulerAngles({local.rotation.x,local.rotation.y,local.rotation.z,local.rotation.w});
    ImGui::TextUnformatted("FK / local rotation (degrees)");
    for(int axis=0;axis<3;++axis) {
        double* value=axis==0?&angles.x:axis==1?&angles.y:&angles.z;
        ImGui::PushID(axis+10);bool changed=ImGui::DragScalar(axes[axis],ImGuiDataType_Double,value,.4f,nullptr,nullptr,"%.2f");
        if(changed){auto q=Pipes::quaternion(angles);local.rotation={q[0],q[1],q[2],q[3]};}transaction(changed);ImGui::PopID();
    }
    if(ImGui::Button("Insert selected key"))insertKeys(false);
    ImGui::EndDisabled();
    if(interactiveScene)return;
    ImGui::Separator();auto requestedIk=ikMode;if(ImGui::Checkbox("IK target mode",&requestedIk))setIkMode(requestedIk);
    if(!editor->asset().chains.empty()) {
        const auto& chains=editor->asset().chains; chainChoice=std::clamp(chainChoice,0,int(chains.size()-1));
        if(ImGui::BeginCombo("Chain",chains[chainChoice].id.c_str())) {
            for(int i=0;i<int(chains.size());++i)if(ImGui::Selectable(chains[i].id.c_str(),chainChoice==i)){chainChoice=i;const auto& c=chains[i];auto t=editor->worldPose().at(c.bones.back());ikTarget=t.position+M::Rotate(t.rotation,c.tipOffset);}
            ImGui::EndCombo();
        }
        if(ImGui::Button("Target at current tip")){const auto& c=chains[chainChoice];auto t=editor->worldPose().at(c.bones.back());ikTarget=t.position+M::Rotate(t.rotation,c.tipOffset);}
        ImGui::InputScalarN("Goal XYZ",ImGuiDataType_Double,&ikTarget.x,3,nullptr,nullptr,"%.3f");
        if(ImGui::Button("Solve IK")) {auto result=editor->solveIk(chains[chainChoice],ikTarget);dirty=true;status="IK residual: "+std::to_string(result.residual)+" m";}
    }
    ImGui::Separator();
    ImGui::InputDouble("Ground Y (metres)",&groundHeight,0,0,"%.3f");
    if(ImGui::Button("Correct ground / lengths")){correction=editor->correctPositions(groundHeight);dirty=true;status=correction.converged?"Correction converged":"Correction budget exhausted; inspect residuals";}
    ImGui::Text("Before %.6g m",correction.residualBefore);ImGui::Text("After  %.6g m",correction.residualAfter);
    ImGui::Text("Iterations %zu",correction.iterations);
}
void AnimationLabUI::timeline() {
    if(!editor)return;
    if(ImGui::Button(playing?"Pause [Space]":"Play [Space]"))playing=!playing;
    ImGui::SameLine();if(ImGui::Button("Start")){playing=false;editor->seek(rangeStart);dirty=true;}
    ImGui::SameLine();ImGui::Checkbox("Loop",&loop);ImGui::SameLine();
    ImGui::SetNextItemWidth(90);ImGui::InputDouble("Speed",&speed,0,0,"%.2fx");speed=std::clamp(speed,.05,4.);
    ImGui::SameLine();if(ImGui::Button("Key all unlocked"))insertKeys(true);
    ImGui::SameLine();if(ImGui::Button("Delete selected key")&&!selected.empty()){editor->removeKey(selected,editor->currentTime());dirty=true;}
    if(interactiveScene) {
        if(ImGui::Button(recording?"Stop recording":"Record result")) {
            recording=!recording;
            if(recording){recorded.clear();recordingClock=0;recorded.push_back({0,editor->pose()});}
        }
        ImGui::SameLine();ImGui::BeginDisabled(recording||recorded.size()<2);
        if(ImGui::Button("Bake new take")){const auto id="Take "+std::to_string(editor->interaction().takes.size()+1);applyInteraction(A::withBakedTake(editor->interaction(),editor->document(),id,recorded));status=id+" baked from "+std::to_string(recorded.size())+" evaluated poses";}
        ImGui::EndDisabled();
        if(!editor->interaction().takes.empty()) {
            ImGui::SameLine();ImGui::SetNextItemWidth(220);
            if(ImGui::BeginCombo("Baked takes","Play saved result")) {
                std::vector<std::string> ids;for(const auto& [id,take]:editor->interaction().takes)ids.push_back(id);
                for(const auto& id:ids)if(ImGui::Selectable(id.c_str())){const auto take=editor->interaction().takes.at(id);editor->applyTake(id);rangeStart=take.start;rangeEnd=std::max(take.start+.01,take.end);playing=true;status="Playing "+id;}
                ImGui::EndCombo();
            }
        }
    }
    auto time=editor->currentTime();ImGui::SetNextItemWidth(-120);
    if(ImGui::SliderScalar("Seconds",ImGuiDataType_Double,&time,&rangeStart,&rangeEnd,"%.3f s")){playing=false;editor->seek(time);dirty=true;}
    ImGui::SetNextItemWidth(85);ImGui::InputDouble("Start s",&rangeStart,0,0,"%.2f");ImGui::SameLine();
    ImGui::SetNextItemWidth(85);ImGui::InputDouble("End s",&rangeEnd,0,0,"%.2f");
    rangeStart=std::clamp(rangeStart,0.,3600.);rangeEnd=std::clamp(rangeEnd,rangeStart+.01,rangeStart+3600.);
    auto track=std::find_if(editor->document().tracks.begin(),editor->document().tracks.end(),[&](const auto& t){return t.bone_id==selected;});
    ImGui::SameLine();ImGui::Text("Selected keys: %zu",track==editor->document().tracks.end()?0:track->keys.size());
    if(track!=editor->document().tracks.end()) {
        ImGui::BeginChild("Keys",{0,36},false,ImGuiWindowFlags_HorizontalScrollbar);
        for(std::size_t i=0;i<track->keys.size();++i){if(i)ImGui::SameLine();ImGui::PushID(int(i));auto label=std::to_string(track->keys[i].time)+" s";
            if(ImGui::Button(label.c_str())){playing=false;editor->seek(track->keys[i].time);dirty=true;}ImGui::PopID();}ImGui::EndChild();
    }
}
void AnimationLabUI::viewport(Renderer& renderer,float x,float y,float width,float height) {
    panel("ANIMATION VIEWPORT",x,y,width,height);
    const bool nodeControls=imported&&imported->importedRig&&imported->statistics.skins==0&&imported->source.extension()==".glb";
    ImGui::Checkbox(nodeControls?"Node controls":"Bones",&showBones);ImGui::SameLine();if(ImGui::Button("Fit"))fit();ImGui::SameLine();
    ImGui::SetNextItemWidth(150);ImGui::Combo("View",&view,"Textured\0Neutral\0Wireframe\0");
    if(nodeControls){ImGui::SameLine();ImGui::Checkbox("All nodes",&showAllNodes);}
    auto size=ImGui::GetContentRegionAvail();size.x=std::max(32.f,size.x);size.y=std::max(32.f,size.y);
    auto origin=ImGui::GetCursorScreenPos();int modes[]={displayed.materials.empty()?0:20,0,2};
    auto* texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,modes[view]);
    renderStatistics=renderer.pipeStats();
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);bool hovered=ImGui::IsItemHovered();
    auto& io=ImGui::GetIO();auto* draw=ImGui::GetWindowDrawList();const auto basis=ViewportDetail::basis(camera);
    auto project=[&](M::Vec3 p)->std::optional<ImVec2>{auto v=projectToViewport(renderPoint(p),camera,unsigned(size.x),unsigned(size.y));if(!v)return {};return ImVec2{origin.x+float(v->x),origin.y+float(v->y)};};
    draw->PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);
    auto line=[&](M::Vec3 a,M::Vec3 b,ImU32 color,float thick=1.f){auto pa=project(a),pb=project(b);if(pa&&pb)draw->AddLine(*pa,*pb,color,thick);};
    const auto grid=std::max(.1,double(camera.distance)/10);
    for(int i=-10;i<=10;++i){line({i*grid,groundHeight,-10*grid},{i*grid,groundHeight,10*grid},IM_COL32(115,135,160,65));line({-10*grid,groundHeight,i*grid},{10*grid,groundHeight,i*grid},IM_COL32(115,135,160,65));}
    std::string hit;float nearest=12;
    if(editor&&showBones) {
        auto world=editor->worldPose();
        for(const auto& bone:editor->pose()) {
            const auto p=world.at(bone.id).position;bool active=bone.id==selected;
            if(nodeControls&&!showAllNodes&&!active) {
                const auto track=std::find_if(editor->document().tracks.begin(),editor->document().tracks.end(),[&](const auto& value){return value.bone_id==bone.id;});
                bool moving=false;
                if(track!=editor->document().tracks.end()&&!track->keys.empty())for(const auto& key:track->keys) {
                    const auto first=track->keys.front().local;
                    const auto dot=first.rotation.x*key.local.rotation.x+first.rotation.y*key.local.rotation.y+first.rotation.z*key.local.rotation.z+first.rotation.w*key.local.rotation.w;
                    if(M::Length(key.local.position-first.position)>1e-8||std::abs(dot)<1-1e-10){moving=true;break;}
                }
                if(!moving)continue;
            }
            const auto color=active?IM_COL32(255,212,65,255):editor->locked(bone.id)?IM_COL32(255,135,160,255):IM_COL32(100,220,255,255);
            if(!bone.parent.empty()&&(!nodeControls||showAllNodes))line(world.at(bone.parent).position,p,color,active?3.f:2.f);
            if(auto pixel=project(p)){draw->AddCircleFilled(*pixel,active?7.f:4.f,color);if(active){const auto label=(editor->locked(bone.id)?"LOCK ":"")+bone.id;draw->AddText({pixel->x+9,pixel->y-12},color,label.c_str());}
                float d=std::hypot(io.MousePos.x-pixel->x,io.MousePos.y-pixel->y);if(d<nearest){nearest=d;hit=bone.id;}}
        }
        if(!interactiveScene&&ikMode)if(auto p=project(ikTarget)){draw->AddCircle(*p,10,IM_COL32(255,190,80,255),24,2);draw->AddText({p->x+12,p->y},IM_COL32_WHITE,"IK goal");}
        if(hovered&&ImGui::IsMouseClicked(0)&&!hit.empty()&&(!playing||interactiveScene)){selected=hit;
            if(!editor->locked(selected)){editor->beginEdit();dragging=true;dragStart=world.at(selected).position;
                auto p=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),io.MousePos.x-origin.x,io.MousePos.y-origin.y,renderPoint(dragStart),basis.forward);dragHit=animationPoint(p.value_or(renderPoint(dragStart)));}}
        if(dragging) {
            if(ImGui::IsKeyPressed(ImGuiKey_Escape)){editor->cancelEdit();dragging=false;dirty=true;status="Drag cancelled";}
            else if(ImGui::IsMouseDown(0)) {
                auto p=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),io.MousePos.x-origin.x,io.MousePos.y-origin.y,renderPoint(dragStart),basis.forward);
                if(p)dragBoneTo(dragStart+animationPoint(*p)-dragHit);
            }else {editor->commitEdit();dragging=false;}
        }
    }
    if(interactiveScene) {
        std::string objectHit,limbHit;
        for(const auto& [id,object]:runtimeState.objects)if(auto p=project(object.world.position)) {
            const auto color=object.authority==A::TransformAuthority::PhysicsBody?IM_COL32(242,145,86,255):IM_COL32(136,240,154,255);
            draw->AddCircle(*p,9,color,24,2);draw->AddText({p->x+12,p->y+10},color,id.c_str());
            if(std::hypot(io.MousePos.x-p->x,io.MousePos.y-p->y)<12)objectHit=id;
        }
        int pointIndex=0;
        for(const auto& [id,point]:runtimeState.grabPoints) {
            auto transform=A::grabPointWorld(runtimeState,id);
            if(auto p=project(transform.position)) {
                const auto color=point.status==A::GrabStatus::Occupied?IM_COL32(255,190,65,255):IM_COL32(130,245,160,255);
                draw->AddCircleFilled(*p,5,color);draw->AddText({p->x+8,p->y-14},color,id.c_str());
                for(int axis=0;axis<3;++axis){M::Vec3 direction{};if(axis==0)direction.x=.08;else if(axis==1)direction.y=.08;else direction.z=.08;
                    const ImU32 colors[]={IM_COL32(255,105,105,255),IM_COL32(115,255,140,255),IM_COL32(110,155,255,255)};
                    line(transform.position,transform.position+M::Rotate(transform.rotation,direction),colors[axis],2);}
                if(hovered&&ImGui::IsMouseClicked(0)&&std::hypot(io.MousePos.x-p->x,io.MousePos.y-p->y)<10)grabChoice=pointIndex;
            }++pointIndex;
        }
        for(const auto& [id,diagnostic]:runtimePose.limbs) {
            const auto active=runtimeState.limbs.at(id).mode==A::LimbMode::Ik;
            if(!active)continue;
            const auto color=diagnostic.converged?IM_COL32(160,245,170,255):IM_COL32(255,155,75,255);
            line(diagnostic.actual.position,diagnostic.target.position,color,2);
            if(auto p=project(diagnostic.target.position)) {
                draw->AddCircle(*p,11,color,24,2);
                if(std::hypot(io.MousePos.x-p->x,io.MousePos.y-p->y)<12)limbHit=id;
            }
        }
        if(hovered&&ImGui::IsMouseClicked(0)&&!dragging&&(!objectHit.empty()||!limbHit.empty())) {
            editor->beginEdit();draggedObject=objectHit;draggedLimb=objectHit.empty()?limbHit:"";
            dragObjectStart=objectHit.empty()?editor->interaction().limbs.at(limbHit).target:runtimeState.objects.at(objectHit).world;
            dragStart=dragObjectStart.position;
            auto p=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),io.MousePos.x-origin.x,io.MousePos.y-origin.y,renderPoint(dragStart),basis.forward);
            dragHit=animationPoint(p.value_or(renderPoint(dragStart)));
        }
        try {if(!draggedObject.empty()||!draggedLimb.empty()) {
            if(ImGui::IsKeyPressed(ImGuiKey_Escape)){editor->cancelEdit();draggedObject.clear();draggedLimb.clear();resetPhysics();dirty=true;status="Scene drag cancelled";}
            else if(ImGui::IsMouseDown(0)) {
                auto p=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),io.MousePos.x-origin.x,io.MousePos.y-origin.y,renderPoint(dragStart),basis.forward);
                if(p){auto target=dragObjectStart;target.position=dragStart+animationPoint(*p)-dragHit;
                    if(!draggedObject.empty())setObjectTarget(draggedObject,target);
                    else {
                        applyInteraction(A::withLimbTarget(editor->interaction(),draggedLimb,target,interactionClock));
                    }}
            }else {if(editor->editing())editor->commitEdit();draggedObject.clear();draggedLimb.clear();}
        }}catch(const std::exception& e){recoverEditError(e.what());}
    }
    if(hovered&&!dragging&&draggedObject.empty()&&draggedLimb.empty()) {
        if(hit.empty()&&ImGui::IsMouseDragging(0)){camera.yaw+=io.MouseDelta.x*.008f;camera.pitch=std::clamp(camera.pitch+io.MouseDelta.y*.008f,-1.5f,1.5f);}
        if(io.MouseWheel)camera.distance=std::clamp(camera.distance*std::pow(.85f,io.MouseWheel),.1f,1000.f);
        if(ImGui::IsMouseDragging(1)||ImGui::IsMouseDragging(2)){const auto shift=ViewportDetail::add(ViewportDetail::mul(basis.right,-io.MouseDelta.x*camera.distance/size.y),ViewportDetail::mul(basis.up,io.MouseDelta.y*camera.distance/size.y));camera.target[0]+=float(shift.x);camera.target[1]+=float(shift.y);camera.target[2]+=float(shift.z);}
    }
    draw->PopClipRect();ImGui::End();
}
void AnimationLabUI::frame(Renderer& renderer,unsigned width,unsigned height) {
    const auto start=std::chrono::steady_clock::now();
    auto action=[&](auto function,bool clearOnSuccess=true){try{function();if(clearOnSuccess)error.clear();}catch(const std::exception& e){recoverEditError(e.what());}};
    auto& io=ImGui::GetIO();
    if(editor&&!io.WantTextInput&&!ImGui::IsAnyItemActive()) {
        if(ImGui::IsKeyPressed(ImGuiKey_Space))playing=!playing;
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Z))action([&]{editor->undo();if(interactiveScene)resetPhysics();else if(ikMode)setIkMode(true);dirty=true;});
        if(io.KeyCtrl&&ImGui::IsKeyPressed(ImGuiKey_Y))action([&]{editor->redo();if(interactiveScene)resetPhysics();else if(ikMode)setIkMode(true);dirty=true;});
        if(ImGui::IsKeyPressed(ImGuiKey_K))action([&]{insertKeys(false);});
    }
    if(playing&&editor)action([&]{editor->advance(double(io.DeltaTime)*speed,rangeStart,rangeEnd,loop);dirty=true;},false);
    if(interactiveScene)action([&]{stepInteraction(double(io.DeltaTime));},false);
    const float left=std::clamp(float(width)*.22f,230.f,310.f),right=std::clamp(float(width)*.24f,290.f,380.f),top=54,bottom=270;
    panel("RIG / ASSETS",0,top,left,float(height)-top-bottom);
    fieldLabel("Scene library");
    ImGui::Combo("##scene",&sceneChoice,"A: Arm + cube\0B: Hinged door\0C: Real valve + two hands\0D: Walking + intervention\0E: Eight-leg supports\0");
    if(ImGui::Button("Open interaction scene"))action([&]{loadScene(sceneChoice);});
    ImGui::Separator();
    fieldLabel("Asset library");
    const char* modelNames[]={"Own eight-leg fixture","Alien quadruped GLB","Giant spider GLB","Philips FBX","Animated valve GLB"};
    if(ImGui::BeginCombo("##model",interactiveScene?"Choose asset...":!evidenceModels.empty()&&imported?imported->label.c_str():modelNames[modelChoice])){for(int index=0;index<5;++index)if(ImGui::Selectable(modelNames[index],!interactiveScene&&index==modelChoice))action([&]{loadModel(index);});ImGui::EndCombo();}
    if(ImGui::Button("Open GLB / FBX..."))action([&]{auto path=chooseModel();if(!path.empty()){auto asset=AnimationImport::loadAsset(path);if(asset.document.bones.empty()){clearDocumentTransients();imported=std::move(asset);editor.reset();selected.clear();dirty=fitPending=true;}else adopt(std::move(asset));}});
    if(imported){ImGui::TextWrapped("%s",imported->label.c_str());ImGui::TextWrapped("%s",imported->rigDescription.c_str());}
    if(imported&&!editor){
        if(ImGui::Button("Detect anatomy / create rig"))action([&]{auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Automatic,7);adopt(std::move(next));});
        if(ImGui::Button("Create quadruped rig"))action([&]{auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Quadruped);adopt(std::move(next));});
        fieldLabel("Spider joints per leg (3..64)");ImGui::InputInt("##spiderJoints",&spiderJointsPerLeg);
        const bool validCount=spiderJointsPerLeg>=3&&spiderJointsPerLeg<=64;ImGui::BeginDisabled(!validCount);
        if(ImGui::Button("Create eight-leg rig"))action([&]{auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Spider,std::size_t(spiderJointsPerLeg));adopt(std::move(next));});
        ImGui::EndDisabled();if(!validCount)ImGui::TextWrapped("Choose 3 to 64 joints per leg before generating the rig.");
    }
    // Available even when the asset already carries an imported rig: replacing a bad
    // imported skeleton with an archetype one is the plain use case.
    if(imported){
        fieldLabel("Archetype joints per limb (1..16)");ImGui::InputInt("##archetypeJoints",&archetypeJointsPerLimb);
        const bool validArchetype=archetypeJointsPerLimb>=1&&archetypeJointsPerLimb<=16;
        ImGui::BeginDisabled(!validArchetype);
        if(ImGui::Button("Create archetype rig"))action([&]{auto next=*imported;AnimationImport::createArchetypeRig(next,std::size_t(archetypeJointsPerLimb));adopt(std::move(next));});
        ImGui::EndDisabled();
        if(!validArchetype)ImGui::TextWrapped("Choose 1 to 16 bones per limb before generating the archetype rig.");
        ImGui::TextWrapped("Classifies the mesh (biped, quadruped, arachnid, axial) and fits geodesic limb chains; the only path that can rig a biped.");
    }
    if(editor){ImGui::BeginDisabled(editor->editing());if(ImGui::Button("Undo"))action([&]{editor->undo();if(interactiveScene)resetPhysics();else if(ikMode)setIkMode(true);dirty=true;});ImGui::SameLine();if(ImGui::Button("Redo"))action([&]{editor->redo();if(interactiveScene)resetPhysics();else if(ikMode)setIkMode(true);dirty=true;});ImGui::EndDisabled();}
    action([&]{sceneHierarchy();},false);
    action([&]{hierarchy();},false);ImGui::End();
    panel("BONE / CONSTRAINTS",float(width)-right,top,right,float(height)-top-bottom);
    if(ImGui::BeginTabBar("Animation inspectors")) {
        if(ImGui::BeginTabItem("Bone")){action([&]{properties();},false);ImGui::EndTabItem();}
        if(interactiveScene&&ImGui::BeginTabItem("IK / Grab",nullptr,selectInteractionTab?ImGuiTabItemFlags_SetSelected:0)){selectInteractionTab=false;action([&]{interactionInspector();},false);ImGui::EndTabItem();}
        ImGui::EndTabBar();
    }
    ImGui::Separator();ImGui::TextWrapped("%s",status.c_str());if(!error.empty()){ImGui::PushTextWrapPos(0);ImGui::TextColored({1,.45f,.4f,1},"%s",error.c_str());ImGui::PopTextWrapPos();if(ImGui::Button("Dismiss error"))error.clear();}ImGui::End();
    if(dirty)action([&]{rebuild(renderer);},false);
    action([&]{viewport(renderer,left,top,std::max(64.f,float(width)-left-right),float(height)-top-bottom);},false);
    panel("TIMELINE / DOCUMENT",0,float(height)-bottom,float(width),bottom);
    action([&]{timeline();},false);ImGui::SetNextItemWidth(std::max(200.f,float(width)-480));ImGui::InputText("Document",documentPath.data(),documentPath.size());ImGui::SameLine();
    if(ImGui::Button("Save")&&editor)action([&]{editor->save(documentPath.data());status="Document saved";});ImGui::SameLine();
    if(ImGui::Button("Load"))action([&]{loadDocument(documentPath.data());});
    ImGui::Text("Skin CPU %.2f ms | UI frame %.2f ms | %zu vertices / %zu triangles | %s",skinMilliseconds,frameMilliseconds,displayed.mesh.mesh.positions.size(),displayed.mesh.mesh.indices.size()/3,renderer.adapter().c_str());
    ImGui::End();frameMilliseconds=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
int AnimationLabUI::prepareEvidence(const std::filesystem::path& folder) {
    evidenceModels.clear();const auto manifest=folder/"models.json";
    if(!std::filesystem::exists(manifest))return evidenceStages;
    std::ifstream input(manifest);const auto models=nlohmann::json::parse(input);
    if(!models.is_array()||models.empty()||models.size()>64)throw std::invalid_argument("Anatomy evidence requires 1..64 model paths");
    for(const auto& model:models) {
        const auto time=model.value("PoseTime",0.);M::Finite(time);if(time<0)throw std::invalid_argument("Anatomy evidence time must be nonnegative");
        evidenceModels.push_back({model.at("Path").get<std::string>(),model.value("Id","Unlabelled "+std::to_string(evidenceModels.size()+1)),time,model.value("FocusMouth",false),model.value("Bones",true),model.value("VerifyIkFk",false)});
    }
    return int(evidenceModels.size())*4;
}
void AnimationLabUI::evidenceStage(int stage,const std::filesystem::path& folder) {
    if(!evidenceModels.empty()) {
        const auto index=std::size_t(stage/4);const auto viewIndex=stage%4;const auto& source=evidenceModels.at(index);
        if(viewIndex==0) {
            auto asset=AnimationImport::loadAsset(source.path);
            // Recognition receives geometry only; opaque IDs prevent semantic filename hints.
            asset.label=source.id;asset.source=source.id+".glb";
            clearDocumentTransients();editor.reset();selected.clear();imported=asset;
            try {AnimationImport::createPresetRig(asset,AnimationImport::RigPreset::Automatic,7);adopt(std::move(asset));}
            catch(const std::invalid_argument& reason) {imported->rigDescription="Anatomy unresolved: "+std::string(reason.what());status=imported->rigDescription;}
            if(editor)editor->seek(source.time);
            displayed=imported->renderMesh;fit();playing=false;showBones=source.bones;error.clear();
            if(source.verifyIkFk) {
                if(!editor||editor->asset().chains.size()<2)throw std::runtime_error("IK/FK evidence requires multiple generated chains");
                const auto root=editor->pose().front().id;const auto rootLocked=editor->locked(root);editor->setLocked(root,true);
                const auto chain=editor->asset().chains.back();selected=editor->asset().chains.front().bones.back();chainChoice=0;setIkMode(true);
                selected=chain.bones.back();
                const auto before=editor->worldPose().at(selected).position;
                const auto other=editor->asset().chains.front().bones.back();const auto otherBefore=editor->worldPose().at(other).position;
                const auto target=before+(editor->worldPose().at(chain.bones.front()).position-before)*.02;
                editor->beginEdit();dragBoneTo(target);editor->commitEdit();
                const auto residual=M::Length(editor->worldPose().at(selected).position-target),otherDrift=M::Length(editor->worldPose().at(other).position-otherBefore);
                nlohmann::json report={{"id",source.id},{"chain",chain.id},{"selected",selected},{"ik_residual_m",residual},{"other_chain_drift_m",otherDrift}};
                auto record=[&]{std::ofstream output(folder/("ik-fk-"+std::to_string(index)+".json"));output<<report.dump(2);if(!output)throw std::runtime_error("Cannot write IK/FK evidence");};record();
                if(residual>1e-4||otherDrift>1e-8)throw std::runtime_error("Selected-limb IK drag used the wrong chain or failed its target");
                editor->undo();if(M::Length(editor->worldPose().at(selected).position-before)>1e-8)throw std::runtime_error("Generated IK undo changed the original pose");
                editor->redo();if(M::Length(editor->worldPose().at(selected).position-target)>1e-4)throw std::runtime_error("Generated IK redo lost its pose");
                setIkMode(false);editor->undo();
                const auto fkTarget=before+M::Vec3{.005,.003,-.002};editor->beginEdit();dragBoneTo(fkTarget);editor->commitEdit();
                if(M::Length(editor->worldPose().at(selected).position-fkTarget)>1e-8)throw std::runtime_error("Generated FK control missed its position");
                const auto fkPose=editor->worldPose();setIkMode(true);
                report["mode_target_error_m"]=M::Length(ikTarget-fkTarget);
                double jump=0,angularJump=0;const auto ikPose=editor->worldPose();
                for(const auto& [id,pose]:fkPose) {
                    const auto& current=ikPose.at(id);jump=std::max(jump,M::Length(current.position-pose.position));
                    const auto a=current.rotation,b=pose.rotation;
                    const auto minus=std::sqrt((a.x-b.x)*(a.x-b.x)+(a.y-b.y)*(a.y-b.y)+(a.z-b.z)*(a.z-b.z)+(a.w-b.w)*(a.w-b.w));
                    const auto plus=std::sqrt((a.x+b.x)*(a.x+b.x)+(a.y+b.y)*(a.y+b.y)+(a.z+b.z)*(a.z+b.z)+(a.w+b.w)*(a.w+b.w));
                    angularJump=std::max(angularJump,4*std::asin(std::clamp(std::min(minus,plus)*.5,0.,1.)));
                }
                report["mode_position_jump_m"]=jump;report["mode_angle_jump_rad"]=angularJump;record();
                if(jump>1e-8||angularJump>1e-8||M::Length(ikTarget-fkTarget)>1e-8)throw std::runtime_error("FK to IK activation retained a stale target or jumped");
                editor->undo();editor->redo();if(M::Length(editor->worldPose().at(selected).position-fkTarget)>1e-8)throw std::runtime_error("Generated FK history lost its pose");
                editor->undo();editor->setLocked(root,rootLocked);setIkMode(true);
                report["undo_redo"]=true;report["passed"]=true;record();status="Generated rig: selected-limb IK, FK, target handover and Undo/Redo verified";
            }
        }
        if(viewIndex==1){camera.yaw=3.1415927f;camera.pitch=-.12f;}
        if(viewIndex==2){camera.yaw=1.5707963f;camera.pitch=-.08f;}
        if(viewIndex==3){camera.yaw=0;camera.pitch=-1.3f;}
        if(source.focusMouth) {
            if(!editor)throw std::runtime_error("Mouth evidence requires a generated mouth rig");
            const auto rest=M::ForwardKinematics(editor->document().bones);const auto center=rest.at("mouth_center").position;
            const std::string ring=rest.contains("mouth_outer_1")?"mouth_outer_":"mouth_sector_";
            const auto right=rest.at(ring+"1").position-rest.at(ring+"9").position,up=rest.at(ring+"5").position-rest.at(ring+"13").position;
            const auto normal=M::Unit(M::Cross(right,up));const auto radius=M::Length(right)*.5;
            camera.target={float(center.x),float(center.y),float(center.z)};camera.distance=float(radius*4);
            camera.yaw=float(std::atan2(-normal.x,-normal.z)+(viewIndex==1?.35:viewIndex==2?-.35:0));
            camera.pitch=float(std::asin(-normal.y)+(viewIndex==3?-.35:0));
        }
        dirty=true;return;
    }
    auto require=[&](bool ok,const char* text){if(!ok){status="FAIL: "+std::string(text);writeEvidence(folder/("failed-stage-"+std::to_string(stage)+".json"),"runtime-check-failed");throw std::runtime_error(text);}};
    auto simulate=[&](int frames){for(int frame=0;frame<frames;++frame){if(playing)editor->advance(1./60,rangeStart,rangeEnd,loop);stepInteraction(1./60);}};
    auto acquire=[&](const std::string& limb,const std::string& point){limbChoice=int(std::distance(runtimeState.limbs.begin(),runtimeState.limbs.find(limb)));grabChoice=int(std::distance(runtimeState.grabPoints.begin(),runtimeState.grabPoints.find(point)));applyInteraction(A::acquireGrab(editor->document(),runtimeState,limb,point,editor->currentTime(),interactionClock,.4,editor->state().locks));};
    auto checkContact=[&](const std::string& limb) {const auto& diagnostic=runtimePose.limbs.at(limb);require(diagnostic.positionError<1e-4&&diagnostic.angularError<1e-4,"Native interaction did not reach the full GrabPoint transform");};
    auto transition=[&](const std::string& id,A::LimbMode mode) {
        const auto before=runtimePose.limbs.at(id).actual;
        applyInteraction(A::withLimbMode(editor->document(),runtimeState,id,mode,editor->currentTime(),interactionClock,editor->state().locks));
        const auto after=runtimePose.limbs.at(id).actual;
        ++transitionChecks;
        const auto distance=M::Length(before.position-after.position);const auto relative=M::Multiply(M::Inverse(before).rotation,after.rotation);
        const auto angle=2*std::acos(std::clamp(std::abs(relative.w),0.,1.));
        maximumTransitionPosition=std::max(maximumTransitionPosition,distance);maximumTransitionAngle=std::max(maximumTransitionAngle,angle);
        require(distance<1e-5&&angle<1e-5,"IK/FK transition introduced a discontinuity");
    };
    if(stage==0){loadModel(0);selected="spider/leg/0/hip";editor->seek(0);}
    if(stage==1){editor->seek(0);insertKeys(false);editor->seek(1);auto bone=*std::find_if(editor->pose().begin(),editor->pose().end(),[&](const auto& b){return b.id==selected;});bone.local.rotation=M::Multiply(bone.local.rotation,M::AxisAngle({0,1,0},.5));editor->setLocal(selected,bone.local);insertKeys(false);}
    if(stage==2){editor->seek(.5);require(meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()))>.001,"Key playback did not deform fixture mesh");status="Key evaluation / mesh deformation verified at 0.5 seconds";}
    if(stage==3){auto expected=editor->serialize();editor->save(folder/"edited-fixture.json");editor->moveWorld(selected,editor->worldPose().at(selected).position+M::Vec3{.1,0,0});require(editor->undo(),"Undo failed");require(editor->serialize()==expected,"Undo did not restore document");require(editor->redo(),"Redo failed");editor->load(folder/"edited-fixture.json");require(editor->serialize()==expected,"Save/load changed fixture");status="Undo / Redo / saved document restored";}
    if(stage==4){modelChoice=1;loadModel(1);require(!editor&&imported&&!imported->importedRig,"Static alien mesh misrepresented as imported rig");}
    if(stage==5){auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Quadruped);adopt(std::move(next));selected=editor->asset().chains.front().bones.front();auto p=editor->worldPose().at(selected).position;editor->moveWorld(selected,p+M::Vec3{0,.15,0});require(meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()))>.001,"Quadruped mesh did not deform");}
    if(stage==6){modelChoice=2;loadModel(2);require(!editor&&imported&&!imported->importedRig,"Static spider mesh misrepresented as imported rig");}
    if(stage==7){auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Spider);adopt(std::move(next));require(editor->asset().chains.size()==8,"Spider preset did not create eight chains");const auto& chain=editor->asset().chains.front();selected=chain.bones.back();auto end=editor->worldPose().at(selected);ikTarget=end.position+M::Rotate(end.rotation,chain.tipOffset)+M::Vec3{0,.15,0};auto result=editor->solveIk(chain,ikTarget);status="Eight-leg authored rig / IK residual "+std::to_string(result.residual)+" m";}
    if(stage==8){modelChoice=3;loadModel(3);require(editor&&imported&&imported->importedRig,"FBX did not import its real rig");status="Philips FBX: native skeleton and original binding";}
    if(stage==9){require(editor->pose().size()>1,"FBX skeleton is missing");selected=editor->pose()[1].id;auto bone=editor->pose()[1];bone.local.rotation=M::Multiply(bone.local.rotation,M::AxisAngle({0,0,1},.25));editor->setLocal(selected,bone.local);insertKeys(false);require(meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()))>.0001,"FBX bone edit did not deform mesh");status="Imported FBX bone / key / original mesh deformation verified";}
    if(stage==10){loadModel(0);editor->seek(0);editor->setLocked("spider/body",true);selected="spider/body";groundHeight=.35;Editor probe(editor->asset());probe.setLocked("spider/body",true);correction=probe.correctPositions(groundHeight,256,1e-5);correction.residualAfter=correction.residualBefore;correction.converged=false;status="Constraint demo: body locked; feet below ground target 0.35 m";}
    if(stage==11){const auto locked=editor->worldPose().at("spider/body");correction=editor->correctPositions(groundHeight,256,1e-5);require(correction.residualBefore>correction.residualAfter,"Constraint correction did not reduce residual");require(M::Length(editor->worldPose().at("spider/body").position-locked.position)<1e-12,"Correction moved locked body");status="Restricted position correction; locked body preserved";}
    if(stage==12){loadModel(4);require(imported&&imported->clips.size()==1,"Real valve animation was not imported");status="VALVE01_OpenClose_Demo: original pivot and stem channels";}
    if(stage==13){editor->seek(2);require(meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()))>.02,"Real valve source clip did not animate its mesh");playing=true;status="Original four-second valve clip playing in native viewport";}
    if(stage==14){loadScene(0);playing=true;recording=true;recorded={{0,editor->pose()}};recordingClock=0;status="Scene A: base walking exercise continues while hand control is acquired";}
    if(stage==15){acquire("right-arm","cube/right");simulate(48);checkContact("right-arm");status="Scene A: right hand acquired the cube with position and orientation";}
    if(stage==16){auto target=runtimeState.objects.at("cube").world;target.position=target.position+M::Vec3{.08,.02,.02};target.rotation=M::AxisAngle({0,1,0},.15);setObjectTarget("cube",target);simulate(24);checkContact("right-arm");status="Scene A: hand follows translated and rotated kinematic cube";}
    if(stage==17){transition("right-arm",A::LimbMode::Fk);transition("right-arm",A::LimbMode::Ik);simulate(12);status="Scene A: IK/FK/IK transfer measured without pausing playback";}
    if(stage==18){changeAuthority("cube",A::TransformAuthority::PhysicsBody);simulate(3);require(physicalGrabs.contains("right-arm"),"Physical cube constraint was not acquired");auto target=runtimeState.objects.at("cube").world;target.position=target.position+M::Vec3{.08,.04,.02};setObjectTarget("cube",target);simulate(120);checkContact("right-arm");require(!physicsSnapshot.grabs.empty()&&physicsSnapshot.grabs.front().positionErrorMeters<.02,"Physical cube did not follow the hand drive");status="Scene A: Jolt owns cube motion; compliant hand constraint follows the drive";}
    if(stage==19){applyInteraction(A::releaseGrab(editor->document(),runtimeState,"right-arm",editor->currentTime(),interactionClock,.4,editor->state().locks));simulate(120);require(physicsSnapshot.grabs.empty(),"Release left a physical constraint alive");require(!physicsSnapshot.bodies.empty()&&physicsSnapshot.bodies.front().contacts>0,"Released cube did not contact the floor");status="Scene A: released cube falls; hand blends back to advancing base";}
    if(stage==20){recording=false;require(recorded.size()>100,"Interaction recording did not collect evaluated poses");applyInteraction(A::withBakedTake(editor->interaction(),editor->document(),"Scene A recorded",recorded));const auto expected=editor->serialize();editor->save(folder/"scene-a-baked.json");require(editor->undo()&&editor->redo(),"Interaction history failed");require(editor->serialize()==expected,"Interaction Undo/Redo failed to restore the scene");editor->load(folder/"scene-a-baked.json");resetPhysics();stepInteraction(0);status="Scene A: evaluated animation baked; scene, grabs and take saved/restored";}
    if(stage==21){loadScene(0);playing=true;acquire("left-arm","cube/left");acquire("right-arm","cube/right");simulate(48);checkContact("left-arm");checkContact("right-arm");auto target=runtimeState.objects.at("cube").world;target.rotation=M::AxisAngle({0,1,0},.2);target.position.y+=.04;setObjectTarget("cube",target);simulate(24);checkContact("left-arm");checkContact("right-arm");status="Bimanual cube: both arms share one coupled torso solve";}
    if(stage==22){changeAuthority("cube",A::TransformAuthority::PhysicsBody);simulate(3);require(physicalGrabs.size()==2,"Bimanual physical constraints were not established");auto target=runtimeState.objects.at("cube").world;target.position=target.position+M::Vec3{.06,.03,0};target.rotation=M::AxisAngle({0,1,0},.3);setObjectTarget("cube",target);simulate(120);for(const auto& grip:physicsSnapshot.grabs)require(grip.positionErrorMeters<.02&&grip.angularErrorRadians<.025,"Bimanual physical cube lost contact");status="Bimanual cube: one Jolt body and two compliant contact constraints";}
    if(stage==23){loadScene(1);playing=true;acquire("right-arm","door/handle");simulate(48);auto q=M::AxisAngle({0,1,0},.5);setObjectTarget("door",{M::Vec3{-.8,1.55,.55}+M::Rotate(q,{.4,0,0}),q});simulate(24);checkContact("right-arm");changeAuthority("door",A::TransformAuthority::PhysicsBody);simulate(3);q=M::AxisAngle({0,1,0},.7);setObjectTarget("door",{M::Vec3{-.8,1.55,.55}+M::Rotate(q,{.4,0,0}),q});simulate(120);require(physicsSnapshot.bodies.front().hingeAnchorErrorMeters<.003,"Door hinge drifted in native scene");status="Scene B: hand follows a physical hinged door handle";}
    if(stage==24){loadScene(2);playing=true;acquire("left-arm","valve:gltf:91");acquire("right-arm","valve:gltf:90");simulate(48);checkContact("left-arm");checkContact("right-arm");status="Scene C: both hands acquire the real valve's original grip frames";}
    if(stage==25){auto target=runtimeState.objects.at("valve:gltf:100").world;target.rotation=M::AxisAngle({0,1,0},.6);setObjectTarget("valve:gltf:100",target);simulate(24);checkContact("left-arm");checkContact("right-arm");status="Scene C: real wheel rotated around its +Y pivot with bimanual IK";}
    if(stage==26){changeAuthority("valve:gltf:100",A::TransformAuthority::PhysicsBody);simulate(3);auto target=runtimeState.objects.at("valve:gltf:100").world;target.rotation=M::AxisAngle({0,1,0},.8);setObjectTarget("valve:gltf:100",target);simulate(120);require(physicalGrabs.size()==2,"Real valve physical grips were not established");for(const auto& grip:physicsSnapshot.grabs)require(grip.positionErrorMeters<.015&&grip.angularErrorRadians<.025,"Real valve physical contact exceeded its budget");require(physicsSnapshot.bodies.front().hingeAnchorErrorMeters<.003,"Real valve hinge drifted");status="Scene C: real valve wheel and stem driven by bimanual physical constraints";}
    if(stage==27){loadScene(3);playing=true;simulate(12);acquire("left-arm","cube/left");simulate(48);checkContact("left-arm");transition("left-arm",A::LimbMode::Fk);transition("left-arm",A::LimbMode::Ik);applyInteraction(A::releaseGrab(editor->document(),runtimeState,"left-arm",editor->currentTime(),interactionClock,.4,editor->state().locks));simulate(36);const auto base=A::Examples::EvaluateWorld(editor->document(),editor->currentTime());require(M::Length(base.at("left/wrist").position-editor->worldPose().at("left/wrist").position)<1e-5,"Walking intervention did not recover the advancing base");status="Scene D: walking, acquire, IK/FK transfer, release and base recovery";}
    if(stage==28){loadScene(4);playing=true;auto next=runtimeState;const auto runtime=runtimePose;for(const auto& [id,limb]:runtimeState.limbs){next=A::withLimbMode(editor->document(),next,id,A::LimbMode::Ik,editor->currentTime(),interactionClock,editor->state().locks);next=A::withLimbTarget(next,id,runtime.limbs.at(id).actual,interactionClock);}applyInteraction(std::move(next));simulate(60);status="Scene E: original eight-leg cycle with eight persistent IK supports";}
    if(stage==29){auto id=runtimeState.limbs.begin()->first;auto target=runtimeState.limbs.begin()->second.target;target.position.y+=.08;applyInteraction(A::withLimbTarget(runtimeState,id,target,interactionClock,.2));simulate(30);require(runtimePose.limbs.size()==8,"Eight-leg runtime lost limb identity");for(const auto& [limbId,diagnostic]:runtimePose.limbs)M::Finite(diagnostic.positionError);editor->save(folder/"scene-e-eight-leg.json");status="Scene E: support target changed while original animation continues; residuals visible";}
    if(stage==30){loadScene(0);recording=true;recorded={{0,editor->pose()}};loadModel(1);require(!editor&&!interactiveScene&&!physics&&!recording&&recorded.empty(),"Static model retained the previous interactive scene");bool rejected=false;try{loadDocument(folder/"missing-document.json");}catch(const std::exception&){rejected=true;}require(rejected&&!editor&&imported&&!interactiveScene,"Invalid document load replaced the static asset");status="Regression: static asset switch and failed document load preserve valid state";}
    if(stage==31){loadScene(0);recording=true;recorded={{0,editor->pose()}};sourceClipPlaying=true;sourceClipTime=2;loadDocument(folder/"scene-e-eight-leg.json");require(interactiveScene&&editor->interaction().sceneId=="spider"&&runtimePose.limbs.size()==8,"Loaded document lost its scene controls");require(!recording&&recorded.empty()&&!sourceClipPlaying&&sourceClipTime==0,"Load retained recording or source preview from another document");status="Regression: document load preserves controls and clears previous recording";}
    if(stage==32){loadScene(0);changeAuthority("cube",A::TransformAuthority::PhysicsBody);editor->beginEdit();draggedObject="cube";dragging=true;bool rejected=false;try{setObjectTarget("cube",runtimeState.objects.at("cube").world);}catch(const std::exception& e){rejected=true;recoverEditError(e.what());}const bool recovered=rejected&&!dragging&&draggedObject.empty()&&draggedLimb.empty()&&!editor->editing();error.clear();require(recovered,"Failed physical drag did not release input and transaction state");status="Regression: rejected physical drive releases drag and edit transaction";}
    if(stage==33){loadScene(2);sourceClipTime=2;sourceClipPlaying=true;stepInteraction(0);const auto sourceWheel=runtimeState.objects.at("valve:gltf:100").world;changeAuthority("valve:gltf:100",A::TransformAuthority::PhysicsBody);require(std::abs(physicsSnapshot.bodies.front().hingeUnwrappedAngle-123.56931104119852)<1e-4,"Valve metadata winding was lost during authority transfer");require(!physicsSnapshot.derived.empty()&&std::abs(physicsSnapshot.derived.front().world.position.y-1.548)<1e-5,"Valve full metadata stroke was not preserved");require(M::Length(runtimeState.objects.at("valve:gltf:100").world.position-sourceWheel.position)<1e-5,"Valve authority transfer moved its pivot");status="Valve source clip untouched: original wheel phase (~19.667 turns) and 118 mm stroke preserved";}
    if(stage==34){loadScene(4);ImGui::GetIO().FontGlobalScale=1.25f;status="UI check: 125% text scale with the original eight-leg scene";}
    if(stage==35) {
        loadScene(0);playing=recording=true;recorded={{0,editor->pose()}};recordingClock=0;
        acquire("left-arm","cube/left");acquire("right-arm","cube/right");simulate(48);
        auto target=runtimeState.objects.at("cube").world;target.position=target.position+M::Vec3{.06,.02,0};target.rotation=M::AxisAngle({0,1,0},.2);setObjectTarget("cube",target);simulate(24);
        for(const auto& id:{"left-arm","right-arm"}){checkContact(id);transition(id,A::LimbMode::Fk);transition(id,A::LimbMode::Ik);}simulate(12);
        status="Bimanual repeat: advancing base, both grips, moved object and four continuous mode transfers";
        writeEvidence(folder/"bimanual-held.json","native-runtime-check");
        for(const auto& id:{"left-arm","right-arm"})applyInteraction(A::releaseGrab(editor->document(),runtimeState,id,editor->currentTime(),interactionClock,.4,editor->state().locks));
        simulate(36);recording=false;
        const auto base=A::Examples::EvaluateWorld(editor->document(),editor->currentTime());
        for(const auto& [id,world]:base){const auto relative=M::Compose(M::Inverse(world),runtimePose.world.at(id));require(M::Length(relative.position)<1e-5&&2*std::acos(std::clamp(std::abs(relative.rotation.w),0.,1.))<1e-5,"Bimanual release did not recover advancing base position and orientation");}
        require(recorded.size()>100&&transitionChecks==4,"Bimanual repeat omitted recording or mode transfers");
        applyInteraction(A::withBakedTake(editor->interaction(),editor->document(),"Bimanual recorded",recorded));
        const auto expected=M::ForwardKinematics(recorded.front().locals);editor->applyTake("Bimanual recorded");resetPhysics();stepInteraction(0);
        for(const auto& [id,world]:expected){const auto relative=M::Compose(M::Inverse(world),runtimePose.world.at(id));require(M::Length(relative.position)<1e-5&&2*std::acos(std::clamp(std::abs(relative.rotation.w),0.,1.))<1e-5,"Bimanual baked take replay changed recorded position or orientation");}
        rangeStart=0;rangeEnd=recordingClock;editor->save(folder/"scene-a-bimanual-baked.json");
        status="Bimanual repeat complete: acquire, follow, IK/FK, release to base, record and baked take replay";
    }
    if(stage==36) {
        modelChoice=2;spiderJointsPerLeg=7;loadModel(2);auto next=*imported;
        AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Spider,std::size_t(spiderJointsPerLeg));adopt(std::move(next));
        require(editor->pose().size()==58&&editor->asset().chains.size()==8,"Expanded spider rig has the wrong topology");
        for(const auto& chain:editor->asset().chains)require(chain.bones.size()==7,"Spider leg joint count was ignored");
        insertKeys(true);editor->seek(1);
        const auto& chain=editor->asset().chains.front();selected=chain.bones.back();ikTarget=editor->worldPose().at(selected).position+M::Vec3{0,.03,0};
        const auto result=editor->solveIk(chain,ikTarget,256,1e-5);require(result.residual<1e-4,"Expanded spider chain did not reach its IK target");
        insertKeys(true);editor->seek(.5);
        require(meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()))>1e-4,"Expanded spider key playback did not deform the source mesh");
        editor->save(folder/"spider-seven-joints-per-leg.json");rangeEnd=1;loop=false;playing=true;
        status="Imported spider: eight legs, seven joints per leg, 58 bones, authored key playback and mesh deformation";
    }
    if(stage==37) {
        loadModel(0);playing=false;ikMode=true;
        const auto root=selected;
        const auto before=editor->worldPose().at(root).position;
        const auto target=before+M::Vec3{.07,.03,0};
        editor->beginEdit();dragBoneTo(target);editor->commitEdit();
        require(M::Length(editor->worldPose().at(root).position-target)<1e-8,"IK mode redirected the body drag to a limb");
        editor->undo();require(M::Length(editor->worldPose().at(root).position-before)<1e-8,"Body drag undo lost the original pose");
        editor->redo();require(M::Length(editor->worldPose().at(root).position-target)<1e-8,"Body drag redo lost the edited pose");
        loadScene(0);require(!ikMode,"Scene switch retained hidden standalone IK mode");
        ikMode=true;loadDocument(folder/"scene-e-eight-leg.json");require(!ikMode,"Document load retained standalone IK mode");
        ikMode=true;loadModel(0);require(!ikMode,"Asset switch retained standalone IK mode");
        status="Root drag preserves body selection and Undo/Redo; document switches clear standalone IK";
    }
    if(stage==38) {
        loadModel(2);auto next=*imported;AnimationImport::createPresetRig(next,AnimationImport::RigPreset::Automatic,7);adopt(std::move(next));
        displayed=imported->renderMesh;fit();playing=false;showBones=true;camera.yaw=0;camera.pitch=-1.3f;
        status="Anatomical spider / top: eight mesh-derived walking chains and original rest surface";
    }
    if(stage==39){camera.yaw=1.5707963f;camera.pitch=-.08f;status="Anatomical spider / side: fitted bends and foot placement";}
    if(stage==40){camera.yaw=0;camera.pitch=-.12f;status="Anatomical spider / front: four bilateral pairs, no radial template";}
    if(stage==41) {
        const auto& chain=editor->asset().chains.front();selected=chain.bones.back();
        ikTarget=editor->worldPose().at(selected).position+M::Vec3{.02,.04,0};
        const auto result=editor->solveIk(chain,ikTarget,256,1e-5);
        require(result.residual<1e-4,"Anatomically fitted spider leg did not reach its IK goal");
        camera.yaw=2.4f;camera.pitch=-.35f;status="Anatomical spider / one leg lifted with IK; other walking chains retain their pose";
    }
    if(stage==42){showBones=false;camera.yaw=3.1415927f;camera.pitch=-.15f;camera.distance=.8f;camera.target={0,.46f,.32f};status="Original facial surface / inspect eyes, mouthparts and pedipalps before semantic fitting";}
    dirty=true;if(stage<12)playing=stage==2;
}
void AnimationLabUI::writeEvidence(const std::filesystem::path& path,const std::string& adapter) const {
    if(!error.empty())throw std::runtime_error("Animation UI evidence rejected: "+error);
    nlohmann::json proof={{"schema","je-animation-playground-evidence-v1"},{"adapter",adapter},{"status",status},{"error",error},{"skin_cpu_ms",skinMilliseconds},{"ui_frame_ms",frameMilliseconds},{"vertices",displayed.mesh.mesh.positions.size()},{"triangles",displayed.mesh.mesh.indices.size()/3},{"time_seconds",editor?editor->currentTime():0},{"playing",playing},{"selected_id",selected},{"bones",editor?editor->pose().size():0},{"chains",editor?editor->asset().chains.size():0},{"residual_before",correction.residualBefore},{"residual_after",correction.residualAfter},{"converged",correction.converged}};
    if(editor){proof["document"]=nlohmann::json::parse(editor->serialize());proof["max_mesh_deformation_m"]=meshDelta(editor->asset().mesh.positions,A::skinPositions(editor->asset().mesh,editor->pose()));}
    if(imported){proof["source"]=imported->source.string();proof["imported_rig"]=imported->importedRig;proof["automatic_binding"]=imported->automaticBinding;}
    if(!evidenceModels.empty())proof["anatomy_probe"]={{"candidate_generated",bool(editor)},{"acceptance","unreviewed geometry hypothesis; not full anatomical acceptance"}};
    proof["render"]={{"materials",renderStatistics.materials},{"textures",renderStatistics.textures},{"draws",renderStatistics.draws},{"transparent_draws",renderStatistics.transparentDraws}};
    if(interactiveScene&&editor) {
        proof["interaction"]={{"scene",editor->interaction().sceneId},{"clock_seconds",interactionClock},{"recorded_frames",recorded.size()},{"baked_takes",editor->interaction().takes.size()},
            {"max_transition_position_m",maximumTransitionPosition},{"max_transition_angle_rad",maximumTransitionAngle},{"max_physics_energy_j",maximumPhysicsEnergy},
            {"transition_checks",transitionChecks},
            {"coupled_unique_joints",runtimePose.uniqueSolverJoints},{"iterations",runtimePose.iterations},{"objective_before",runtimePose.objectiveBefore},{"objective_after",runtimePose.objectiveAfter}};
        for(const auto& [id,limb]:runtimePose.limbs)proof["interaction"]["limbs"][id]={{"position_error_m",limb.positionError},{"angular_error_rad",limb.angularError},{"contact_error_m",limb.contactError},{"limit_violation_rad",limb.limitViolation},{"converged",limb.converged},{"unreachable",limb.unreachable},{"singular",limb.singular},{"hard_lock_conflict",limb.hardLockConflict}};
        proof["physics"]={{"fixed_steps",physicsSnapshot.fixedSteps},{"seconds",physicsSnapshot.simulatedSeconds},{"step_cpu_ms",physicsSnapshot.stepCpuMilliseconds},{"dropped_seconds",physicsSnapshot.droppedSeconds},{"conflicting",physicsSnapshot.conflicting}};
        for(const auto& body:physicsSnapshot.bodies)proof["physics"]["bodies"][body.id]={{"position",{body.world.position.x,body.world.position.y,body.world.position.z}},{"rotation",{body.world.rotation.x,body.world.rotation.y,body.world.rotation.z,body.world.rotation.w}},
            {"linear_velocity",{body.linearVelocity.x,body.linearVelocity.y,body.linearVelocity.z}},{"angular_velocity",{body.angularVelocity.x,body.angularVelocity.y,body.angularVelocity.z}},
            {"authority",body.authority==A::TransformAuthority::PhysicsBody?"physics":"scene"},{"kinetic_energy_j",body.kineticEnergyJoules},{"contacts",body.contacts},{"max_penetration_m",body.maximumPenetrationMeters},{"hinge_angle_rad",body.hingeAngle},{"hinge_anchor_error_m",body.hingeAnchorErrorMeters}};
        for(const auto& body:physicsSnapshot.bodies)proof["physics"]["bodies"][body.id]["hinge_unwrapped_angle_rad"]=body.hingeUnwrappedAngle;
        for(const auto& derived:physicsSnapshot.derived)proof["physics"]["derived"][derived.id]={{"position",{derived.world.position.x,derived.world.position.y,derived.world.position.z}}};
        auto frame=[](M::RigidTransform value){return nlohmann::json{{"position",{value.position.x,value.position.y,value.position.z}},{"rotation",{value.rotation.x,value.rotation.y,value.rotation.z,value.rotation.w}}};};
        for(const auto& grab:physicsSnapshot.grabs)proof["physics"]["grabs"][grab.handId]={{"object_id",grab.objectId},{"position_error_m",grab.positionErrorMeters},{"angular_error_rad",grab.angularErrorRadians},{"controller_error_m",grab.controllerPositionErrorMeters},{"conflicting",grab.conflicting},
            {"requested",frame(grab.requestedHandWorld)},{"controller",frame(grab.controllerWorld)},{"resolved_grip",frame(grab.grabWorld)},
            {"motor_enabled",runtimeState.limbs.at(grab.handId).motorTargetEnabled},{"mode",runtimeState.limbs.at(grab.handId).mode==A::LimbMode::Ik?"IK":"FK"},{"blend",runtimePose.limbs.at(grab.handId).weight}};
    }
    proof["camera"]={{"distance",camera.distance},{"target",camera.target},{"yaw",camera.yaw},{"pitch",camera.pitch}};
    std::ofstream output(path);if(!output)throw std::runtime_error("Cannot write animation evidence");output<<proof.dump(2);if(!output)throw std::runtime_error("Animation evidence write failed");
}
}
