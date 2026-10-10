#include "RockAddonMath.hpp"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace Playground
{
namespace
{
double dot(Vec3d a, Vec3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3d subtract(Vec3d a, Vec3d b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
Vec3d multiply(Vec3d a, double value)
{
    return {a.x * value, a.y * value, a.z * value};
}
double length(Vec3d value)
{
    return std::sqrt(dot(value, value));
}
double saturate(double value)
{
    return std::clamp(value, 0.0, 1.0);
}
double smoothstep(double low, double high, double value)
{
    if (!(high > low))
        throw std::invalid_argument("Smooth interval must be increasing");
    auto x = saturate((value - low) / (high - low));
    return x * x * (3 - 2 * x);
}
void finite(double value, const char *message)
{
    if (!std::isfinite(value))
        throw std::invalid_argument(message);
}
void finite(Vec3d value, const char *message)
{
    finite(value.x, message);
    finite(value.y, message);
    finite(value.z, message);
}
void normalizedInput(double value, const char *message)
{
    finite(value, message);
    if (value < 0 || value > 1)
        throw std::invalid_argument(message);
}
std::uint64_t mix64(std::uint64_t value)
{
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    return value ^ (value >> 31);
}
double periodicDistance(double coordinate, double spacing, double offset)
{
    return std::abs(coordinate - (offset + std::round((coordinate - offset) / spacing) * spacing));
}
} // namespace

Vec3d projectorCoordinates(const DamageProjector &projector, Vec3d position)
{
    for (auto value : {projector.center.x, projector.center.y, projector.center.z, projector.tangent.x,
                       projector.tangent.y, projector.tangent.z, projector.bitangent.x,
                       projector.bitangent.y, projector.bitangent.z, projector.normal.x,
                       projector.normal.y, projector.normal.z, projector.scale.x, projector.scale.y,
                       projector.scale.z, projector.rotationRadians, position.x, position.y, position.z})
        finite(value, "Damage projector contains a non-finite value");
    constexpr double tolerance = 1e-6;
    if (std::abs(length(projector.tangent) - 1) > tolerance ||
        std::abs(length(projector.bitangent) - 1) > tolerance ||
        std::abs(length(projector.normal) - 1) > tolerance ||
        std::abs(dot(projector.tangent, projector.bitangent)) > tolerance ||
        std::abs(dot(projector.tangent, projector.normal)) > tolerance ||
        std::abs(dot(projector.bitangent, projector.normal)) > tolerance || projector.scale.x <= 0 ||
        projector.scale.y <= 0 || projector.scale.z <= 0)
        throw std::invalid_argument("Damage projector requires an orthonormal basis and positive scale");
    auto local = subtract(position, projector.center);
    double u = dot(local, projector.tangent) / projector.scale.x;
    double v = dot(local, projector.bitangent) / projector.scale.y;
    const auto cosine = std::cos(projector.rotationRadians), sine = std::sin(projector.rotationRadians);
    return {cosine * u - sine * v, sine * u + cosine * v,
            dot(local, projector.normal) / projector.scale.z};
}

double combineDamageLayers(double first, double second, DamageBlend blend, double weight)
{
    normalizedInput(first, "First damage layer outside [0,1]");
    normalizedInput(second, "Second damage layer outside [0,1]");
    normalizedInput(weight, "Damage blend weight outside [0,1]");
    switch (blend)
    {
    case DamageBlend::Lerp:
        return first + (second - first) * weight;
    case DamageBlend::Maximum:
        return std::max(first, second);
    case DamageBlend::Minimum:
        return std::min(first, second);
    case DamageBlend::Multiply:
        return first * second;
    case DamageBlend::Add:
        return saturate(first + second);
    }
    throw std::invalid_argument("Unknown damage blend");
}

DamageDecalSample sampleDamageDecal(Vec3d q, const DamageDecalSettings &settings)
{
    for (auto value : {q.x, q.y, q.z, settings.radius.x, settings.radius.y, settings.innerRadius,
                       settings.outerRadius, settings.depth, settings.profilePower, settings.profileShape,
                       settings.lipInner, settings.lipOuter, settings.lipHeight})
        finite(value, "Damage decal contains a non-finite value");
    if (settings.radius.x <= 0 || settings.radius.y <= 0 || settings.innerRadius < 0 ||
        !(settings.outerRadius > settings.innerRadius) || settings.depth < 0 ||
        settings.profilePower <= 0 || settings.profileShape <= 0 || settings.lipInner < 0 ||
        !(settings.lipOuter > 1) || settings.lipInner >= 1 || settings.lipHeight < 0)
        throw std::invalid_argument("Damage decal settings outside limits");
    const auto rho = std::hypot(q.x / settings.radius.x, q.y / settings.radius.y);
    const auto mask = 1 - smoothstep(settings.innerRadius, settings.outerRadius, rho);
    const auto profile = std::pow(std::max(0.0, 1 - std::pow(saturate(rho), settings.profilePower)),
                                  settings.profileShape);
    const auto axial = std::max(q.z, -q.z - settings.depth);
    const auto enterLip = smoothstep(settings.lipInner, 1, rho);
    const auto leaveLip = 1 - smoothstep(1, settings.lipOuter, rho);
    return {.radialDistance = rho,
            .softMask = mask,
            .indentation = -settings.depth * profile * mask,
            .cavityField = std::max(rho - 1, axial),
            .lipDisplacement = settings.lipHeight * enterLip * leaveLip};
}

double sphereField(Vec3d position, Vec3d center, double radius)
{
    finite(position, "Sphere query non-finite");
    finite(center, "Sphere center non-finite");
    finite(radius, "Sphere radius non-finite");
    if (radius <= 0)
        throw std::invalid_argument("Sphere radius must be positive");
    return length(subtract(position, center)) - radius;
}

double segmentCrackField(Vec3d position, Vec3d first, Vec3d second, double radius)
{
    finite(position, "Crack query non-finite");
    finite(first, "Crack endpoint non-finite");
    finite(second, "Crack endpoint non-finite");
    finite(radius, "Crack radius non-finite");
    auto segment = subtract(second, first);
    auto lengthSquared = dot(segment, segment);
    if (!(radius > 0) || !(lengthSquared > 1e-18) || !std::isfinite(lengthSquared))
        throw std::invalid_argument("Crack segment or radius invalid");
    auto t = std::clamp(dot(subtract(position, first), segment) / lengthSquared, 0.0, 1.0);
    return length(subtract(position, {first.x + segment.x * t, first.y + segment.y * t,
                                      first.z + segment.z * t})) -
           radius;
}

double reinforcementGridField(Vec3d q, std::array<double, 2> spacing, std::array<double, 2> offset,
                              double rodRadius)
{
    finite(q, "Reinforcement query non-finite");
    if (!(spacing[0] > 0) || !(spacing[1] > 0) || !(rodRadius > 0) ||
        !std::isfinite(spacing[0]) || !std::isfinite(spacing[1]) || !std::isfinite(offset[0]) ||
        !std::isfinite(offset[1]) || !std::isfinite(rodRadius))
        throw std::invalid_argument("Reinforcement grid settings invalid");
    return std::min(periodicDistance(q.x, spacing[0], offset[0]),
                    periodicDistance(q.y, spacing[1], offset[1])) -
           rodRadius;
}

double staggeredMortarField(Vec3d q, std::array<double, 2> cell, std::array<double, 2> offset,
                            double mortarHalfWidth)
{
    finite(q, "Mortar query non-finite");
    if (!(cell[0] > 0) || !(cell[1] > 0) || !(mortarHalfWidth >= 0) ||
        !std::isfinite(cell[0]) || !std::isfinite(cell[1]) || !std::isfinite(offset[0]) ||
        !std::isfinite(offset[1]) || !std::isfinite(mortarHalfWidth))
        throw std::invalid_argument("Brick lattice settings invalid");
    auto row = std::floor((q.y - offset[1]) / cell[1]);
    auto stagger = (static_cast<long long>(row) & 1LL) ? 0.5 * cell[0] : 0.0;
    auto x = (q.x - offset[0] - stagger) / cell[0];
    auto y = (q.y - offset[1]) / cell[1];
    auto fx = x - std::floor(x), fy = y - std::floor(y);
    return std::min(std::min(fx, 1 - fx) * cell[0], std::min(fy, 1 - fy) * cell[1]) -
           mortarHalfWidth;
}

std::uint64_t proceduralElementId(std::uint64_t seed, std::uint64_t logicalElement,
                                  std::uint64_t stageTag)
{
    return mix64(seed ^ mix64(logicalElement + 0x9e3779b97f4a7c15ULL) ^
                 mix64(stageTag + 0xd1b54a32d192ed03ULL));
}

AgingFields agingFields(const AgingInput &input)
{
    for (auto value : {input.convexity, input.concavity, input.contactOcclusion,
                       input.ambientOcclusion, input.edgeProximity, input.exposure, input.wetHistory,
                       input.noise})
        normalizedInput(value, "Aging input outside [0,1]");
    const auto shelter = saturate(0.45 * input.concavity + 0.3 * input.contactOcclusion +
                                 0.15 * input.ambientOcclusion + 0.1 * (1 - input.exposure));
    const auto features = saturate(0.55 * shelter + 0.3 * input.edgeProximity + 0.15 * input.wetHistory);
    return {.dirt = saturate(input.noise * features),
            .scratches = saturate(input.noise * (0.65 * input.edgeProximity + 0.35 * input.convexity) *
                                  (1 - 0.5 * input.wetHistory)),
            .moss = saturate(input.noise * input.wetHistory *
                             (0.65 * shelter + 0.35 * (1 - input.exposure)))};
}
} // namespace Playground
