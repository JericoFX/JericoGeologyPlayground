#include "AnimationCore.hpp"
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
}
int main() {
    try { skinAndFixture(); curvesAndHistory(); ikAndLocks(); correction(); std::cout<<"AnimationCore "<<checks<<" checks passed\n"; return 0; }
    catch(const std::exception& e) { std::cerr<<"AnimationCore: "<<e.what()<<'\n'; return 1; }
}
