#pragma once

// PROPOSED CONTRACT ONLY. No engine adapter or animation algorithm is implemented.
// C++20, standard library only. See docs/17-api-cpp-animation-tools.md.
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace JE::AnimationTools {

struct Uuid { std::uint64_t high{}, low{}; bool operator==(const Uuid&) const = default; };
template<class Tag> struct Id { Uuid value{}; bool operator==(const Id&) const = default; };
struct DocumentTag; struct SkeletonTag; struct JointTag; struct MeshTag; struct BindingTag;
struct RigTag; struct ChainTag; struct LimbTag; struct ControlTag; struct ConstraintTag;
struct BodyTag; struct ContactTag; struct SurfaceTag; struct LayerTag; struct ChannelTag;
struct KeyTag; struct EventTag; struct ClipTag; struct ModelTag; struct DatasetTag;
struct JobTag; struct ProposalTag; struct TransactionTag; struct SnapshotTag;
using DocumentId=Id<DocumentTag>; using SkeletonId=Id<SkeletonTag>; using JointId=Id<JointTag>;
using MeshId=Id<MeshTag>; using BindingId=Id<BindingTag>; using RigId=Id<RigTag>;
using ChainId=Id<ChainTag>; using LimbId=Id<LimbTag>; using ControlId=Id<ControlTag>;
using ConstraintId=Id<ConstraintTag>; using BodyId=Id<BodyTag>; using ContactId=Id<ContactTag>;
using SurfaceId=Id<SurfaceTag>; using LayerId=Id<LayerTag>; using ChannelId=Id<ChannelTag>;
using KeyId=Id<KeyTag>; using EventId=Id<EventTag>; using ClipId=Id<ClipTag>;
using ModelId=Id<ModelTag>; using DatasetId=Id<DatasetTag>; using JobId=Id<JobTag>;
using ProposalId=Id<ProposalTag>; using TransactionId=Id<TransactionTag>;
using SnapshotId=Id<SnapshotTag>;

struct ContentDigest { std::array<std::uint8_t,32> sha256{}; };
struct Revision { std::uint64_t value{}; bool operator==(const Revision&) const = default; };
struct Tick { std::int64_t value{}; bool operator==(const Tick&) const = default; };
struct Timebase { std::int64_t ticks_per_second{}; };
struct TickRange { Tick first{}, last{}; }; // Inclusive endpoints, first <= last.
struct Radians { double value{}; };
struct Vec3 { double x{}, y{}, z{}; };
struct Quaternion { double x{}, y{}, z{}, w{1.0}; };
struct Transform { Vec3 translation{}; Quaternion rotation{}; Vec3 scale{1,1,1}; };
struct Matrix4 { std::array<double,16> row_major{}; }; // Column vectors; not identity by default.
struct AxisMask { bool x{}, y{}, z{}; };
enum class Space { World, JointLocal, ParentLocal, ControlLocal, SurfaceLocal };
using SpaceEntity = std::variant<JointId,ControlId,SurfaceId>;
struct SpaceRef { Space kind{Space::World}; std::optional<SpaceEntity> entity; };

enum class Status { Success, InvalidInput, NotFound, Conflict, Unsupported, Infeasible,
                    Cancelled, ResourceLimit, BackendFailure };
enum class ErrorCode { None, InvalidId, BrokenReference, ParentCycle, InvalidTime,
    NonFinite, InvalidRotation, SingularTransform, UnsupportedShear, InvalidWeights,
    InvalidBinding, InvalidGeometry, InvalidLimits, LockedData, HardConstraintConflict,
    StaleRevision, DigestMismatch, ModelSchemaMismatch, TopologyNotCovered,
    InvalidContact, NoFeasibleFoothold, InvalidMassInertia, BudgetExceeded, InternalFailure };
using Subject = std::variant<DocumentId,SkeletonId,JointId,MeshId,BindingId,RigId,ChainId,
    LimbId,ControlId,ConstraintId,BodyId,ContactId,LayerId,ChannelId,KeyId,JobId>;
struct Diagnostic { ErrorCode code{ErrorCode::None}; std::string message;
    std::optional<Subject> subject; std::optional<Tick> time; };
template<class T> struct Result { Status status{Status::InvalidInput};
    std::optional<T> value; std::vector<Diagnostic> diagnostics; };
using Outcome=Result<std::monostate>;

struct DocumentInfo { DocumentId id; Revision revision; ContentDigest content;
    Timebase timebase; TickRange range; };
struct SnapshotInfo { SnapshotId id; DocumentInfo source; ContentDigest dependencies; };
struct DocumentAssets { std::vector<SkeletonId> skeletons; std::vector<MeshId> meshes;
    std::vector<BindingId> bindings; std::vector<RigId> rigs; std::vector<LayerId> layers; };
enum class Preserve { Local, WorldAtTime, WorldOverRange };
struct ReparentRequest { JointId joint; std::optional<JointId> parent;
    Preserve preserve{Preserve::Local}; Tick time; std::optional<TickRange> range; };
enum class RestChange { Rebind, CompensateAnimation, RejectAffectedData };
enum class JointKind { Fixed, Hinge, Ball, Slider, General };
struct JointLimits { JointKind kind{JointKind::Ball}; Vec3 axis_local{1,0,0};
    Radians hinge_min{}, hinge_max{}, swing_max{}, twist_min{}, twist_max{};
    double slider_min_metres{}, slider_max_metres{}; };
struct JointDescription { SkeletonId skeleton; std::string name;
    std::optional<JointId> parent; Transform rest_local; JointLimits limits; };
struct JointRecord { JointId id; JointDescription description; };
struct DofLocks { AxisMask translation, rotation; };

struct MeshGeometry { std::vector<Vec3> positions, normals;
    std::vector<std::uint32_t> triangle_indices; };
struct MeshDescription { std::string name; Transform mesh_world_bind; MeshGeometry geometry; };
struct Influence { JointId joint; double weight{}; };
struct VertexWeights { std::uint32_t vertex{}; std::vector<Influence> influences; };
struct JointBind { JointId joint; Matrix4 offset; };
enum class SkinningMode { LinearBlend, DualQuaternionRigid };
struct BindingDescription { MeshId mesh; SkeletonId skeleton; SkinningMode mode; Transform mesh_world_bind;
    std::vector<JointBind> joint_binds; std::vector<VertexWeights> weights; };
struct WeightEdit { std::uint32_t vertex{}; std::vector<Influence> influences;
    std::vector<JointId> locked_influences; };
struct SkinInitialization { BindingId binding; double radius_metres{};
    std::vector<JointId> allowed_joints; std::vector<WeightEdit> seeds;
    std::uint32_t smoothing_iterations{}; };
enum class RemovalPolicy { RejectReferences, ExplicitRemap };
struct JointRemap { JointId from, to; };
struct VertexRemap { std::uint32_t old_vertex{}, new_vertex{}; };

enum class ControlKind { Root, Body, JointFk, IkEffector, IkPole, Direction, Twist,
    ContactAnchor, CenterOfMass, Trajectory, Generic };
enum class SemanticRole { None, Pelvis, Head, Finger, Support, Grasp, Tail, Wing, Sensor };
struct ControlDescription { RigId rig; std::string name; ControlKind kind;
    SemanticRole role{SemanticRole::None}; std::vector<JointId> joints;
    std::optional<ChainId> chain; SpaceRef space; Transform initial;
    bool derived_read_only{}; };
struct ChainDescription { RigId rig; std::vector<JointId> joints; JointId effector_joint;
    Transform effector_offset; std::optional<Vec3> preferred_bend_direction_world; };
struct RigDescription { SkeletonId skeleton; std::string name; std::uint32_t recipe_version{}; };
enum class Priority { Hard, High, Normal, Posture };
struct ConstraintPolicy { Priority priority{Priority::Normal}; double weight{1.0}; bool enabled{true}; };
struct EffectorConstraint { ChainId chain; ControlId target; std::optional<ControlId> pole;
    bool position{true}, orientation{true}; };
struct AimConstraint { JointId joint; ControlId target; Vec3 forward_local{0,0,1};
    Vec3 up_local{0,1,0}; bool preserve_roll{}; };
struct TwistConstraint { std::vector<JointId> joints; ControlId control;
    std::vector<double> distribution; };
struct CoMConstraint { std::vector<BodyId> bodies; ControlId control; bool target_enabled{}; };
struct ContactConstraint { ContactId contact; bool position{true}, orientation{true}; };
struct BodyJointConstraint { BodyId a, b; Transform frame_a_local, frame_b_local;
    JointLimits limits; double compliance{}, damping{}; };
using ConstraintSpec=std::variant<EffectorConstraint,AimConstraint,TwistConstraint,
    CoMConstraint,ContactConstraint,BodyJointConstraint>;
struct ConstraintDescription { RigId rig; std::string name; ConstraintPolicy policy;
    ConstraintSpec spec; };
struct RigPlanInfo { SnapshotInfo source; RigId rig; ContentDigest plan;
    std::vector<ControlId> controls; std::vector<ConstraintId> constraints; };
enum class ChainMode { Fk, Ik, Blend };
struct SwitchRequest { ChainId chain; ChainMode mode; double ik_weight{}; Tick time;
    std::optional<TickRange> bake_range; double position_tolerance_metres{};
    Radians rotation_tolerance; };
struct Residual { Subject subject; std::optional<Tick> time; double position_metres{};
    Radians rotation; double normalized_constraint_error{}; };
struct SwitchReport { std::vector<KeyId> generated_keys; std::vector<Residual> residuals; };
struct SolveSettings { std::uint32_t iterations{}; double damping{};
    double characteristic_length_metres{}, position_tolerance_metres{};
    Radians rotation_tolerance, maximum_angular_step; double max_work_seconds{}; };
struct KinematicTarget { ControlId control; Tick time; Transform target; SpaceRef space;
    bool position{true}, orientation{true}; ConstraintPolicy policy; };

using ChannelTarget=std::variant<JointId,ControlId,ConstraintId,BodyId,ContactId,LayerId>;
enum class Property { LocalTranslation, LocalRotation, LocalScale, ControlPosition,
    ControlRotation, ControlScalar, ConstraintWeight, ContactState, LayerWeight, Custom };
enum class ValueKind { Scalar, Vector3, Rotation, Boolean, Integer };
using Value=std::variant<double,Vec3,Quaternion,bool,std::int64_t>;
enum class Interpolation { Step, Linear, Hermite, Bezier, RotationCubic };
struct AngularVelocity { Vec3 radians_per_second; SpaceRef space; };
using TangentValue=std::variant<double,Vec3,AngularVelocity>;
struct Tangent { TangentValue derivative; std::optional<Tick> bezier_handle_time;
    std::optional<Value> bezier_handle_value; };
// Extra signed full turns on the outgoing interval, only for a Hinge joint's
// LocalRotation around its fixed JointLimits.axis_local. General rotation must
// use intermediate keys and winding unset/zero; Twist scalars stay unwrapped.
struct KeyDescription { ChannelId channel; Tick time; Value value; Interpolation interpolation;
    std::optional<Tangent> incoming, outgoing; bool locked{}; std::optional<std::int64_t> winding; };
struct ChannelDescription { LayerId layer; ChannelTarget target; Property property;
    ValueKind value_kind; std::string custom_schema; Value default_value; };
enum class LayerComposition { Override, AdditiveLocal };
struct JointTransform { JointId joint; Transform local; };
struct LayerDescription { std::string name; LayerComposition composition; double weight{1.0};
    std::vector<ChannelTarget> mask; std::vector<JointTransform> additive_reference; bool muted{}; };
struct ChannelLock { ChannelId channel; TickRange range; bool lock_values{true}; };
struct RetimePoint { Tick source, destination; };
struct RetimeRequest { TickRange range; std::vector<RetimePoint> mapping;
    bool move_events{true}, move_contacts{true}; };
struct EventDescription { Tick time; std::string schema, payload; };

struct BoxShape { Vec3 full_side_lengths_metres; };
struct SphereShape { double radius_metres{}; };
struct CapsuleShape { double cylinder_length_metres{}, radius_metres{}; Vec3 axis_local{0,1,0}; };
struct ConvexShape { MeshId mesh; ContentDigest cooked_geometry; };
using CollisionShape=std::variant<BoxShape,SphereShape,CapsuleShape,ConvexShape>;
struct Collider { Transform body_local; CollisionShape shape; double friction{}, restitution{}; };
struct InertiaTensor { std::array<double,9> row_major_kg_metres_squared{}; };
enum class BodyMode { Dynamic, Kinematic, Static };
// Body frame origin is its CoM. joint_local places that frame in joint space,
// or world space when joint is absent. inertia_local is about the CoM in body axes.
struct BodyDescription { RigId rig; std::optional<JointId> joint; Transform joint_local;
    BodyMode mode; double mass_kg{}; InertiaTensor inertia_local;
    std::vector<Collider> colliders; };
struct MotorDescription { ConstraintId body_joint; ControlId rotation_target;
    double stiffness_newton_metres_per_radian{}, damping_newton_metres_seconds_per_radian{};
    double maximum_torque_newton_metres{}, maximum_power_watts{}; };
enum class EffectorState { Stance, Release, Swing, Acquire, Disabled };
struct ContactDescription { RigId rig; ControlId effector; SurfaceId surface;
    Transform surface_local_anchor; TickRange range; double friction{}, max_normal_force_newtons{};
    bool authored{}, unilateral{true}; };
struct TrajectoryKey { Tick time; Vec3 position_metres; Quaternion heading; };
struct TrajectoryDescription { ControlId root; std::vector<TrajectoryKey> keys; bool cyclic{}; };
struct LimbDescription { RigId rig; ChainId chain; ControlId effector;
    SemanticRole role; Transform nominal_body_local; double phase_offset_cycles{}, duty_factor{};
    double clearance_metres{}, max_effector_speed_metres_per_second{}; };
struct LimbPhase { LimbId limb; double offset_cycles{}, duty_factor{}; };
struct GaitDescription { double frequency_hz{}; std::vector<LimbPhase> phases; };
struct EnvironmentRef { Uuid asset; Revision revision; ContentDigest content; };
struct PhysicsSettings { double step_seconds{}; std::uint32_t substeps{}, iterations{};
    Vec3 gravity_metres_per_second_squared; double max_work_seconds{}; };

struct EvaluationRequest { SnapshotId snapshot; Tick time; SolveSettings solve;
    std::vector<KinematicTarget> targets; };
struct EvaluatedJoint { JointId joint; Transform local; Matrix4 world; };
struct EvaluatedControl { ControlId control; Transform world; };
struct EvaluatedContact { ContactId contact; EffectorState state; Vec3 world_position, world_normal;
    Vec3 force_newtons; double penetration_metres{}, slip_metres_per_second{}; };
struct Evaluation { SnapshotInfo source; Tick time; std::vector<EvaluatedJoint> joints;
    std::vector<EvaluatedControl> controls; std::vector<EvaluatedContact> contacts;
    Vec3 center_of_mass_world; std::vector<Residual> residuals; };
struct BindingEvaluation { BindingId binding; std::vector<Vec3> positions, normals; };

struct PhysicsRequest { SnapshotId snapshot; TickRange range; EnvironmentRef environment;
    PhysicsSettings physics; SolveSettings solve; std::vector<KeyId> hard_keys; };
struct MovementRequest { SnapshotId snapshot; TickRange range; EnvironmentRef environment;
    TrajectoryDescription trajectory; GaitDescription gait; std::vector<LimbId> limbs;
    PhysicsSettings physics; SolveSettings solve; std::uint64_t seed{}; };
enum class MissingMapping { Reject, RestPose, ExplicitAuthoring };
struct RetargetJoint { JointId source, destination; Quaternion source_to_destination_basis; };
struct RetargetChain { ChainId source, destination; };
// Both documents must have identical ticks_per_second; range is shared ticks
// in both documents. Different timebases require explicit prior resampling.
struct RetargetRequest { SnapshotId source, destination; TickRange range;
    std::vector<RetargetJoint> joints; std::vector<RetargetChain> chains;
    double root_translation_scale{}; MissingMapping missing; EnvironmentRef environment;
    SolveSettings solve; };
struct CookRequest { SnapshotId snapshot; TickRange range; Tick sample_step;
    JointId root_motion_joint; std::vector<BindingId> validation_bindings;
    double max_position_error_metres{}, max_vertex_error_metres{};
    Radians max_rotation_error; std::uint32_t runtime_influence_limit{};
    std::string codec_schema; bool include_contacts{}, include_events{}; };
enum class InferenceTask { PoseCompletion, Inbetweening, MovementSuggestion };
enum class ModelOrigin { EngineProduced, ImportedReference };
struct ModelCapabilities { ModelId id; ContentDigest model, schema, normalization;
    ModelOrigin origin; ContentDigest provenance_manifest;
    std::vector<InferenceTask> tasks; bool local_backend_available{}, variable_topology{};
    std::vector<std::string> evaluated_topology_profiles; };
struct KnownKey { ChannelTarget target; Property property; Tick time; Value value; bool hard{}; };
struct InferenceRequest { SnapshotId snapshot; ModelId model; InferenceTask task;
    TickRange range; std::vector<KinematicTarget> targets; std::vector<KnownKey> known;
    std::vector<ChannelLock> locks; std::vector<ContactId> contacts;
    SolveSettings solve; std::uint64_t seed{}; };
using JobSpec=std::variant<PhysicsRequest,MovementRequest,RetargetRequest,CookRequest,InferenceRequest>;
enum class JobState { Queued, Running, CancelRequested, Cancelled, Failed, Succeeded };
using JobOutput=std::variant<ProposalId,ClipId>; // Dataset/training jobs belong to their own contract.
struct JobInfo { JobId id; JobState state; std::string stage; std::optional<double> progress;
    std::vector<SnapshotInfo> inputs; ContentDigest manifest; std::vector<JobOutput> outputs;
    std::vector<Diagnostic> diagnostics; bool published{}; };
struct ProposalInfo { ProposalId id; SnapshotInfo base; ContentDigest manifest;
    std::vector<Residual> residuals; std::vector<Diagnostic> diagnostics; };
struct ApplyProposalRequest { ProposalId proposal; Revision expected_revision;
    LayerId destination_layer; bool preserve_hard_keys{true}; };
struct PublishRequest { JobId job; ContentDigest expected_manifest; };

// All methods are abstract declarations. An engine-specific adapter must implement them.
// Input references/spans are borrowed for the call; async jobs must own validated copies.
class IAnimationTools {
public:
    virtual ~IAnimationTools() = default;
    virtual Result<DocumentInfo> create_document(Timebase, TickRange)=0;
    virtual Result<DocumentInfo> read_document(DocumentId)=0;
    virtual Result<TransactionId> begin_edit(DocumentId, Revision expected)=0;
    virtual Result<DocumentInfo> commit_edit(TransactionId)=0;
    virtual Outcome rollback_edit(TransactionId)=0;
    virtual Result<DocumentInfo> undo(DocumentId, Revision expected)=0;
    virtual Result<DocumentInfo> redo(DocumentId, Revision expected)=0;
    virtual Result<SnapshotInfo> freeze(DocumentId, Revision expected)=0;
    virtual Outcome release_snapshot(SnapshotId)=0;
    virtual Result<DocumentAssets> list_assets(SnapshotId)=0;
    virtual Result<std::vector<JointRecord>> list_joints(SnapshotId, SkeletonId)=0;

    virtual Result<SkeletonId> create_skeleton(TransactionId, const std::string& name)=0;
    virtual Result<JointId> create_joint(TransactionId, const JointDescription&)=0;
    virtual Outcome reparent_joint(TransactionId, const ReparentRequest&)=0;
    virtual Outcome set_joint_rest(TransactionId, JointId, const Transform&, RestChange)=0;
    virtual Outcome set_joint_limits(TransactionId, JointId, const JointLimits&)=0;
    virtual Outcome set_joint_locks(TransactionId, JointId, TickRange, DofLocks)=0;
    virtual Outcome remove_joint(TransactionId, JointId, RemovalPolicy, std::span<const JointRemap>)=0;
    virtual Result<MeshId> create_mesh(TransactionId, const MeshDescription&)=0;
    virtual Outcome replace_mesh_geometry(TransactionId, MeshId, const MeshGeometry&, RemovalPolicy,
                                          std::span<const VertexRemap>)=0;
    virtual Result<BindingId> create_binding(TransactionId, const BindingDescription&)=0;
    virtual Outcome edit_weights(TransactionId, BindingId, std::span<const WeightEdit>)=0;
    virtual Outcome initialize_skin(TransactionId, const SkinInitialization&)=0;

    virtual Result<RigId> create_rig(TransactionId, const RigDescription&)=0;
    virtual Result<RigPlanInfo> compile_rig(SnapshotId, RigId)=0;
    virtual Result<ChainId> create_chain(TransactionId, const ChainDescription&)=0;
    virtual Result<ControlId> create_control(TransactionId, const ControlDescription&)=0;
    virtual Result<ConstraintId> create_constraint(TransactionId, const ConstraintDescription&)=0;
    virtual Outcome set_constraint_policy(TransactionId, ConstraintId, ConstraintPolicy)=0;
    virtual Result<SwitchReport> switch_chain_mode(TransactionId, const SwitchRequest&)=0;
    virtual Outcome set_control_target(TransactionId, LayerId, const KinematicTarget&)=0;
    virtual Result<BodyId> create_body(TransactionId, const BodyDescription&)=0;
    virtual Outcome update_body(TransactionId, BodyId, const BodyDescription&)=0;
    virtual Outcome set_motor(TransactionId, const MotorDescription&)=0;
    virtual Result<ContactId> create_contact(TransactionId, const ContactDescription&)=0;
    virtual Outcome update_contact(TransactionId, ContactId, const ContactDescription&)=0;
    virtual Result<LimbId> create_limb(TransactionId, const LimbDescription&)=0;
    virtual Outcome update_limb(TransactionId, LimbId, const LimbDescription&)=0;

    virtual Result<LayerId> create_layer(TransactionId, const LayerDescription&)=0;
    virtual Outcome update_layer(TransactionId, LayerId, const LayerDescription&)=0;
    virtual Outcome set_layer_order(TransactionId, std::span<const LayerId>)=0;
    virtual Result<ChannelId> create_channel(TransactionId, const ChannelDescription&)=0;
    virtual Outcome update_channel(TransactionId, ChannelId, const ChannelDescription&)=0;
    virtual Result<KeyId> insert_key(TransactionId, const KeyDescription&)=0;
    virtual Outcome replace_key(TransactionId, KeyId, const KeyDescription&)=0;
    virtual Outcome move_key(TransactionId, KeyId, Tick destination)=0;
    virtual Outcome remove_key(TransactionId, KeyId)=0;
    virtual Outcome set_key_lock(TransactionId, KeyId, bool locked)=0;
    virtual Outcome set_channel_lock(TransactionId, const ChannelLock&)=0;
    virtual Outcome retime(TransactionId, const RetimeRequest&)=0;
    virtual Result<EventId> insert_event(TransactionId, const EventDescription&)=0;

    virtual Result<Evaluation> evaluate(const EvaluationRequest&)=0;
    virtual Result<BindingEvaluation> deform_binding(SnapshotId, BindingId, const Evaluation&)=0;
    virtual Result<ModelCapabilities> model_capabilities(ModelId)=0;
    virtual Result<JobId> enqueue(const JobSpec&)=0;
    virtual Result<JobInfo> read_job(JobId)=0;
    virtual Outcome cancel_job(JobId)=0;
    virtual Result<ProposalInfo> read_proposal(ProposalId)=0;
    virtual Outcome apply_proposal(TransactionId, const ApplyProposalRequest&)=0;
    virtual Outcome publish(const PublishRequest&)=0;
};

} // namespace JE::AnimationTools
