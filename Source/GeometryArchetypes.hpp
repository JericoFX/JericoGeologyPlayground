// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

// Portable classification of a mesh into a motion archetype. Pure C++20: no
// renderer, UI, asset path or Jolt dependency. The caller supplies geometry and
// receives measured evidence plus ranked hypotheses; it never receives a name.
namespace JE::GeometryArchetypes {
struct Vec3 {double x{},y{},z{};};
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,double s){return {a.x*s,a.y*s,a.z*s};}
inline double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline double length(Vec3 a){return std::sqrt(dot(a,a));}

enum class Class {Unknown,Biped,Quadruped,Arthropod,Axial,HingedPanel,Drawer,RigidFrame};
const char* name(Class);

struct Settings {
    // Every threshold is relative to the mesh diagonal so scans of any size use
    // the same rule. They are analysis hypotheses, never changes to the input.
    double groundBandRelative{0.03};
    double planarAngleRadians{0.18};
    double planarOffsetRelative{0.01};
    double panelMinimumAreaRelative{0.02};
    double symmetryCellRelative{0.01};
    double supportClusterRelative{0.04};
    std::size_t maximumVertices{2'000'000},maximumTriangles{4'000'000};
};

struct Panel {
    Vec3 normal{},centre{};
    double offset{},area{};
    Vec3 minimum{},maximum{};
    Vec3 longestEdgeDirection{};
    double longestEdgeMeters{};
    std::size_t triangles{};
};

// A kinematic candidate extracted from the mesh: a straight border run (revolute)
// or a face travel (prismatic). It is a geometric candidate with a measured length,
// never a completed rig.
struct Joint {
    Vec3 point{},axis{};
    double lengthMeters{},score{};
    bool prismatic{};
};

// A rectangular step in the dominant face: the drawer-front signature. A wardrobe
// shell has none; a flush door leaf has a step that spans the full height.
struct FaceStep {
    double areaFraction{},fillRatio{},heightFraction{},depthMeters{};
    bool rectangular{};
};

// Reconstructed drawer: the front is measured, the cavity behind it is measured,
// and the travel is derived from the cavity, not guessed from the asset name.
struct DrawerBox {
    bool cavity{};
    Vec3 axis{};
    double width{},height{},depth{},travel{},faceFraction{};
};

struct Evidence {
    Vec3 minimum{},maximum{},extent{},centroid{};
    double diagonal{},area{},boxiness{},planarAreaFraction{},thinness{1},elongation{1},symmetryError{1};
    std::size_t vertexCount{},triangleCount{},degenerateTriangles{},componentCount{},supportCount{},groundedSupportCount{};
    double groundedAreaFraction{},verticalPanelFraction{},horizontalPanelFraction{},verticalCoverage{},supportSpread{1};
    Vec3 thinAxis{}; // Object-space normal of the panel plane (smallest principal extent).
    std::vector<Vec3> supportPoints; // Grounded contacts, in object space.
    std::size_t regionCount{};
    double largestRegionFraction{}; // Area of the largest merged planar region.
    std::vector<Panel> panels;
    std::vector<Joint> joints;
    std::vector<FaceStep> faceSteps;
    double faceStepDepth{},faceStepCoverage{},faceStepOffsetFraction{};
    DrawerBox drawer;
};

struct Hypothesis {Class type{Class::Unknown};double score{};std::string reason;};

struct Report {
    Evidence evidence;
    std::vector<Hypothesis> ranked; // Descending score; the first entry is the accepted hypothesis.
    bool ambiguous{};
};

Report classify(std::span<const Vec3>,std::span<const std::uint32_t>,const Settings& = {});

// Skeleton hypothesis: one bone chain per grounded contact, following the surface
// geodesic from the body to the contact. Geometry only; no skinning, no name lookup.
struct Bone {
    std::size_t parent{};
    Vec3 head{},tail{};
    double length{};
};
struct Chain {
    std::vector<std::size_t> bones;
    double length{};
    std::size_t contact{}; // Index into Evidence::supportPoints, or noContact.
};
inline constexpr std::size_t noContact=static_cast<std::size_t>(-1);
struct Skeleton {
    Vec3 root{};
    std::vector<Bone> bones;
    std::vector<Chain> chains;
    double totalLength{};
};
// jointsPerChain counts bones per limb; an axial body uses the geodesic diameter.
Skeleton skeleton(std::span<const Vec3>,std::span<const std::uint32_t>,const Report&,
                  std::size_t jointsPerChain = 4,const Settings& = {});

// Linear blend skinning over the skeleton hypothesis. Weights are distances to
// bone segments, so no anatomical naming and no rig import are involved.
struct Skin {
    std::vector<std::array<std::uint32_t,4>> bones;
    std::vector<std::array<double,4>> weights;    // Each row sums to 1.
};
struct BoneTransform {
    std::array<double,9> rotation{{1,0,0,0,1,0,0,0,1}}; // Row major.
    // A solved chain moves each joint, so the transform needs the posed head too:
    // p' = origin + R * (p - bone.headRest). The rest pose leaves this unused.
    Vec3 head{};
    bool usePosedHead{};
};

struct IkSettings {
    // Anatomical budget: a knee or elbow should not fold past the given interior angle.
    // 150 degrees is a declared default, not a measured human value.
    double maximumJointAngleRadians{2.6179938779914944}; // 150 degrees.
    bool enforceJointLimits{true};
    std::size_t limitPasses{6};
};
struct IkResult {
    bool reachable{};
    double error{};            // Distance from the solved tip to the target.
    double totalLength{};
    double maximumJointAngle{}; // Largest interior angle reached by any joint.
    std::size_t limitPassesUsed{};
    std::vector<Vec3> joints;  // chain.bones.size() + 1 joint positions.
    std::vector<BoneTransform> transforms;
};
// Forward and backward reaching IK over one chain, with an interior-angle limit.
IkResult solveIk(const Skeleton&,std::size_t chain,Vec3 target,const IkSettings& = {});
IkResult solveIk(const Skeleton&,std::size_t chain,Vec3 target,std::size_t iterations,double tolerance);
Skin skin(std::span<const Vec3>,const Skeleton&,std::size_t maximumInfluences = 4);
struct DeformSettings {
    // Linear blend skinning is the default and the measured one: at 90 degrees it pinches the
    // joint band by 8.6 % (humanoid) and 1.8 % (spider). The dual quaternion path is kept as an
    // experiment and is NOT adopted: as implemented it expands the band by 60 %, so do not
    // enable it without re-deriving the blend and measuring again.
    bool dualQuaternion{};
};
// Identity transforms must reproduce the input exactly; that is the rest-pose gate.
std::vector<Vec3> deform(std::span<const Vec3>,const Skin&,const Skeleton&,std::span<const BoneTransform>,
                         const DeformSettings& = {});
}
