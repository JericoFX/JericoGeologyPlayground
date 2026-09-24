#include "RockAddonMath.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

using namespace Playground;
namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
bool close(double a, double b, double epsilon = 1e-9)
{
    return std::abs(a - b) <= epsilon;
}
template <class F> void rejects(F action)
{
    bool rejected = false;
    try { action(); }
    catch (const std::invalid_argument &) { rejected = true; }
    require(rejected, "non-finite field coordinates must be rejected");
}
} // namespace

int main()
try
{
    DamageProjector projector;
    projector.center = {1, 2, 3};
    projector.tangent = {0, 1, 0};
    projector.bitangent = {0, 0, 1};
    projector.normal = {1, 0, 0};
    auto q = projectorCoordinates(projector, {5, 4, 6});
    require(close(q.x, 2) && close(q.y, 3) && close(q.z, 4),
            "projector converts world position to its orthonormal local frame");

    require(close(combineDamageLayers(0.25, 0.75, DamageBlend::Lerp, 0.4), 0.45),
            "damage lerp uses explicit authored weight");
    require(close(combineDamageLayers(0.25, 0.75, DamageBlend::Maximum), 0.75) &&
                close(combineDamageLayers(0.25, 0.75, DamageBlend::Minimum), 0.25) &&
                close(combineDamageLayers(0.25, 0.75, DamageBlend::Multiply), 0.1875) &&
                close(combineDamageLayers(0.25, 0.75, DamageBlend::Add), 1.0),
            "all documented layer operators have independent semantics");

    DamageDecalSettings decal;
    decal.radius = {2, 1};
    decal.innerRadius = 0.7;
    decal.outerRadius = 1;
    decal.depth = 0.5;
    decal.profilePower = 2;
    decal.profileShape = 1;
    decal.lipInner = 0.65;
    decal.lipOuter = 1.15;
    decal.lipHeight = 0.1;
    auto center = sampleDamageDecal({0, 0, 0}, decal);
    auto cavity = sampleDamageDecal({0, 0, -0.25}, decal);
    require(close(center.softMask, 1) && close(center.indentation, -0.5) &&
                close(center.cavityField, 0) && cavity.cavityField < 0,
            "decal center reaches authored dent depth and cavity field has a front boundary and interior");
    auto outside = sampleDamageDecal({2.4, 0, 0}, decal);
    require(close(outside.softMask, 0) && close(outside.indentation, 0) && outside.cavityField > 0,
            "decal outside ellipse does not alter surface");
    auto lip = sampleDamageDecal({1.7, 0, 0}, decal);
    require(lip.lipDisplacement > 0 && lip.lipDisplacement <= decal.lipHeight,
            "decal lip is bounded and only active around the crater rim");

    require(close(sphereField({0, 0, 0}, {0, 0, 0}, 0.75), -0.75) &&
                close(sphereField({1.75, 0, 0}, {0, 0, 0}, 0.75), 1),
            "internal sphere uses signed distance");
    require(close(segmentCrackField({1, 0.25, 0}, {0, 0, 0}, {2, 0, 0}, 0.1), 0.15) &&
                close(segmentCrackField({-1, 0, 0}, {0, 0, 0}, {2, 0, 0}, 0.1), 0.9),
            "crack field clamps distance to finite segment endpoints");
    require(close(reinforcementGridField({0, 0, 0}, {1, 1}, {0, 0}, 0.1), -0.1) &&
                close(reinforcementGridField({0.5, 0.5, 0}, {1, 1}, {0, 0}, 0.1), 0.4),
            "reinforcement grid returns distance to nearest rod");
    require(close(staggeredMortarField({0, 0, 0}, {2, 1}, {0, 0}, 0.1), -0.1) &&
                close(staggeredMortarField({1, 0.5, 0}, {2, 1}, {0, 0}, 0.1), 0.4),
            "brick lattice identifies mortar and cell interior");

    auto first = proceduralElementId(42, 7, 11), repeat = proceduralElementId(42, 7, 11),
         differentStage = proceduralElementId(42, 7, 12), differentElement = proceduralElementId(42, 8, 11);
    require(first == repeat && first != differentStage && first != differentElement,
            "stable IDs isolate element and stage seed streams");

    AgingInput clean, sheltered;
    clean.noise = sheltered.noise = 0.8;
    clean.exposure = sheltered.exposure = 0.2;
    clean.wetHistory = sheltered.wetHistory = 0.6;
    sheltered.concavity = 0.9;
    sheltered.contactOcclusion = 0.8;
    sheltered.edgeProximity = 0.7;
    auto cleanResult = agingFields(clean), cavityResult = agingFields(sheltered);
    require(cavityResult.dirt > cleanResult.dirt && cavityResult.dirt <= 1,
            "physical cavity/contact/edge fields increase bounded dirt");
    require(cavityResult.moss > cleanResult.moss && cavityResult.moss <= 1,
            "wet sheltered cavities increase bounded moss likelihood");
    require(cavityResult.scratches >= 0 && cavityResult.scratches <= 1,
            "scratch mask remains normalized");

    bool rejected = false;
    try
    {
        decal.radius.x = 0;
        sampleDamageDecal({0, 0, 0}, decal);
    }
    catch (const std::invalid_argument &)
    {
        rejected = true;
    }
    require(rejected, "invalid projector dimensions are rejected");

    for (double bad : {std::numeric_limits<double>::quiet_NaN(),
                       std::numeric_limits<double>::infinity()})
    {
        rejects([&] { sphereField({bad, 0, 0}, {}, 1); });
        rejects([&] { sphereField({}, {0, bad, 0}, 1); });
        rejects([&] { segmentCrackField({0, 0, bad}, {}, {1, 0, 0}, 0.1); });
        rejects([&] { segmentCrackField({}, {bad, 0, 0}, {1, 0, 0}, 0.1); });
        rejects([&] { segmentCrackField({}, {}, {1, bad, 0}, 0.1); });
        rejects([&] { reinforcementGridField({0, bad, 0}, {1, 1}, {0, 0}, 0.1); });
        rejects([&] { staggeredMortarField({0, bad, 0}, {1, 1}, {0, 0}, 0.1); });
    }

    std::cout << "PASS native addon math: projector, layered damage, crater, cavity, crack, semantic "
                 "lattices, stable IDs and aging fields\n";
    return 0;
}
catch (const std::exception &error)
{
    std::cerr << error.what() << '\n';
    return 1;
}
