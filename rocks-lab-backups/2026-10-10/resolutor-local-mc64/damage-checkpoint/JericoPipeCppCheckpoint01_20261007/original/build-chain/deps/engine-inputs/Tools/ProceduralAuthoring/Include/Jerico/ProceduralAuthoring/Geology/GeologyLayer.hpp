#pragma once

#include <Jerico/ProceduralAuthoring/Geology/GeologyMask.hpp>
#include <Jerico/ProceduralAuthoring/Geology/GeologyTypes.hpp>

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace Jerico::Tools::ProceduralAuthoring
{
struct GeologyBlockGrammarRule
{
    std::array<GeologyBlockGrammarSymbol, 8U> children{};

    bool operator==(const GeologyBlockGrammarRule&) const = default;
};

struct GeologyBlockGrammarSourceDefinition
{
    Vec3d rootHalfExtentMeters{1.0, 1.0, 1.0};
    GeologyBlockGrammarSymbol axiom{GeologyBlockGrammarSymbol::Rule1};
    std::array<GeologyBlockGrammarRule, 4U> rules{};
    std::uint32_t rewriteDepth{1U};
    bool implode{};

    bool operator==(const GeologyBlockGrammarSourceDefinition&) const = default;
};

struct GeologyAperiodicTilingSourceDefinition
{
    Vec3d rootHalfExtentMeters{1.0, 1.0, 1.0};
    std::array<std::uint32_t, 3U> tileCounts{1U, 1U, 1U};

    bool operator==(const GeologyAperiodicTilingSourceDefinition&) const = default;
};

struct GeologySourcePayload
{
    GeologySourceKind kind{GeologySourceKind::BspGroup};
    Guid documentGuid;
    Guid sourceGuid;
    std::uint64_t sourceRevision{};
    std::uint64_t sourceContentHash{};
    std::vector<Guid> selectedBrushGuids;
    GeologyBlockGrammarSourceDefinition blockGrammar;
    GeologyAperiodicTilingSourceDefinition aperiodicTiling;

    bool operator==(const GeologySourcePayload&) const = default;
};

struct GeologyMacroWarpPayload
{
    Vec3d frequencyPerMeter{1.0, 1.0, 1.0};
    Vec3d amplitudeMeters;

    bool operator==(const GeologyMacroWarpPayload&) const = default;
};

struct GeologyNoisePayload
{
    GeologyNoiseKind noise{GeologyNoiseKind::OpenSimplex2};
    GeologyFractalKind fractal{GeologyFractalKind::None};
    Vec3d frequencyPerMeter{1.0, 1.0, 1.0};
    double amplitudeMeters{1.0};
    std::uint32_t octaves{1U};
    double lacunarity{2.0};
    double gain{0.5};
    double domainWarpAmplitudeMeters{};
    Vec3d domainWarpFrequencyPerMeter{1.0, 1.0, 1.0};
    GeologyNoiseDisplacement displacement{
        GeologyNoiseDisplacement::SurfaceNormal};
    double surfaceBandMeters{1.0};
    double clampMeters{1.0};
    GeologyLinearRemap remap;

    bool operator==(const GeologyNoisePayload&) const = default;
};

struct GeologyRebuildPayload
{
    double halfBandWidthVoxels{3.0};

    bool operator==(const GeologyRebuildPayload&) const = default;
};

struct GeologyPlaneCutPayload
{
    Vec3d normalWorld{0.0, 1.0, 0.0};
    double offsetMeters{};
    double roughnessMeters{};
    Vec3d roughnessFrequencyPerMeter{1.0, 1.0, 1.0};

    bool operator==(const GeologyPlaneCutPayload&) const = default;
};

struct GeologyDirectionalWarpPayload
{
    Vec3d directionWorld{1.0, 0.0, 0.0};
    double amplitudeMeters{};
    Vec3d frequencyPerMeter{1.0, 1.0, 1.0};
    std::uint32_t octaves{1U};
    double lacunarity{2.0};
    double gain{0.5};

    bool operator==(const GeologyDirectionalWarpPayload&) const = default;
};

struct GeologyFaultDisplacePayload
{
    Vec3d faultNormalWorld{0.0, 1.0, 0.0};
    double planeOffsetMeters{};
    Vec3d slipDirectionWorld{1.0, 0.0, 0.0};
    double displacementMeters{};
    double transitionHalfWidthMeters{0.1};
    double boundaryRoughnessMeters{};
    Vec3d roughnessFrequencyPerMeter{1.0, 1.0, 1.0};

    bool operator==(const GeologyFaultDisplacePayload&) const = default;
};

struct GeologyAnisotropicVoronoiCell
{
    Vec3d positionWorld;
    std::array<double, 4U> localToWorldRotation{0.0, 0.0, 0.0, 1.0};
    Vec3d metricWeights{1.0, 1.0, 1.0};
    double relativeScale{1.0};

    bool operator==(const GeologyAnisotropicVoronoiCell&) const = default;
};

struct GeologyAnisotropicVoronoiPayload
{
    std::vector<GeologyAnisotropicVoronoiCell> cells;

    bool operator==(const GeologyAnisotropicVoronoiPayload&) const = default;
};

enum class GeologyFractureCellsMode : std::uint8_t
{
    Carve = 1U,
    Weakening = 2U
};

struct GeologyFractureCellsPayload
{
    double widthMeters{0.05};
    double softnessMeters{0.05};
    GeologyFractureCellsMode mode{GeologyFractureCellsMode::Carve};

    bool operator==(const GeologyFractureCellsPayload&) const = default;
};

struct GeologyStrataPayload
{
    Vec3d directionWorld{0.0, 1.0, 0.0};
    double spacingMeters{1.0};
    double phaseOffsetCycles{};
    double warpAmplitudeMeters{};
    Vec3d warpFrequencyPerMeter{0.1, 0.1, 0.1};
    double profileMinimum{0.2};
    double profileMaximum{0.8};
    double shapeDisplacementMeters{};
    double grooveDepthMeters{};
    double grooveWidth{0.1};
    double surfaceBandMeters{1.0};
    double materialClassMinimum{};
    double materialClassMaximum{1.0};
    double hardnessMinimum{1.0};
    double hardnessMaximum{1.0};
    double erodibilityMinimum{};
    double erodibilityMaximum{};

    bool operator==(const GeologyStrataPayload&) const = default;
};

struct GeologyCliffShelfPayload
{
    Vec3d faceDirectionWorld{0.0, 0.0, 1.0};
    double minimumFacingDot{0.2};
    double maximumFacingDot{0.8};
    Vec3d coordinateDirectionWorld{0.0, 1.0, 0.0};
    double terraceSpacingMeters{1.0};
    double phaseOffsetCycles{};
    double terraceWidth{0.3};
    double shelfDepthMeters{};
    double undercutDepthMeters{};
    double undercutWidth{0.2};
    double surfaceBandMeters{1.0};
    double irregularityAmplitudeMeters{};
    Vec3d irregularityFrequencyPerMeter{0.1, 0.1, 0.1};
    double depthModulation{0.25};

    bool operator==(const GeologyCliffShelfPayload&) const = default;
};

struct GeologyFracturePayload
{
    Vec3d orientationWorld{1.0, 0.0, 0.0};
    double spacingMeters{1.0};
    double widthMeters{0.05};
    double depthMeters{0.10};
    double density{0.5};
    double blend{0.5};
    double surfaceDepthMeters{0.35};
    double minimumWallThicknessMeters{0.20};

    bool operator==(const GeologyFracturePayload&) const = default;
};

struct GeologyLegacySurfaceWeatheringPayload
{
    Vec3d gravityWorld{0.0, -1.0, 0.0};
    Vec3d windWorld{1.0, 0.0, 0.0};
    Vec3d weatherFrequencyPerMeter{0.2, 0.2, 0.2};
    double maximumWearMeters{0.10};
    double surfaceBandMeters{0.35};
    double baseWear{0.10};
    double slopeResponse{0.25};
    double curvatureResponse{0.25};
    double exposureResponse{0.25};
    double fractureResponse{0.25};
    double hardnessResponse{0.25};
    double erodibilityResponse{0.25};
    double gravityTangentResponse{0.25};
    double windTangentResponse{0.25};
    double tangentTraceMeters{0.50};

    bool operator==(const GeologyLegacySurfaceWeatheringPayload&) const = default;
};

struct GeologyBubbleWeatheringPayload
{
    double exposureRadiusMeters{0.10};
    double maximumWearMetersPerIteration{0.025};
    double durability{};
    std::uint32_t iterations{1U};

    bool operator==(const GeologyBubbleWeatheringPayload&) const = default;
};

enum class GeologyCurvatureWeatheringMode : std::uint8_t
{
    Convex,
    Concave
};

struct GeologyCurvatureWeatheringPayload
{
    GeologyCurvatureWeatheringMode mode{
        GeologyCurvatureWeatheringMode::Convex};
    double sampleSpacingMeters{0.10};
    double curvatureResponseLengthMeters{0.10};
    double maximumWearMetersPerIteration{0.025};
    double durability{};
    std::uint32_t iterations{1U};

    bool operator==(const GeologyCurvatureWeatheringPayload&) const = default;
};

struct GeologySurfaceWeatheringPayload
{
    std::variant<
        GeologyLegacySurfaceWeatheringPayload,
        GeologyBubbleWeatheringPayload,
        GeologyCurvatureWeatheringPayload> model{
            GeologyLegacySurfaceWeatheringPayload{}};

    bool operator==(const GeologySurfaceWeatheringPayload&) const = default;
};

enum class GeologyHydrologyBoundaryMode : std::uint8_t
{
    Closed,
    Outlet
};

struct GeologyHeightfieldErosionPayload
{
    GeologyHydrologyBoundaryMode boundaryMode{
        GeologyHydrologyBoundaryMode::Closed};
    std::uint32_t streamPowerIterations{0U};
    double maximumDepressionFillMeters{0.05};
    double drainageConcentration{1.50};
    double streamPowerStrength{0.20};
    double streamPowerConcavity{0.50};
    std::uint32_t iterations{16U};
    double rainfallMetersPerIteration{0.005};
    double flowRate{0.50};
    double maximumHydraulicDepthMeters{0.20};
    double sedimentCapacity{1.0};
    double erosionRate{0.20};
    double depositionRate{0.20};
    double evaporationRate{0.05};
    double thermalRate{0.10};
    double talusAngleDegrees{35.0};

    bool operator==(const GeologyHeightfieldErosionPayload&) const = default;
};

struct GeologyCaveCrossSection
{
    Guid pointGuid;
    double leftWidthMeters{1.0};
    double rightWidthMeters{1.0};
    double floorDepthMeters{1.0};
    double ceilingHeightMeters{1.0};
    double profileExponent{2.0};
    double blendLengthMeters{1.0};
    double localWarpWeight{};
    double detailWeight{};

    bool operator==(const GeologyCaveCrossSection&) const = default;
};

enum class GeologyChamberShape : std::uint8_t
{
    Superellipsoid = 1U
};

struct GeologyCaveChamber
{
    Guid chamberGuid;
    GeologyChamberShape shape{GeologyChamberShape::Superellipsoid};
    Guid attachmentSplineGuid;
    double attachmentDistanceMeters{};
    Vec3d localOffsetMeters;
    std::array<double, 4U> localRotation{0.0, 0.0, 0.0, 1.0};
    double leftWidthMeters{1.0};
    double rightWidthMeters{1.0};
    double floorDepthMeters{1.0};
    double ceilingHeightMeters{1.0};
    double backDepthMeters{1.0};
    double frontDepthMeters{1.0};
    double profileExponent{2.0};
    double ceilingBiasMeters{};
    double floorBiasMeters{};
    double blendLengthMeters{};

    bool operator==(const GeologyCaveChamber&) const = default;
};

struct GeologyCaveInteriorDetail
{
    bool enabled{};
    Vec3d frequencyPerMeter{1.0, 1.0, 1.0};
    double maximumDisplacementMeters{0.10};
    double wallWeight{1.0};
    double ceilingWeight{1.0};
    double floorWeight{1.0};
    std::uint64_t seed{};

    bool operator==(const GeologyCaveInteriorDetail&) const = default;
};

struct GeologyCaveBranch
{
    Guid branchGuid;
    Guid splineGuid;
    Guid attachmentSplineGuid;
    double attachmentDistanceMeters{};
    bool attachesAtStart{true};
    double blendLengthMeters{};
    std::vector<GeologyCaveCrossSection> crossSections;
    GeologyCaveInteriorDetail detail;

    bool operator==(const GeologyCaveBranch&) const = default;
};

struct GeologyCaveSweepPayload
{
    Guid splineGuid;
    std::vector<GeologyCaveCrossSection> crossSections;
    GeologyCaveInteriorDetail detail;
    std::vector<GeologyCaveChamber> chambers;
    std::vector<GeologyCaveBranch> branches;

    bool operator==(const GeologyCaveSweepPayload&) const = default;
};

struct GeologyCavityCarvePayload
{
    double radiusMeters{1.0};
    double smoothnessMeters{};

    bool operator==(const GeologyCavityCarvePayload&) const = default;
};

struct GeologyImplicitBlockDefinition
{
    GeologyBlockArchetype archetype{GeologyBlockArchetype::Equidimensional};
    Vec3d halfExtentMeters{1.0, 1.0, 1.0};
    double edgeSmoothingMeters{};
    std::uint32_t radialSideCount{6U};
    double obliqueness{};
    double irregularity{};

    bool operator==(const GeologyImplicitBlockDefinition&) const = default;
};

struct GeologyImplicitBlockPayload
{
    GeologyImplicitBlockDefinition definition;

    bool operator==(const GeologyImplicitBlockPayload&) const = default;
};

struct GeologyJointFamily
{
    Vec3d dominantNormalLocal{0.0, 1.0, 0.0};
    std::vector<double> planeOffsetsMeters;
    double maximumAngularJitterDegrees{};
    double maximumOffsetJitterMeters{};

    bool operator==(const GeologyJointFamily&) const = default;
};

struct GeologyJointSetPayload
{
    double halfWidthMeters{0.05};
    std::vector<GeologyJointFamily> families;

    bool operator==(const GeologyJointSetPayload&) const = default;
};

struct GeologyFoldDomainPayload
{
    Vec3d foldAxisLocal{0.0, 1.0, 0.0};
    Vec3d displacementDirectionLocal{1.0, 0.0, 0.0};
    double amplitudeMeters{0.25};
    double frequencyCyclesPerMeter{0.25};
    double phaseOffsetCycles{};
    Vec3d envelopeHalfExtentMeters{1.0, 1.0, 1.0};
    double envelopeFalloffFraction{0.25};

    bool operator==(const GeologyFoldDomainPayload&) const = default;
};

struct GeologyGradientDetailWarpPayload
{
    double maximumDisplacementMeters{0.1};
    double surfaceBandMeters{0.5};
    double frequencyCyclesPerMeter{1.0};
    std::uint32_t octaves{1U};
    double lacunarity{2.0};
    double gain{0.5};

    bool operator==(const GeologyGradientDetailWarpPayload&) const = default;
};

enum class GeologyDirectedCrackProjectionMode : std::uint8_t
{
    ImplicitSurfacePlanarChart = 1U
};

struct GeologyDirectedCrackProfileKey
{
    double normalizedDepth{};
    double widthScale{};

    bool operator==(const GeologyDirectedCrackProfileKey&) const = default;
};

struct GeologyDirectedCrackBranch
{
    Guid branchGuid;
    Guid splineGuid;
    Guid parentBranchGuid;
    Guid attachmentPointGuid;
    double depthMeters{0.1};
    double activationProbability{1.0};
    double propagation{1.0};

    bool operator==(const GeologyDirectedCrackBranch&) const = default;
};

struct GeologyDirectedCrackPayload
{
    GeologyDirectedCrackProjectionMode projection{
        GeologyDirectedCrackProjectionMode::ImplicitSurfacePlanarChart};
    std::vector<GeologyDirectedCrackBranch> branches;
    std::vector<GeologyDirectedCrackProfileKey> profile;
    double maximumResampleSpacingMeters{0.1};
    double openingOutsetMeters{};
    double surfaceBandMeters{0.5};

    bool operator==(const GeologyDirectedCrackPayload&) const = default;
};

struct GeologyDurabilityGraphKey
{
    double hardness{};
    double durability{};

    bool operator==(const GeologyDurabilityGraphKey&) const = default;
};

struct GeologyDurabilityFieldPayload
{
    std::vector<GeologyDurabilityGraphKey> graph;
    double fractureWeakening{};

    bool operator==(const GeologyDurabilityFieldPayload&) const = default;
};

struct GeologyRockfallPayload
{
    Vec3d gravityDirectionWorld{0.0, -1.0, 0.0};
    double supportPlaneOffsetMeters{};
    double fractureCutoff{0.5};
    double activationCutoff{0.5};

    bool operator==(const GeologyRockfallPayload&) const = default;
};

inline constexpr std::uint32_t geologyColluviumMaximumTransportCells = 64U;

struct GeologyColluviumPayload
{
    double maximumTransportDistanceMeters{};
    double activationCutoff{0.5};

    bool operator==(const GeologyColluviumPayload&) const = default;
};

struct GeologyContactPreservingErosionPayload
{
    std::vector<Vec3d> contactPointsWorld;
    double preservationRadiusMeters{0.125};
    double transitionWidthMeters{0.125};
    double maximumWearMeters{0.0625};
    double surfaceBandMeters{0.25};
    double wearFrequencyPerMeter{1.0};

    bool operator==(const GeologyContactPreservingErosionPayload&) const = default;
};

struct GeologyNarrowBandMineralWeatheringPayload
{
    double susceptibleMineralClass{0.5};
    double mineralClassHalfWidth{0.125};
    double maximumWeatheredMineralFraction{1.0};
    double interiorBandMeters{0.25};
    double weatheringFrequencyPerMeter{1.0};

    bool operator==(
        const GeologyNarrowBandMineralWeatheringPayload&) const = default;
};

struct GeologyMoistureTransportPayload
{
    double maximumSurfacePressureNormalized{1.0};
    double interiorBandMeters{0.5};
    double effectiveDiffusivitySquareMetersPerSecond{0.25};
    double exposureDurationSeconds{1.0};
    double sourceFrequencyPerMeter{1.0};

    bool operator==(const GeologyMoistureTransportPayload&) const = default;
};

struct GeologyMineralDissolutionPayload
{
    double availableMineralFractionNormalized{1.0};
    double dissolutionRatePerSecond{1.0};
    double exposureDurationSeconds{1.0};
    double minimumMoisturePressureNormalized{0.0};
    double reactionFrequencyPerMeter{1.0};

    bool operator==(const GeologyMineralDissolutionPayload&) const = default;
};

struct GeologyMultiScaleMicrostructurePayload
{
    double fineCellWidthMeters{1.0};
    double fineRadiusMeters{0.25};
    double fineDensityNormalized{1.0};
    double mesoCellWidthMeters{3.0};
    double mesoRadiusMeters{1.0};
    double mesoDensityNormalized{1.0};

    bool operator==(const GeologyMultiScaleMicrostructurePayload&) const =
        default;
};

using GeologyLayerPayload = std::variant<
    GeologySourcePayload,
    GeologyMacroWarpPayload,
    GeologyNoisePayload,
    GeologyRebuildPayload,
    GeologyPlaneCutPayload,
    GeologyDirectionalWarpPayload,
    GeologyFaultDisplacePayload,
    GeologyAnisotropicVoronoiPayload,
    GeologyFractureCellsPayload,
    GeologyStrataPayload,
    GeologyCliffShelfPayload,
    GeologyFracturePayload,
    GeologySurfaceWeatheringPayload,
    GeologyHeightfieldErosionPayload,
    GeologyCaveSweepPayload,
    GeologyCavityCarvePayload,
    GeologyImplicitBlockPayload,
    GeologyJointSetPayload,
    GeologyFoldDomainPayload,
    GeologyGradientDetailWarpPayload,
    GeologyDirectedCrackPayload,
    GeologyDurabilityFieldPayload,
    GeologyRockfallPayload,
    GeologyColluviumPayload,
    GeologyContactPreservingErosionPayload,
    GeologyNarrowBandMineralWeatheringPayload,
    GeologyMoistureTransportPayload,
    GeologyMineralDissolutionPayload,
    GeologyMultiScaleMicrostructurePayload>;

struct GeologyLayerHeader
{
    Guid id;
    std::string name;
    GeologyLayerKind kind{GeologyLayerKind::Source};
    bool enabled{true};
    double strength{1.0};
    std::uint64_t seed{};
    GeologyMaskRef mask;
    GeologyTransform transform;

    bool operator==(const GeologyLayerHeader&) const = default;
};

struct GeologyLayer
{
    GeologyLayerHeader header;
    GeologyLayerPayload payload;

    bool operator==(const GeologyLayer&) const = default;
};
}
