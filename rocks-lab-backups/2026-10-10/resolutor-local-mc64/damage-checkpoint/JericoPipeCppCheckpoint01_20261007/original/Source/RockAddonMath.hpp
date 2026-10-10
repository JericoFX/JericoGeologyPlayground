#pragma once
#include "Model.hpp"
#include <array>
#include <cstdint>

namespace Playground
{
enum class DamageBlend
{
    Lerp,
    Maximum,
    Minimum,
    Multiply,
    Add
};

struct DamageProjector
{
    Vec3d center{};
    Vec3d tangent{1, 0, 0};
    Vec3d bitangent{0, 1, 0};
    Vec3d normal{0, 0, 1};
    Vec3d scale{1, 1, 1};
    double rotationRadians{};
};

struct DamageDecalSettings
{
    Vec3d radius{1, 1, 1};
    double innerRadius{0.7};
    double outerRadius{1};
    double depth{0.1};
    double profilePower{2};
    double profileShape{1};
    double lipInner{0.7};
    double lipOuter{1.15};
    double lipHeight{};
};

struct DamageDecalSample
{
    double radialDistance{};
    double softMask{};
    double indentation{};
    double cavityField{};
    double lipDisplacement{};
};

struct AgingInput
{
    double convexity{};
    double concavity{};
    double contactOcclusion{};
    double ambientOcclusion{};
    double edgeProximity{};
    double exposure{};
    double wetHistory{};
    double noise{1};
};

struct AgingFields
{
    double dirt{};
    double scratches{};
    double moss{};
};

Vec3d projectorCoordinates(const DamageProjector &projector, Vec3d position);
double combineDamageLayers(double first, double second, DamageBlend blend, double weight = 0.5);
DamageDecalSample sampleDamageDecal(Vec3d projectorPosition, const DamageDecalSettings &settings);
double sphereField(Vec3d position, Vec3d center, double radius);
double segmentCrackField(Vec3d position, Vec3d first, Vec3d second, double radius);
double reinforcementGridField(Vec3d projectorPosition, std::array<double, 2> spacing,
                              std::array<double, 2> offset, double rodRadius);
double staggeredMortarField(Vec3d projectorPosition, std::array<double, 2> cell,
                            std::array<double, 2> offset, double mortarHalfWidth);
std::uint64_t proceduralElementId(std::uint64_t seed, std::uint64_t logicalElement,
                                  std::uint64_t stageTag);
AgingFields agingFields(const AgingInput &input);
} // namespace Playground
