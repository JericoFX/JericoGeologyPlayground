#pragma once
#include "AnimationCore.hpp"
#include <map>
#include <span>

namespace JE::AnimationTools {
enum class TransformAuthority { KinematicScene, PhysicsBody };
enum class Hand { None = 0, Left = 1, Right = 2, Either = 3 };
enum class GrabType { Fixed, Handle, Wheel };
enum class GrabStatus { Free, Occupied, Released };
enum class LimbMode { Fk, Ik };

struct SceneObject {
    std::string id;
    Math::RigidTransform world{};
    TransformAuthority authority{TransformAuthority::KinematicScene};
};
struct GrabMotionLimits {
    Math::Vec3 translationMin{}, translationMax{};
    Math::Vec3 rotationMin{}, rotationMax{}; // Local rotation-vector radians.
};
struct GrabPoint {
    std::string id, objectId;
    Math::RigidTransform local{}; // Hand target in object space, orientation included.
    Hand permittedHands{Hand::Either};
    GrabType type{GrabType::Fixed};
    GrabMotionLimits limits{};
    GrabStatus status{GrabStatus::Free};
    std::string occupant; // Limb ID, empty unless occupied.
};
struct JointLimit {
    std::string boneId;
    Math::Vec3 twistAxis{1,0,0}; // Axis in rest-local frame.
    double swingRadians{3.141592653589793};
    double twistMin{-3.141592653589793}, twistMax{3.141592653589793};
};
struct PoseOverride {
    Math::RigidTransform delta{}; // rest/base-local * delta, explicit order.
    double startTime{}, fadeDuration{};
    bool fadeToBase{};
};
struct LimbControl {
    RigChain chain;
    std::vector<std::string> sharedJoints; // Additional declared ancestors, merged by ID.
    Hand hand{Hand::None};
    LimbMode mode{LimbMode::Fk};
    bool enabled{true};
    double weight{1}, weightFrom{1}, weightStart{}, weightDuration{};
    Math::RigidTransform target{}; // World, including effector orientation.
    std::optional<Math::Vec3> pole; // World point; rest-plane default if absent.
    std::string grabPointId;
    Math::RigidTransform grabOffset{}; // Limited hand transform relative to GrabPoint.
    bool motorTargetEnabled{}; // Explicit world request to physics; IK reads solved grip.
    Math::RigidTransform targetFrom{};
    double targetStart{}, targetDuration{};
};
struct InteractionSettings {
    std::size_t maxIterations{96};
    double positionTolerance{1e-5}, angularTolerance{1e-5};
    double angularLengthScale{.35}; // Converts angle residual to metres in DLS.
    double damping{1e-3}, maxJointStep{.3}, poleWeight{.04};
};
struct RecordedPose { double time{}; Pose locals; };
struct BakedTake {
    std::string id;
    Document document;
    double start{}, end{};
};
struct InteractionState {
    std::string sceneId;
    std::map<std::string, SceneObject> objects;
    std::map<std::string, GrabPoint> grabPoints;
    std::map<std::string, LimbControl> limbs;
    std::map<std::string, JointLimit> jointLimits;
    std::map<std::string, PoseOverride> overrides;
    std::map<std::string, BakedTake> takes;
    InteractionSettings settings;
};
struct LimbDiagnostics {
    Math::RigidTransform target{}, actual{};
    double weight{}, positionError{}, angularError{}, contactError{}, limitViolation{};
    double solverPositionError{}, solverAngularError{};
    bool converged{}, singular{}, unreachable{}, hardLockConflict{}, partialBlend{};
};
struct RuntimePose {
    Pose locals;
    Math::WorldPose world;
    std::map<std::string, LimbDiagnostics> limbs;
    std::size_t iterations{}, uniqueSolverJoints{};
    double objectiveBefore{}, objectiveAfter{};
};

void validateInteraction(const Document& base, const InteractionState& state);
RuntimePose sampleInteraction(const Document& base, const InteractionState& state,
    double clipTime, double interactionTime, const std::set<std::string>& locks = {});
Math::RigidTransform grabPointWorld(const InteractionState& state, const std::string& id);

// Pure commands: return an atomic candidate. Editor is the sole history owner.
InteractionState withObjectTransform(const InteractionState& state, const std::string& id,
    Math::RigidTransform world); // Rejects PhysicsBody authority; backend owns it.
InteractionState withPhysicsSnapshot(const InteractionState& state, const std::string& id,
    Math::RigidTransform world); // Explicit physics-owned publication.
InteractionState withLimbTarget(const InteractionState& state, const std::string& id,
    Math::RigidTransform target, double interactionTime, double duration = 0);
InteractionState withLimbBlend(const InteractionState& state, const std::string& id,
    double weight, double interactionTime, double duration = .15);
InteractionState withLimbMode(const Document& base, const InteractionState& state,
    const std::string& id, LimbMode mode, double clipTime, double interactionTime,
    const std::set<std::string>& locks = {});
InteractionState acquireGrab(const Document& base, const InteractionState& state,
    const std::string& limbId, const std::string& grabPointId,
    double clipTime, double interactionTime, double duration = .3,
    const std::set<std::string>& locks = {});
InteractionState releaseGrab(const Document& base, const InteractionState& state,
    const std::string& limbId, double clipTime, double interactionTime,
    double fadeDuration = .3, const std::set<std::string>& locks = {});
InteractionState withPoseOverride(const Document& base, const InteractionState& state,
    const std::string& boneId, Math::RigidTransform local,
    double clipTime, double interactionTime);
InteractionState withBakedTake(const InteractionState& state, const Document& base,
    const std::string& id, std::span<const RecordedPose> frames);

// Original source-generated actor: body, two arms, two legs, skinned geometry,
// and authored walking exercise. No imported animation or learned prediction.
struct InteractionFixture { AnimationAsset actor; InteractionState interaction; };
InteractionFixture humanoidInteractionFixture();
InteractionState interactionForAsset(const AnimationAsset& asset);
} // namespace JE::AnimationTools
