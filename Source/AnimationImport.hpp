#pragma once
#include "AnimationCore.hpp"
#include "PipeSockets.hpp"
#include <array>
#include <filesystem>

namespace Playground::AnimationImport {
namespace Animation = JE::AnimationTools;
using Matrix = std::array<double,16>; // Column major, column vectors.
struct SourceJoint {
    std::string id, name, parentId;
    Matrix world{}, inverseBind{}; // Original affine data, before rigid conversion.
};
struct Clip { std::string name; Animation::Document document; double start{}, end{}; };
struct SourceStatistics {
    std::size_t nodes{}, meshes{}, bones{}, skins{}, animations{}, vertices{}, triangles{};
    std::size_t omittedDegenerateTriangles{}, reconstructedNormals{};
};
struct Asset {
    Pipes::ImportedMesh renderMesh; // Original material/UV data and world-space bind geometry.
    Animation::Document document;
    Animation::SkinMesh skin;
    std::vector<Animation::RigChain> chains;
    std::vector<SourceJoint> sourceJoints;
    std::vector<Clip> clips;
    std::vector<std::string> warnings;
    std::filesystem::path source;
    std::string label, rigDescription;
    SourceStatistics statistics;
    bool importedRig{}, automaticBinding{};
};
enum class RigPreset { Quadruped, Spider, Automatic };
Asset loadAsset(const std::filesystem::path& path);
// Explicit authoring operation for static meshes; never called by loadAsset().
// Spider generation accepts 3..64 joints per leg; imported rigs are unrestricted.
void createPresetRig(Asset& asset, RigPreset preset, std::size_t jointsPerLeg = 3);
// Archetype-driven rig: the classifier decides the family, the geodesic skeleton supplies
// the bones. Works for bipeds, which the support-count presets could not express.
void createArchetypeRig(Asset& asset, std::size_t jointsPerLimb = 4);
void bindRig(Asset& asset, const Animation::Document& document,
             const std::vector<Animation::RigChain>& chains = {});
Animation::AnimationAsset animationAsset(const Asset& asset);
// Returns a new render mesh; invalid poses/bindings never modify the source asset.
Pipes::ImportedMesh deform(const Asset& asset, const Animation::Pose& pose);
} // namespace Playground::AnimationImport
