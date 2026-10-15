// Analytical behavioral checks for the original portable authoring examples.
#include "authoring_example.hpp"
#include "evaluation_example.hpp"
#include "physics_example.hpp"
#include "rigging_checks.hpp"
#include "skinning_checks.hpp"
#include "rotation_example.hpp"
#include <iostream>
#include <limits>

namespace Ex = JE::AnimationTools::Examples;
namespace M = JE::AnimationTools::Playground;
std::size_t checks = 0;
void Check(bool value, const char* description) {
    if (!value) throw std::runtime_error(description);
    ++checks;
}
bool Near(M::Vec3 a, M::Vec3 b) { return M::Length(a-b) < 1e-9; }
template<class Action> void Reject(Action action) {
    try { action(); } catch (const std::invalid_argument&) { ++checks; return; }
    throw std::runtime_error("invalid input was accepted");
}
int main() {
    try {
        Ex::AuthoringDocument doc(Ex::Document{});
        Check(!doc.Undo() && !doc.Redo(), "empty history");
        doc.AddBone({"root", "", {{1,0,0},{}}});
        doc.AddBone({"limb", "root", {{0,2,0},{}}});
        doc.AddBone({"other", "", {{5,0,0},{}}});
        auto world = M::ForwardKinematics(doc.Get().bones);
        Check(Near(world.at("limb").position, {1,2,0}), "initial FK");
        doc.Reparent("limb", "other", true);
        Check(Near(M::ForwardKinematics(doc.Get().bones).at("limb").position, {1,2,0}), "reparent preserves world");
        Check(doc.Undo(), "undo exists");
        Check(doc.Get().bones.at(1).parent == "root", "undo restores parent");
        Check(doc.Redo(), "redo exists");
        Check(doc.Get().bones.at(1).parent == "other", "redo restores parent");
        Reject([&]{doc.Reparent("other", "limb", true);});
        Check(doc.Get().bones.at(2).parent.empty(), "failed edit is atomic");
        Reject([&]{doc.AddBone({"limb", "", {}});});
        Reject([&]{doc.RemoveBone("other");});
        Reject([&]{doc.Reparent("limb", "unknown", false);});
        doc.SetKey("limb", {0, {{0,0,0},{}}, {1,0,0}});
        doc.SetKey("limb", {2, {{2,0,0},M::AxisAngle({0,1,0},3.141592653589793)}, {1,0,0}});
        auto mid = Ex::EvaluateLocals(doc.Get(), 1);
        Check(Near(mid.at(1).local.position, {1,0,0}), "Hermite time and derivative units");
        Check(Near(Ex::EvaluateLocals(doc.Get(), 0.5).at(1).local.position, {0.5,0,0}), "off-midpoint velocity scales with seconds");
        Check(Near(Ex::EvaluateLocals(doc.Get(), -1).at(1).local.position, {}), "preroll clamps first key");
        Check(Near(Ex::EvaluateLocals(doc.Get(), 3).at(1).local.position, {2,0,0}), "postroll clamps last key");
        Check(Near(M::Rotate(mid.at(1).local.rotation,{1,0,0}), {0,0,-1}), "shortest quaternion interpolation");
        auto clip = Ex::Bake(doc.Get(), 0, 2, 0.75);
        Check(clip.size() == 4 && clip.back().time == 2, "bake includes non-grid endpoint");
        Check(Near(clip.back().world.at("limb").position, {7,0,0}), "bake evaluates world hierarchy");
        Reject([&]{Ex::Bake(doc.Get(), 0, 2, 0);});
        Reject([&]{Ex::EvaluateLocals(doc.Get(),std::numeric_limits<double>::infinity());});
        Check(Ex::Bake(doc.Get(), -1, -1, 1).size() == 1, "point bake");
        Reject([&]{Ex::Bake(doc.Get(), 1e20, 2e20, 1);});
        Reject([&]{Ex::Bake(doc.Get(), 0, 1, 1e-7);});
        auto reordered = doc.Get();
        std::reverse(reordered.bones.begin(), reordered.bones.end());
        Check(Near(Ex::EvaluateWorld(reordered, 1).at("limb").position, {6,0,0}), "evaluation ignores bone order");
        doc.RemoveKey("limb", 2);
        Check(doc.Undo(), "undo key deletion");
        Check(doc.Undo(), "undo key creation");
        doc.SetKey("limb", {1, {{1,0,0},{}}, {1,0,0}});
        Check(!doc.Redo(), "edit invalidates redo branch");
        Reject([&]{doc.SetKey("unknown", {0,{},{}});});
        Reject([&]{doc.SetKey("limb", {std::numeric_limits<double>::quiet_NaN(),{},{}});});
        Reject([&]{doc.Reparent("limb", "root", true);});
        Ex::AuthoringDocument single(Ex::Document{});
        single.AddBone({"leaf", "", {{},{0,0,0,2}}});
        Check(single.Get().bones.front().local.rotation.w == 1, "stored rest quaternion normalized");
        single.SetKey("leaf", {-1, {{1,0,0},{0,0,0,2}}, {}});
        single.SetKey("leaf", {0, {{2,0,0},{}}, {}});
        single.SetKey("leaf", {0, {{3,0,0},{}}, {}});
        Check(single.Get().tracks.front().keys.size() == 2, "key replacement retains unique times");
        Check(single.Get().tracks.front().keys.front().time == -1, "negative preroll preserved");
        single.RemoveKey("leaf", 0);
        Check(Near(Ex::EvaluateWorld(single.Get(), 100).at("leaf").position, {1,0,0}), "single key holds pose");
        single.RemoveKey("leaf", -1);
        Check(single.Get().tracks.empty(), "last key removes track");
        single.RemoveBone("leaf");
        Check(single.Get().bones.empty(), "unreferenced leaf deletion");
        Check(single.Undo(), "deletion undo");
        Reject([&]{single.RemoveBone("unknown");});
        Check(single.Redo() && single.Get().bones.empty(), "failed edit preserves redo branch");
        Ex::Document invalid{{{"a", "", {}}}, {{"a", {{0,{},{}},{0,{},{}}}}}};
        Reject([&]{Ex::AuthoringDocument rejected(invalid);});
        invalid.tracks = {{"missing", {}}};
        Reject([&]{Ex::EvaluateWorld(invalid, 0);});
        invalid.tracks.clear(); invalid.bones.front().parent = "a";
        Reject([&]{Ex::AuthoringDocument rejected(invalid);});
        Ex::AuthoringDocument rotated(Ex::Document{});
        rotated.AddBone({"r", "", {{1,0,0},M::AxisAngle({0,0,1},1.5707963267948966)}});
        rotated.AddBone({"c", "r", {{1,0,0},{}}});
        rotated.AddBone({"d", "c", {{1,0,0},{}}});
        rotated.AddBone({"p", "", {{4,0,0},M::AxisAngle({0,1,0},1.5707963267948966)}});
        const auto old_rotated = M::ForwardKinematics(rotated.Get().bones);
        rotated.Reparent("c", "p", true);
        const auto new_rotated = M::ForwardKinematics(rotated.Get().bones);
        for (const auto& id : {"c", "d"}) {
            Check(Near(old_rotated.at(id).position, new_rotated.at(id).position), "rotated reparent preserves descendants");
            Check(Near(M::Rotate(old_rotated.at(id).rotation,{1,0,0}), M::Rotate(new_rotated.at(id).rotation,{1,0,0})), "rotated reparent preserves orientation");
        }
        const auto residual = Ex::BallisticIntervalResidual({0,0,0},{1,1,0},{2,0,0},1,2,2);
        Check(Near(residual, {}), "analytic parabola has zero residual");
        Check(Near(Ex::BallisticIntervalResidual({0,0,0},{0,0,0},{2,0,0},0,2,2), {}), "initial endpoint");
        Check(Near(Ex::BallisticIntervalResidual({0,0,0},{2,0,0},{2,0,0},2,2,2), {}), "final endpoint");
        const auto jacobian = Ex::BallisticPositionJacobian(0.5,2);
        const double epsilon = 1e-5;
        const auto base = Ex::BallisticIntervalResidual({0,0,0},{0,0,0},{2,0,0},0.5,2,2);
        const auto shifted = Ex::BallisticIntervalResidual({epsilon,0,0},{0,0,0},{2,0,0},0.5,2,2);
        Check(std::abs((shifted.x-base.x)/epsilon-jacobian[0]) < 1e-9, "own Jacobian agrees with finite differences");
        Check(Ex::ExcessDistanceCubic({3,0,0},{},{},{},1) == 8, "known cubic excess");
        Check(Ex::ExcessDistanceCubic({0.5,0,0},{},{},{},1) == 0, "below threshold");
        Reject([&]{Ex::BallisticIntervalResidual({},{},{},0,0,1);});
        Ex::RunRiggingChecks();
        Ex::RunSkinningChecks();
        Check(Near(M::Rotate(Ex::ComposeEulerXyz({0,0,0}), {1,2,3}), {1,2,3}), "Euler identity");
        Check(Near(M::Rotate(Ex::ComposeEulerXyz({1.5707963267948966,1.5707963267948966,0}), {1,0,0}), {0,1,0}), "Euler product applies Y before X");
        const auto mixed = Ex::ComposeEulerXyz({0.7853981633974483,0.5235987755982989,1.0471975511965976});
        Check(Near(M::Rotate(mixed,{1,0,0}), {0.4330127018922193,0.7891491309924314,0.4355957403991576}), "mixed Euler agrees with independent matrix column");
        Check(Near(M::Rotate(Ex::ComposeEulerXyz({0,0,6.283185307179586}), {1,0,0}), {1,0,0}), "Euler full turn quaternion equivalent");
        Reject([&]{Ex::ComposeEulerXyz({std::numeric_limits<double>::infinity(),0,0});});
        std::cout << "PASS portable authoring/evaluation/physics checks=" << checks << "; rigging/skinning checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
