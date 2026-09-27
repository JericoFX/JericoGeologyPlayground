#include "AssemblyLabEvidence.hpp"
#include "ProceduralAssemblies.hpp"
#include "Renderer.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string_view>

namespace Playground
{
namespace
{
PA::ProceduralSpline path(const std::vector<Vec3d> &positions)
{
    PA::ProceduralSpline spline;
    spline.splineGuid=Solid::region(2302,1);
    spline.name="Procedural Assemblies evidence";
    spline.interpolation=PA::SplineInterpolation::CatmullRom;
    for (std::size_t i=0;i<positions.size();++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid=Solid::region(2302,unsigned(i+2));
        point.position=positions[i];
        spline.points.push_back(point);
    }
    return spline;
}
std::uint64_t semanticHash(const std::vector<Assembly::Part> &parts)
{
    std::uint64_t hash=14695981039346656037ull;
    for (const auto &part:parts)
    {
        for (unsigned char byte:part.id)
        {hash^=byte;hash*=1099511628211ull;}
        hash^=unsigned(part.state);hash*=1099511628211ull;
        hash^=part.triangleCount;hash*=1099511628211ull;
    }
    return hash;
}
}
void captureProceduralAssemblies(Renderer &renderer,const std::filesystem::path &directory)
{
    std::filesystem::create_directories(directory);
    const auto gentle=path({{-3,0,0},{-1,0,.10},{1,0,-.10},{3,0,0}});
    const auto bend=path({{-3,0,-1},{-1,0,-1},{0,0,-.35},{.7,0,.6},{1.5,0,1}});
    struct Case
    {
        const char *name;
        Assembly::Kind kind;
        const PA::ProceduralSpline *path;
        int mode{};
        bool damage{};
    };
    const Case cases[]={
        {"pipe-couplings",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-uv",Assembly::Kind::PipeCouplings,&bend,8},
        {"pipe-high-curvature-rmf",Assembly::Kind::PipeCouplings,&bend,10},
        {"railway",Assembly::Kind::Railway,&gentle},
        {"broken-railway",Assembly::Kind::BrokenRailway,&gentle},
        {"railing",Assembly::Kind::Railing,&gentle},
        {"broken-railing",Assembly::Kind::BrokenRailing,&gentle},
        {"wooden-pier",Assembly::Kind::WoodenPier,&gentle},
        {"pier-manual-damage",Assembly::Kind::WoodenPier,&gentle,0,true},
        {"catwalk",Assembly::Kind::Catwalk,&gentle},
        {"truss-warren",Assembly::Kind::TrussWarren,&gentle},
        {"truss-pratt",Assembly::Kind::TrussPratt,&gentle},
        {"truss-howe",Assembly::Kind::TrussHowe,&gentle},
        {"suspension-bridge",Assembly::Kind::SuspensionBridge,&gentle},
        {"suspension-hanger-debug",Assembly::Kind::SuspensionBridge,&gentle,3},
        {"rope-plank-bridge",Assembly::Kind::RopePlankBridge,&gentle},
        {"rope-manual-damage",Assembly::Kind::RopePlankBridge,&gentle,0,true}};
    nlohmann::json proof;
    proof["adapter"]=renderer.adapter();
    proof["resolution"]={1280,960};
    proof["cases"]=nlohmann::json::array();
    for (const auto &item:cases)
    {
        Assembly::Settings settings;
        settings.kind=item.kind;
        if (item.kind==Assembly::Kind::PipeCouplings)
        {settings.pipeRadiusMeters=.09;settings.couplingSpacingMeters=.8;}
        if (item.damage)
        {
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,5)]=Assembly::State::Missing;
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,7)]=Assembly::State::Broken;
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,9)]=Assembly::State::Loose;
        }
        auto built=Assembly::build(*item.path,settings);
        const auto replay=Assembly::build(*item.path,settings);
        if (meshHash(built.mesh.mesh)!=meshHash(replay.mesh.mesh) ||
            semanticHash(built.parts)!=semanticHash(replay.parts))
            throw std::runtime_error(std::string("Nondeterministic assembly: ")+item.name);
        if (item.mode==10)
        {
            Construction::ConstructionResult axes;
            axes.path=built.path;axes.mesh=built.mesh;axes.cornerUV=built.cornerUV;
            const auto debug=Construction::withFrameAxes(axes);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
        auto lo=built.mesh.mesh.positions.front(),hi=lo;
        for (auto p:built.mesh.mesh.positions)
        {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
         hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
        Camera camera;
        camera.yaw=.35f;camera.pitch=-.25f;
        camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
        camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.25f,
                                   2.0f,90.0f);
        renderer.draw(1280,960,camera,item.mode);
        if (std::string_view(item.name)=="pipe-couplings")
        {
            const auto hit=renderer.pickTriangle(1280,960,camera,640,480);
            if (!hit || *hit>=built.mesh.mesh.triangleRegions.size())
                throw std::runtime_error("Visible pipe triangle pick failed");
            bool resolved=false;
            for (const auto &part:built.parts)
                resolved|=part.guid==built.mesh.mesh.triangleRegions[*hit];
            if (!resolved)
                throw std::runtime_error("Picked triangle does not resolve to a semantic part");
            renderer.draw(1280,960,camera,item.mode);
        }
        renderer.capture(directory/(std::string(item.name)+".png"));
        nlohmann::json states=nlohmann::json::object(),parts=nlohmann::json::array();
        for (const auto &part:built.parts)
        {
            states[Assembly::name(part.state)]=
                states.value(Assembly::name(part.state),std::size_t(0))+1;
            parts.push_back({{"id",part.id},{"role",Assembly::name(part.role)},
                {"state",Assembly::name(part.state)},{"stationMeters",part.stationMeters},
                {"triangles",part.triangleCount}});
        }
        nlohmann::json points=nlohmann::json::array();
        for (const auto &point:item.path->points)
            points.push_back({{"position",{point.position.x,point.position.y,point.position.z}},
                {"incomingHandle",{point.incomingHandle.x,point.incomingHandle.y,
                                     point.incomingHandle.z}},
                {"outgoingHandle",{point.outgoingHandle.x,point.outgoingHandle.y,
                                     point.outgoingHandle.z}},
                {"rollRadians",point.rollRadians},{"widthMeters",point.widthMeters}});
        nlohmann::json manual=nlohmann::json::object();
        for (const auto &[id,state]:settings.damage.manual)
            manual[id]=Assembly::name(state);
        proof["cases"].push_back({
            {"name",item.name},{"kind",Assembly::name(item.kind)},
            {"viewMode",item.mode},{"interpolation",int(item.path->interpolation)},
            {"controlPoints",points},
            {"settings",{{"lod",settings.lod},{"widthMeters",settings.widthMeters},
                         {"heightMeters",settings.heightMeters},{"spanMeters",settings.spanMeters},
                         {"sagMeters",settings.sagMeters},
                         {"towerHeightMeters",settings.towerHeightMeters},
                         {"hangerSpacingMeters",settings.hangerSpacingMeters},
                         {"cableRadiusMeters",settings.cableRadiusMeters},
                         {"pipeRadiusMeters",settings.pipeRadiusMeters},
                         {"couplingSpacingMeters",settings.couplingSpacingMeters},
                         {"couplingWidthMeters",settings.couplingWidthMeters},
                         {"couplingRadialThicknessMeters",settings.couplingRadialThicknessMeters},
                         {"couplingStartPaddingMeters",settings.couplingStartPaddingMeters},
                         {"couplingEndPaddingMeters",settings.couplingEndPaddingMeters},
                         {"couplingVariationMeters",settings.couplingVariationMeters},
                         {"variationSeed",settings.variationSeed},
                         {"couplingsEnabled",settings.couplingsEnabled},
                         {"gaugeMeters",settings.gaugeMeters},
                         {"sleeperSpacingMeters",settings.sleeperSpacingMeters},
                         {"sleeperLengthMeters",settings.sleeperLengthMeters},
                         {"sleeperWidthMeters",settings.sleeperWidthMeters},
                         {"sleeperHeightMeters",settings.sleeperHeightMeters},
                         {"railWidthMeters",settings.railWidthMeters},
                         {"railHeightMeters",settings.railHeightMeters},
                         {"railJointsEnabled",settings.railJointsEnabled},
                         {"boardWidthMeters",settings.boardWidthMeters},
                         {"boardGapMeters",settings.boardGapMeters},
                         {"boardThicknessMeters",settings.boardThicknessMeters},
                         {"beamWidthMeters",settings.beamWidthMeters},
                         {"beamHeightMeters",settings.beamHeightMeters},
                         {"supportSpacingMeters",settings.supportSpacingMeters},
                         {"baySpacingMeters",settings.baySpacingMeters},
                         {"lowerRailEnabled",settings.lowerRailEnabled},
                         {"irregularityMeters",settings.irregularityMeters},
                         {"quality",{{"maximumStepMeters",settings.quality.maximumStepMeters},
                                     {"maximumDeviationMeters",settings.quality.maximumDeviationMeters},
                                     {"maximumTangentAngleRadians",
                                      settings.quality.maximumTangentAngleRadians},
                                     {"maximumSamples",settings.quality.maximumSamples}}},
                         {"damage",{{"manual",manual},{"proceduralEnabled",
                                     settings.damage.proceduralEnabled},
                                    {"seed",settings.damage.seed},
                                    {"missingProbability",settings.damage.missingProbability},
                                    {"brokenProbability",settings.damage.brokenProbability},
                                    {"looseProbability",settings.damage.looseProbability},
                                    {"bentProbability",settings.damage.bentProbability},
                                    {"clusterCenterMeters",settings.damage.clusterCenterMeters},
                                    {"clusterRadiusMeters",settings.damage.clusterRadiusMeters},
                                    {"clusterStrength",settings.damage.clusterStrength}}}}},
            {"lengthMeters",built.path.lengthMeters},
            {"cpuMilliseconds",built.generationCpuMilliseconds},
            {"vertices",built.mesh.mesh.positions.size()},
            {"triangles",built.mesh.mesh.indices.size()/3},
            {"semanticPartCount",built.parts.size()},
            {"repeatStations",built.repeatedStations},
            {"manualDamageCount",settings.damage.manual.size()},
            {"promotedBodies",built.promotedBodyCount},
            {"constraints",built.constraintCount},{"stateCounts",states},
            {"meshHash",meshHash(built.mesh.mesh)},
            {"semanticHash",semanticHash(built.parts)},
            {"parts",parts},{"image",std::string(item.name)+".png"}});
    }
    Assembly::Settings editSettings;
    editSettings.kind=Assembly::Kind::WoodenPier;
    const auto baseline=Assembly::build(gentle,editSettings);
    editSettings.damage.manual[Assembly::stablePartId(editSettings.kind,
        Assembly::Role::DeckPlank,8)]=Assembly::State::Broken;
    const auto damageEdit=Assembly::build(gentle,editSettings);
    auto editedPath=gentle;
    editedPath.points[2].position.z+=.15;
    const auto pathEdit=Assembly::build(editedPath,editSettings);
    proof["editMeasurements"]={{"baselineCpuMilliseconds",baseline.generationCpuMilliseconds},
        {"localDamageRebuildCpuMilliseconds",damageEdit.generationCpuMilliseconds},
        {"splinePointRebuildCpuMilliseconds",pathEdit.generationCpuMilliseconds},
        {"algorithm","full synchronous rebuild"}};
    std::ofstream(directory/"procedural-assemblies-proof.json")<<proof.dump(2)<<'\n';
}
}
