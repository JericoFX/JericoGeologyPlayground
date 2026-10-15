#pragma once
#include "rigging_example.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace JE::AnimationTools::Examples {
inline void RunRiggingChecks() {
    const auto require = [](bool value, const char* message) {
        if (!value) throw std::runtime_error(message);
    };
    const auto rejects = [](const auto& operation) {
        try { operation(); } catch (const std::invalid_argument&) { return true; }
        return false;
    };
    const auto close = [](Math::Vec3 a, Math::Vec3 b, double tolerance = 1e-9) {
        return Math::Length(a - b) <= tolerance;
    };
    const auto tip = [](const auto& bones, const CcdRequest& request) {
        const auto world = Math::ForwardKinematics(bones);
        const auto& end = world.at(request.chain.back());
        return end.position + Math::Rotate(end.rotation, request.tip_offset);
    };
    const auto same_rotation = [](Math::Quat a, Math::Quat b) {
        return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w;
    };
    std::vector<Math::Bone> bones{
        {"anchor", "", {{2, 3, 1}, Math::AxisAngle({0, 1, 0}, .4)}},
        {"j0", "anchor", {}},
        {"j1", "j0", {{1, 0, 0}, {}}},
        {"j2", "j1", {{1, 0, 0}, {}}},
        {"j3", "j2", {{1, 0, 0}, {}}},
        {"sibling", "j1", {{0, 1, 0}, Math::AxisAngle({0, 0, 1}, .25)}}};
    const auto initial = Math::ForwardKinematics(bones);
    CcdRequest request{{"j0", "j1", "j2", "j3"}, {1, 0, 0},
        initial.at("j0").position +
            Math::Rotate(initial.at("anchor").rotation, {1.5, 2, 0}), 128, 1e-6};
    const auto solved = SolveCcd(bones, request);
    require(solved.converged && solved.residual <= request.tolerance,
            "CCD reachable four-link chain did not converge");
    require(close(tip(solved.bones, request), request.target, request.tolerance),
            "CCD reported convergence disagrees with full FK");
    require(solved.iterations > 0 && solved.iterations <= request.max_iterations,
            "CCD iteration count is invalid");
    const auto world = Math::ForwardKinematics(solved.bones);
    require(close(world.at("j0").position, initial.at("j0").position),
            "CCD moved root position");
    for (std::size_t i = 0; i < bones.size(); ++i) {
        require(solved.bones[i].id == bones[i].id &&
                    solved.bones[i].parent == bones[i].parent &&
                    close(solved.bones[i].local.position, bones[i].local.position, 0),
                "CCD changed IDs, parents or local segment lengths");
    }
    for (const auto id : {"anchor", "sibling"}) {
        const auto a = std::find_if(bones.begin(), bones.end(),
            [&](const auto& bone) { return bone.id == id; });
        const auto b = std::find_if(solved.bones.begin(), solved.bones.end(),
            [&](const auto& bone) { return bone.id == id; });
        require(same_rotation(a->local.rotation, b->local.rotation),
                "CCD changed rotation outside chain");
    }
    for (std::size_t i = 1; i < request.chain.size(); ++i) {
        const auto length = Math::Length(world.at(request.chain[i]).position -
                                         world.at(request.chain[i - 1]).position);
        require(std::abs(length - 1) < 1e-9, "CCD changed world segment length");
    }
    std::reverse(bones.begin(), bones.end());
    const auto reordered = SolveCcd(bones, request);
    require(reordered.converged &&
                close(tip(reordered.bones, request), tip(solved.bones, request)),
            "CCD depends on bone array order");

    CcdRequest far = request;
    far.target = initial.at("j0").position +
        Math::Rotate(initial.at("anchor").rotation, {100, 0, 0});
    far.max_iterations = 8;
    const auto unreachable = SolveCcd(bones, far);
    require(!unreachable.converged && std::isfinite(unreachable.residual) &&
                unreachable.residual > 95 && unreachable.iterations == 8,
            "CCD unreachable result is invalid");

    const std::vector<Math::Bone> single{{"one", "", {}}};
    CcdRequest opposite{{"one"}, {1, 0, 0}, {-1, 0, 0}, 4, 1e-9};
    const auto flipped = SolveCcd(single, opposite);
    require(flipped.converged && close(tip(flipped.bones, opposite), opposite.target),
            "CCD antiparallel fallback failed");
    CcdRequest zero{{"one"}, {}, {1, 0, 0}, 2, 0};
    const auto no_lever = SolveCcd(single, zero);
    require(!no_lever.converged && no_lever.residual == 1 && no_lever.iterations == 2,
            "CCD zero lever or zero tolerance is invalid");
    zero.target = {};
    const auto already = SolveCcd(single, zero);
    require(already.converged && already.residual == 0 && already.iterations == 0,
            "CCD initial exact convergence was not preserved");

    auto invalid = request;
    invalid.chain.clear();
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted empty chain");
    invalid = request; invalid.chain = {"j0", "j0"};
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted duplicate joint");
    invalid.chain = {"j0", "absent"};
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted missing joint");
    invalid.chain = {"j0", "j2"};
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted indirect parent");
    invalid = request; invalid.max_iterations = 0;
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted zero iterations");
    invalid = request; invalid.tolerance = -1;
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted negative tolerance");
    invalid.tolerance = std::numeric_limits<double>::quiet_NaN();
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted NaN tolerance");
    invalid = request; invalid.target.x = std::numeric_limits<double>::quiet_NaN();
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted NaN target");
    invalid = request; invalid.tip_offset.x = std::numeric_limits<double>::infinity();
    require(rejects([&] { (void)SolveCcd(bones, invalid); }), "CCD accepted infinite offset");
    auto bad_bones = bones; bad_bones.front().local.position.x =
        std::numeric_limits<double>::quiet_NaN();
    require(rejects([&] { (void)SolveCcd(bad_bones, request); }), "CCD accepted NaN bone");
    invalid = opposite; invalid.target.x = -std::numeric_limits<double>::max();
    auto overflowing = single;
    overflowing.front().local.position.x = std::numeric_limits<double>::max();
    require(rejects([&] { (void)SolveCcd(overflowing, invalid); }),
            "CCD did not reject residual overflow");
}
} // namespace JE::AnimationTools::Examples
