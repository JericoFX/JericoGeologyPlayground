#pragma once
// Original portable educational authoring, not an engine/editor adapter.
#include "animation_suite_types.hpp"
#include <algorithm>
#include <utility>

namespace JE::AnimationTools::Examples {

class AuthoringDocument {
public:
    explicit AuthoringDocument(Document document) : document_(std::move(document)) {
        for (auto& bone : document_.bones) bone.local.rotation = Math::Unit(bone.local.rotation);
        for (auto& track : document_.tracks)
            for (auto& key : track.keys) key.local.rotation = Math::Unit(key.local.rotation);
        Validate(document_);
    }

    [[nodiscard]] const Document& Get() const noexcept { return document_; }

    void AddBone(Math::Bone bone) {
        bone.local.rotation = Math::Unit(bone.local.rotation);
        Edit([&](Document& next) { next.bones.push_back(std::move(bone)); });
    }

    void RemoveBone(const std::string& id) {
        Edit([&](Document& next) {
            auto bone = FindBone(next, id);
            if (std::any_of(next.bones.begin(), next.bones.end(),
                            [&](const auto& item) { return item.parent == id; }))
                throw std::invalid_argument("cannot remove a bone with children");
            if (FindTrack(next, id) != next.tracks.end())
                throw std::invalid_argument("cannot remove a bone with a track");
            next.bones.erase(bone);
        });
    }

    // Preserve the reference pose only. Animated edited bones require rebaking.
    void Reparent(const std::string& id, const std::string& parent, bool preserveWorld) {
        Edit([&](Document& next) {
            auto bone = FindBone(next, id);
            if (!parent.empty()) (void)FindBone(next, parent);
            const auto track = FindTrack(next, id);
            if (track != next.tracks.end() && !track->keys.empty())
                throw std::invalid_argument("reparent of an animated bone requires an explicit rebake");
            if (preserveWorld) {
                const auto world = Math::ForwardKinematics(next.bones);
                bone->local = parent.empty()
                    ? world.at(id)
                    : Math::Compose(Math::Inverse(world.at(parent)), world.at(id));
            }
            bone->parent = parent;
        });
    }

    void SetKey(const std::string& id, Keyframe key) {
        key.local.rotation = Math::Unit(key.local.rotation);
        Math::Finite(key.time);
        Math::Finite(key.local.position);
        Math::Finite(key.position_velocity);
        Edit([&](Document& next) {
            (void)FindBone(next, id);
            auto track = FindTrack(next, id);
            if (track == next.tracks.end()) {
                next.tracks.push_back({id, {key}});
                return;
            }
            auto at = std::lower_bound(track->keys.begin(), track->keys.end(), key.time,
                                       [](const auto& item, double time) { return item.time < time; });
            if (at != track->keys.end() && at->time == key.time) *at = key;
            else track->keys.insert(at, key);
        });
    }

    void RemoveKey(const std::string& id, double time) {
        Math::Finite(time);
        Edit([&](Document& next) {
            (void)FindBone(next, id);
            auto track = FindTrack(next, id);
            if (track == next.tracks.end()) throw std::invalid_argument("bone has no track");
            auto key = std::lower_bound(track->keys.begin(), track->keys.end(), time,
                                       [](const auto& item, double value) { return item.time < value; });
            if (key == track->keys.end() || key->time != time)
                throw std::invalid_argument("key time does not exist");
            track->keys.erase(key);
            if (track->keys.empty()) next.tracks.erase(track);
        });
    }

    [[nodiscard]] bool Undo() { return Restore(undo_, redo_); }
    [[nodiscard]] bool Redo() { return Restore(redo_, undo_); }

private:
    Document document_;
    std::vector<Document> undo_, redo_;

    static std::vector<Math::Bone>::iterator FindBone(Document& document, const std::string& id) {
        auto at = std::find_if(document.bones.begin(), document.bones.end(),
                               [&](const auto& item) { return item.id == id; });
        if (at == document.bones.end()) throw std::invalid_argument("unknown bone ID");
        return at;
    }

    static std::vector<BoneTrack>::iterator FindTrack(Document& document, const std::string& id) {
        return std::find_if(document.tracks.begin(), document.tracks.end(),
                            [&](const auto& item) { return item.bone_id == id; });
    }

    static void Swap(Document& a, Document& b) noexcept {
        a.bones.swap(b.bones);
        a.tracks.swap(b.tracks);
    }

    template<class Operation>
    void Edit(Operation&& operation) {
        Document next = document_;
        std::forward<Operation>(operation)(next);
        Validate(next);
        undo_.push_back(document_); // May throw; published state is still intact.
        Swap(document_, next);     // No-throw commit with default vector allocators.
        redo_.clear();
    }

    bool Restore(std::vector<Document>& from, std::vector<Document>& to) {
        if (from.empty()) return false;
        to.push_back(document_); // Reserve/copy failure leaves both histories intact.
        Swap(document_, from.back());
        from.pop_back();
        return true;
    }
};

} // namespace JE::AnimationTools::Examples
