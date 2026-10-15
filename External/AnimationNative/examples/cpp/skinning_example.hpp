#pragma once
// Original rigid LBS positions example. No engine, scale, normals or DQS.
#include "animation_suite_types.hpp"
#include <set>

namespace JE::AnimationTools::Examples {

struct SkinBinding {
    std::vector<std::string> joint_ids;
    std::vector<Math::RigidTransform> inverse_bind;
};

inline SkinBinding BuildSkinBinding(const std::vector<Math::Bone>& bones,
                                    const std::vector<std::string>& joint_ids) {
    const auto world = Math::ForwardKinematics(bones);
    SkinBinding binding{joint_ids, {}};
    binding.inverse_bind.reserve(joint_ids.size());
    std::set<std::string> unique;
    for (const auto& id : joint_ids) {
        if (id.empty() || !unique.insert(id).second || !world.contains(id))
            throw std::invalid_argument("empty, duplicate or missing binding joint ID");
        binding.inverse_bind.push_back(Math::Inverse(world.at(id)));
    }
    return binding;
}

inline std::vector<Math::Influence> NormalizeInfluences(
    const std::vector<Math::Influence>& influences, std::size_t joint_count) {
    std::set<std::size_t> unique;
    double sum = 0;
    for (const auto& influence : influences) {
        Math::Finite(influence.weight);
        if (influence.weight < 0 || influence.joint >= joint_count ||
            !unique.insert(influence.joint).second)
            throw std::invalid_argument("invalid or duplicate skin influence");
        sum += influence.weight;
    }
    if (!std::isfinite(sum) || sum <= 0)
        throw std::invalid_argument("skin weight total must be finite and positive");
    auto normalized = influences;
    for (auto& influence : normalized) influence.weight /= sum;
    return normalized;
}

inline std::vector<Math::Vec3> SkinMeshPositions(
    const std::vector<Math::Vec3>& bind_vertices,
    const std::vector<std::vector<Math::Influence>>& per_vertex,
    const SkinBinding& binding, const std::vector<Math::Bone>& posed_bones) {
    if (bind_vertices.size() != per_vertex.size() ||
        binding.joint_ids.size() != binding.inverse_bind.size())
        throw std::invalid_argument("skin mesh or binding cardinality mismatch");
    const auto pose = Math::ForwardKinematics(posed_bones);
    std::vector<Math::RigidTransform> world, bind_world;
    world.reserve(binding.joint_ids.size());
    bind_world.reserve(binding.joint_ids.size());
    std::set<std::string> ids;
    for (std::size_t joint = 0; joint < binding.joint_ids.size(); ++joint) {
        const auto& id = binding.joint_ids[joint];
        if (id.empty() || !ids.insert(id).second || !pose.contains(id))
            throw std::invalid_argument("empty, duplicate or missing posed binding joint ID");
        world.push_back(pose.at(id));
        // SkinPosition accepts bind world, not inverse bind; keep its contract.
        bind_world.push_back(Math::Inverse(binding.inverse_bind[joint]));
    }
    std::vector<Math::Vec3> result;
    result.reserve(bind_vertices.size());
    for (std::size_t vertex = 0; vertex < bind_vertices.size(); ++vertex) {
        // Validate uniqueness without silently normalizing the authored weights.
        (void)NormalizeInfluences(per_vertex[vertex], world.size());
        result.push_back(Math::SkinPosition(bind_vertices[vertex], per_vertex[vertex], world, bind_world));
    }
    return result;
}

} // namespace JE::AnimationTools::Examples
