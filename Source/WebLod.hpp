#pragma once

#include "SplineConstruction.hpp"

#include <array>
#include <atomic>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <vector>

namespace Playground::Web
{
enum class Preset { Orb, Corner, Messy, Old, Dense, Trap };
enum class Authoring { Plane, Anchors, Area };
enum class EdgeState { Intact, Missing, Broken, Disabled };
enum class Style { Heavy, Medium, Ghost };
struct Optics
{
    float roughness{.65f},brightness{1},wetness{},frost{},dust{},distanceSoftness{1};
    float baseVisibility{1},specularResponse{.6f},lightRevealStrength{},
          viewAngleResponse{},dropletDensity{},dropletSizeMin{.008f},
          dropletSizeMax{.016f};
    unsigned dropletSeed{123};
    std::array<float,3> lightPosition{-1.5f,1.4f,-2.0f};
    float lightReach{5};
    float shaderOnlyDistance{30},cullDistance{50};
};
struct Settings
{
    Preset preset{Preset::Orb};
    Authoring authoring{Authoring::Plane};
    Style style{Style::Heavy};
    Optics optics;
    std::uint32_t seed{123};
    unsigned spokes{12},rings{5};
    double widthMeters{3.0},heightMeters{3.0};
    Vec3d center{},axisU{1,0,0},axisV{0,1,0};
    std::vector<Vec3d> anchors;
    std::vector<Vec3d> area;
    double density{1},secondaryStrands{.15},irregularity{.12};
    double microStrandDensity{};
    double missingFraction{},sagMeters{},hubOffsetU{},hubOffsetV{};
    double radiusMeters{.012},warpMeters{};
    double maximumPlaneErrorMeters{.03},maximumRmsErrorMeters{.01};
    double cardPaddingMeters{.10},minimumRadiusPixels{.6};
    double normalStrength{.12};
    unsigned resolution{256},supersamples{4},maximumCards{4};
    unsigned physicalEdgeBudget{32};
    unsigned structuralStrandCount{12};
    double gravityInfluence{1};
    double geometryDistance{3},hybridDistance{10},shaderOnlyDistance{30},
           cullDistance{50};
    std::unordered_set<std::string> brokenEdges;
};
struct Edge
{
    std::string id;
    std::uint32_t start{},end{};
    unsigned patch{};
    double radiusMeters{};
    EdgeState state{EdgeState::Intact};
    Vec3d sagPoint{};
    bool sagged{};
};
struct Graph
{
    std::vector<Vec3d> nodes;
    std::vector<std::string> nodeIds;
    std::vector<Edge> edges;
    std::vector<std::vector<Vec3d>> areaMasks;
};
struct CardPatch
{
    Vec3d center{},axisU{},axisV{},normal{};
    double minU{},maxU{},minV{},maxV{};
    double maximumPlaneErrorMeters{},rmsPlaneErrorMeters{};
    unsigned index{};
};
struct TextureLevel
{
    unsigned width{},height{};
    std::vector<std::uint8_t> coverageRgba,normalRgba,distanceRgba;
    double occupiedFraction{},minimumRepresentedWidthPixels{};
};
struct Result
{
    Graph graph;
    MeshResult source,ribbons,card,coarsePhysical;
    MeshResult structural;
    std::vector<std::array<float,2>> sourceUV,ribbonUV,cardUV,coarsePhysicalUV,
                                      structuralUV;
    std::vector<std::string> physicalEdgeIds;
    std::vector<CardPatch> patches;
    std::vector<TextureLevel> mips;
    bool cardAccepted{};
    std::string fallbackReason;
    double bakeCpuMilliseconds{},minimumRadiusMeters{};
    std::size_t textureBytes{},activeEdgeCount{},sourceRenderedEdges{};
    unsigned bakeResolution{},bakeSupersamples{};
    bool bakeBudgetReduced{},sourceTruncated{};
};

Graph makeGraph(const Settings &settings);
CardPatch fitPlane(const Graph &graph,unsigned patch,double paddingMeters);
std::uint32_t edgeToken(const std::string &id);
double segmentPhi(std::array<double,2> sample,std::array<double,2> start,
                  std::array<double,2> end,double radiusMeters);
Result build(const Settings &settings,bool geometryDebug=false,
             const std::atomic_bool *cancelled=nullptr);
std::uint64_t textureHash(const TextureLevel &level);
}
