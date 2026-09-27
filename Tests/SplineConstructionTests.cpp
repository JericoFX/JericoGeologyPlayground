#include "SplineConstruction.hpp"
#include "SolidGeometry.hpp"

#include <cmath>
#include <limits>
#include <numbers>
#include <iostream>
#include <stdexcept>

using namespace Playground;
using namespace Playground::Construction;

namespace
{
void require(bool condition, const char *message)
{
    if (!condition)
        throw std::runtime_error(message);
}
double dot(Vec3d a, Vec3d b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
void inspectMesh(const ConstructionResult &built)
{
    const auto &mesh=built.mesh.mesh;
    require(built.cornerUV.size()==mesh.indices.size() &&
            built.mesh.cornerNormals.size()==mesh.indices.size() &&
            mesh.triangleRegions.size()==mesh.indices.size()/3,
            "mesh corner attributes and triangle regions must align");
    for (std::size_t i=0;i<mesh.indices.size();i+=3)
    {
        require(mesh.indices[i]<mesh.positions.size() && mesh.indices[i+1]<mesh.positions.size() &&
                mesh.indices[i+2]<mesh.positions.size(),"sweep indices must remain in range");
        const auto a=mesh.positions[mesh.indices[i]], b=mesh.positions[mesh.indices[i+1]],
                   c=mesh.positions[mesh.indices[i+2]];
        require(dot(cross(sub(b,a),sub(c,a)),cross(sub(b,a),sub(c,a)))>1e-18,
                "valid sweep fixtures must not create degenerate triangles");
        for (unsigned k=0;k<3;++k)
            require(std::isfinite(built.cornerUV[i+k][0]) && std::isfinite(built.cornerUV[i+k][1]),
                    "analytic UVs must stay finite");
    }
}
PA::ProceduralSpline verticalInflection()
{
    PA::ProceduralSpline spline;
    spline.splineGuid = Solid::region(801, 1);
    spline.name = "Vertical inflection";
    spline.interpolation = PA::SplineInterpolation::CatmullRom;
    const Vec3d positions[] = {{0, 0, 0}, {0.12, 1, 0}, {-0.12, 2, 0}, {0, 3, 0}};
    for (unsigned i = 0; i < 4; ++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid = Solid::region(801, i + 2);
        point.position = positions[i];
        spline.points.push_back(point);
    }
    return spline;
}
PA::ProceduralSpline unevenStraight()
{
    auto spline = verticalInflection();
    spline.name = "Uneven straight";
    spline.interpolation = PA::SplineInterpolation::Polyline;
    const double x[] = {0, 0.2, 2.2, 5};
    for (unsigned i = 0; i < 4; ++i)
        spline.points[i].position = {x[i], 0, 0};
    return spline;
}
PA::ProceduralSpline straightRun()
{
    auto spline = unevenStraight();
    spline.name = "Straight run";
    spline.points.resize(2);
    spline.points[1].position = {0, 0, 3};
    return spline;
}
PA::ProceduralSpline closedLoop()
{
    auto spline=verticalInflection();
    spline.name="Closed loop";
    spline.closed=true;
    spline.points[0].position={1,0,0};
    spline.points[1].position={0,0,1};
    spline.points[2].position={-1,0,0};
    spline.points[3].position={0,0,-1};
    return spline;
}
}

int main()
{
    try
    {
        const auto path = samplePath(verticalInflection(), {.maximumDeviationMeters = 0.002,
                                                             .maximumTangentAngleRadians = 0.12,
                                                             .maximumStepMeters = 0.12});
        require(path.frames.size() > 20, "inflection needs enough geometric samples");
        for (std::size_t i = 0; i < path.frames.size(); ++i)
        {
            const auto &frame = path.frames[i];
            require(std::abs(dot(frame.tangent, frame.normal)) < 1e-6 &&
                    std::abs(dot(frame.tangent, frame.binormal)) < 1e-6 &&
                    std::abs(dot(frame.normal, frame.binormal)) < 1e-6 &&
                    std::abs(dot(frame.normal, frame.normal) - 1) < 1e-6,
                    "vertical inflection frame must stay orthonormal");
            if (i)
                require(dot(frame.normal, path.frames[i - 1].normal) > 0.5,
                        "vertical inflection must not flip the profile frame");
        }
        const auto uneven = samplePath(unevenStraight(), {});
        const auto placements = repeatAlongPath(uneven, {.spacingMeters = 1,
                                                         .offsetMeters = 0.5,
                                                         .startPaddingMeters = 0.3,
                                                         .endPaddingMeters = 0.3});
        std::cout << "uneven length/markers=" << uneven.lengthMeters << '/' << placements.size() << '\n';
        require(placements.size() == 5, "five markers must fit the five-metre path");
        for (std::size_t i = 0; i < placements.size(); ++i)
        {
            const double expected = 0.5 + double(i);
            require(std::abs(placements[i].frame.distanceMeters - expected) < 1e-9 &&
                    std::abs(placements[i].frame.position.x - expected) < 1e-8,
                    "markers must use world arc distance across uneven control points");
        }
        ConstructionSettings pipe;
        pipe.radiusMeters = 0.1;
        pipe.metresPerTileU = 0.5;
        pipe.metresPerTileV = 0.25;
        pipe.profileChordErrorMeters = 0.001;
        const auto built = buildConstruction(straightRun(), pipe);
        require(built.profileSegments >= 16 && built.bodyVertexCount > 0,
                "pipe circle must tessellate by chord error");
        const auto ring = built.profileSegments + 1;
        const auto first = built.mesh.mesh.positions.front();
        const auto seam = built.mesh.mesh.positions[built.profileSegments];
        require(std::abs(first.x - seam.x) < 1e-9 && std::abs(first.y - seam.y) < 1e-9 &&
                    std::abs(first.z - seam.z) < 1e-9 &&
                    std::abs(built.vertexUV[0][1] - built.vertexUV[built.profileSegments][1]) > 2.5,
                "pipe seam must duplicate position with distinct metric perimeter UV");
        require(std::abs(built.vertexUV[(built.path.frames.size() - 1) * ring][0] - 6.0f) < 1e-5,
                "pipe U must represent three metres at half-metre tiles");
        require(std::abs(built.vertexUV[built.profileSegments][1]-
                         float(2*std::numbers::pi*pipe.radiusMeters/pipe.metresPerTileV))<1e-5,
                "circular V must use analytic arc length rather than polygon chord length");
        for (std::size_t frame=0;frame<built.path.frames.size();++frame)
            require(std::abs(built.vertexUV[frame*ring][0]-
                        float(built.path.frames[frame].distanceMeters/pipe.metresPerTileU))<1e-5,
                    "UV U must follow sampled world arc length through every frame");
        require(built.mesh.field[0]==1 && built.mesh.field[built.profileSegments]==1 &&
                built.mesh.field[1]==0,
                "seam diagnostic must mark actual duplicated profile vertices");
        require(built.mesh.mesh.indices.size() / 3 > built.bodyTriangleCount &&
                    built.cornerUV.size() == built.mesh.mesh.indices.size() &&
                    built.mesh.cornerNormals.size() == built.mesh.mesh.indices.size(),
                "pipe needs planar end caps and per-corner UV/normals");
        inspectMesh(built);
        auto effected=pipe;
        effected.taperLengthMeters=.5;
        effected.tipScale=.4;
        effected.ribAmplitude=.2;
        effected.ribSpacingMeters=.25;
        effected.twistTurns=.5;
        const auto effects=buildConstruction(straightRun(),effected);
        inspectMesh(effects);
        require(effects.path.frames.size()>built.path.frames.size() &&
                std::hypot(effects.mesh.mesh.positions.front().x,
                           effects.mesh.mesh.positions.front().y)<
                    std::hypot(built.mesh.mesh.positions.front().x,
                               built.mesh.mesh.positions.front().y)*.6,
                "taper and ribs must affect geometry at adequate sampling density");
        require(meshHash(effects.mesh.mesh)==meshHash(
                    buildConstruction(straightRun(),effected).mesh.mesh),
                "spline effects must be deterministic");
        effected.breakEnabled=true;
        effected.breakGapMeters=.2;
        const auto fracture=buildConstruction(straightRun(),effected);
        inspectMesh(fracture);
        require(fracture.mesh.closedSolid &&
                fracture.bodyTriangleCount<effects.bodyTriangleCount &&
                fracture.mesh.mesh.indices.size()/3>fracture.bodyTriangleCount,
                "broken sweep needs a genuine gap and closed fracture faces");
        effected.breakGapMeters=10;
        bool fractureRejected=false;
        try {buildConstruction(straightRun(),effected);}
        catch (const std::exception &) {fractureRejected=true;}
        require(fractureRejected,"oversize fracture gap must be rejected");
        for (int kind=1;kind<6;++kind)
        {
            auto candidate=pipe;
            candidate.profile=ProfileKind(kind);
            candidate.widthMeters=.25;candidate.heightMeters=.15;
            candidate.wallThicknessMeters=.02;candidate.cornerRadiusMeters=.025;
            candidate.repeatEnabled=kind==4;
            candidate.repeat.spacingMeters=.5;
            const auto shape=buildConstruction(straightRun(),candidate);
            inspectMesh(shape);
            require(shape.profileSegments>=4 && shape.mesh.mesh.indices.size()>shape.bodyTriangleCount*3,
                    "every initial profile needs a swept body and valid caps");
            if (kind==4)
                require(shape.repeats.size()==7 &&
                        shape.mesh.mesh.positions.size()>shape.bodyVertexCount,
                        "channel crossbars must be visible geometry at arc-distance placements");
        }
        auto invalid=pipe;invalid.radiusMeters=0;
        bool rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"zero radius must return a diagnostic");
        invalid=pipe;invalid.metresPerTileU=std::numeric_limits<double>::quiet_NaN();
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"NaN UV scale must return a diagnostic");
        const auto axes=withFrameAxes(built);
        inspectMesh(axes);
        require(axes.mesh.mesh.indices.size()>built.mesh.mesh.indices.size(),
                "RMF debug axes must be actual viewport geometry");
        const auto closed=buildConstruction(closedLoop(),pipe);
        inspectMesh(closed);
        require(closed.mesh.closedSolid &&
                closed.mesh.mesh.indices.size()/3==closed.bodyTriangleCount,
                "closed spline sweep must join the seam without end caps");
        require(dot(closed.path.frames.front().normal,closed.path.frames.back().normal)>.999 &&
                dot(closed.path.frames.front().tangent,closed.path.frames.back().tangent)>.999,
                "closed RMF seam must match orientation and tangent");
        auto bezier=straightRun();
        bezier.interpolation=PA::SplineInterpolation::CubicBezier;
        bezier.points[0].outgoingHandle={.6,.2,.3};
        bezier.points[1].incomingHandle={-.2,.4,-.3};
        auto reverse=bezier;
        std::reverse(reverse.points.begin(),reverse.points.end());
        for (auto &point:reverse.points)
            std::swap(point.incomingHandle,point.outgoingHandle);
        const auto forwardPath=samplePath(bezier,{}), reversePath=samplePath(reverse,{});
        require(std::abs(forwardPath.lengthMeters-reversePath.lengthMeters)<1e-6,
                "reversing Bezier must preserve arc length");
        require(std::abs(forwardPath.frames.front().position.z-reversePath.frames.back().position.z)<1e-9 &&
                std::abs(forwardPath.frames.back().position.z-reversePath.frames.front().position.z)<1e-9,
                "reversing Bezier must preserve endpoints");
        auto vertical=straightRun();
        vertical.points[1].position={0,3,0};
        const auto verticalPath=samplePath(vertical,{});
        for (const auto &frame:verticalPath.frames)
            require(std::isfinite(frame.normal.x) && std::isfinite(frame.binormal.y) &&
                    std::abs(dot(frame.tangent,frame.normal))<1e-8 &&
                    dot(cross(frame.tangent,frame.normal),frame.binormal)>.999,
                    "fully vertical path must retain finite right-handed RMF frames");
        auto nearLine=verticalInflection();
        for (std::size_t i=0;i<nearLine.points.size();++i)
            nearLine.points[i].position={double(i)*.01,double(i),0};
        const auto nearPath=samplePath(nearLine,{});
        for (std::size_t i=1;i<nearPath.frames.size();++i)
            require(dot(nearPath.frames[i-1].normal,nearPath.frames[i].normal)>.5,
                    "near-vertical nearly straight curve must not flip RMF");
        auto duplicate=straightRun();
        duplicate.points[1].position=duplicate.points[0].position;
        rejected=false;
        try {samplePath(duplicate,{});} catch (const std::exception &) {rejected=true;}
        require(rejected,"duplicate control positions must fail with diagnostic");
        auto nonfinite=straightRun();
        nonfinite.points[1].position.y=std::numeric_limits<double>::infinity();
        rejected=false;
        try {samplePath(nonfinite,{});} catch (const std::exception &) {rejected=true;}
        require(rejected,"infinite spline coordinate must fail with diagnostic");
        invalid=pipe;invalid.profileChordErrorMeters=0;
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"zero profile quality must fail with diagnostic");
        invalid=pipe;invalid.maximumVertices=32;
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"vertex budget must fail before runaway allocation");
        invalid=pipe;invalid.maximumVertices=1000000000;
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"unbounded vertex budget must be rejected");
        invalid=pipe;invalid.radiusMeters=1000;
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"profile segment cap must reject unmet requested chord tolerance");
        invalid=pipe;invalid.profile=ProfileKind::RoundedRectangle;
        invalid.widthMeters=.2;invalid.heightMeters=.1;invalid.cornerRadiusMeters=.05;
        rejected=false;
        try {buildConstruction(straightRun(),invalid);} catch (const std::exception &) {rejected=true;}
        require(rejected,"rounded rectangle with collapsed straight side must fail clearly");
        std::cout << "PASS vertical inflection RMF and distance-driven repeat\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
