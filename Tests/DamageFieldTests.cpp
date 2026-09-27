#include "DamageField.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

using namespace Playground;

namespace
{
void require(bool condition, const char *message)
{if (!condition) throw std::runtime_error(message);}
bool close(double a,double b)
{return std::abs(a-b)<1e-9;}
template<class Action> void rejects(Action action)
{
    bool rejected=false;
    try {action();} catch(const std::invalid_argument &) {rejected=true;}
    require(rejected,"invalid damage field input must be rejected");
}
}

int main()
try
{
    DamageFieldWeights weights;
    weights.edge=.5;weights.impact=.75;
    DamageFieldInputs inputs;
    inputs.edge=.6;inputs.impact=.4;
    require(close(evaluateDamageField(inputs,weights),.6),
            "shared damage terms must compose with authored weights");
    inputs.protectedMask=1;
    require(close(evaluateDamageField(inputs,weights),0),
            "protection must cancel procedural damage");
    inputs.protectedMask=0;inputs.manual=1;
    require(close(evaluateDamageField(inputs,weights),1),
            "manual damage must remain authoritative");

    DamageArea area;
    area.id="impact-1";
    area.footprint.radius={.5,.5,1};
    area.semanticPartId="DeckPlank_018";
    require(close(sampleDamageArea(area,{0,0,0},"DeckPlank_018"),1),
            "selected part must receive local damage");
    require(close(sampleDamageArea(area,{0,0,3},"DeckPlank_018"),0),
            "projector depth must exclude geometry outside its local volume");
    require(close(sampleDamageArea(area,{0,0,0},"DeckPlank_019"),0),
            "unselected semantic part must remain untouched");
    area.projector.center={2,0,0};
    require(close(sampleDamageArea(area,{0,0,0},"DeckPlank_018"),0) &&
            close(sampleDamageArea(area,{2,0,0},"DeckPlank_018"),1),
            "moving a non-destructive damage area must move its effect");
    area.enabled=false;
    require(close(sampleDamageArea(area,{2,0,0},"DeckPlank_018"),0),
            "disabling the area must restore the source appearance");
    area.enabled=true;area.semanticPartId.clear();
    area.projector.center={0,0,0};
    area.indexRange=std::pair<std::uint32_t,std::uint32_t>{3,5};
    area.pathRangeMeters=std::pair<double,double>{2,4};
    require(close(sampleDamageArea(area,{{0,0,0},"part","plank",4,3}),1) &&
            close(sampleDamageArea(area,{{0,0,0},"part","plank",6,3}),0) &&
            close(sampleDamageArea(area,{{0,0,0},"part","plank",4,5}),0),
            "the shared selector must combine spatial, index and path ranges");

    DamageEdgeSettings edge;
    edge.widthMode=DamageWidthMode::Relative;
    edge.width=.5;
    edge.curvatureStart=.1;
    edge.curvatureEnd=.2;
    edge.taperFraction=.2;
    require(close(resolveDamageEdgeWidth(edge,2),1) &&
            close(sampleEdgeDamage(edge,.3,0,2,.5),1) &&
            close(sampleEdgeDamage(edge,.3,0,2,0),0),
            "relative edge width and endpoint taper must use separate semantics");
    edge.widthMode=DamageWidthMode::Metric;edge.width=.15;
    require(close(resolveDamageEdgeWidth(edge,2),.15) &&
            close(sampleEdgeDamage(edge,-.3,0,2,.5),0),
            "metric width must not scale and concave edges must remain excluded");
    require(close(sampleEdgeDamage(edge,.3,
                std::numeric_limits<double>::infinity(),2,.5),0),
            "an absent feature edge must produce no damage");

    require(selectDamageTier({.visual=true})==DamageTier::Material &&
            selectDamageTier({.normalDetail=true})==DamageTier::NormalBake &&
            selectDamageTier({.silhouette=true})==DamageTier::Geometry &&
            selectDamageTier({.penetration=true})==DamageTier::LocalSdf &&
            selectDamageTier({.interaction=true})==DamageTier::PreparedVariant &&
            selectDamageTier({.dynamicMotion=true})==DamageTier::Physics,
            "representation must escalate only to the cheapest tier meeting the need");
    require(close(boundedDamageAmplitude(5,.1,std::nullopt),.02) &&
            close(boundedDamageAmplitude(-5,.1,.04),-.008),
            "shared displacement cap must respect triangle scale and measured thickness");
    rejects([]{boundedDamageAmplitude(1,0,std::nullopt);});

    const auto cut=damageRandom01(123,"DeckPlank_018","cut-profile");
    require(cut>=0 && cut<1 &&
            close(cut,damageRandom01(123,"DeckPlank_018","cut-profile")) &&
            cut!=damageRandom01(123,"DeckPlank_019","cut-profile") &&
            cut!=damageRandom01(123,"DeckPlank_018","cut-position"),
            "stable part and operation identity must isolate damage random streams");

    require(close(subtractDamageCavity(-.5,-.2),.2) &&
            close(subtractDamageCavity(-.5,.2),-.2),
            "local cavity subtraction must remove only the cutter interior");
    require(isThroughHole(.4,.3,.05) && !isThroughHole(.34,.3,.05),
            "penetration requires measured host thickness plus exit margin");
    require(isFreshDamageInterior(-.5,0,0,.01) &&
            !isFreshDamageInterior(.5,0,.5,.01),
            "fresh interior must be tagged at generation from host and cutter fields");
    DamageBoundarySettings boundary;
    boundary.irregularity=.15;boundary.rimFraction=.2;boundary.seed=42;
    const auto boundaryRadius=damageBoundaryRadiusScale(.4,boundary);
    require(boundaryRadius>=.85 && boundaryRadius<=1.15 &&
            close(boundaryRadius,damageBoundaryRadiusScale(.4+2*std::numbers::pi,boundary)) &&
            boundaryRadius!=damageBoundaryRadiusScale(2.1,boundary),
            "shared boundary must be bounded, periodic and vary around the projector");
    DamageDecalSettings cavityFootprint;
    cavityFootprint.radius={.4,.4,1};cavityFootprint.depth=.3;
    require(sampleDamageCavityField({0,0,-.1},cavityFootprint,boundary)<0 &&
            sampleDamageCavityField({1,0,-.1},cavityFootprint,boundary)>0,
            "shared cavity field must distinguish bounded inside and outside");
    require(damageCavityFloorDepth({},cavityFootprint,boundary)>cavityFootprint.depth &&
            close(damageCavityFloorDepth({.8,0,0},cavityFootprint,boundary),
                  cavityFootprint.depth),
            "irregular cavity floor must deepen locally and meet the outer ring");
    boundary.irregularity=0;boundary.rimFraction=0;
    require(close(sampleDamageCavityField({.4,0,-.1},cavityFootprint,boundary),0),
            "legacy circular boundary must remain unchanged when opt-in controls are zero");
    require(close(damageCavityFloorDepth({},cavityFootprint,boundary),
                  cavityFootprint.depth),
            "legacy cavity floor must remain planar");
    boundary.irregularity=.3;
    rejects([&]{damageBoundaryRadiusScale(0,boundary);});
    const std::vector<Vec3d> crackPath{{0,0,0},{1,0,0},{1,1,0}};
    require(close(pathCrackField({1,.5,.2},crackPath,.1),.1) &&
            pathCrackField({3,3,0},crackPath,.1)>1,
            "shared crack field must follow all segments of the canonical path");
    rejects([&]{pathCrackField({},{{0,0,0}},.1);});
    rejects([&]{isThroughHole(.2,-1,.01);});
    const auto fracture=fractureProfileOffset(.23,.2,.5,123,"plank-7");
    require(std::abs(fracture)<=.15+1e-12 &&
            close(fracture,fractureProfileOffset(.23,.2,.5,123,"plank-7")) &&
            fracture!=fractureProfileOffset(.23,.2,.5,124,"plank-7") &&
            close(fractureProfileOffset(0,.2,.5,123,"plank-7"),
                  fractureProfileOffset(1,.2,.5,123,"plank-7")),
            "shared fracture profile must be bounded, stable and closed around a contour");
    rejects([&]{fractureProfileOffset(.5,.2,0,123,"plank-7");});
    DamageNoiseSettings noise;
    noise.seed=42;noise.frequency=2;
    const auto wearNoise=structuredDamageNoise01({.3,.7,1.1},noise);
    require(wearNoise>=0 && wearNoise<=1 &&
            close(wearNoise,structuredDamageNoise01({.3,.7,1.1},noise)),
            "shared structured damage noise must remain bounded and deterministic");
    ++noise.seed;
    require(wearNoise!=structuredDamageNoise01({.3,.7,1.1},noise),
            "authored damage seed must move structured wear without changing geometry input");
    noise.frequency=0;
    rejects([&]{structuredDamageNoise01({.3,.7,1.1},noise);});

    rejects([&]{inputs.edge=std::numeric_limits<double>::quiet_NaN();
                evaluateDamageField(inputs,weights);});
    rejects([&]{edge.width=0;resolveDamageEdgeWidth(edge,1);});
    area.indexRange.reset();area.pathRangeMeters.reset();
    rejects([&]{area.enabled=true;area.projector.scale.x=0;
                sampleDamageArea(area,{2,0,0},"DeckPlank_018");});

    std::cout<<"PASS shared damage field, movable area, edge policy and tiers\n";
    return 0;
}
catch(const std::exception &error)
{
    std::cerr<<error.what()<<'\n';
    return 1;
}
