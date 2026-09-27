#include "DamageField.hpp"
#include <Jerico/ProceduralAuthoring/Geology/GeologyNoise.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>
#include <stdexcept>

namespace Playground
{
namespace
{
void normalized(double value)
{
    if (!std::isfinite(value) || value<0 || value>1)
        throw std::invalid_argument("Damage field input outside [0,1]");
}
double saturate(double value)
{return std::clamp(value,0.0,1.0);}
double smoothstep(double low,double high,double value)
{
    if (!(high>low)) throw std::invalid_argument("Damage interval is not increasing");
    const auto t=saturate((value-low)/(high-low));
    return t*t*(3-2*t);
}
}
double evaluateDamageField(const DamageFieldInputs &inputs,
                           const DamageFieldWeights &weights)
{
    for (double value:{inputs.manual,inputs.edge,inputs.cavity,inputs.impact,
                      inputs.spline,inputs.age,inputs.noise,inputs.protectedMask,
                      weights.edge,weights.cavity,weights.impact,weights.spline,
                      weights.age,weights.noise}) normalized(value);
    const auto procedural=saturate(weights.edge*inputs.edge+
        weights.cavity*inputs.cavity+weights.impact*inputs.impact+
        weights.spline*inputs.spline+weights.age*inputs.age+
        weights.noise*inputs.noise);
    return std::max(inputs.manual,procedural*(1-inputs.protectedMask));
}
double damageRandom01(std::uint64_t seed,std::string_view semanticId,
                      std::string_view operationTag)
{
    const auto hash=[](std::string_view value)
    {
        std::uint64_t result=14695981039346656037ull;
        for (unsigned char byte:value)
        {result^=byte;result*=1099511628211ull;}
        return result;
    };
    return double(proceduralElementId(seed,hash(semanticId),hash(operationTag))>>11)*
        (1.0/9007199254740992.0);
}
double structuredDamageNoise01(Vec3d position,const DamageNoiseSettings &settings)
{
    for (double value:{position.x,position.y,position.z,
                      settings.coordinateScale.x,settings.coordinateScale.y,
                      settings.coordinateScale.z,settings.frequency,
                      settings.lacunarity,settings.roughness})
        if (!std::isfinite(value)) throw std::invalid_argument("Damage noise input non-finite");
    if (settings.frequency<=0 || settings.coordinateScale.x<=0 ||
        settings.coordinateScale.y<=0 || settings.coordinateScale.z<=0 ||
        settings.lacunarity<=0 || settings.roughness<0 || settings.roughness>1 ||
        settings.octaves<1 || settings.octaves>8)
        throw std::invalid_argument("Damage noise settings outside limits");
    const Vec3d q{position.x*settings.frequency*settings.coordinateScale.x,
                  position.y*settings.frequency*settings.coordinateScale.y,
                  position.z*settings.frequency*settings.coordinateScale.z};
    if (!std::isfinite(q.x) || !std::isfinite(q.y) || !std::isfinite(q.z))
        throw std::invalid_argument("Damage noise coordinates non-finite");
    return saturate(.5+.5*PA::evaluateGeologyFbmReference(
        q,settings.seed,settings.octaves,settings.lacunarity,settings.roughness));
}
double damageBoundaryRadiusScale(double angleRadians,const DamageBoundarySettings &settings)
{
    if (!std::isfinite(angleRadians) || !std::isfinite(settings.irregularity) ||
        !std::isfinite(settings.frequency) || !std::isfinite(settings.rimFraction) ||
        settings.irregularity<0 || settings.irregularity>.25 ||
        (settings.irregularity>0 &&
         (settings.frequency<.1 || settings.frequency>20)) ||
        settings.rimFraction<0 || settings.rimFraction>.5)
        throw std::invalid_argument("Damage boundary settings outside limits");
    if (settings.irregularity==0) return 1;
    DamageNoiseSettings noise;
    noise.frequency=settings.frequency;
    noise.octaves=3;
    noise.seed=settings.seed;
    const auto value=structuredDamageNoise01(
        {std::cos(angleRadians),std::sin(angleRadians),0},noise);
    return 1+settings.irregularity*std::clamp(3*(2*value-1),-1.0,1.0);
}
double damageCavityFloorDepth(Vec3d q,const DamageDecalSettings &footprint,
                              const DamageBoundarySettings &boundary)
{
    sampleDamageDecal(q,footprint);
    if (footprint.depth<=0)
        throw std::invalid_argument("Damage cavity requires positive depth");
    damageBoundaryRadiusScale(0,boundary);
    const auto centerDepth=footprint.depth*(1+boundary.irregularity*1.3);
    if (boundary.irregularity==0) return footprint.depth;
    const auto angle=std::atan2(q.y/footprint.radius.y,q.x/footprint.radius.x);
    const auto radius=damageBoundaryRadiusScale(angle,boundary);
    const auto rho=std::hypot(q.x/footprint.radius.x,q.y/footprint.radius.y)/radius;
    if (rho==0) return centerDepth;
    DamageNoiseSettings noise;
    noise.frequency=std::max(1.0,boundary.frequency*.5);
    noise.octaves=3;
    noise.seed=boundary.seed;
    const auto value=structuredDamageNoise01(
        {.55*std::cos(angle),.55*std::sin(angle),.37},noise);
    const auto ringDepth=footprint.depth*(1+boundary.irregularity*2*(.35+.65*value));
    if (rho<=.55) return std::lerp(centerDepth,ringDepth,rho/.55);
    return std::lerp(ringDepth,footprint.depth,std::clamp((rho-.55)/.45,0.0,1.0));
}
double sampleDamageCavityField(Vec3d q,const DamageDecalSettings &footprint,
                               const DamageBoundarySettings &boundary)
{
    sampleDamageDecal(q,footprint);
    if (footprint.depth<=0)
        throw std::invalid_argument("Damage cavity requires positive depth");
    const auto rho=std::hypot(q.x/footprint.radius.x,q.y/footprint.radius.y);
    const auto radius=damageBoundaryRadiusScale(
        std::atan2(q.y/footprint.radius.y,q.x/footprint.radius.x),boundary);
    const auto rimDepth=std::min(footprint.depth*.4,
        std::min(footprint.radius.x,footprint.radius.y)*boundary.rimFraction);
    const auto rim=rimDepth>0 ?
        boundary.rimFraction*std::clamp(1+q.z/rimDepth,0.0,1.0):0;
    return std::max(rho/(radius*(1+rim))-1,
                    std::max(q.z,-q.z-damageCavityFloorDepth(q,footprint,boundary)));
}
double sampleDamageArea(const DamageArea &area,const DamageSampleContext &context)
{
    if (!area.enabled || (!area.semanticPartId.empty() &&
                          area.semanticPartId!=context.semanticPartId) ||
        (!area.semanticRole.empty() && area.semanticRole!=context.semanticRole)) return 0;
    if (area.indexRange)
    {
        const auto [first,last]=*area.indexRange;
        if (first>last) throw std::invalid_argument("Damage index range invalid");
        if (context.partIndex<first || context.partIndex>last) return 0;
    }
    if (area.pathRangeMeters)
    {
        const auto [start,end]=*area.pathRangeMeters;
        if (!std::isfinite(start) || !std::isfinite(end) || start>end ||
            !std::isfinite(context.pathDistanceMeters))
            throw std::invalid_argument("Damage path range invalid");
        if (context.pathDistanceMeters<start || context.pathDistanceMeters>end) return 0;
    }
    normalized(area.amount);
    auto local=projectorCoordinates(area.projector,context.position);
    if (std::abs(local.z)>1) return 0;
    const auto radial=damageBoundaryRadiusScale(
        std::atan2(local.y/area.footprint.radius.y,
                   local.x/area.footprint.radius.x),area.boundary);
    local.x/=radial;local.y/=radial;
    return area.amount*sampleDamageDecal(local,area.footprint).softMask;
}
double sampleDamageArea(const DamageArea &area,Vec3d worldPosition,
                        std::string_view semanticPartId)
{return sampleDamageArea(area,{worldPosition,semanticPartId});}
double resolveDamageEdgeWidth(const DamageEdgeSettings &settings,
                              double characteristicSize)
{
    if (!std::isfinite(settings.width) || settings.width<=0 ||
        !std::isfinite(characteristicSize) || characteristicSize<=0)
        throw std::invalid_argument("Damage edge width or characteristic size invalid");
    const auto width=settings.widthMode==DamageWidthMode::Metric ?
        settings.width:settings.width*characteristicSize;
    if (!std::isfinite(width) || width<=0)
        throw std::invalid_argument("Resolved damage edge width invalid");
    return width;
}
double sampleEdgeDamage(const DamageEdgeSettings &settings,double signedCurvature,
                        double edgeDistance,double characteristicSize,
                        double pathFraction,double semanticMask)
{
    for (double value:{signedCurvature,pathFraction,semanticMask,
                      settings.curvatureStart,settings.curvatureEnd,
                      settings.taperFraction,settings.amount})
        if (!std::isfinite(value)) throw std::invalid_argument("Damage edge input non-finite");
    if (std::isnan(edgeDistance) || edgeDistance<0 || pathFraction<0 || pathFraction>1 ||
        settings.taperFraction<0 || settings.taperFraction>.5 ||
        !(settings.curvatureEnd>settings.curvatureStart))
        throw std::invalid_argument("Damage edge settings outside limits");
    normalized(semanticMask);normalized(settings.amount);
    if (settings.curvaturePolicy!=DamageCurvaturePolicy::Convex &&
        settings.curvaturePolicy!=DamageCurvaturePolicy::Concave &&
        settings.curvaturePolicy!=DamageCurvaturePolicy::Both)
        throw std::invalid_argument("Damage curvature policy invalid");
    const auto width=resolveDamageEdgeWidth(settings,characteristicSize);
    if (std::isinf(edgeDistance)) return 0;
    const auto eligibleCurvature=settings.curvaturePolicy==DamageCurvaturePolicy::Convex ?
        std::max(0.0,signedCurvature):
        settings.curvaturePolicy==DamageCurvaturePolicy::Concave ?
            std::max(0.0,-signedCurvature):std::abs(signedCurvature);
    const auto curvature=smoothstep(settings.curvatureStart,settings.curvatureEnd,
                                    eligibleCurvature);
    const auto edge=1-smoothstep(0,width,edgeDistance);
    const auto taper=settings.taperFraction>0 ?
        smoothstep(0,settings.taperFraction,pathFraction)*
        smoothstep(0,settings.taperFraction,1-pathFraction):1;
    return saturate(settings.amount*curvature*edge*taper*semanticMask);
}
DamageTier selectDamageTier(const DamageNeeds &needs)
{
    if (needs.dynamicMotion) return DamageTier::Physics;
    if (needs.interaction) return DamageTier::PreparedVariant;
    if (needs.penetration) return DamageTier::LocalSdf;
    if (needs.silhouette || needs.collision) return DamageTier::Geometry;
    if (needs.normalDetail) return DamageTier::NormalBake;
    if (needs.visual) return DamageTier::Material;
    return DamageTier::None;
}
double boundedDamageAmplitude(double requestedMeters,double localFeatureSizeMeters,
                              std::optional<double> measuredThicknessMeters)
{
    if (!std::isfinite(requestedMeters) || !std::isfinite(localFeatureSizeMeters) ||
        localFeatureSizeMeters<=0 ||
        (measuredThicknessMeters && (!std::isfinite(*measuredThicknessMeters) ||
                                     *measuredThicknessMeters<=0)))
        throw std::invalid_argument("Damage displacement scale invalid");
    auto limit=.2*localFeatureSizeMeters;
    if (measuredThicknessMeters) limit=std::min(limit,.2 * *measuredThicknessMeters);
    return std::clamp(requestedMeters,-limit,limit);
}
double subtractDamageCavity(double hostField,double cutterField)
{
    if (!std::isfinite(hostField) || !std::isfinite(cutterField))
        throw std::invalid_argument("Cavity fields must be finite");
    return std::max(hostField,-cutterField);
}
bool isThroughHole(double depth,double hostThickness,double exitMargin)
{
    if (!std::isfinite(depth) || !std::isfinite(hostThickness) ||
        !std::isfinite(exitMargin) || depth<0 || hostThickness<=0 || exitMargin<0)
        throw std::invalid_argument("Through-hole dimensions are invalid");
    return depth>=hostThickness+exitMargin;
}
bool isFreshDamageInterior(double hostField,double cutterField,
                           double resultField,double tolerance)
{
    if (!std::isfinite(hostField) || !std::isfinite(cutterField) ||
        !std::isfinite(resultField) || !std::isfinite(tolerance) || tolerance<=0)
        throw std::invalid_argument("Fresh-interior field inputs are invalid");
    return std::abs(resultField)<tolerance && hostField< -tolerance &&
           std::abs(cutterField)<tolerance;
}
double pathCrackField(Vec3d position,const std::vector<Vec3d> &path,
                      double radius)
{
    if (path.size()<2 || path.size()>4096)
        throw std::invalid_argument("Crack path point budget invalid");
    double field=std::numeric_limits<double>::infinity();
    for (std::size_t i=1;i<path.size();++i)
        field=std::min(field,segmentCrackField(position,path[i-1],path[i],radius));
    return field;
}
double fractureProfileOffset(double contourFraction,double amplitudeMeters,
                             double localFeatureSizeMeters,std::uint64_t seed,
                             std::string_view semanticId)
{
    if (!std::isfinite(contourFraction) || contourFraction<0 || contourFraction>1 ||
        !std::isfinite(amplitudeMeters) || amplitudeMeters<0 ||
        !std::isfinite(localFeatureSizeMeters) || localFeatureSizeMeters<=0)
        throw std::invalid_argument("Fracture profile settings outside limits");
    const auto amplitude=std::min(amplitudeMeters,localFeatureSizeMeters*.3);
    const auto phaseA=damageRandom01(seed,semanticId,"fracture-low");
    const auto phaseB=damageRandom01(seed,semanticId,"fracture-high");
    return amplitude*(.7*std::sin(2*std::numbers::pi*(2*contourFraction+phaseA))+
                      .3*std::sin(2*std::numbers::pi*(5*contourFraction+phaseB)));
}
} // namespace Playground
