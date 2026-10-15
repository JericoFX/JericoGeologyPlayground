#pragma once
#include "../External/AnimationNative/examples/cpp/evaluation_example.hpp"
#include "../External/AnimationNative/examples/cpp/rigging_example.hpp"
#include "../External/AnimationNative/examples/cpp/skinning_example.hpp"
#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <set>

namespace JE::AnimationTools {
namespace Math = Playground;
using Document = Examples::Document;
using Pose = std::vector<Math::Bone>;
using Keyframe = Examples::Keyframe;
using SkinBinding = Examples::SkinBinding;

// Right-handed Y-up, metres/seconds/radians. Rigid transforms; no scale.
struct SkinMesh {
    std::vector<Math::Vec3> positions;
    std::vector<std::array<std::uint32_t, 3>> triangles;
    std::vector<std::vector<Math::Influence>> influences;
    SkinBinding binding;
};
struct RigChain { std::string id; std::vector<std::string> bones; Math::Vec3 tipOffset{}; };
struct AnimationAsset { Document document; SkinMesh mesh; std::vector<RigChain> chains; };
void validateAsset(const AnimationAsset& asset);
std::vector<Math::Vec3> skinPositions(const SkinMesh& mesh, const Pose& pose);
AnimationAsset ownSpiderFixture();

struct CorrectionResult {
    double residualBefore{}, residualAfter{};
    std::size_t iterations{};
    bool converged{};
};
struct EditorState {
    AnimationAsset asset;
    Pose pose; // Evaluated locals plus current manual edits, never rest-pose aliases.
    std::set<std::string> locks; // Hard world-transform locks for editing/solving.
    double time{};
};

// One mutating owner. Pose, keys and locks share exactly one transaction history.
class Editor {
public:
    explicit Editor(AnimationAsset asset = ownSpiderFixture());
    // Borrowed state/asset/pose references expire at the next mutating operation.
    // Snapshot IDs before a loop that edits multiple bones.
    const EditorState& state() const noexcept { return state_; }
    const AnimationAsset& asset() const noexcept { return state_.asset; }
    const Document& document() const noexcept { return state_.asset.document; }
    const Pose& pose() const noexcept { return state_.pose; }
    double currentTime() const noexcept { return state_.time; }
    std::uint64_t revision() const noexcept { return revision_; }
    std::size_t undoCount() const noexcept { return undo_.size(); }
    std::size_t redoCount() const noexcept { return redo_.size(); }
    bool editing() const noexcept { return transaction_.has_value(); }
    bool locked(const std::string& id) const;
    Math::WorldPose worldPose() const;
    void beginEdit();
    bool commitEdit();
    void cancelEdit();
    void setLocal(const std::string& id, Math::RigidTransform local);
    void moveWorld(const std::string& id, Math::Vec3 position);
    void setLocked(const std::string& id, bool value);
    Examples::CcdResult solveIk(const RigChain& chain, Math::Vec3 target,
                               std::size_t maxIterations = 128, double tolerance = 1e-5);
    CorrectionResult correctPositions(double groundHeight = 0, std::size_t iterations = 64,
                                      double tolerance = 1e-5);
    void setKey(const std::string& id, Math::Vec3 velocity = {});
    void removeKey(const std::string& id, double time);
    void seek(double seconds);
    void advance(double deltaSeconds, double start, double end, bool loop);
    bool undo();
    bool redo();
    std::string serialize() const;
    void loadText(const std::string& text); // Parse/validate before one atomic history entry.
    void save(const std::filesystem::path& path) const;
    void load(const std::filesystem::path& path);
private:
    EditorState state_;
    std::optional<EditorState> transaction_;
    std::vector<EditorState> undo_, redo_;
    std::uint64_t revision_{};
    void publish(EditorState next);
    void preserveLocks(Pose& next) const;
};
} // namespace JE::AnimationTools
