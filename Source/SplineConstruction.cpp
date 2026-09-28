#include "SplineConstruction.hpp"
#include "AutoUv.hpp"
#include "DamageField.hpp"
#include "DamageGeometry.hpp"
#include "DamageUvTransfer.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <functional>
#include <numbers>
#include <optional>
#include <set>
#include <stdexcept>
#include <tuple>

namespace Playground::Construction
{
namespace
{
Vec3d add(Vec3d a, Vec3d b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
Vec3d sub(Vec3d a, Vec3d b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
Vec3d mul(Vec3d a, double k) { return {a.x * k, a.y * k, a.z * k}; }
double dot(Vec3d a, Vec3d b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
Vec3d cross(Vec3d a, Vec3d b)
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
double length(Vec3d a) { return std::sqrt(dot(a, a)); }
Vec3d unit(Vec3d a)
{
    const double magnitude = length(a);
    if (!(magnitude > 1e-12))
        throw std::runtime_error("Spline has an undefined tangent");
    return mul(a, 1.0 / magnitude);
}
std::size_t pointIndex(const PA::ProceduralSpline &spline, std::ptrdiff_t index)
{
    const auto count = std::ptrdiff_t(spline.points.size());
    return std::size_t(spline.closed ? (index % count + count) % count :
                       std::clamp(index, std::ptrdiff_t(0), count - 1));
}
struct CurvePoint
{
    Vec3d position{}, derivative{};
    double width{}, roll{};
    std::size_t segment{};
};
CurvePoint evaluateSegment(const PA::ProceduralSpline &spline, std::size_t segment, double t)
{
    const auto &start = spline.points[segment];
    const auto &end = spline.points[pointIndex(spline, std::ptrdiff_t(segment) + 1)];
    CurvePoint point;
    point.segment = segment;
    point.width = std::lerp(start.widthMeters, end.widthMeters, t);
    point.roll = std::lerp(start.rollRadians, end.rollRadians, t);
    if (spline.interpolation == PA::SplineInterpolation::Polyline)
    {
        point.position = add(start.position, mul(sub(end.position, start.position), t));
        point.derivative = sub(end.position, start.position);
    }
    else if (spline.interpolation == PA::SplineInterpolation::CubicBezier)
    {
        const auto b = add(start.position, start.outgoingHandle);
        const auto c = add(end.position, end.incomingHandle);
        const double u = 1 - t;
        point.position = add(add(mul(start.position, u * u * u), mul(b, 3 * u * u * t)),
                             add(mul(c, 3 * u * t * t), mul(end.position, t * t * t)));
        point.derivative = add(add(mul(sub(b, start.position), 3 * u * u),
                                   mul(sub(c, b), 6 * u * t)),
                               mul(sub(end.position, c), 3 * t * t));
    }
    else
    {
        const auto previous = spline.points[pointIndex(spline, std::ptrdiff_t(segment) - 1)].position;
        const auto next = spline.points[pointIndex(spline, std::ptrdiff_t(segment) + 2)].position;
        const auto quadratic = add(add(previous, mul(start.position, -2.5)),
                                   add(mul(end.position, 2), mul(next, -0.5)));
        const auto cubic = add(add(mul(previous, -0.5), mul(start.position, 1.5)),
                               add(mul(end.position, -1.5), mul(next, 0.5)));
        point.position = mul(add(add(mul(start.position, 2), mul(sub(end.position, previous), t)),
                                 add(mul(quadratic, 2 * t * t), mul(cubic, 2 * t * t * t))), 0.5);
        point.derivative = mul(add(add(sub(end.position, previous), mul(quadratic, 4 * t)),
                                   mul(cubic, 6 * t * t)), 0.5);
    }
    return point;
}
double deviationFromChord(Vec3d point, Vec3d a, Vec3d b)
{
    const auto chord = sub(b, a);
    const auto squared = dot(chord, chord);
    if (squared < 1e-20)
        return length(sub(point, a));
    const auto t = std::clamp(dot(sub(point, a), chord) / squared, 0.0, 1.0);
    return length(sub(point, add(a, mul(chord, t))));
}
double turn(Vec3d a, Vec3d b)
{
    return std::acos(std::clamp(dot(unit(a), unit(b)), -1.0, 1.0));
}
Vec3d rotateAbout(Vec3d vector, Vec3d axis, double angle)
{
    return add(add(mul(vector, std::cos(angle)), mul(cross(axis, vector), std::sin(angle))),
               mul(axis, dot(axis, vector) * (1 - std::cos(angle))));
}
Jerico::Guid damageRegion(Jerico::Guid source,const PathDamageRule &rule)
{
    std::uint64_t hash=Jerico::stableGuidHash(source);
    for (unsigned char byte:rule.id)
    {hash^=byte;hash*=1099511628211ull;}
    for (unsigned shift=0;shift<64;shift+=8)
    {hash^=(rule.cutter.seed>>shift)&255u;hash*=1099511628211ull;}
    return Solid::region(2409,hash);
}
void rebuildDamageNormals(MeshResult &result)
{
    const auto &mesh=result.mesh;
    std::set<Jerico::Guid> fresh;
    for (const auto &region:result.regions)
        if (region.role==SurfaceRole::DamageInterior) fresh.insert(region.id);
    std::vector<Vec3d> faces(mesh.indices.size()/3);
    std::vector<std::vector<std::size_t>> adjacent(mesh.positions.size());
    for (std::size_t face=0;face<faces.size();++face)
    {
        const auto a=mesh.indices[face*3],b=mesh.indices[face*3+1],
                   c=mesh.indices[face*3+2];
        if (a>=mesh.positions.size() || b>=mesh.positions.size() ||
            c>=mesh.positions.size())
            throw std::runtime_error("Damaged spline has an invalid triangle");
        const auto area=cross(sub(mesh.positions[b],mesh.positions[a]),
                              sub(mesh.positions[c],mesh.positions[a]));
        if (length(area)<=1e-12)
            throw std::runtime_error("Damaged spline has a degenerate triangle");
        faces[face]=unit(area);
        for (const auto vertex:{a,b,c}) adjacent[vertex].push_back(face);
    }
    result.normals.assign(mesh.positions.size(),{});
    result.cornerNormals.resize(mesh.indices.size());
    for (std::size_t face=0;face<faces.size();++face)
        for (int corner=0;corner<3;++corner)
        {
            const auto vertex=mesh.indices[face*3+corner];
            auto normal=faces[face];
            if (!fresh.contains(mesh.triangleRegions[face]))
            {
                Vec3d smooth{};
                for (const auto neighbor:adjacent[vertex])
                    if (mesh.triangleRegions[neighbor]==mesh.triangleRegions[face] &&
                        dot(faces[neighbor],faces[face])>.82)
                        smooth=add(smooth,faces[neighbor]);
                if (length(smooth)>1e-12) normal=unit(smooth);
            }
            result.cornerNormals[face*3+corner]=normal;
            result.normals[vertex]=add(result.normals[vertex],normal);
        }
    for (auto &normal:result.normals)
        if (length(normal)>1e-12) normal=unit(normal);
}
}

PathResult samplePath(const PA::ProceduralSpline &spline, const PathQuality &quality)
{
    const auto valid = PA::validateProceduralSpline(spline);
    if (!valid.hasValue())
        throw std::runtime_error(valid.error().message);
    if (!std::isfinite(quality.maximumDeviationMeters) || quality.maximumDeviationMeters < 1e-5 ||
        quality.maximumDeviationMeters > 1 ||
        !std::isfinite(quality.maximumTangentAngleRadians) ||
        quality.maximumTangentAngleRadians < 0.001 ||
        quality.maximumTangentAngleRadians > std::numbers::pi / 2 ||
        !std::isfinite(quality.maximumStepMeters) || quality.maximumStepMeters < 0.005 ||
        quality.maximumStepMeters > 10 || quality.maximumSamples < 8 ||
        quality.maximumSamples > 65536)
        throw std::runtime_error("Spline quality exceeds admitted tolerances or sample budget");

    const auto segmentCount = spline.closed ? spline.points.size() : spline.points.size() - 1;
    std::vector<CurvePoint> nodes;
    nodes.reserve(std::min(quality.maximumSamples, segmentCount * 16));
    nodes.push_back(evaluateSegment(spline, 0, 0));
    double measuredDeviation = 0;
    std::function<void(std::size_t, double, CurvePoint, double, CurvePoint, unsigned)> refine;
    refine = [&](std::size_t segment, double t0, CurvePoint a, double t1, CurvePoint b, unsigned depth)
    {
        const auto q1 = evaluateSegment(spline, segment, std::lerp(t0, t1, 0.25));
        const auto middle = evaluateSegment(spline, segment, 0.5 * (t0 + t1));
        const auto q3 = evaluateSegment(spline, segment, std::lerp(t0, t1, 0.75));
        const auto deviation = std::max({deviationFromChord(q1.position, a.position, b.position),
                                         deviationFromChord(middle.position, a.position, b.position),
                                         deviationFromChord(q3.position, a.position, b.position)});
        const auto angle = std::max({turn(a.derivative, q1.derivative),
                                     turn(q1.derivative, middle.derivative),
                                     turn(middle.derivative, q3.derivative),
                                     turn(q3.derivative, b.derivative)});
        if (deviation <= quality.maximumDeviationMeters &&
            angle <= quality.maximumTangentAngleRadians &&
            length(sub(b.position, a.position)) <= quality.maximumStepMeters)
        {
            if (nodes.size() >= quality.maximumSamples)
                throw std::runtime_error("Spline exceeds sample budget at requested quality");
            measuredDeviation = std::max(measuredDeviation, deviation);
            nodes.push_back(b);
            return;
        }
        if (depth >= 18)
            throw std::runtime_error("Spline cannot satisfy quality near a cusp or tight bend");
        refine(segment, t0, a, 0.5 * (t0 + t1), middle, depth + 1);
        refine(segment, 0.5 * (t0 + t1), middle, t1, b, depth + 1);
    };
    for (std::size_t segment = 0; segment < segmentCount; ++segment)
        refine(segment, 0, evaluateSegment(spline, segment, 0), 1,
               evaluateSegment(spline, segment, 1), 0);

    PathResult result;
    result.maximumMeasuredDeviationMeters = measuredDeviation;
    result.frames.reserve(nodes.size());
    for (std::size_t i = 0; i < nodes.size(); ++i)
    {
        const auto &node = nodes[i];
        PathFrame frame;
        frame.position = node.position;
        frame.tangent = unit(node.derivative);
        frame.widthMeters = node.width;
        frame.rollRadians = node.roll;
        frame.segment = node.segment;
        if (i)
            result.lengthMeters += length(sub(node.position, nodes[i - 1].position));
        frame.distanceMeters = result.lengthMeters;
        result.frames.push_back(frame);
    }
    if (!(result.lengthMeters > 1e-6))
        throw std::runtime_error("Spline path has no measurable length");

    auto &first = result.frames.front();
    const Vec3d seed = std::abs(first.tangent.y) < 0.9 ? Vec3d{0, 1, 0} : Vec3d{1, 0, 0};
    first.normal = unit(sub(seed, mul(first.tangent, dot(seed, first.tangent))));
    first.binormal = unit(cross(first.tangent, first.normal));
    for (std::size_t i = 1; i < result.frames.size(); ++i)
    {
        const auto &previous = result.frames[i - 1];
        auto &current = result.frames[i];
        const auto chord = sub(current.position, previous.position);
        const auto chordSquared = dot(chord, chord);
        if (chordSquared <= 1e-18)
            throw std::runtime_error("Spline sampling produced coincident path frames");
        const auto reflectedNormal = sub(previous.normal,
                                         mul(chord, 2 * dot(chord, previous.normal) / chordSquared));
        const auto reflectedTangent = sub(previous.tangent,
                                          mul(chord, 2 * dot(chord, previous.tangent) / chordSquared));
        const auto second = sub(current.tangent, reflectedTangent);
        const auto secondSquared = dot(second, second);
        auto normal = secondSquared > 1e-18 ?
            sub(reflectedNormal, mul(second, 2 * dot(second, reflectedNormal) / secondSquared)) :
            reflectedNormal;
        normal = unit(sub(normal, mul(current.tangent, dot(normal, current.tangent))));
        current.normal = normal;
        current.binormal = unit(cross(current.tangent, normal));
    }
    if (spline.closed)
    {
        auto &last = result.frames.back();
        if (dot(last.tangent, first.tangent) < 0.999)
            throw std::runtime_error("Closed spline has a sharp tangent seam; use a smooth bend");
        const double seamAngle = std::atan2(dot(first.tangent, cross(last.normal, first.normal)),
                                            dot(last.normal, first.normal));
        for (auto &frame : result.frames)
        {
            frame.normal = unit(rotateAbout(frame.normal, frame.tangent,
                                            seamAngle * frame.distanceMeters / result.lengthMeters));
            frame.binormal = unit(cross(frame.tangent, frame.normal));
        }
        last.normal = first.normal;
        last.binormal = first.binormal;
    }
    for (auto &frame : result.frames)
    {
        frame.normal = unit(rotateAbout(frame.normal, frame.tangent, frame.rollRadians));
        frame.binormal = unit(cross(frame.tangent, frame.normal));
    }
    return result;
}
PathFrame frameAtDistance(const PathResult &path, double distance)
{
    if (path.frames.size()<2 || !(path.lengthMeters>0) || !std::isfinite(distance) ||
        distance<0 || distance>path.lengthMeters+1e-9)
        throw std::runtime_error("Distance is outside the sampled spline path");
    const auto right = std::lower_bound(path.frames.begin(), path.frames.end(), distance,
        [](const PathFrame &frame, double value) { return frame.distanceMeters < value; });
    const auto upper = std::size_t(right - path.frames.begin());
    const auto lower = upper ? upper - 1 : 0;
    const auto &a = path.frames[lower];
    const auto &b = path.frames[std::min(upper, path.frames.size() - 1)];
    const double span = b.distanceMeters - a.distanceMeters;
    const double t = span > 0 ? std::clamp((distance - a.distanceMeters) / span, 0.0, 1.0) : 0;
    PathFrame frame;
    frame.distanceMeters = distance;
    frame.position = add(a.position, mul(sub(b.position, a.position), t));
    frame.tangent = unit(add(a.tangent, mul(sub(b.tangent, a.tangent), t)));
    auto normal = add(a.normal, mul(sub(b.normal, a.normal), t));
    frame.normal = unit(sub(normal, mul(frame.tangent, dot(normal, frame.tangent))));
    frame.binormal = unit(cross(frame.tangent, frame.normal));
    frame.widthMeters = std::lerp(a.widthMeters, b.widthMeters, t);
    frame.rollRadians = std::lerp(a.rollRadians, b.rollRadians, t);
    frame.segment = a.segment;
    return frame;
}
std::vector<RepeatPlacement> repeatAlongPath(const PathResult &path, const RepeatSettings &settings)
{
    if (path.frames.size() < 2 || !(path.lengthMeters > 0) ||
        !std::isfinite(settings.spacingMeters) || settings.spacingMeters <= 0 ||
        !std::isfinite(settings.offsetMeters) || settings.offsetMeters < 0 ||
        !std::isfinite(settings.startPaddingMeters) || settings.startPaddingMeters < 0 ||
        !std::isfinite(settings.endPaddingMeters) || settings.endPaddingMeters < 0 ||
        settings.maximumParts < 1 || settings.maximumParts > 65536)
        throw std::runtime_error("Invalid arc-distance repeat settings");
    const double first = settings.offsetMeters +
        std::ceil(std::max(0.0, settings.startPaddingMeters - settings.offsetMeters) /
                  settings.spacingMeters) * settings.spacingMeters;
    const double end = path.lengthMeters - settings.endPaddingMeters;
    if (first > end)
        return {};
    if ((end - first) / settings.spacingMeters + 1 > double(settings.maximumParts) + 1e-9)
        throw std::runtime_error("Arc-distance repeat exceeds part budget");
    std::vector<RepeatPlacement> placements;
    for (std::size_t index = 0; index < settings.maximumParts; ++index)
    {
        const double distance = first + double(index) * settings.spacingMeters;
        if (distance > end + 1e-9)
            break;
        placements.push_back({frameAtDistance(path,distance)});
    }
    return placements;
}
ConstructionResult buildConstruction(const PA::ProceduralSpline &spline,
                                     const ConstructionSettings &settings)
{
    const auto started = std::chrono::steady_clock::now();
    const auto positive = [](double value) { return std::isfinite(value) && value > 0; };
    if (!positive(settings.profileScale) || !positive(settings.metresPerTileU) ||
        !positive(settings.metresPerTileV) || !positive(settings.profileChordErrorMeters) ||
        !positive(settings.profileMaximumAngleRadians) || !positive(settings.repeatCount) ||
        !std::isfinite(settings.seamAngleRadians) ||
        !std::isfinite(settings.profileRotationRadians) || settings.maximumVertices < 8 ||
        settings.maximumVertices > 500000 || settings.profileScale > 1000 ||
        settings.profileChordErrorMeters > 1 ||
        settings.profileMaximumAngleRadians > std::numbers::pi/2 ||
        !std::isfinite(settings.taperLengthMeters) || settings.taperLengthMeters<0 ||
        !std::isfinite(settings.tipScale) || settings.tipScale<=0 || settings.tipScale>1 ||
        !std::isfinite(settings.ribAmplitude) || settings.ribAmplitude<0 ||
        settings.ribAmplitude>.4 || !std::isfinite(settings.ribSpacingMeters) ||
        settings.ribSpacingMeters<.04 ||
        settings.ribSpacingMeters>10 || !std::isfinite(settings.twistTurns) ||
        std::abs(settings.twistTurns)>16 ||
        !std::isfinite(settings.breakFraction) ||
        settings.breakFraction<=0 || settings.breakFraction>=1 ||
        !positive(settings.breakGapMeters) ||
        !std::isfinite(settings.breakRoughness) ||
        settings.breakRoughness<0 || settings.breakRoughness>1)
        throw std::runtime_error("Invalid profile, UV or vertex-budget settings");
    const bool radial = settings.profile == ProfileKind::Circle || settings.profile == ProfileKind::Ring;
    if ((radial && (!positive(settings.radiusMeters) || settings.radiusMeters > 1000)) ||
        (!radial && (!positive(settings.widthMeters) || !positive(settings.heightMeters))) ||
        settings.widthMeters > 1000 || settings.heightMeters > 1000 ||
        settings.wallThicknessMeters > 1000 || settings.cornerRadiusMeters > 1000 ||
        (settings.profile == ProfileKind::Ring &&
         (!positive(settings.wallThicknessMeters) || settings.wallThicknessMeters >= settings.radiusMeters)) ||
        (settings.profile == ProfileKind::RoundedRectangle &&
         (!positive(settings.cornerRadiusMeters) ||
          settings.cornerRadiusMeters >= 0.5 * std::min(settings.widthMeters, settings.heightMeters))) ||
        ((settings.profile == ProfileKind::Channel || settings.profile == ProfileKind::IBeam) &&
         (!positive(settings.wallThicknessMeters) ||
          settings.wallThicknessMeters >= 0.5 * std::min(settings.widthMeters, settings.heightMeters))))
        throw std::runtime_error("Invalid profile dimensions or wall thickness");
    if (settings.damagePaths.size()>64)
        throw std::runtime_error("Spline damage path budget exceeded");
    std::set<std::string> damageIds;
    for (const auto &rule:settings.damagePaths)
    {
        if (rule.id.empty() || rule.id.size()>64 ||
            !damageIds.insert(rule.id).second)
            throw std::runtime_error("Spline damage path ID is invalid or repeated");
        sampleDamagePathField(rule.cutter.projector.center,rule.cutter);
    }
    ConstructionResult result;
    auto quality=settings.pathQuality;
    if (settings.ribAmplitude>0)
        quality.maximumStepMeters=std::min(quality.maximumStepMeters,
                                          settings.ribSpacingMeters/8);
    result.path = samplePath(spline, quality);
    if (spline.closed && std::abs(settings.twistTurns-std::round(settings.twistTurns))>1e-8)
        throw std::runtime_error("Closed spline twist needs a whole number of turns");
    if (settings.repeatEnabled)
        result.repeats = repeatAlongPath(result.path, settings.repeat);
    std::optional<std::size_t> breakLeftRing;
    if (settings.breakEnabled)
    {
        if (spline.closed)
            throw std::runtime_error("Open path required for a spline fracture");
        const auto center=settings.breakFraction*result.path.lengthMeters;
        const auto left=center-settings.breakGapMeters*.5;
        const auto right=center+settings.breakGapMeters*.5;
        if (left<=.01 || right>=result.path.lengthMeters-.01)
            throw std::runtime_error("Spline fracture must leave both ends intact");
        const auto leftFrame=frameAtDistance(result.path,left);
        const auto rightFrame=frameAtDistance(result.path,right);
        std::vector<PathFrame> frames;
        frames.reserve(result.path.frames.size()+2);
        for (const auto &frame:result.path.frames)
            if (frame.distanceMeters<left-1e-8) frames.push_back(frame);
        frames.push_back(leftFrame);
        breakLeftRing=frames.size()-1;
        frames.push_back(rightFrame);
        for (const auto &frame:result.path.frames)
            if (frame.distanceMeters>right+1e-8) frames.push_back(frame);
        result.path.frames=std::move(frames);
        std::erase_if(result.repeats,[&](const RepeatPlacement &placement)
            {return placement.frame.distanceMeters>left &&
                    placement.frame.distanceMeters<right;});
    }

    using Point = std::array<double, 2>;
    std::vector<std::vector<Point>> contours;
    if (radial)
    {
        const double sagitta = std::min(settings.profileChordErrorMeters / settings.radiusMeters, 1.0);
        const double chordAngle = 2 * std::acos(1 - sagitta);
        const double requiredSegments = std::ceil(2 * std::numbers::pi /
            std::min(chordAngle, settings.profileMaximumAngleRadians));
        if (!std::isfinite(requiredSegments) || requiredSegments > 256)
            throw std::runtime_error("Circle profile needs more than 256 segments at requested tolerance");
        const auto segments = std::max(std::size_t(8),std::size_t(requiredSegments));
        auto circle = [&](double radius, bool inner)
        {
            std::vector<Point> points;
            points.reserve(segments);
            for (std::size_t i = 0; i < segments; ++i)
            {
                const double angle = settings.seamAngleRadians +
                    (inner ? -1.0 : 1.0) * 2 * std::numbers::pi * double(i) / double(segments);
                points.push_back({radius * std::cos(angle), radius * std::sin(angle)});
            }
            return points;
        };
        contours.push_back(circle(settings.radiusMeters, false));
        if (settings.profile == ProfileKind::Ring)
            contours.push_back(circle(settings.radiusMeters - settings.wallThicknessMeters, true));
    }
    else if (settings.profile == ProfileKind::Rectangle)
    {
        const double x = settings.widthMeters * 0.5, y = settings.heightMeters * 0.5;
        contours.push_back({{-x, -y}, {x, -y}, {x, y}, {-x, y}});
    }
    else if (settings.profile == ProfileKind::RoundedRectangle)
    {
        const double x = settings.widthMeters * 0.5, y = settings.heightMeters * 0.5;
        const double r = settings.cornerRadiusMeters;
        auto &outline = contours.emplace_back();
        const double requiredPerCorner = std::ceil((std::numbers::pi / 2) /
            std::min(2 * std::acos(1 - std::min(settings.profileChordErrorMeters / r, 1.0)),
                     settings.profileMaximumAngleRadians));
        if (!std::isfinite(requiredPerCorner) || requiredPerCorner > 64)
            throw std::runtime_error("Rounded profile needs more than 64 segments per corner at requested tolerance");
        const auto perCorner = std::max(std::size_t(2),std::size_t(requiredPerCorner));
        const Point centers[] = {{x-r,-y+r},{x-r,y-r},{-x+r,y-r},{-x+r,-y+r}};
        const double starts[] = {-std::numbers::pi/2,0,std::numbers::pi/2,std::numbers::pi};
        for (std::size_t corner = 0; corner < 4; ++corner)
            for (std::size_t i = 0; i <= perCorner; ++i)
            {
                const double angle = starts[corner] + double(i) *
                    (std::numbers::pi / 2) / double(perCorner);
                outline.push_back({centers[corner][0] + r * std::cos(angle),
                                   centers[corner][1] + r * std::sin(angle)});
            }
    }
    else if (settings.profile == ProfileKind::Channel)
    {
        const double x = settings.widthMeters * 0.5, y = settings.heightMeters * 0.5;
        const double t = settings.wallThicknessMeters;
        contours.push_back({{-x,-y},{x,-y},{x,y},{x-t,y},{x-t,-y+t},{-x+t,-y+t},{-x+t,y},{-x,y}});
    }
    else
    {
        const double x = settings.widthMeters * 0.5, y = settings.heightMeters * 0.5;
        const double t = settings.wallThicknessMeters;
        contours.push_back({{-x,-y},{x,-y},{x,-y+t},{t*0.5,-y+t},{t*0.5,y-t},
                            {x,y-t},{x,y},{-x,y},{-x,y-t},{-t*0.5,y-t},
                            {-t*0.5,-y+t},{-x,-y+t}});
    }
    result.profileSegments = contours.front().size();
    std::size_t ringSize = 0;
    for (const auto &contour : contours)
        ringSize += contour.size() + 1;
    const auto ringCount = result.path.frames.size();
    if (ringSize > settings.maximumVertices / ringCount ||
        ringSize * ringCount + 2 * ringSize + 8 * result.repeats.size() > settings.maximumVertices)
        throw std::runtime_error("Sweep exceeds vertex budget at requested quality");

    std::vector<std::vector<double>> perimeter;
    std::vector<std::size_t> contourStart;
    for (const auto &contour : contours)
    {
        const auto contourIndex=perimeter.size();
        contourStart.push_back(perimeter.empty() ? 0 : contourStart.back() + contours[perimeter.size()-1].size()+1);
        auto &distance = perimeter.emplace_back(contour.size()+1, 0);
        for (std::size_t i = 1; i <= contour.size(); ++i)
        {
            if (radial)
            {
                const double radius=contourIndex==0 ? settings.radiusMeters :
                    settings.radiusMeters-settings.wallThicknessMeters;
                distance[i]=radius*2*std::numbers::pi*double(i)/double(contour.size());
            }
            else
            {
                const auto &a = contour[i-1], &b = contour[i%contour.size()];
                distance[i] = distance[i-1] + std::hypot(b[0]-a[0], b[1]-a[1]);
            }
        }
    }
    auto surfaceScale = [&](const PathFrame &frame)
    {
        double taper=1;
        if (!spline.closed && settings.taperLengthMeters>0)
        {
            const auto edge=std::min(frame.distanceMeters,
                                     result.path.lengthMeters-frame.distanceMeters);
            const auto t=std::clamp(edge/settings.taperLengthMeters,0.0,1.0);
            taper=std::lerp(settings.tipScale,1.0,t*t*(3-2*t));
        }
        const auto cycles=spline.closed?
            std::max(1.0,std::round(result.path.lengthMeters/settings.ribSpacingMeters)):
            result.path.lengthMeters/settings.ribSpacingMeters;
        const auto rib=1+settings.ribAmplitude*std::sin(
            2*std::numbers::pi*cycles*frame.distanceMeters/result.path.lengthMeters);
        return settings.profileScale*frame.widthMeters*taper*rib;
    };
    auto surfacePosition = [&](const PathFrame &frame, Point point,std::size_t ring)
    {
        const auto angle=settings.profileRotationRadians+
            2*std::numbers::pi*settings.twistTurns*
            frame.distanceMeters/result.path.lengthMeters;
        const double c = std::cos(angle),s = std::sin(angle);
        const auto scale=surfaceScale(frame);
        const double x = scale*(point[0]*c-point[1]*s);
        const double y = scale*(point[0]*s+point[1]*c);
        auto position=add(frame.position,add(mul(frame.normal,x),mul(frame.binormal,y)));
        if (breakLeftRing && (ring==*breakLeftRing || ring==*breakLeftRing+1))
        {
            const auto transverse=radial?settings.radiusMeters:
                std::min(settings.widthMeters,settings.heightMeters)*.5;
            const auto amplitude=settings.breakRoughness*
                std::min(settings.breakGapMeters*.2,transverse*.3);
            const auto contourFraction=.5+std::atan2(point[1],point[0])/
                (2*std::numbers::pi);
            const std::string_view id(reinterpret_cast<const char *>(
                spline.splineGuid.bytes.data()),spline.splineGuid.bytes.size());
            position=add(position,mul(frame.tangent,
                fractureProfileOffset(contourFraction,amplitude,transverse,
                                      settings.breakSeed,id)));
        }
        return position;
    };
    auto addVertex = [&](Vec3d position, float u, float v)
    {
        const auto index = std::uint32_t(result.mesh.mesh.positions.size());
        result.mesh.mesh.positions.push_back(position);
        result.vertexUV.push_back({u,v});
        return index;
    };
    auto addTriangle = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c,
                           std::array<float,2> uvA, std::array<float,2> uvB,
                           std::array<float,2> uvC)
    {
        auto normal = unit(cross(sub(result.mesh.mesh.positions[b], result.mesh.mesh.positions[a]),
                                 sub(result.mesh.mesh.positions[c], result.mesh.mesh.positions[a])));
        result.mesh.mesh.indices.insert(result.mesh.mesh.indices.end(), {a,b,c});
        result.mesh.cornerNormals.insert(result.mesh.cornerNormals.end(), 3, normal);
        result.cornerUV.insert(result.cornerUV.end(), {uvA,uvB,uvC});
        result.mesh.mesh.triangleRegions.push_back(spline.splineGuid);
    };
    for (std::size_t ring=0;ring<result.path.frames.size();++ring)
    {
        const auto &frame=result.path.frames[ring];
        const float u = float(settings.uvMode == UvMode::WorldMetric ?
            frame.distanceMeters / settings.metresPerTileU :
            settings.uvMode == UvMode::Stretch01 ? frame.distanceMeters / result.path.lengthMeters :
            settings.repeatCount * frame.distanceMeters / result.path.lengthMeters);
        for (std::size_t contourIndex = 0; contourIndex < contours.size(); ++contourIndex)
        {
            const auto &outline = contours[contourIndex];
            const auto &distances = perimeter[contourIndex];
            for (std::size_t i = 0; i <= outline.size(); ++i)
            {
                const float v = float(distances[i] * surfaceScale(frame) /
                    settings.metresPerTileV);
                addVertex(surfacePosition(frame, outline[i%outline.size()],ring), u, v);
            }
        }
    }
    result.bodyVertexCount = result.mesh.mesh.positions.size();
    for (std::size_t ring = 0; ring + 1 < ringCount; ++ring)
    {
        if (breakLeftRing && ring==*breakLeftRing) continue;
        for (std::size_t contourIndex = 0; contourIndex < contours.size(); ++contourIndex)
            for (std::size_t side = 0; side < contours[contourIndex].size(); ++side)
            {
                const auto a = std::uint32_t(ring * ringSize + contourStart[contourIndex] + side);
                const auto b = a + 1, c = a + std::uint32_t(ringSize), d = c + 1;
                addTriangle(a,b,c,result.vertexUV[a],result.vertexUV[b],result.vertexUV[c]);
                addTriangle(b,d,c,result.vertexUV[b],result.vertexUV[d],result.vertexUV[c]);
                if (radial)
                {
                    const auto orientation=contourIndex==0?1.0:-1.0;
                    const auto normalAt=[&](std::uint32_t vertex,const PathFrame &frame)
                    {
                        const auto offset=sub(result.mesh.mesh.positions[vertex],frame.position);
                        return mul(unit(sub(offset,mul(frame.tangent,dot(offset,frame.tangent)))),
                                   orientation);
                    };
                    const auto size=result.mesh.cornerNormals.size();
                    result.mesh.cornerNormals[size-6]=normalAt(a,result.path.frames[ring]);
                    result.mesh.cornerNormals[size-5]=normalAt(b,result.path.frames[ring]);
                    result.mesh.cornerNormals[size-4]=normalAt(c,result.path.frames[ring+1]);
                    result.mesh.cornerNormals[size-3]=normalAt(b,result.path.frames[ring]);
                    result.mesh.cornerNormals[size-2]=normalAt(d,result.path.frames[ring+1]);
                    result.mesh.cornerNormals[size-1]=normalAt(c,result.path.frames[ring+1]);
                }
            }
    }
    result.bodyTriangleCount = result.mesh.mesh.indices.size()/3;

    auto cap = [&](bool start,std::size_t ring)
    {
        const auto &frame = result.path.frames[ring];
        // Cap corners need planar UVs independent of the side's longitudinal UV.
        auto planar = [&](std::uint32_t index)
        {
            const auto displacement = sub(result.mesh.mesh.positions[index], frame.position);
            return std::array<float,2>{float(dot(displacement,frame.normal)/settings.metresPerTileV),
                                        float(dot(displacement,frame.binormal)/settings.metresPerTileV)};
        };
        auto capTriangle = [&](std::uint32_t a, std::uint32_t b, std::uint32_t c)
        {
            if (start) std::swap(b,c);
            addTriangle(a,b,c,planar(a),planar(b),planar(c));
        };
        if (contours.size() == 2)
        {
            const auto n = contours.front().size();
            const auto outer = std::uint32_t(ring*ringSize), inner = std::uint32_t(ring*ringSize+contourStart[1]);
            for (std::size_t i = 0; i < n; ++i)
            {
                const auto j = (n-i)%n, k = (n-i-1+n)%n;
                capTriangle(outer+std::uint32_t(i),outer+std::uint32_t(i+1),inner+std::uint32_t(j));
                capTriangle(outer+std::uint32_t(i+1),inner+std::uint32_t(k),inner+std::uint32_t(j));
            }
        }
        else
        {
            // Ear clipping handles concave Channel and IBeam profiles as well as round sections.
            std::vector<std::size_t> polygon(contours.front().size());
            for (std::size_t i=0;i<polygon.size();++i) polygon[i]=i;
            const auto &points=contours.front();
            const auto orient=[](Point a,Point b,Point c)
            { return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]); };
            while (polygon.size()>2)
            {
                bool clipped=false;
                for (std::size_t i=0;i<polygon.size();++i)
                {
                    const auto a=polygon[(i+polygon.size()-1)%polygon.size()],
                               b=polygon[i],c=polygon[(i+1)%polygon.size()];
                    if (orient(points[a],points[b],points[c])<=1e-12) continue;
                    bool contains=false;
                    for (const auto p:polygon)
                        if (p!=a && p!=b && p!=c &&
                            orient(points[a],points[b],points[p])>=-1e-12 &&
                            orient(points[b],points[c],points[p])>=-1e-12 &&
                            orient(points[c],points[a],points[p])>=-1e-12)
                            {contains=true;break;}
                    if (contains) continue;
                    const auto base=std::uint32_t(ring*ringSize);
                    capTriangle(base+std::uint32_t(a),base+std::uint32_t(b),base+std::uint32_t(c));
                    polygon.erase(polygon.begin()+std::ptrdiff_t(i));
                    clipped=true;
                    break;
                }
                if (!clipped) throw std::runtime_error("Profile cap cannot be triangulated");
            }
        }
    };
    if (!spline.closed && settings.capStart) cap(true,0);
    if (!spline.closed && settings.capEnd) cap(false,ringCount-1);
    if (breakLeftRing)
    {cap(false,*breakLeftRing);cap(true,*breakLeftRing+1);}
    for (const auto &placement : result.repeats)
    {
        const auto &frame = placement.frame;
        const double span = radial ? settings.radiusMeters * 2.5 : settings.widthMeters * 0.65;
        const double depth = radial ? settings.radiusMeters * 1.35 : settings.heightMeters * 0.6;
        const double thickness = std::max(0.004, span * 0.08);
        const auto center = add(frame.position, mul(frame.binormal, -depth * frame.widthMeters));
        const Vec3d axis[] = {mul(frame.normal,span*frame.widthMeters),
                              mul(frame.binormal,thickness),mul(frame.tangent,thickness)};
        const auto base = std::uint32_t(result.mesh.mesh.positions.size());
        for (unsigned corner=0;corner<8;++corner)
        {
            const auto point = add(center,add(mul(axis[0],corner&1?1:-1),
                              add(mul(axis[1],corner&2?1:-1),mul(axis[2],corner&4?1:-1))));
            addVertex(point,float(corner&1),float((corner>>1)&1));
        }
        constexpr unsigned faces[][4]={{0,4,6,2},{1,3,7,5},{0,1,5,4},
                                        {2,6,7,3},{0,2,3,1},{4,5,7,6}};
        for (const auto &face:faces)
        {
            const auto a=base+face[0],b=base+face[1],c=base+face[2],d=base+face[3];
            addTriangle(a,b,c,result.vertexUV[a],result.vertexUV[b],result.vertexUV[c]);
            addTriangle(a,c,d,result.vertexUV[a],result.vertexUV[c],result.vertexUV[d]);
        }
    }
    result.mesh.closedSolid = spline.closed || (settings.capStart && settings.capEnd);
    result.mesh.normals.resize(result.mesh.mesh.positions.size(), {});
    for (std::size_t corner=0;corner<result.mesh.mesh.indices.size();++corner)
        result.mesh.normals[result.mesh.mesh.indices[corner]] =
            add(result.mesh.normals[result.mesh.mesh.indices[corner]], result.mesh.cornerNormals[corner]);
    for (auto &normal:result.mesh.normals)
        normal=unit(normal);
    result.mesh.field.resize(result.mesh.mesh.positions.size(), 0);
    for (std::size_t ring=0;ring<ringCount;++ring)
        for (std::size_t contourIndex=0;contourIndex<contours.size();++contourIndex)
        {
            const auto base=ring*ringSize+contourStart[contourIndex];
            result.mesh.field[base]=1;
            result.mesh.field[base+contours[contourIndex].size()]=1;
        }
    std::set<std::tuple<double,double,double>> seamPositions;
    for (std::size_t vertex=0;vertex<result.mesh.field.size();++vertex)
        if (result.mesh.field[vertex]>0)
        {
            const auto p=result.mesh.mesh.positions[vertex];
            seamPositions.emplace(p.x,p.y,p.z);
        }
    bool damaged=false;
    for (const auto &rule:settings.damagePaths)
    {
        if (!rule.cutter.enabled || rule.cutter.amount==0) continue;
        if (!result.mesh.closedSolid)
            throw std::runtime_error("Shared spline damage needs a closed sweep");
        const auto fresh=damageRegion(spline.splineGuid,rule);
        if (fresh==spline.splineGuid ||
            std::find(result.mesh.mesh.triangleRegions.begin(),
                      result.mesh.mesh.triangleRegions.end(),fresh)!=
                      result.mesh.mesh.triangleRegions.end())
            throw std::runtime_error("Spline fresh interior region ID collision");
        const auto cut=cutDamagePath(result.mesh.mesh,rule.cutter,fresh,240000);
        if (meshHash(cut)==meshHash(result.mesh.mesh)) continue;
        if (cut.positions.size()>settings.maximumVertices ||
            std::find(cut.triangleRegions.begin(),cut.triangleRegions.end(),fresh)==
                cut.triangleRegions.end())
            throw std::runtime_error("Spline crack exceeded budget or lost fresh interior");
        auto transferred=transferDamageCornerUv(result.mesh.mesh,result.cornerUV,
                                                cut,fresh,rule.cutter.projector);
        result.mesh.mesh=cut;
        result.cornerUV=std::move(transferred);
        result.mesh.regions.push_back({fresh,spline.splineGuid,
            SurfaceRole::DamageInterior,-1,-1});
        damaged=true;
    }
    if (damaged)
    {
        rebuildDamageNormals(result.mesh);
        result.mesh.field.assign(result.mesh.mesh.positions.size(),0);
        for (std::size_t vertex=0;vertex<result.mesh.mesh.positions.size();++vertex)
        {
            const auto p=result.mesh.mesh.positions[vertex];
            if (seamPositions.contains(std::tuple{p.x,p.y,p.z}))
                result.mesh.field[vertex]=1;
        }
        result.vertexUV.assign(result.mesh.mesh.positions.size(),{});
        std::vector<bool> assigned(result.vertexUV.size());
        for (std::size_t corner=0;corner<result.mesh.mesh.indices.size();++corner)
        {
            const auto vertex=result.mesh.mesh.indices[corner];
            if (!assigned[vertex])
            {result.vertexUV[vertex]=result.cornerUV[corner];assigned[vertex]=true;}
        }
        result.bodyVertexCount=result.mesh.mesh.positions.size();
        result.bodyTriangleCount=result.mesh.mesh.indices.size()/3;
        const auto atlas=planAutomaticUv(result.mesh.mesh);
        result.bakeCornerUV=atlas.cornerUv;
        result.bakeIslandCount=atlas.islandCount;
    }
    result.generationCpuMilliseconds = std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-started).count();
    result.mesh.milliseconds = result.generationCpuMilliseconds;
    return result;
}
ConstructionResult withFrameAxes(const ConstructionResult &construction)
{
    ConstructionResult result=construction;
    if (result.path.frames.empty()) return result;
    std::fill(result.mesh.field.begin(),result.mesh.field.end(),0.0f);
    const auto stride=std::max(std::size_t(1),result.path.frames.size()/48);
    const auto extent=std::clamp(result.path.lengthMeters*.035, .035, .2);
    auto &mesh=result.mesh.mesh;
    for (std::size_t index=0;index<result.path.frames.size();index+=stride)
    {
        const auto &frame=result.path.frames[index];
        const Vec3d directions[]={frame.tangent,frame.normal,frame.binormal};
        for (unsigned axis=0;axis<3;++axis)
        {
            const auto first=std::uint32_t(mesh.positions.size());
            const auto side=directions[(axis+1)%3], other=directions[(axis+2)%3];
            const auto radius=extent*.08;
            const Vec3d points[]={add(frame.position,mul(side,radius)),
                                  add(frame.position,mul(side,-radius)),
                                  add(frame.position,mul(other,radius)),
                                  add(frame.position,mul(directions[axis],extent))};
            for (auto p:points)
            {
                mesh.positions.push_back(p);
                result.mesh.normals.push_back(directions[axis]);
                result.mesh.field.push_back(float(axis+1));
                result.vertexUV.push_back({0,0});
            }
            constexpr unsigned faces[][3]={{0,2,1},{0,1,3},{1,2,3},{2,0,3}};
            for (const auto &face:faces)
            {
                const auto a=first+face[0],b=first+face[1],c=first+face[2];
                mesh.indices.insert(mesh.indices.end(),{a,b,c});
                mesh.triangleRegions.push_back({});
                result.cornerUV.insert(result.cornerUV.end(),{{0,0},{0,0},{0,0}});
                const auto normal=unit(cross(sub(mesh.positions[b],mesh.positions[a]),
                                             sub(mesh.positions[c],mesh.positions[a])));
                result.mesh.cornerNormals.insert(result.mesh.cornerNormals.end(),3,normal);
            }
        }
    }
    return result;
}
}
