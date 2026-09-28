#pragma once

#include "SplineConstruction.hpp"
#include "DamageField.hpp"

#include <cstdint>
#include <iosfwd>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Playground { struct AutoUvPlan; }

namespace Playground::Assembly
{
inline constexpr std::size_t maximumDamageAreas=64;
enum class Kind
{
    PipeCouplings, Railway, BrokenRailway, Railing, BrokenRailing,
    WoodenPier, DamagedPier, Catwalk, BeamBridge, TrussWarren,
    TrussPratt, TrussHowe, SuspensionBridge, RopePlankBridge
};
enum class Role
{
    PipeBody, Coupling, Clamp, Flange, Support, Rail, Sleeper, RailJoint,
    Post, TopRail, MiddleRail, LowerRail, Baluster, DeckPlank, Beam,
    CrossBeam, Pile, Brace, Fascia, Girder, Tower, Cable, Hanger,
    HandRope, Anchor
};
enum class State
{
    Intact, Missing, Broken, Bent, Loose, Detached, Decayed, Collapsed
};
enum class SelectorMode { IndexRange, DistanceRange };
struct DamageRule
{
    SelectorMode mode{SelectorMode::IndexRange};
    Role role{Role::DeckPlank};
    std::uint32_t firstIndex{}, lastIndex{};
    double startMeters{}, endMeters{};
    State state{State::Missing};
};
enum class AreaOperation { State, Cavity, Dent, ThroughHole, EdgeDamage };
struct AreaDamageRule
{
    DamageArea area;
    State state{State::Broken};
    AreaOperation operation{AreaOperation::State};
    DamageEdgeSettings edge;
};
struct PathDamageRule
{
    std::string id;
    std::string semanticPartId;
    DamagePathCutter cutter;
};
struct DamageSettings
{
    bool proceduralEnabled{};
    std::uint32_t seed{123};
    double missingProbability{}, brokenProbability{}, looseProbability{}, bentProbability{};
    double clusterCenterMeters{}, clusterRadiusMeters{}, clusterStrength{};
    std::vector<DamageRule> rules;
    std::vector<AreaDamageRule> areaRules;
    std::vector<PathDamageRule> pathRules;
    std::unordered_map<std::string, State> manual;
    std::unordered_set<std::string> protectedIds;
};
struct Settings
{
    Kind kind{Kind::PipeCouplings};
    Construction::PathQuality quality;
    int lod{};
    double widthMeters{2.4}, heightMeters{1.0}, gaugeMeters{1.435};
    double sleeperSpacingMeters{0.65}, sleeperLengthMeters{2.6};
    double sleeperWidthMeters{0.24}, sleeperHeightMeters{0.17};
    double boardWidthMeters{0.24}, boardThicknessMeters{0.06}, boardGapMeters{0.02};
    double beamWidthMeters{0.16}, beamHeightMeters{0.22};
    double supportSpacingMeters{2.0}, baySpacingMeters{1.5};
    double couplingSpacingMeters{1.0}, couplingWidthMeters{0.12};
    double couplingRadialThicknessMeters{0.025}, pipeRadiusMeters{0.08};
    double couplingStartPaddingMeters{}, couplingEndPaddingMeters{};
    double couplingVariationMeters{};
    std::uint32_t variationSeed{123};
    double railWidthMeters{0.07}, railHeightMeters{0.14};
    double spanMeters{12.0}, towerHeightMeters{3.5}, sagMeters{1.2};
    double hangerSpacingMeters{1.0}, cableRadiusMeters{0.035};
    double irregularityMeters{0.02};
    bool couplingsEnabled{true}, railJointsEnabled{true}, lowerRailEnabled{};
    DamageSettings damage;
    std::size_t maximumParts{4096}, maximumVertices{400000};
};
struct Part
{
    std::string id;
    Jerico::Guid guid;
    Role role{};
    std::uint32_t index{};
    int side{};
    double stationMeters{};
    Vec3d position{};
    State state{State::Intact};
    bool protectedPart{};
    std::size_t firstTriangle{}, triangleCount{};
    std::vector<Jerico::Guid> freshInteriorRegions;
};
struct FitResult
{
    std::size_t count{};
    double actualWidthMeters{}, gapMeters{};
};
struct Result
{
    Construction::PathResult path;
    MeshResult mesh;
    std::vector<std::array<float, 2>> cornerUV;
    std::vector<Part> parts;
    std::size_t repeatedStations{}, promotedBodyCount{}, constraintCount{};
    double generationCpuMilliseconds{}, damageEditCpuMilliseconds{}, pathEditCpuMilliseconds{};
};

const char *name(Kind kind);
const char *name(Role role);
const char *name(State state);
bool stateAllowed(Role role, State state);
std::string stablePartId(Kind kind, Role role, std::uint32_t index, int side = 0);
Jerico::Guid stablePartGuid(const std::string &id);
State resolveDamage(const DamageSettings &settings, const Part &part);
FitResult fitCountSpacing(double lengthMeters, double preferredWidthMeters, double gapMeters);
Construction::PathResult offsetPath(const Construction::PathResult &source,
                                    double normalOffsetMeters, double binormalOffsetMeters);
double parabolicSag(double xMeters, double spanMeters, double sagMeters);
double solveCatenaryA(double spanMeters, double sagMeters);
double catenaryHeight(double xMeters, double spanMeters, double sagMeters, double aMeters);
Result build(const PA::ProceduralSpline &spline, const Settings &settings);
void writeBakeObj(std::ostream &stream,const Result &result,const AutoUvPlan &atlas);
std::optional<Vec3d> raycastPartSurface(const Result &result,const Part &part,
                                        Vec3d origin,Vec3d direction);
}
