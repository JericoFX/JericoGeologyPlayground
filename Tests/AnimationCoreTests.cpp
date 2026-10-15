#include "AnimationCore.hpp"
#include "AnimationInteraction.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace AT = JE::AnimationTools;
namespace M = AT::Math;
namespace {
int checks{};
void check(bool value, const char* message) { ++checks; if (!value) throw std::runtime_error(message); }
bool near(M::Vec3 a, M::Vec3 b, double tolerance = 1e-8) { return M::Length(a-b) <= tolerance; }
template<class F> void rejects(F&& f) { bool rejected{}; try { f(); } catch (const std::exception&) { rejected=true; } check(rejected,"invalid operation accepted"); }
AT::AnimationAsset chain() {
    AT::AnimationAsset a;
    a.document.bones = {{"root","",{}},{"joint","root",{{1,0,0},{}}},{"tip","joint",{{1,0,0},{}}}};
    a.chains = {{"arm",{"root","joint","tip"},{}}};
    return a;
}
void skinAndFixture() {
    auto asset=AT::ownSpiderFixture();
    check(asset.chains.size()==8,"eight leg fixture");
    check(asset.document.bones.size()==25,"shared body spider");
    check(!asset.mesh.triangles.empty(),"own deformable triangles");
    const auto rest=AT::skinPositions(asset.mesh,asset.document.bones);
    for(std::size_t i=0;i<rest.size();++i) check(near(rest[i],asset.mesh.positions[i]),"rest LBS moved vertex");
    auto reordered=asset;
    std::reverse(reordered.document.bones.begin(),reordered.document.bones.end());
    AT::Editor a(asset),b(reordered);
    a.seek(.65); b.seek(.65);
    for(const auto& [id,world]:a.worldPose()) check(near(world.position,b.worldPose().at(id).position),"pose depends on storage order");
    const auto animated=AT::skinPositions(asset.mesh,a.pose());
    bool changed{}; for(std::size_t i=0;i<animated.size();++i) changed=changed||!near(animated[i],rest[i]);
    check(changed,"animation did not deform own mesh");
    std::ifstream fixture("External/AnimationNative/playground/fixtures/own-spider-cpp.json");
    check(static_cast<bool>(fixture),"donor spider fixture missing");
    const auto reference=nlohmann::json::parse(fixture);
    check(reference.at("schema")=="own-evaluated-v1","original fixture schema");
    for(const auto& sample:reference.at("samples")) {
        a.seek(sample.at("time_seconds").get<double>());
        const auto world=a.worldPose();
        for(const auto& [id,p]:sample.at("positions").items())
            check(near(world.at(id).position,{p.at(0).get<double>(),p.at(1).get<double>(),p.at(2).get<double>()}),"original spider sample disagrees");
    }
}
void curvesAndHistory() {
    AT::Editor editor(chain());
    const auto rest=editor.document().bones.front().local;
    editor.beginEdit();
    editor.setLocal("root",{{0,0,0},{}}); editor.setKey("root",{1,0,0});
    editor.seek(2); editor.setLocal("root",{{2,0,0},M::AxisAngle({0,1,0},std::numbers::pi)}); editor.setKey("root",{1,0,0});
    editor.setLocked("tip",true);
    check(editor.commitEdit(),"transaction missing");
    check(editor.undoCount()==1,"one shared history");
    editor.seek(1);
    check(near(editor.worldPose().at("root").position,{1,0,0}),"Hermite seconds interpolation");
    check(near(M::Rotate(editor.pose().front().local.rotation,{1,0,0}),{0,0,-1}),"Slerp midpoint");
    check(near(editor.document().bones.front().local.position,rest.position),"manual pose changed rest");
    check(editor.undo(),"undo"); check(editor.document().tracks.empty()&&!editor.locked("tip"),"undo keys and locks");
    check(editor.redo(),"redo"); check(editor.locked("tip"),"redo lock");
    auto before=editor.serialize(); auto count=editor.undoCount();
    editor.beginEdit(); editor.moveWorld("root",{9,9,9}); editor.cancelEdit();
    check(editor.serialize()==before&&editor.undoCount()==count,"cancel mutated state/history");
    editor.beginEdit(); check(!editor.commitEdit(),"no-op transaction"); check(editor.undoCount()==count,"no-op history");
    rejects([&]{editor.loadText("{\"schema\":\"broken\"}");});
    check(editor.serialize()==before&&editor.undoCount()==count,"failed load mutated state");
    AT::Editor copy(chain()); copy.loadText(before); check(copy.serialize()==before,"serialization roundtrip");
    check(copy.undo(),"load transaction undo"); check(copy.document().tracks.empty(),"load undo original state");
    const auto path=std::filesystem::temp_directory_path()/("je-animation-core-roundtrip-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".json");
    editor.save(path);editor.save(path);copy.load(path);
    check(copy.serialize()==before,"atomic file replacement/roundtrip");
    { std::ofstream invalid(path,std::ios::trunc);invalid<<"{invalid"; }
    rejects([&]{copy.load(path);});check(copy.serialize()==before,"failed file load changed state");
    std::filesystem::remove(path);
    rejects([&]{editor.seek(std::numeric_limits<double>::quiet_NaN());});
    editor.seek(.5); editor.advance(4,0,2,true); check(std::abs(editor.currentTime()-.5)<1e-10,"time-based loop");
}
void ikAndLocks() {
    AT::Editor editor(chain());
    auto r=editor.solveIk(editor.asset().chains.front(),{1,1,0});
    check(r.converged&&r.residual<1e-5,"reachable CCD");
    editor.seek(0); r=editor.solveIk(editor.asset().chains.front(),{4,0,0});
    check(!r.converged&&std::abs(r.residual-2)<1e-8,"unreachable CCD diagnosed");
    editor.setLocked("joint",true); const auto locked=editor.worldPose().at("joint");
    r=editor.solveIk(editor.asset().chains.front(),{1,1,0});
    check(!r.converged,"locked unreachable CCD");
    check(near(editor.worldPose().at("joint").position,locked.position),"IK moved locked joint");
    check(near(M::Rotate(editor.worldPose().at("joint").rotation,{1,0,0}),M::Rotate(locked.rotation,{1,0,0})),"IK rotated locked joint");
    editor.moveWorld("root",{0,2,0});
    check(near(editor.worldPose().at("joint").position,locked.position),"FK moved locked descendant");
    const auto before=editor.serialize(); editor.setLocal("joint",{{7,8,9},{}});
    check(editor.serialize()==before,"direct edit changed locked bone");
    for(const auto& c:AT::ownSpiderFixture().chains) {
        AT::Editor spider;
        const auto world=spider.worldPose();
        const auto target=world.at(c.bones.back()).position+M::Vec3{0,.08,0};
        const auto result=spider.solveIk(c,target);
        check(result.residual<.002,"spider leg CCD by ID");
        check(near(spider.worldPose().at("spider/body").position,world.at("spider/body").position),"leg solver moved shared body");
    }
}
void correction() {
    AT::Editor editor(chain()); editor.setLocked("root",true);
    editor.setLocal("joint",{{1.7,-.4,0},{}});
    const auto anchor=editor.worldPose().at("root");
    const auto r=editor.correctPositions(0,128,1e-5);
    check(r.residualBefore>.3&&r.residualAfter<r.residualBefore,"coupled SPD did not reduce residual");
    check(near(editor.worldPose().at("root").position,anchor.position),"correction moved hard lock");
    AT::Editor blocked(chain()); blocked.moveWorld("root",{0,-1,0});
    std::vector<std::string> lockedIds;
    for(const auto& bone:blocked.pose()) lockedIds.push_back(bone.id);
    for(const auto& id:lockedIds) blocked.setLocked(id,true);
    for(const auto& id:lockedIds) check(blocked.locked(id),"correction fixture failed to lock every bone");
    const auto before=blocked.serialize(); const auto conflict=blocked.correctPositions();
    check(!conflict.converged&&conflict.residualAfter==conflict.residualBefore,"locked conflict hidden");
    check(blocked.serialize()==before,"locked correction mutation");
}
void interactionHistoryAndPersistence() {
    const auto fixture=AT::humanoidInteractionFixture();AT::Editor editor(fixture.actor);
    editor.setInteraction(fixture.interaction);editor.seek(.1);(void)editor.evaluateRuntime(.1);
    const auto initialHistory=editor.undoCount();
    editor.beginEdit();
    editor.setInteraction(AT::acquireGrab(editor.document(),editor.interaction(),"left-arm","cube/left",.1,.1,.25));
    editor.setKey("left/wrist");editor.setLocked("left/foot",true);
    check(editor.commitEdit(),"interactive transaction not committed");
    check(editor.undoCount()==initialHistory+1,"interaction created a second history");
    check(editor.undo(),"interaction undo");
    check(editor.interaction().grabPoints.at("cube/left").status==AT::GrabStatus::Free&&!editor.locked("left/foot"),"undo failed to restore grip and locks");
    check(editor.redo(),"interaction redo");
    const auto history=editor.undoCount();const auto revision=editor.revision();
    auto authored=editor.interaction();authored.objects.at("cube").authority=AT::TransformAuthority::PhysicsBody;editor.setInteraction(authored);
    auto transient=AT::withPhysicsSnapshot(editor.interaction(),"cube",{{0,1.6,.6},{}});
    const auto historyAfterAuthority=editor.undoCount();const auto revisionAfterAuthority=editor.revision();
    (void)editor.evaluateRuntime(.5,&transient);(void)editor.evaluateRuntime(.6);
    check(editor.undoCount()==historyAfterAuthority&&editor.revision()==revisionAfterAuthority,"runtime physics snapshot entered authored history");
    check(editor.interaction().objects.at("cube").world.position.y==1.55&&editor.runtimeInteraction().objects.at("cube").world.position.y==1.6,"runtime object snapshot changed authored object");
    check(historyAfterAuthority==history+1&&revisionAfterAuthority==revision+1,"authority edit history missing");
    editor.beginEdit();editor.seek(.3);(void)editor.evaluateRuntime(.7);check(!editor.commitEdit(),"playback-only transaction created history");
    const auto beforeCancel=editor.undoCount();editor.beginEdit();
    const auto heldBeforeRelease=editor.worldPose().at("left/wrist");
    editor.setInteraction(AT::releaseGrab(editor.document(),editor.runtimeInteraction(),"left-arm",.3,.7));
    check(near(editor.worldPose().at("left/wrist").position,heldBeforeRelease.position),"physics release captured stale authored object pose");
    check(editor.interaction().objects.at("cube").world.position.y==1.55,"release persisted a continuous physics snapshot");
    editor.seek(.4);(void)editor.evaluateRuntime(.8);editor.cancelEdit();
    check(editor.undoCount()==beforeCancel&&editor.interaction().grabPoints.at("cube/left").status==AT::GrabStatus::Occupied,"cancel did not restore grip");
    check(editor.currentTime()==.4&&editor.interactionTime()==.8,"cancel rewound active playback clocks");
    const M::RigidTransform motorRequest{{-.2,1.65,.65},M::AxisAngle({0,1,0},.1)};
    editor.setInteraction(AT::withLimbTarget(editor.runtimeInteraction(),"left-arm",motorRequest,.8));
    check(editor.undoCount()==beforeCancel+1&&editor.interaction().limbs.at("left-arm").motorTargetEnabled,"motor request did not enter shared history");
    check(editor.undo()&&!editor.interaction().limbs.at("left-arm").motorTargetEnabled,"motor request undo lost authored controls");
    check(editor.redo()&&editor.interaction().limbs.at("left-arm").motorTargetEnabled,"motor request redo lost drive flag");
    const auto encoded=editor.serialize();AT::Editor restored(fixture.actor);restored.loadText(encoded);
    check(restored.serialize()==encoded,"interactive document roundtrip");
    check(restored.interaction().sceneId=="cube"&&restored.interaction().objects.at("cube").authority==AT::TransformAuthority::PhysicsBody,"scene authority not persisted");
    check(restored.interaction().objects.at("cube").world.position.y==1.55,"transient physics object saved as authored pose");
    const auto& restoredMotor=restored.interaction().limbs.at("left-arm");
    check(restoredMotor.motorTargetEnabled&&near(restoredMotor.target.position,motorRequest.position)&&near(M::Rotate(restoredMotor.target.rotation,{1,0,0}),M::Rotate(motorRequest.rotation,{1,0,0})),"physical motor request not persisted");
    auto legacy=nlohmann::json::parse(encoded);for(auto& limb:legacy["interaction"]["limbs"])limb.erase("motor_target_enabled");
    AT::Editor legacyRestored(fixture.actor);legacyRestored.loadText(legacy.dump());
    check(!legacyRestored.interaction().limbs.at("left-arm").motorTargetEnabled,"legacy interaction enabled a motor request");
    auto invalid=nlohmann::json::parse(encoded);invalid["interaction"]["grab_points"][0]["object_id"]="missing-object";
    const auto before=restored.serialize();const auto count=restored.undoCount();
    rejects([&]{restored.loadText(invalid.dump());});check(restored.serialize()==before&&restored.undoCount()==count,"invalid interaction load published partial scene");
    std::vector<AT::RecordedPose> frames;
    for(const double time:{0.,.2})frames.push_back({time,AT::Examples::EvaluateLocals(fixture.actor.document,time)});
    restored.setInteraction(AT::withBakedTake(restored.interaction(),restored.document(),"walk-bake",frames));
    const auto priorTracks=restored.document().tracks.size();restored.applyTake("walk-bake");
    check(restored.document().tracks.size()==fixture.actor.document.bones.size(),"apply take did not select recorded tracks");
    for(const auto& [_,control]:restored.interaction().limbs)check(control.mode==AT::LimbMode::Fk&&control.grabPointId.empty(),"apply take double-applied procedural grip");
    const auto result=restored.evaluateRuntime(1);
    const auto reference=AT::Examples::EvaluateWorld(restored.interaction().takes.at("walk-bake").document,0);
    for(const auto& [id,t]:reference)check(near(result.world.at(id).position,t.position),"selected take replay changed recorded pose");
    check(restored.undo()&&restored.document().tracks.size()==priorTracks,"take selection undo failed");
    const auto saved=restored.serialize();AT::Editor again(fixture.actor);again.loadText(saved);
    check(again.interaction().takes.contains("walk-bake")&&again.serialize()==saved,"baked take not persisted");
}
void preparedPaletteMatchesOriginalReference() {
    const auto asset=AT::ownSpiderFixture();
    double maxError{};
    for(int sample=0;sample<=20;++sample) {
        const auto pose=AT::Examples::EvaluateLocals(asset.document,sample/10.);
        const auto palette=AT::prepareSkinPalette(asset.mesh.binding,pose);
        const auto actual=AT::skinPositions(asset.mesh,palette);
        const auto reference=AT::Examples::SkinMeshPositions(asset.mesh.positions,asset.mesh.influences,asset.mesh.binding,pose);
        for(std::size_t i=0;i<actual.size();++i)maxError=std::max(maxError,M::Length(actual[i]-reference[i]));
    }
    auto weighted=chain();
    weighted.mesh.positions={{.5,.2,.1},{1.3,-.4,.2},{1.5,.6,-.2}};weighted.mesh.triangles={{{0,1,2}}};
    weighted.mesh.binding=AT::Examples::BuildSkinBinding(weighted.document.bones,{"tip","root","joint"});
    weighted.mesh.influences={{{0,.5},{1,.2},{2,.3}},{{0,.75},{2,.25}},{{0,.5},{1,.5}}};
    for(int sample=0;sample<=12;++sample) {
        auto pose=weighted.document.bones;const double time=sample/12.;
        pose[0].local={{.2*time,.1,-.3},M::AxisAngle({0,1,0},.4*time)};
        pose[1].local.rotation=M::AxisAngle({0,0,1},-1.2*time);pose[2].local.rotation=M::AxisAngle({1,0,0},.6*time);
        const auto palette=AT::prepareSkinPalette(weighted.mesh.binding,pose);
        const auto actual=AT::skinPositions(weighted.mesh,palette);
        const auto reference=AT::Examples::SkinMeshPositions(weighted.mesh.positions,weighted.mesh.influences,weighted.mesh.binding,pose);
        const auto world=M::ForwardKinematics(pose);
        for(std::size_t i=0;i<actual.size();++i)maxError=std::max(maxError,M::Length(actual[i]-reference[i]));
        for(std::size_t joint=0;joint<palette.size();++joint) {
            const auto transform=M::Compose(world.at(weighted.mesh.binding.joint_ids[joint]),weighted.mesh.binding.inverse_bind[joint]);
            check(near(AT::applySkinVector(palette[joint],{.2,.7,-.3}),M::Rotate(transform.rotation,{.2,.7,-.3}),1e-12),"palette vector rotation differs from original quaternion math");
        }
    }
    check(maxError<1e-10,"prepared matrix palette differs from original rigid LBS");
    auto mesh=asset.mesh;mesh.influences.front().push_back(mesh.influences.front().front());
    rejects([&]{(void)AT::skinPositions(mesh,asset.document.bones);});
    mesh=asset.mesh;mesh.influences.front().front().weight=.9;
    rejects([&]{(void)AT::skinPositions(mesh,asset.document.bones);});
    auto invalid=AT::prepareSkinPalette(asset.mesh.binding,asset.document.bones);invalid.front().rotation[0]=2;
    rejects([&]{(void)AT::skinPositions(asset.mesh,invalid);});
    std::cout<<"Prepared palette max original-reference error "<<maxError<<" m\n";
}
}
int main() {
    try { skinAndFixture(); curvesAndHistory(); ikAndLocks(); correction(); interactionHistoryAndPersistence(); preparedPaletteMatchesOriginalReference(); std::cout<<"AnimationCore "<<checks<<" checks passed\n"; return 0; }
    catch(const std::exception& e) { std::cerr<<"AnimationCore: "<<e.what()<<'\n'; return 1; }
}
