#include "AnimationInteraction.hpp"
#include <cmath>
#include <iostream>
#include <numbers>
#include <stdexcept>

namespace AT = JE::AnimationTools;
namespace M = AT::Math;
namespace {
std::size_t checks{};
void check(bool ok,const char* message) { ++checks;if(!ok)throw std::runtime_error(message); }
double angle(M::Quat a,M::Quat b) {
    const auto q=M::Multiply(AT::Examples::RiggingDetail::Conjugate(a),b);
    return 2*std::atan2(std::hypot(q.x,q.y,q.z),std::abs(q.w));
}
void sameEffector(M::RigidTransform a,M::RigidTransform b,const char* message) {
    check(M::Length(a.position-b.position)<=1e-5&&angle(a.rotation,b.rotation)<=1e-5,message);
}
template<class Operation> void rejects(Operation&& operation) {
    bool rejected{};try{operation();}catch(const std::exception&){rejected=true;}check(rejected,"invalid interaction accepted");
}
AT::Document armDocument() {
    AT::Document base;
    base.bones={{"shoulder","",{}},{"elbow","shoulder",{{1,0,0},{}}},{"wrist","elbow",{{1,0,0},{}}}};
    return base;
}
AT::InteractionState armState() {
    AT::InteractionState state;
    AT::LimbControl control;
    control.chain={"arm",{"shoulder","elbow","wrist"},{}};
    control.mode=AT::LimbMode::Ik;control.pole=M::Vec3{0,1,0};
    control.target={{1,1,0},M::AxisAngle({0,0,1},.35)};
    state.limbs.emplace("arm",control);
    return state;
}
void activeTargetChangesTheEvaluatedPose() {
    const auto base=armDocument();const auto state=armState();const auto& control=state.limbs.at("arm");
    const auto result=AT::sampleInteraction(base,state,.4,.4);
    check(M::Length(result.world.at("wrist").position-control.target.position)<=1e-5,
        "active IK target was ignored: positional residual exceeds 1e-5 metres");
    const auto relative=M::Compose(M::Inverse(control.target),result.world.at("wrist"));
    check(2*std::acos(std::clamp(std::abs(M::Unit(relative.rotation).w),0.0,1.0))<=1e-5,
        "active IK target orientation was ignored");
    check(result.limbs.at("arm").converged,"reachable target diagnosed as failed");
    for(std::size_t i=0;i<base.bones.size();++i) check(M::Length(result.locals[i].local.position-base.bones[i].local.position)<1e-12,"IK stretched local translations");
}
void blendAndPolesHaveVisibleEffect() {
    const auto base=armDocument();auto state=armState();
    state=AT::withLimbBlend(state,"arm",.5,.4,0);
    const auto half=AT::sampleInteraction(base,state,.4,.4);
    check(M::Length(half.world.at("wrist").position-M::Vec3{1+std::sqrt(.5),std::sqrt(.5),0})<1e-5,"blend 0.5 is not intermediate FK/IK pose");
    check(angle(half.world.at("wrist").rotation,M::AxisAngle({0,0,1},.175))<1e-5,"blend 0.5 lost wrist orientation");
    check(!half.limbs.at("arm").unreachable&&!half.limbs.at("arm").singular&&half.limbs.at("arm").partialBlend,"feasible half-blend labelled impossible");
    check(half.limbs.at("arm").positionError>.3&&half.limbs.at("arm").solverPositionError<=1e-5,"half-blend hid actual residual or solver feasibility");
    state=armState();state.limbs.at("arm").target={{1,0,0},{}};
    const auto above=AT::sampleInteraction(base,state,0,0);
    check(above.world.at("elbow").position.y>.8,"positive pole ignored");
    state.limbs.at("arm").pole=M::Vec3{0,-1,0};
    const auto below=AT::sampleInteraction(base,state,0,0);
    check(below.world.at("elbow").position.y<-.8,"negative pole ignored");
    sameEffector(above.world.at("wrist"),below.world.at("wrist"),"pole changed achievable hand goal");
}
void locksLimitsAndImpossibleTargetsAreDiagnosed() {
    const auto base=armDocument();auto state=armState();
    auto result=AT::sampleInteraction(base,state,0,0,{"wrist"});
    check(M::Length(result.world.at("wrist").position-M::Vec3{2,0,0})<1e-12,"world lock moved hand");
    check(result.limbs.at("arm").hardLockConflict&&!result.limbs.at("arm").converged,"locked conflict hidden");
    state.limbs.at("arm").target={{4,0,0},{}};
    result=AT::sampleInteraction(base,state,0,0);
    check(result.limbs.at("arm").unreachable&&result.limbs.at("arm").positionError>1.9,"unreachable target hidden");
    check(result.limbs.at("arm").singular,"straight impossible chain singularity hidden");
    state=armState();
    for(const auto& id:{"shoulder","elbow"}) state.jointLimits.emplace(id,AT::JointLimit{id,{0,0,1},0,-.05,.05});
    result=AT::sampleInteraction(base,state,0,0);
    check(angle(result.world.at("shoulder").rotation,{})<=.050001,"shoulder limit violated");
    check(angle(result.locals.at(1).local.rotation,{})<=.050001,"elbow limit violated");
    check(!result.limbs.at("arm").converged&&result.limbs.at("arm").positionError>.5,"infeasible joint limits hidden");
    check(result.limbs.at("arm").limitViolation<1e-8,"solver published outside joint limit");
}
void playbackAcquireFollowTransferRelease() {
    auto fixture=AT::humanoidInteractionFixture();auto state=fixture.interaction;
    const auto before=AT::sampleInteraction(fixture.actor.document,state,.1,.1);
    state=AT::withLimbMode(fixture.actor.document,state,"left-arm",AT::LimbMode::Ik,.1,.1);
    const auto entered=AT::sampleInteraction(fixture.actor.document,state,.1,.1);
    sameEffector(before.world.at("left/wrist"),entered.world.at("left/wrist"),"FK to IK transfer jumped");
    state=AT::acquireGrab(fixture.actor.document,state,"left-arm","cube/left",.1,.1,.25);
    const auto acquiredStart=AT::sampleInteraction(fixture.actor.document,state,.1,.1);
    sameEffector(entered.world.at("left/wrist"),acquiredStart.world.at("left/wrist"),"acquire teleported hand");
    const auto during=AT::sampleInteraction(fixture.actor.document,state,.2,.2);
    check(angle(before.world.at("left/knee").rotation,during.world.at("left/knee").rotation)>.02,"playback stopped during acquire");
    const auto held=AT::sampleInteraction(fixture.actor.document,state,.4,.4);
    sameEffector(held.world.at("left/wrist"),{{-.25,1.55,.55},{}},"acquire did not reach object");
    state=AT::withObjectTransform(state,"cube",{{0,1.58,.58},M::AxisAngle({0,1,0},.1)});
    const auto moved=AT::sampleInteraction(fixture.actor.document,state,.45,.45);
    sameEffector(moved.world.at("left/wrist"),AT::grabPointWorld(state,"cube/left"),"hand did not follow moved and rotated object");
    state=AT::withLimbMode(fixture.actor.document,state,"left-arm",AT::LimbMode::Fk,.45,.45);
    const auto fk=AT::sampleInteraction(fixture.actor.document,state,.45,.45);
    sameEffector(moved.world.at("left/wrist"),fk.world.at("left/wrist"),"IK to FK transfer jumped");
    state=AT::withLimbMode(fixture.actor.document,state,"left-arm",AT::LimbMode::Ik,.45,.45);
    const auto ik=AT::sampleInteraction(fixture.actor.document,state,.45,.45);
    sameEffector(fk.world.at("left/wrist"),ik.world.at("left/wrist"),"FK to IK during grip jumped");
    const auto priorRelease=AT::sampleInteraction(fixture.actor.document,state,.65,.65);
    state=AT::releaseGrab(fixture.actor.document,state,"left-arm",.65,.65,.3);
    const auto released=AT::sampleInteraction(fixture.actor.document,state,.65,.65);
    sameEffector(priorRelease.world.at("left/wrist"),released.world.at("left/wrist"),"release jumped");
    const auto recovered=AT::sampleInteraction(fixture.actor.document,state,.95,.95);
    const auto base=AT::Examples::EvaluateWorld(fixture.actor.document,.95);
    for(const auto& [id,t]:base) sameEffector(t,recovered.world.at(id),"release did not recover advancing base animation");
    check(state.grabPoints.at("cube/left").status==AT::GrabStatus::Released&&state.grabPoints.at("cube/left").occupant.empty(),"release occupancy not cleared");
    state=fixture.interaction;
    const auto local=AT::Examples::EvaluateLocals(fixture.actor.document,.3).at(3).local;
    auto edited=local;edited.rotation=M::AxisAngle({0,0,1},.3);
    state=AT::withPoseOverride(fixture.actor.document,state,"left/elbow",edited,.3,.3);
    const auto editing=AT::sampleInteraction(fixture.actor.document,state,.3,.3);
    check(angle(editing.world.at("left/elbow").rotation,base.at("left/elbow").rotation)>.1,"procedural pose edit ignored");
    const auto continued=AT::sampleInteraction(fixture.actor.document,state,.5,.5);
    check(angle(editing.world.at("left/knee").rotation,continued.world.at("left/knee").rotation)>.02,"pose edit stopped walking base");
}
void coupledBimanualUsesOneSharedBodySolve() {
    AT::Document base;
    base.bones={{"pelvis","",{}},{"torso","pelvis",{{0,1,0},{}}},
        {"left/shoulder","torso",{{-.4,.3,0},{}}},{"left/elbow","left/shoulder",{{-.5,0,0},{}}},{"left/wrist","left/elbow",{{-.5,0,0},{}}},
        {"right/shoulder","torso",{{.4,.3,0},{}}},{"right/elbow","right/shoulder",{{.5,0,0},{}}},{"right/wrist","right/elbow",{{.5,0,0},{}}}};
    AT::InteractionState state;
    for(const auto& side:std::array<std::string,2>{"left","right"}) {
        AT::LimbControl control;control.chain={side,{side+"/shoulder",side+"/elbow",side+"/wrist"},{}};
        control.sharedJoints={"torso"};control.mode=AT::LimbMode::Ik;control.target.rotation=M::AxisAngle({0,1,0},.2);
        const double sign=side=="left"?-1:1;
        control.target.position={sign*1.4*std::cos(.2),1.3,-sign*1.4*std::sin(.2)};
        state.limbs.emplace(side,control);
        for(const auto& bone:control.chain.bones) state.jointLimits.emplace(bone,AT::JointLimit{bone,{1,0,0},0,0,0});
    }
    const auto result=AT::sampleInteraction(base,state,0,0,{"pelvis"});
    check(result.uniqueSolverJoints==1,"shared torso duplicated or fixed joints kept as free DOFs");
    check(angle(result.world.at("torso").rotation,M::AxisAngle({0,1,0},.2))<1e-5,"bimanual shared torso did not solve");
    for(const auto& [id,limb]:state.limbs) {sameEffector(result.world.at(limb.chain.bones.back()),limb.target,"bimanual contact failed");check(result.limbs.at(id).converged,"feasible bimanual goal rejected");}
    state.limbs.at("left").target.position={-8,1.3,0};
    const auto impossible=AT::sampleInteraction(base,state,0,0,{"pelvis"});
    check(!impossible.limbs.at("left").converged&&impossible.limbs.at("left").positionError>5,"bimanual conflict hidden");
}
void bimanualGrabAndBakeReplaySkinnedEndpoints() {
    auto fixture=AT::humanoidInteractionFixture();auto state=fixture.interaction;
    for(const auto& side:std::array<std::string,2>{"left","right"})
        state=AT::acquireGrab(fixture.actor.document,state,side+"-arm","cube/"+side,0,0,.25);
    std::vector<AT::RecordedPose> frames;
    double maxContact{},maxAngle{};
    for(int frame=0;frame<=30;++frame) {
        const double time=frame/60.;
        state=AT::withObjectTransform(state,"cube",{{.02*std::sin(time),1.55,.55+.03*std::sin(time)},M::AxisAngle({0,1,0},.1*std::sin(time))});
        const auto pose=AT::sampleInteraction(fixture.actor.document,state,time+.3,time+.3);
        for(const auto& side:std::array<std::string,2>{"left","right"}) {
            const auto target=AT::grabPointWorld(state,"cube/"+side);
            maxContact=std::max(maxContact,M::Length(pose.world.at(side+"/wrist").position-target.position));
            maxAngle=std::max(maxAngle,angle(pose.world.at(side+"/wrist").rotation,target.rotation));
        }
        frames.push_back({time,pose.locals});
    }
    check(maxContact<=1e-5&&maxAngle<=1e-5,"moving bimanual object lost contact");
    state=AT::withBakedTake(state,fixture.actor.document,"recorded-grip",frames);
    const auto& take=state.takes.at("recorded-grip");
    check(take.document.tracks.size()==fixture.actor.document.bones.size(),"bake omitted skeleton channels");
    double maximumMeshError{};
    for(const auto& frame:frames) {
        const auto replay=AT::Examples::EvaluateLocals(take.document,frame.time);
        const auto actual=AT::skinPositions(fixture.actor.mesh,replay),expected=AT::skinPositions(fixture.actor.mesh,frame.locals);
        for(std::size_t i=0;i<actual.size();++i) maximumMeshError=std::max(maximumMeshError,M::Length(actual[i]-expected[i]));
    }
    check(maximumMeshError<1e-8,"bake replay changed measured mesh sample endpoints");
    std::cout<<"Bimanual max contact "<<maxContact<<" m, angle "<<maxAngle<<" rad; bake mesh error "<<maximumMeshError<<" m\n";
}
void arbitraryEightLegIdsAndCommandFailures() {
    const auto asset=AT::ownSpiderFixture();auto state=AT::interactionForAsset(asset);
    const auto before=AT::Examples::EvaluateWorld(asset.document,.3);
    std::vector<std::string> ids;for(const auto& [id,_]:state.limbs)ids.push_back(id);
    for(const auto& id:ids) {
        state=AT::withLimbMode(asset.document,state,id,AT::LimbMode::Ik,.3,.3,{"spider/body"});
        auto target=before.at(state.limbs.at(id).chain.bones.back());target.position.y+=.04;
        state=AT::withLimbTarget(state,id,target,.3);
        state.limbs.at(id).sharedJoints={"spider/body"};
    }
    const auto result=AT::sampleInteraction(asset.document,state,.3,.3,{"spider/body"});
    check(result.limbs.size()==8,"eight legs truncated to humanoid topology");
    for(const auto& [_,d]:result.limbs)check(d.positionError<=1e-5&&d.angularError<=1e-5,"eight-leg coupled target failed");
    sameEffector(result.world.at("spider/body"),before.at("spider/body"),"eight-leg solver changed locked shared body");
    auto reordered=asset.document;std::reverse(reordered.bones.begin(),reordered.bones.end());
    const auto re=AT::sampleInteraction(reordered,state,.3,.3,{"spider/body"});
    for(const auto& [id,world]:result.world)sameEffector(world,re.world.at(id),"interaction depends on bone storage order");
    auto fixture=AT::humanoidInteractionFixture();
    rejects([&]{(void)AT::acquireGrab(fixture.actor.document,fixture.interaction,"right-arm","cube/left",0,0);});
    check(fixture.interaction.grabPoints.at("cube/left").status==AT::GrabStatus::Free,"failed acquire mutated original state");
    fixture.interaction.objects.at("cube").authority=AT::TransformAuthority::PhysicsBody;
    rejects([&]{(void)AT::withObjectTransform(fixture.interaction,"cube",{{9,9,9},{}});});
    const auto physics=AT::withPhysicsSnapshot(fixture.interaction,"cube",{{0,1.6,.55},{}});
    check(physics.objects.at("cube").world.position.y==1.6&&fixture.interaction.objects.at("cube").world.position.y==1.55,"physics snapshot mutated source or failed publication");
    const std::array<AT::RecordedPose,2> duplicate{{{0,fixture.actor.document.bones},{0,fixture.actor.document.bones}}};
    rejects([&]{(void)AT::withBakedTake(fixture.interaction,fixture.actor.document,"invalid",duplicate);});
    check(fixture.interaction.takes.empty(),"failed bake published partial take");
}
void approachUsesLiveObjectsAndAnIndependentClock() {
    const auto fixture=AT::humanoidInteractionFixture();auto state=fixture.interaction;
    const auto first=AT::sampleInteraction(fixture.actor.document,state,.2,5);
    state=AT::acquireGrab(fixture.actor.document,state,"left-arm","cube/left",.2,5,.4);
    state=AT::withObjectTransform(state,"cube",{{.03,1.55,.57},M::AxisAngle({0,1,0},.2)});
    const auto half=AT::sampleInteraction(fixture.actor.document,state,.2,5.2);
    const auto goal=AT::grabPointWorld(state,"cube/left");
    check(M::Length(half.world.at("left/wrist").position-(first.world.at("left/wrist").position+goal.position)*.5)<1e-5,"approach used stale GrabPoint or clip clock");
    const auto held=AT::sampleInteraction(fixture.actor.document,state,.05,5.5);
    sameEffector(held.world.at("left/wrist"),goal,"looped clip reset independent acquire clock");
    auto limited=state;limited.grabPoints.at("cube/left").limits.translationMax={.04,0,0};
    auto outside=goal;outside.position.x+=2;
    limited=AT::withLimbTarget(limited,"left-arm",outside,5.5);
    check(M::Length(limited.limbs.at("left-arm").grabOffset.position)<=.040001,"GrabPoint motion limit ignored");
}
void fixedNonzeroJointLimitsAreEnforced() {
    const auto base=armDocument();auto state=armState();
    state.limbs.at("arm").pole.reset();
    state.limbs.at("arm").target={{2*std::cos(.2),2*std::sin(.2),0},M::AxisAngle({0,0,1},.2)};
    state.jointLimits.emplace("shoulder",AT::JointLimit{"shoulder",{0,0,1},0,.2,.2});
    const auto pose=AT::sampleInteraction(base,state,0,0);
    check(angle(pose.world.at("shoulder").rotation,M::AxisAngle({0,0,1},.2))<1e-8,"nonzero fixed joint limit not enforced");
    sameEffector(pose.world.at("wrist"),state.limbs.at("arm").target,"feasible pinned joint target failed");
}
void physicsMotorRequestsDoNotTeleportHandsOrObjects() {
    const auto fixture=AT::humanoidInteractionFixture();auto state=fixture.interaction;
    state.objects.at("cube").authority=AT::TransformAuthority::PhysicsBody;
    state=AT::acquireGrab(fixture.actor.document,state,"left-arm","cube/left",0,0,.3);
    sameEffector(state.limbs.at("left-arm").target,{{-.25,1.55,.55},{}},"acquire persisted old hand-start as physics motor target");
    check(!state.limbs.at("left-arm").motorTargetEnabled,"acquire unexpectedly enabled a motor drive request");
    const auto before=AT::sampleInteraction(fixture.actor.document,state,.4,.4);
    const auto objectBefore=state.objects.at("cube").world;
    const M::RigidTransform request{{-.2,1.65,.65},M::AxisAngle({0,1,0},.1)};
    state=AT::withLimbTarget(state,"left-arm",request,.4);
    check(state.limbs.at("left-arm").motorTargetEnabled,"physical controller request lost");
    sameEffector(state.limbs.at("left-arm").target,request,"physical world controller request clamped to GrabPoint offset");
    sameEffector(state.objects.at("cube").world,objectBefore,"controller request wrote a physics-owned object transform");
    const auto after=AT::sampleInteraction(fixture.actor.document,state,.4,.4);
    sameEffector(before.world.at("left/wrist"),after.world.at("left/wrist"),"physics request teleported hand instead of following solved body");
    state=AT::releaseGrab(fixture.actor.document,state,"left-arm",.4,.4);
    check(!state.limbs.at("left-arm").motorTargetEnabled,"release retained active physics motor request");
}
}
int main() {
    try {activeTargetChangesTheEvaluatedPose();blendAndPolesHaveVisibleEffect();locksLimitsAndImpossibleTargetsAreDiagnosed();playbackAcquireFollowTransferRelease();coupledBimanualUsesOneSharedBodySolve();bimanualGrabAndBakeReplaySkinnedEndpoints();arbitraryEightLegIdsAndCommandFailures();approachUsesLiveObjectsAndAnIndependentClock();fixedNonzeroJointLimitsAreEnforced();physicsMotorRequestsDoNotTeleportHandsOrObjects();std::cout<<"AnimationInteraction "<<checks<<" checks passed\n";return 0;}
    catch(const std::exception& error) {std::cerr<<"AnimationInteraction: "<<error.what()<<'\n';return 1;}
}
