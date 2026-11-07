#pragma once
#include "AnimationImport.hpp"
#include "Renderer.hpp"
#include "AnimationScene.hpp"
#include <memory>

namespace Playground {
class AnimationLabUI {
    using Editor = JE::AnimationTools::Editor;
    std::unique_ptr<Editor> editor;
    std::optional<AnimationImport::Asset> imported;
    Camera camera;
    std::string selected, error, status;
    std::array<char, 512> documentPath{};
    int modelChoice{}, chainChoice{}, view{};
    int spiderJointsPerLeg{7}, archetypeJointsPerLimb{4};
    bool dirty{true}, playing{}, loop{true}, showBones{true}, ikMode{}, dragging{};
    bool fitPending{true};
    bool showAllNodes{};
    double rangeStart{}, rangeEnd{2}, speed{1}, groundHeight{}, skinMilliseconds{}, frameMilliseconds{};
    JE::AnimationTools::Math::Vec3 ikTarget{}, dragHit{}, dragStart{};
    JE::AnimationTools::CorrectionResult correction;
    Pipes::ImportedMesh displayed;
    PipeRenderStats renderStatistics;
    struct EvidenceModel {std::filesystem::path path;std::string id;double time{};bool focusMouth{},bones{true},verifyIkFk{};};
    std::vector<EvidenceModel> evidenceModels;
    int sceneChoice{}, limbChoice{}, grabChoice{}, objectChoice{};
    bool interactiveScene{}, recording{};
    bool selectInteractionTab{};
    double maximumTransitionPosition{}, maximumTransitionAngle{}, maximumPhysicsEnergy{};
    std::size_t transitionChecks{};
    double interactionClock{}, recordingClock{}, mechanismAngle{}, sourceClipTime{};
    bool sourceClipPlaying{};
    std::string draggedObject, draggedLimb;
    JE::AnimationTools::Math::RigidTransform dragObjectStart{};
    std::vector<JE::AnimationTools::RecordedPose> recorded;
    std::optional<AnimationImport::Asset> sceneValve;
    std::unique_ptr<JE::AnimationTools::InteractionPhysics::World> physics;
    JE::AnimationTools::InteractionPhysics::Snapshot physicsSnapshot;
    JE::AnimationTools::InteractionState runtimeState;
    JE::AnimationTools::RuntimePose runtimePose;
    std::map<std::string,std::string> physicalGrabs;
    std::map<std::string,JE::AnimationTools::Math::RigidTransform> holdingTargets;
    std::map<std::string,JE::AnimationTools::LimbMode> previousLimbModes;
    void loadScene(int);
    void resetPhysics();
    void stepInteraction(double);
    void interactionInspector();
    void sceneHierarchy();
    void changeAuthority(const std::string&, JE::AnimationTools::TransformAuthority);
    void setObjectTarget(const std::string&, JE::AnimationTools::Math::RigidTransform);
    void applyInteraction(JE::AnimationTools::InteractionState);
    void loadModel(int);
    void loadDocument(const std::filesystem::path&);
    void clearDocumentTransients();
    void recoverEditError(const char*);
    void dragBoneTo(JE::AnimationTools::Math::Vec3);
    void setIkMode(bool);
    void adopt(AnimationImport::Asset);
    void fit();
    void rebuild(Renderer&);
    void viewport(Renderer&, float, float, float, float);
    void hierarchy();
    void properties();
    void timeline();
    void insertKeys(bool all);
public:
    AnimationLabUI();
    void activate() { dirty = true; }
    void frame(Renderer&, unsigned width, unsigned height);
    int prepareEvidence(const std::filesystem::path&);
    void evidenceStage(int, const std::filesystem::path&);
    void writeEvidence(const std::filesystem::path&, const std::string& adapter) const;
    static constexpr int evidenceStages = 43;
};
}
