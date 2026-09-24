#pragma once
#include <Jerico/Jobs/Cancellation.hpp>
#include <Jerico/ProceduralAuthoring/Geometry.hpp>
#include <array>
#include <filesystem>
#include <string>
#include <vector>

namespace Playground
{
namespace PA = Jerico::Tools::ProceduralAuthoring;
using PA::TriangleMesh;
using PA::Vec3d;
enum class Base
{
    Cube,
    Sphere,
    Cylinder,
    Cone,
    Plane,
    BspWall
};
enum class Kind
{
    Transform,
    NormalNoise,
    VectorNoise,
    Voronoi,
    Layers,
    Warp,
    Fracture,
    Relax,
    Planes,
    SharedWarp,
    Envelope,
    EdgeBreakup,
    StrataRelief,
    RidgedNoise,
    Solidify,
    Reduce,
    SurfaceDamage,
    DamageDecal,
    LayerSettle,
    CliffCutLoop
};
enum class Mask
{
    None,
    Height,
    Slope,
    Noise,
    Direction,
    Radial,
    Gradient,
    Curvature,
    EdgeProximity,
    Contact,
    Convexity,
    Concavity,
    AmbientOcclusion,
    Exposure,
    BoundaryProximity,
    Thickness
};
struct Layer
{
    Kind kind{Kind::NormalNoise};
    bool enabled{true};
    std::uint32_t seed{123};
    float amplitude{0.08f}, frequency{2.0f}, lacunarity{2}, roughness{0.5f};
    int octaves{3};
    std::array<float, 3> coordinateScale{1, 1, 1}, scale{1, 1, 1}, rotation{}, translation{};
    std::array<float, 3> outputScale{1, 1, 1};
    std::array<float, 3> direction{0, 1, 0};
    std::array<float, 3> maskAxis{}; // Zero inherits the operation direction for older recipes.
    int voronoiMode{2};
    float randomness{1}, threshold{0.1f}, width{0.16f};
    int count{4};
    float thickness{0.36f}, spacing{0.06f}, layerScale{0.035f}, jitter{0.15f}, lateral{0.18f};
    float perimeterAmplitude{};
    float beddingTilt{};
    bool shapeOuterCaps{false};
    int iterations{3};
    float relaxStrength{0.25f};
    Mask mask{Mask::None};
    float maskLow{-0.5f}, maskHigh{0.5f};
    bool maskInvert{false};
    bool coherentLayers{false};
    float thicknessFrequency{0.8f}, thicknessCorrelation{0.8f};
    float minimumThickness{0.08f}, maximumThickness{0.8f};
    float driftCorrelation{0.85f}, rotationDrift{}, scaleDrift{};
    std::array<float, 3> driftBias{};
    int envelopeShape{}; // 0 ellipsoid, 1 box, 2 rounded box, 3 cylinder
    float envelopeRoundness{0.25f};
    int cutSubdivisions{2};
    float phase{}, profileSharpness{2}, distortion{0.2f};
    int profile{0};       // sine, ledge, triangle
    float solidOffset{0}; // -1 back, 0 centered, 1 front
    bool solidAlongNormals{true};
    float reductionError{0.02f};
    float featureRadius{0.3f}, featureAngle{30};
    int contactSamples{16};
    std::array<float, 2> projectorRadius{0.6f, 0.45f};
    float projectorRotation{};
    float damageLip{0.025f};
};
struct Recipe
{
    int version{10};
    Base base{Base::Cube};
    int previewResolution{20}, finalResolution{44};
    std::array<float, 2> planeSize{6, 4};
    std::vector<Layer> layers;
};
enum class SurfaceRole
{
    Source,
    Stratum,
    PlaneCut,
    EnvelopeCut,
    SolidBack,
    SolidRim,
    CutLoop,
    CutFace
};
struct RegionInfo
{
    Jerico::Guid id{}, parent{};
    SurfaceRole role{SurfaceRole::Source};
    int operation{-1}, stratum{-1};
};
struct StageTiming
{
    int operation{};
    Kind kind{};
    double milliseconds{};
    std::size_t inputVertices{}, outputVertices{}, outputTriangles{};
};
struct MeshResult
{
    TriangleMesh mesh;
    std::vector<Vec3d> normals;
    std::vector<Vec3d> cornerNormals;
    bool hardRegions{false};
    std::vector<float> field;
    std::vector<int> layerIndices;
    std::vector<float> layerThickness;
    std::vector<float> layerEdgeDistance;
    std::vector<unsigned char> layerContactFaces;
    std::vector<RegionInfo> regions;
    std::vector<StageTiming> stages;
    double milliseconds{};
    bool closedSolid{true};
    std::vector<Jerico::Guid> sourceSelection;
    std::uint64_t sourceRevision{};
    double sampledReductionError{};
    std::size_t trianglesBeforeReduction{};
};
const char *name(Kind kind);
Layer defaultLayer(Kind kind);
Recipe preset(int index);
void validate(const Recipe &recipe);
MeshResult evaluate(const Recipe &recipe, int resolution, int stopAfter = -1, int fieldLayer = -1,
                    const Jerico::CancellationToken *cancel = nullptr);
MeshResult evaluateSource(const Recipe &recipe, const MeshResult &source, int resolution, int stopAfter = -1,
                          int fieldLayer = -1, const Jerico::CancellationToken *cancel = nullptr);
std::string serialize(const Recipe &recipe);
Recipe deserialize(const std::string &text);
void save(const Recipe &recipe, const std::filesystem::path &path);
Recipe load(const std::filesystem::path &path);
std::uint64_t meshHash(const TriangleMesh &mesh);
void normals(MeshResult &mesh);
} // namespace Playground
