#pragma once
// Original portable evaluation example; no Cascadeur/native evaluator dependency.
#include "animation_suite_types.hpp"
#include <algorithm>
#include <cstddef>
#include <limits>
#include <map>
#include <vector>

namespace JE::AnimationTools::Examples {
namespace EvaluationDetail {
inline constexpr std::size_t NoParent = std::numeric_limits<std::size_t>::max();
inline constexpr std::size_t MaxBakeSamples = 1'000'000;

struct Layout {
    std::vector<const BoneTrack*> tracks;
    std::vector<std::size_t> parents;
    std::vector<std::size_t> order;
};

inline Layout Prepare(const Document& document) {
    Validate(document);
    Layout layout;
    const auto count = document.bones.size();
    layout.tracks.resize(count, nullptr);
    layout.parents.resize(count, NoParent);
    layout.order.reserve(count);
    std::map<std::string, std::size_t> indices;
    for (std::size_t i = 0; i < count; ++i)
        indices.emplace(document.bones[i].id, i);
    for (const auto& track : document.tracks)
        layout.tracks[indices.at(track.bone_id)] = &track;
    for (std::size_t i = 0; i < count; ++i)
        if (!document.bones[i].parent.empty())
            layout.parents[i] = indices.at(document.bones[i].parent);

    // Validate already rejected cycles/missing parents. Walk each chain once.
    std::vector<bool> ordered(count, false);
    std::vector<std::size_t> chain;
    for (std::size_t i = 0; i < count; ++i) {
        auto cursor = i;
        chain.clear();
        while (cursor != NoParent && !ordered[cursor]) {
            chain.push_back(cursor);
            cursor = layout.parents[cursor];
        }
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            ordered[*it] = true;
            layout.order.push_back(*it);
        }
    }
    return layout;
}

inline Math::RigidTransform Normalized(Math::RigidTransform value) {
    value.rotation = Math::Unit(value.rotation);
    return value;
}

inline Math::RigidTransform LocalAt(const Math::Bone& bone,
                                    const BoneTrack* track, double time) {
    if (!track || track->keys.empty()) return Normalized(bone.local);
    const auto& keys = track->keys;
    if (time <= keys.front().time) return Normalized(keys.front().local);
    if (time >= keys.back().time) return Normalized(keys.back().local);
    const auto right = std::upper_bound(
        keys.begin(), keys.end(), time,
        [](double value, const Keyframe& key) { return value < key.time; });
    const auto& b = *right;
    const auto& a = *(right - 1);
    const double duration = b.time - a.time;
    Math::Finite(duration);
    const double u = (time - a.time) / duration;
    const auto& pa = a.local.position;
    const auto& pb = b.local.position;
    const auto& va = a.position_velocity;
    const auto& vb = b.position_velocity;
    return {{Math::Hermite(pa.x, pb.x, va.x, vb.x, duration, u),
             Math::Hermite(pa.y, pb.y, va.y, vb.y, duration, u),
             Math::Hermite(pa.z, pb.z, va.z, vb.z, duration, u)},
            Math::Slerp(a.local.rotation, b.local.rotation, u)};
}

inline Math::WorldPose WorldAt(const Document& document,
                              const Layout& layout, double time) {
    Math::WorldPose world;
    for (const auto i : layout.order) {
        const auto& bone = document.bones[i];
        const auto local = LocalAt(bone, layout.tracks[i], time);
        const auto parent = layout.parents[i];
        const auto value = parent == NoParent ? local :
            Math::Compose(world.at(document.bones[parent].id), local);
        Math::Finite(value.position);
        world.emplace(bone.id, value);
    }
    return world;
}

inline std::vector<double> SampleTimes(double start, double end, double step) {
    Math::Finite(start); Math::Finite(end); Math::Finite(step);
    if (start > end || step <= 0)
        throw std::invalid_argument("invalid bake interval or step");
    std::vector<double> times{start};
    while (times.back() < end) {
        if (times.size() == MaxBakeSamples)
            throw std::invalid_argument("bake exceeds one million samples");
        const auto current = times.back();
        const auto next = current + step;
        if (!(next > current))
            throw std::invalid_argument("bake step does not advance time");
        // Positive overflow also passes end: retain the finite endpoint.
        times.push_back(next >= end ? end : next);
    }
    return times;
}
} // namespace EvaluationDetail

inline std::vector<Math::Bone> EvaluateLocals(const Document& document, double time) {
    Math::Finite(time);
    const auto layout = EvaluationDetail::Prepare(document);
    auto bones = document.bones;
    for (std::size_t i = 0; i < bones.size(); ++i)
        bones[i].local = EvaluationDetail::LocalAt(bones[i], layout.tracks[i], time);
    return bones;
}

inline Math::WorldPose EvaluateWorld(const Document& document, double time) {
    Math::Finite(time);
    const auto layout = EvaluationDetail::Prepare(document);
    return EvaluationDetail::WorldAt(document, layout, time);
}

inline std::vector<EvaluatedFrame> Bake(const Document& document,
                                      double start, double end, double step) {
    const auto times = EvaluationDetail::SampleTimes(start, end, step);
    const auto layout = EvaluationDetail::Prepare(document);
    std::vector<EvaluatedFrame> frames;
    frames.reserve(times.size());
    for (const auto time : times)
        frames.push_back({time, EvaluationDetail::WorldAt(document, layout, time)});
    return frames;
}
} // namespace JE::AnimationTools::Examples
