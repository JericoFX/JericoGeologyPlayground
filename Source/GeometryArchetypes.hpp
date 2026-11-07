// SPDX-License-Identifier: GPL-3.0-only
#pragma once
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
    std::size_t vertexCount{},triangleCount{},degenerateTriangles{},componentCount{},supportCount{};
    double groundedAreaFraction{},verticalPanelFraction{},horizontalPanelFraction{},verticalCoverage{},supportSpread{1};
    Vec3 thinAxis{}; // Object-space normal of the panel plane (smallest principal extent).
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
}
