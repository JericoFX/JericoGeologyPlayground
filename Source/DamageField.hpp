#pragma once

#include "RockAddonMath.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <utility>

namespace Playground
{
struct DamageFieldInputs
{
    double manual{}, edge{}, cavity{}, impact{}, spline{}, age{}, noise{};
    double protectedMask{};
};
struct DamageFieldWeights
{
    double edge{}, cavity{}, impact{}, spline{}, age{}, noise{};
};
double evaluateDamageField(const DamageFieldInputs &inputs,
                           const DamageFieldWeights &weights);
double damageRandom01(std::uint64_t seed, std::string_view semanticId,
                      std::string_view operationTag);

struct DamageNoiseSettings
{
    Vec3d coordinateScale{1,1,1};
    double frequency{1}, lacunarity{2}, roughness{.5};
    int octaves{3};
    std::uint32_t seed{123};
};
double structuredDamageNoise01(Vec3d position,const DamageNoiseSettings &settings);

struct DamageArea
{
    std::string id;
    bool enabled{true};
    std::string semanticPartId;
    std::string semanticRole;
    std::optional<std::pair<std::uint32_t,std::uint32_t>> indexRange;
    std::optional<std::pair<double,double>> pathRangeMeters;
    DamageProjector projector;
    DamageDecalSettings footprint;
    double amount{1};
};
struct DamageSampleContext
{
    Vec3d position{};
    std::string_view semanticPartId, semanticRole;
    std::uint32_t partIndex{};
    double pathDistanceMeters{};
};
double sampleDamageArea(const DamageArea &area,const DamageSampleContext &context);
double sampleDamageArea(const DamageArea &area, Vec3d worldPosition,
                        std::string_view semanticPartId = {});

enum class DamageWidthMode { Metric, Relative };
struct DamageEdgeSettings
{
    DamageWidthMode widthMode{DamageWidthMode::Metric};
    double width{0.1}, curvatureStart{}, curvatureEnd{1};
    double taperFraction{}, amount{1};
};
double resolveDamageEdgeWidth(const DamageEdgeSettings &settings,
                              double characteristicSize);
double sampleEdgeDamage(const DamageEdgeSettings &settings, double signedCurvature,
                        double edgeDistance, double characteristicSize,
                        double pathFraction, double semanticMask = 1);

enum class DamageTier
{
    None, Material, NormalBake, Geometry, LocalSdf, PreparedVariant, Physics
};
struct DamageNeeds
{
    bool visual{}, normalDetail{}, silhouette{}, penetration{}, collision{},
         interaction{}, dynamicMotion{};
};
DamageTier selectDamageTier(const DamageNeeds &needs);
double boundedDamageAmplitude(double requestedMeters,double localFeatureSizeMeters,
                              std::optional<double> measuredThicknessMeters);
double subtractDamageCavity(double hostField,double cutterField);
bool isThroughHole(double depth,double hostThickness,double exitMargin);
bool isFreshDamageInterior(double hostField,double cutterField,
                           double resultField,double tolerance);
double pathCrackField(Vec3d position,const std::vector<Vec3d> &path,
                      double radius);
double fractureProfileOffset(double contourFraction,double amplitudeMeters,
                             double localFeatureSizeMeters,std::uint64_t seed,
                             std::string_view semanticId);
} // namespace Playground
