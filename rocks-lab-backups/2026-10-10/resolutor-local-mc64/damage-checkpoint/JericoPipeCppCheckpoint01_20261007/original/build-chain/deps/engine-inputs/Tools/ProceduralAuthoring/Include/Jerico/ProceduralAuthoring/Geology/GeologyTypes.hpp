#pragma once

#include <Jerico/ProceduralAuthoring/Geometry.hpp>

#include <array>
#include <cstdint>

namespace Jerico::Tools::ProceduralAuthoring
{
enum class GeologyLayerKind : std::uint16_t
{
    Source = 1U,
    MacroWarp = 2U,
    Noise = 3U,
    Rebuild = 4U,
    Strata = 5U,
    CliffShelf = 6U,
    Fracture = 7U,
    SurfaceWeathering = 8U,
    HeightfieldErosion = 9U,
    CaveSweep = 10U,
    PlaneCut = 11U,
    DirectionalWarp = 12U,
    FaultDisplace = 13U,
    AnisotropicVoronoiCells = 14U,
    FractureCells = 15U,
    CavityCarve = 16U,
    ImplicitBlock = 17U,
    JointSet = 18U,
    FoldDomain = 19U,
    GradientDetailWarp = 20U,
    DirectedCrack = 21U,
    DurabilityField = 22U,
    Rockfall = 23U,
    Colluvium = 24U,
    ContactPreservingErosion = 25U,
    NarrowBandMineralWeathering = 26U,
    MoistureTransport = 27U,
    MineralDissolution = 28U,
    MultiScaleMicrostructure = 29U
};

enum class GeologyBlockArchetype : std::uint8_t
{
    Tabular = 1U,
    Columnar = 2U,
    Equidimensional = 3U,
    Rhombohedral = 4U,
    IrregularPolyhedral = 5U
};

enum class GeologySourceKind : std::uint8_t
{
    BspBrush = 1U,
    BspGroup = 2U,
    BspSelection = 3U,
    BlockGrammar = 4U,
    AperiodicTiling = 5U
};

enum class GeologyBlockGrammarSymbol : std::uint8_t
{
    Empty = 0U,
    Rule1 = 1U,
    Rule2 = 2U,
    Rule3 = 3U,
    Rule4 = 4U
};

enum class GeologyNoiseKind : std::uint8_t
{
    OpenSimplex2 = 1U,
    Perlin = 2U,
    Cellular = 3U,
    ValueLatticeV1 = 4U
};

enum class GeologyFractalKind : std::uint8_t
{
    None = 1U,
    Fbm = 2U,
    Ridged = 3U
};

enum class GeologyNoiseDisplacement : std::uint8_t
{
    SurfaceNormal = 1U,
    Vector = 2U
};

struct GeologyTransform
{
    Vec3d translation;
    std::array<double, 4U> rotation{0.0, 0.0, 0.0, 1.0};
    Vec3d scale{1.0, 1.0, 1.0};

    bool operator==(const GeologyTransform&) const = default;
};

struct GeologyLinearRemap
{
    double inputMinimum{-1.0};
    double inputMaximum{1.0};
    double outputMinimum{-1.0};
    double outputMaximum{1.0};

    bool operator==(const GeologyLinearRemap&) const = default;
};
}
