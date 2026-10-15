#pragma once
// Original single-effector rotational CCD; no native Cascadeur solver.
#include "animation_suite_types.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <numbers>
#include <string>
#include <vector>

namespace JE::AnimationTools::Examples {
struct CcdRequest {
    std::vector<std::string> chain; // First joint to last joint, direct parents.
    Math::Vec3 tip_offset{}, target{}; // Tip local to last bone; target in world.
    std::size_t max_iterations{64};
    double tolerance{1e-5}; // Metres; zero is valid, not a convergence guarantee.
};
struct CcdResult {
    std::vector<Math::Bone> bones;
    bool converged{};
    double residual{};
    std::size_t iterations{}; // Sweeps attempted, including a final partial sweep.
};

namespace RiggingDetail {
inline constexpr double DirectionEpsilon = 1e-12;

inline Math::Quat Conjugate(Math::Quat value) {
    value = Math::Unit(value);
    return {-value.x, -value.y, -value.z, value.w};
}

inline Math::Quat RotationBetween(Math::Vec3 from, Math::Vec3 to) {
    from = Math::Unit(from);
    to = Math::Unit(to);
    const auto dot = std::clamp(Math::Dot(from, to), -1.0, 1.0);
    const auto cross = Math::Cross(from, to);
    const auto sine = Math::Length(cross);
    if (sine > DirectionEpsilon)
        return Math::AxisAngle(cross, std::atan2(sine, dot));
    if (dot >= 0) return {};
    // Antiparallel: choose the least-aligned Cartesian axis, ties X then Y.
    const auto x = std::abs(from.x), y = std::abs(from.y), z = std::abs(from.z);
    const Math::Vec3 basis = x <= y && x <= z ? Math::Vec3{1, 0, 0} :
        (y <= z ? Math::Vec3{0, 1, 0} : Math::Vec3{0, 0, 1});
    return Math::AxisAngle(Math::Cross(from, basis), std::numbers::pi);
}

inline Math::Vec3 Tip(const std::vector<Math::RigidTransform>& world,
                      Math::Vec3 offset) {
    const auto& end = world.back();
    const auto point = end.position + Math::Rotate(end.rotation, offset);
    Math::Finite(point);
    return point;
}

inline double Residual(Math::Vec3 tip, Math::Vec3 target) {
    const auto difference = target - tip;
    Math::Finite(difference);
    const auto distance = Math::Length(difference);
    Math::Finite(distance);
    return distance;
}
} // namespace RiggingDetail

inline CcdResult SolveCcd(const std::vector<Math::Bone>& bones,
                          const CcdRequest& request) {
    Math::Finite(request.target); Math::Finite(request.tip_offset);
    Math::Finite(request.tolerance);
    if (request.chain.empty() || request.max_iterations == 0 || request.tolerance < 0)
        throw std::invalid_argument("invalid CCD chain, iteration limit or tolerance");
    const auto initial_world = Math::ForwardKinematics(bones);
    std::map<std::string, std::size_t> indices;
    for (std::size_t i = 0; i < bones.size(); ++i) indices.emplace(bones[i].id, i);
    std::vector<std::size_t> chain;
    std::map<std::string, bool> seen;
    for (const auto& id : request.chain) {
        const auto found = indices.find(id);
        if (found == indices.end() || !seen.emplace(id, true).second)
            throw std::invalid_argument("missing or duplicate CCD joint ID");
        if (!chain.empty() && bones[found->second].parent != bones[chain.back()].id)
            throw std::invalid_argument("CCD chain must follow direct parents");
        chain.push_back(found->second);
    }
    const auto& root = bones[chain.front()];
    const Math::RigidTransform parent_world = root.parent.empty() ?
        Math::RigidTransform{} : initial_world.at(root.parent);
    std::vector<Math::RigidTransform> world;
    world.reserve(chain.size());
    for (const auto i : chain) world.push_back(initial_world.at(bones[i].id));
    CcdResult result{bones};
    result.residual = RiggingDetail::Residual(
        RiggingDetail::Tip(world, request.tip_offset), request.target);
    result.converged = result.residual <= request.tolerance;
    if (result.converged) return result;

    for (std::size_t sweep = 0; sweep < request.max_iterations; ++sweep) {
        result.iterations = sweep + 1;
        for (std::size_t cursor = chain.size(); cursor > 0; --cursor) {
            const auto joint = cursor - 1;
            const auto tip = RiggingDetail::Tip(world, request.tip_offset);
            const auto from = tip - world[joint].position;
            const auto to = request.target - world[joint].position;
            Math::Finite(from); Math::Finite(to);
            const auto from_length = Math::Length(from), to_length = Math::Length(to);
            Math::Finite(from_length); Math::Finite(to_length);
            // No usable direction if either lever is numerically zero.
            if (from_length <= RiggingDetail::DirectionEpsilon ||
                to_length <= RiggingDetail::DirectionEpsilon) continue;
            const auto delta = RiggingDetail::RotationBetween(from, to);
            const auto parent_rotation = joint == 0 ? parent_world.rotation :
                world[joint - 1].rotation;
            const auto local_delta = Math::Multiply(
                Math::Multiply(RiggingDetail::Conjugate(parent_rotation), delta),
                parent_rotation);
            auto& local = result.bones[chain[joint]].local;
            local.rotation = Math::Multiply(local_delta, local.rotation);

            // Only refresh the chain suffix. Other bone locals are never edited.
            for (std::size_t i = joint; i < chain.size(); ++i) {
                const auto& parent = i == 0 ? parent_world : world[i - 1];
                world[i] = Math::Compose(parent, result.bones[chain[i]].local);
                Math::Finite(world[i].position);
            }
            result.residual = RiggingDetail::Residual(
                RiggingDetail::Tip(world, request.tip_offset), request.target);
            if (result.residual <= request.tolerance) {
                result.converged = true;
                return result;
            }
        }
    }
    return result;
}
} // namespace JE::AnimationTools::Examples
