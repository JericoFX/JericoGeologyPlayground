#pragma once
// Original portable examples. Metres, seconds, right-handed Y-up; rigid TR.
#include "../../playground/animation_core.hpp"
#include <set>

namespace JE::AnimationTools::Examples {
namespace Math = JE::AnimationTools::Playground;
struct Keyframe {
    double time{};
    Math::RigidTransform local{};
    Math::Vec3 position_velocity{}; // Derivative in metres per second.
};
struct BoneTrack { std::string bone_id; std::vector<Keyframe> keys; };
struct Document { std::vector<Math::Bone> bones; std::vector<BoneTrack> tracks; };
struct EvaluatedFrame { double time{}; Math::WorldPose world; };

inline void Validate(const Document& document) {
    (void)Math::ForwardKinematics(document.bones);
    std::set<std::string> ids, tracks;
    for (const auto& bone : document.bones) ids.insert(bone.id);
    for (const auto& track : document.tracks) {
        if (!ids.contains(track.bone_id) || !tracks.insert(track.bone_id).second)
            throw std::invalid_argument("unknown or duplicate bone track");
        bool first = true;
        double previous{};
        for (const auto& key : track.keys) {
            Math::Finite(key.time);
            Math::Finite(key.local.position);
            (void)Math::Unit(key.local.rotation);
            Math::Finite(key.position_velocity);
            if (!first && key.time <= previous)
                throw std::invalid_argument("keys must have strictly increasing times");
            previous = key.time;
            first = false;
        }
    }
}
} // namespace JE::AnimationTools::Examples
