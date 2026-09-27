#include "SplineLabEvidence.hpp"
#include "SplineLabUI.hpp"
#include "SolidGeometry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>

namespace Playground
{
namespace
{
PA::ProceduralSpline path(const std::vector<Vec3d> &positions, PA::SplineInterpolation interpolation)
{
    PA::ProceduralSpline spline;
    spline.splineGuid=Solid::region(1902,1);
    spline.name="Spline Construction evidence";
    spline.interpolation=interpolation;
    for (std::size_t i=0;i<positions.size();++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid=Solid::region(1902,unsigned(i+2));
        point.position=positions[i];
        point.incomingHandle={-.25,0,0};
        point.outgoingHandle={.25,0,0};
        spline.points.push_back(point);
    }
    return spline;
}
std::uint64_t uvHash(const std::vector<std::array<float,2>> &uvs)
{
    std::uint64_t hash=14695981039346656037ull;
    for (const auto &uv:uvs)
        for (auto value:uv)
        {
            const auto *bytes=reinterpret_cast<const unsigned char *>(&value);
            for (std::size_t i=0;i<sizeof(float);++i)
            {hash^=bytes[i];hash*=1099511628211ull;}
        }
    return hash;
}
}
void captureSplineConstruction(Renderer &renderer, const std::filesystem::path &directory)
{
    std::filesystem::create_directories(directory);
    const auto line=path({{-1.5,0,0},{1.5,0,0}},PA::SplineInterpolation::Polyline);
    const auto s=path({{-1.5,0,-.6},{-.6,.55,.2},{.4,-.5,-.15},{1.5,0,.65}},
                      PA::SplineInterpolation::CatmullRom);
    const auto vertical=path({{0,0,0},{.12,1,0},{-.12,2,0},{0,3,0}},
                             PA::SplineInterpolation::CatmullRom);
    const auto bend=path({{-1,0,-1},{-.15,0,-1},{.15,0,-.75},{.3,0,0},{.3,0,.75}},
                         PA::SplineInterpolation::CatmullRom);
    struct Case {const char *name;int preset;const PA::ProceduralSpline *spline;int mode;};
    const Case cases[]={
        {"pipe-straight",1,&line,0},{"pipe-s-curve",1,&s,0},
        {"pipe-uv-checker",1,&s,8},{"pipe-vertical-rmf",1,&vertical,10},
        {"pipe-seam",1,&s,9},{"rectangular-duct",6,&s,0},
        {"cable-tray-crossbars",7,&s,0},{"i-beam-run",9,&s,0},
        {"high-curvature-bend",2,&bend,2},{"cable",3,&s,0},
        {"corrugated-hose",10,&s,0},
        {"fractured-pipe",11,&s,0},
        {"twisted-duct",12,&s,0}};
    nlohmann::json proof;
    proof["adapter"]=renderer.adapter();
    proof["resolution"]={1280,960};
    proof["cases"]=nlohmann::json::array();
    for (const auto &item:cases)
    {
        const auto settings=splinePreset(item.preset);
        const auto built=Construction::buildConstruction(*item.spline,settings);
        const auto replay=Construction::buildConstruction(*item.spline,settings);
        if (meshHash(built.mesh.mesh)!=meshHash(replay.mesh.mesh) ||
            uvHash(built.cornerUV)!=uvHash(replay.cornerUV))
            throw std::runtime_error(std::string("Nondeterministic spline construction: ")+item.name);
        if (item.mode==10)
        {
            const auto debug=Construction::withFrameAxes(built);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
        auto lo=built.mesh.mesh.positions.front(),hi=lo;
        for (auto p:built.mesh.mesh.positions)
        {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
         hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
        Camera camera;
        camera.yaw=.52f;camera.pitch=-.22f;
        camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
        camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.35f,1.0f,90.0f);
        renderer.draw(1280,960,camera,item.mode);
        renderer.capture(directory/(std::string(item.name)+".png"));
        nlohmann::json points=nlohmann::json::array();
        for (const auto &point:item.spline->points)
            points.push_back({point.position.x,point.position.y,point.position.z});
        proof["cases"].push_back({
            {"name",item.name},{"preset",item.preset},{"viewMode",item.mode},
            {"controlPoints",points},{"interpolation",int(item.spline->interpolation)},
            {"profile",int(settings.profile)},{"radiusMeters",settings.radiusMeters},
            {"widthMeters",settings.widthMeters},{"heightMeters",settings.heightMeters},
            {"wallThicknessMeters",settings.wallThicknessMeters},
            {"taperLengthMeters",settings.taperLengthMeters},
            {"tipScale",settings.tipScale},
            {"ribAmplitude",settings.ribAmplitude},
            {"ribSpacingMeters",settings.ribSpacingMeters},
            {"twistTurns",settings.twistTurns},
            {"breakEnabled",settings.breakEnabled},
            {"breakFraction",settings.breakFraction},
            {"breakGapMeters",settings.breakGapMeters},
            {"breakRoughness",settings.breakRoughness},
            {"pathErrorMeters",settings.pathQuality.maximumDeviationMeters},
            {"profileChordErrorMeters",settings.profileChordErrorMeters},
            {"measuredDeviationMeters",built.path.maximumMeasuredDeviationMeters},
            {"lengthMeters",built.path.lengthMeters},{"cpuMilliseconds",built.generationCpuMilliseconds},
            {"samples",built.path.frames.size()},{"frames",built.path.frames.size()},
            {"profileSegments",built.profileSegments},
            {"vertices",built.mesh.mesh.positions.size()},
            {"triangles",built.mesh.mesh.indices.size()/3},
            {"repeats",built.repeats.size()},
            {"meshHash",meshHash(built.mesh.mesh)},{"uvHash",uvHash(built.cornerUV)},
            {"geometryBytesApprox",built.mesh.mesh.positions.size()*sizeof(Vec3d)+
                built.mesh.mesh.indices.size()*sizeof(std::uint32_t)},
            {"image",std::string(item.name)+".png"}});
    }
    std::ofstream(directory/"spline-construction-proof.json")<<proof.dump(2)<<'\n';
}
}
