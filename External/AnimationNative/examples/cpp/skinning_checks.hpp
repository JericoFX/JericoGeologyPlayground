#pragma once
#include "skinning_example.hpp"
#include <algorithm>
#include <limits>
#include <numbers>

namespace JE::AnimationTools::Examples {

inline void RunSkinningChecks() {
    const auto check = [](bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    };
    const auto near = [](Math::Vec3 a, Math::Vec3 b) { return Math::Length(a - b) < 1e-9; };
    const auto reject = [](auto&& operation) {
        try { operation(); }
        catch (const std::invalid_argument&) { return; }
        throw std::runtime_error("skinning invalid input was accepted");
    };
    const std::vector<Math::Bone> rest{
        {"tip", "root", {{0,2,0}, {}}},
        {"root", "", {{1,0,0}, {}}}
    };
    const auto binding = BuildSkinBinding(rest, {"tip", "root"});
    const std::vector<Math::Vec3> vertices{{2,2,0}};
    const std::vector<std::vector<Math::Influence>> weights{{{0,1}}};
    check(binding.joint_ids == std::vector<std::string>{"tip", "root"}, "binding order changed");
    check(near(SkinMeshPositions(vertices, weights, binding, rest).at(0), vertices.at(0)),
          "rest LBS must preserve vertex");

    auto animated = rest;
    animated.at(1).local.rotation = Math::AxisAngle({0,1,0}, std::numbers::pi / 2);
    check(near(SkinMeshPositions(vertices, weights, binding, animated).at(0), {1,2,-1}),
          "90 degree rigid skin result");
    auto reordered_rest = rest;
    auto reordered_pose = animated;
    std::reverse(reordered_rest.begin(), reordered_rest.end());
    std::reverse(reordered_pose.begin(), reordered_pose.end());
    const auto reordered_binding = BuildSkinBinding(reordered_rest, {"tip", "root"});
    check(near(SkinMeshPositions(vertices, weights, reordered_binding, reordered_pose).at(0), {1,2,-1}),
          "bone storage order must not identify joints");
    const auto swapped_binding = BuildSkinBinding(rest, {"root", "tip"});
    check(near(SkinMeshPositions(vertices, {{{1,1}}}, swapped_binding, animated).at(0), {1,2,-1}),
          "binding order requires remapped weight indices");

    const std::vector<Math::Bone> blend_rest{
        {"a", "", {{0,0,0}, {}}}, {"b", "", {{2,0,0}, {}}}
    };
    const std::vector<Math::Bone> blend_pose{
        {"b", "", {{2,2,0}, {}}}, {"a", "", {{2,0,0}, {}}}
    };
    const auto blend_binding = BuildSkinBinding(blend_rest, {"a", "b"});
    const auto blend_weights = NormalizeInfluences({{0,1}, {1,3}}, 2);
    check(std::abs(blend_weights.at(0).weight - .25) < 1e-12 &&
          std::abs(blend_weights.at(1).weight - .75) < 1e-12, "explicit weight normalization");
    check(near(SkinMeshPositions({{1,0,0}}, {blend_weights}, blend_binding, blend_pose).at(0), {1.5,1.5,0}),
          "known two-joint blend");
    check(NormalizeInfluences({{0,0}, {1,2}}, 2).at(0).weight == 0, "retain explicit zero influence");

    reject([&] { (void)NormalizeInfluences({}, 2); });
    reject([&] { (void)NormalizeInfluences({{0,0}}, 2); });
    reject([&] { (void)NormalizeInfluences({{0,-1}}, 2); });
    reject([&] { (void)NormalizeInfluences({{0,std::numeric_limits<double>::quiet_NaN()}}, 2); });
    reject([&] { (void)NormalizeInfluences({{0,std::numeric_limits<double>::infinity()}}, 2); });
    reject([&] { (void)NormalizeInfluences({{2,1}}, 2); });
    reject([&] { (void)NormalizeInfluences({{0,1}, {0,0}}, 2); });
    reject([&] { (void)BuildSkinBinding(rest, {"tip", "tip"}); });
    reject([&] { (void)BuildSkinBinding(rest, {""}); });
    reject([&] { (void)BuildSkinBinding(rest, {"missing"}); });
    reject([&] { (void)SkinMeshPositions(vertices, weights, binding, {{"root", "", {}}}); });
    reject([&] { (void)SkinMeshPositions(vertices, {}, binding, rest); });
    auto bad_binding = binding;
    bad_binding.inverse_bind.pop_back();
    reject([&] { (void)SkinMeshPositions(vertices, weights, bad_binding, rest); });
    bad_binding = binding;
    bad_binding.joint_ids.at(1) = "tip";
    reject([&] { (void)SkinMeshPositions(vertices, weights, bad_binding, rest); });
    bad_binding = binding;
    bad_binding.inverse_bind.at(0).rotation = {0,0,0,0};
    reject([&] { (void)SkinMeshPositions(vertices, weights, bad_binding, rest); });
    reject([&] { (void)SkinMeshPositions(vertices, {{{0,2}}}, binding, rest); });
    reject([&] { (void)SkinMeshPositions(vertices, {{{0,.5}, {0,.5}}}, binding, rest); });
    reject([&] { (void)SkinMeshPositions({{std::numeric_limits<double>::infinity(),0,0}}, weights, binding, rest); });
}

} // namespace JE::AnimationTools::Examples
