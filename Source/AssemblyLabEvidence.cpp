#include "AssemblyLabEvidence.hpp"
#include "AssemblyFragmentPhysics.hpp"
#include "AutoUv.hpp"
#include "ProceduralAssemblies.hpp"
#include "Renderer.hpp"
#include "SolidGeometry.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <optional>
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
        int areaSide{};
    };
    const Case cases[]={
        {"pipe-couplings",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-cavity",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-dent",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-through-hole",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-edge",Assembly::Kind::PipeCouplings,&gentle},
        {"broken-coupling",Assembly::Kind::PipeCouplings,&gentle},
        {"broken-pipe-body",Assembly::Kind::PipeCouplings,&gentle},
        {"pipe-uv",Assembly::Kind::PipeCouplings,&bend,8},
        {"pipe-high-curvature-rmf",Assembly::Kind::PipeCouplings,&bend,10},
        {"railway",Assembly::Kind::Railway,&gentle},
        {"broken-railway",Assembly::Kind::BrokenRailway,&gentle},
        {"broken-rail-member",Assembly::Kind::Railway,&gentle},
        {"broken-rail-joint",Assembly::Kind::Railway,&gentle},
        {"railing",Assembly::Kind::Railing,&gentle},
        {"broken-railing",Assembly::Kind::BrokenRailing,&gentle},
        {"broken-railing-post",Assembly::Kind::Railing,&gentle},
        {"broken-top-rail",Assembly::Kind::Railing,&gentle},
        {"wooden-pier",Assembly::Kind::WoodenPier,&gentle},
        {"broken-beam-member",Assembly::Kind::WoodenPier,&gentle},
        {"broken-cross-beam",Assembly::Kind::WoodenPier,&gentle},
        {"broken-pile",Assembly::Kind::WoodenPier,&gentle},
        {"broken-fascia",Assembly::Kind::WoodenPier,&gentle},
        {"plank-cavity",Assembly::Kind::WoodenPier,&gentle},
        {"plank-dent",Assembly::Kind::WoodenPier,&gentle},
        {"plank-through-hole",Assembly::Kind::WoodenPier,&gentle},
        {"plank-edge",Assembly::Kind::WoodenPier,&gentle},
        {"pier-bake-uv",Assembly::Kind::WoodenPier,&gentle,8},
        {"plank-bake-uv",Assembly::Kind::WoodenPier,&gentle,8},
        {"pier-manual-damage",Assembly::Kind::WoodenPier,&gentle,0,true},
        {"pier-break-closeup",Assembly::Kind::WoodenPier,&gentle,0,true},
        {"plank-fracture-isolated",Assembly::Kind::WoodenPier,&gentle,0,true},
        {"catwalk",Assembly::Kind::Catwalk,&gentle},
        {"truss-warren",Assembly::Kind::TrussWarren,&gentle},
        {"broken-truss-brace",Assembly::Kind::TrussWarren,&gentle},
        {"truss-pratt",Assembly::Kind::TrussPratt,&gentle},
        {"truss-howe",Assembly::Kind::TrussHowe,&gentle},
        {"suspension-bridge",Assembly::Kind::SuspensionBridge,&gentle},
        {"broken-suspension-cable",Assembly::Kind::SuspensionBridge,&gentle},
        {"broken-hanger",Assembly::Kind::SuspensionBridge,&gentle},
        {"suspension-hanger-debug",Assembly::Kind::SuspensionBridge,&gentle,3},
        {"rope-plank-bridge",Assembly::Kind::RopePlankBridge,&gentle},
        {"broken-rope-cable",Assembly::Kind::RopePlankBridge,&gentle},
        {"broken-hand-rope",Assembly::Kind::RopePlankBridge,&gentle},
        {"rope-manual-damage",Assembly::Kind::RopePlankBridge,&gentle,0,true},
        {"pier-area-left",Assembly::Kind::WoodenPier,&gentle,0,false,-1},
        {"pier-area-right",Assembly::Kind::WoodenPier,&gentle,0,false,1}};
    nlohmann::json proof;
    proof["adapter"]=renderer.adapter();
    proof["resolution"]={1280,960};
    proof["cases"]=nlohmann::json::array();
    for (const auto &item:cases)
    {
        const std::string_view caseName(item.name);
        Assembly::Settings settings;
        settings.kind=item.kind;
        if (item.kind==Assembly::Kind::PipeCouplings)
        {settings.pipeRadiusMeters=.09;settings.couplingSpacingMeters=.8;}
        std::string brokenTargetId;
        if (caseName=="broken-coupling")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Coupling,0);
        else if (caseName=="broken-pipe-body")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::PipeBody,0);
        else if (caseName=="broken-rail-member")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Rail,0,-1);
        else if (caseName=="broken-rail-joint")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::RailJoint,1,-1);
        else if (caseName=="broken-beam-member")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Beam,0,-1);
        else if (caseName=="broken-cross-beam")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::CrossBeam,0);
        else if (caseName=="broken-railing-post")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Post,0,-1);
        else if (caseName=="broken-top-rail")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::TopRail,0,-1);
        else if (caseName=="broken-pile")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Pile,0,-1);
        else if (caseName=="broken-fascia")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Fascia,0,-1);
        else if (caseName=="broken-suspension-cable" ||
                 caseName=="broken-rope-cable")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Cable,0,-1);
        else if (caseName=="broken-hand-rope")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::HandRope,0,-1);
        else if (caseName=="broken-hanger")
            brokenTargetId=Assembly::stablePartId(item.kind,Assembly::Role::Hanger,0,-1);
        else if (caseName=="broken-truss-brace")
        {
            const auto source=Assembly::build(*item.path,settings);
            const auto brace=std::find_if(source.parts.rbegin(),source.parts.rend(),
                [](const Assembly::Part &part)
                {return part.role==Assembly::Role::Brace;});
            if (brace==source.parts.rend())
                throw std::runtime_error("Truss evidence lacks a diagonal brace");
            brokenTargetId=brace->id;
        }
        if (!brokenTargetId.empty())
            settings.damage.manual[brokenTargetId]=Assembly::State::Broken;
        if (item.damage)
        {
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,5)]=Assembly::State::Missing;
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,7)]=Assembly::State::Broken;
            settings.damage.manual[Assembly::stablePartId(item.kind,
                Assembly::Role::DeckPlank,9)]=Assembly::State::Loose;
        }
        if (item.areaSide)
        {
            const auto source=Assembly::build(*item.path,settings);
            const auto id=Assembly::stablePartId(item.kind,Assembly::Role::DeckPlank,7);
            const auto part=std::find_if(source.parts.begin(),source.parts.end(),
                [&](const Assembly::Part &candidate){return candidate.id==id;});
            if (part==source.parts.end())
                throw std::runtime_error("Damage area evidence lacks target plank");
            const auto frame=Construction::frameAtDistance(source.path,part->stationMeters);
            Assembly::AreaDamageRule rule;
            rule.area.id="area-plank-7";
            rule.area.semanticPartId=id;
            rule.area.projector.tangent=frame.tangent;
            rule.area.projector.bitangent=frame.binormal;
            rule.area.projector.normal=frame.normal;
            rule.area.projector.center={part->position.x+frame.binormal.x*item.areaSide*.3,
                part->position.y+frame.binormal.y*item.areaSide*.3,
                part->position.z+frame.binormal.z*item.areaSide*.3};
            rule.area.projector.scale.z=.35;
            rule.area.footprint.radius={2,2,1};
            settings.damage.areaRules.push_back(rule);
        }
        const bool geometric=caseName=="pipe-cavity" ||
            caseName=="pipe-dent" || caseName=="pipe-through-hole" ||
            caseName=="pipe-edge" ||
            caseName=="plank-cavity" || caseName=="plank-dent" ||
            caseName=="plank-through-hole" || caseName=="plank-edge" ||
            caseName=="plank-bake-uv";
        if (geometric)
        {
            const auto source=Assembly::build(*item.path,settings);
            const bool pipe=caseName.substr(0,5)=="pipe-";
            const auto id=Assembly::stablePartId(item.kind,
                pipe?Assembly::Role::PipeBody:Assembly::Role::DeckPlank,
                pipe?0:7);
            const auto part=std::find_if(source.parts.begin(),source.parts.end(),
                [&](const Assembly::Part &candidate){return candidate.id==id;});
            if (part==source.parts.end())
                throw std::runtime_error("Geometric cavity evidence lacks target part");
            const auto frame=Construction::frameAtDistance(source.path,
                part->stationMeters);
            Assembly::AreaDamageRule rule;
            rule.operation=caseName.find("edge")!=std::string_view::npos?
                Assembly::AreaOperation::EdgeDamage:
                caseName.find("through-hole")!=std::string_view::npos?
                Assembly::AreaOperation::ThroughHole:
                caseName.find("dent")!=std::string_view::npos?
                    Assembly::AreaOperation::Dent:Assembly::AreaOperation::Cavity;
            rule.state=Assembly::State::Intact;
            rule.area.id=std::string(caseName);
            rule.area.semanticPartId=id;
            rule.area.projector.tangent=frame.tangent;
            rule.area.projector.bitangent={
                frame.normal.y*frame.tangent.z-frame.normal.z*frame.tangent.y,
                frame.normal.z*frame.tangent.x-frame.normal.x*frame.tangent.z,
                frame.normal.x*frame.tangent.y-frame.normal.y*frame.tangent.x};
            rule.area.projector.normal=frame.normal;
            const auto height=pipe?settings.pipeRadiusMeters:
                                   settings.boardThicknessMeters*.5;
            rule.area.projector.center={
                part->position.x+frame.normal.x*height,
                part->position.y+frame.normal.y*height,
                part->position.z+frame.normal.z*height};
            if (const auto hit=Assembly::raycastPartSurface(source,*part,
                    rule.area.projector.center,
                    {-frame.normal.x,-frame.normal.y,-frame.normal.z}))
                rule.area.projector.center=*hit;
            if (rule.operation==Assembly::AreaOperation::EdgeDamage)
            {
                if (pipe)
                {
                    const auto end=Construction::frameAtDistance(source.path,
                        source.path.lengthMeters);
                    rule.area.projector.center={
                        end.position.x+end.normal.x*settings.pipeRadiusMeters,
                        end.position.y+end.normal.y*settings.pipeRadiusMeters,
                        end.position.z+end.normal.z*settings.pipeRadiusMeters};
                }
                else
                {
                    rule.area.projector.center.x+=frame.tangent.x*
                        settings.boardWidthMeters*.5;
                    rule.area.projector.center.y+=frame.tangent.y*
                        settings.boardWidthMeters*.5;
                    rule.area.projector.center.z+=frame.tangent.z*
                        settings.boardWidthMeters*.5;
                }
                rule.edge.width=.08;
                rule.edge.curvatureEnd=.1;
            }
            rule.area.footprint.radius=pipe?
                (rule.operation==Assembly::AreaOperation::ThroughHole?
                    Vec3d{.06,.045,1}:rule.operation==
                    Assembly::AreaOperation::EdgeDamage?
                    Vec3d{.06,.06,1}:Vec3d{.12,.12,1}):
                rule.operation==Assembly::AreaOperation::EdgeDamage?
                    Vec3d{.035,.08,1}:Vec3d{.08,.08,1};
            rule.area.footprint.depth=rule.operation==
                Assembly::AreaOperation::EdgeDamage?.025:(pipe?.14:.045);
            rule.area.boundary.irregularity=.13;
            rule.area.boundary.rimFraction=.2;
            rule.area.boundary.seed=83;
            settings.damage.areaRules.push_back(rule);
        }
        auto built=Assembly::build(*item.path,settings);
        const auto replay=Assembly::build(*item.path,settings);
        if (meshHash(built.mesh.mesh)!=meshHash(replay.mesh.mesh) ||
            semanticHash(built.parts)!=semanticHash(replay.parts))
            throw std::runtime_error(std::string("Nondeterministic assembly: ")+item.name);
        std::optional<AutoUvPlan> bakeAtlas;
        if (caseName=="pier-bake-uv" || caseName=="plank-bake-uv")
        {
            bakeAtlas=planAutomaticUv(built.mesh.mesh);
            if (caseName=="plank-bake-uv")
            {
                std::ofstream stream(directory/"damaged-pier-bake.obj");
                Assembly::writeBakeObj(stream,built,*bakeAtlas);
            }
        }
        if (item.mode==10)
        {
            Construction::ConstructionResult axes;
            axes.path=built.path;axes.mesh=built.mesh;axes.cornerUV=built.cornerUV;
            const auto debug=Construction::withFrameAxes(axes);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else if (caseName=="plank-fracture-isolated" || geometric ||
                 !brokenTargetId.empty())
        {
            const bool pipe=caseName.substr(0,5)=="pipe-";
            const auto id=!brokenTargetId.empty()?brokenTargetId:
                Assembly::stablePartId(item.kind,
                pipe?Assembly::Role::PipeBody:Assembly::Role::DeckPlank,
                pipe?0:7);
            const auto part=std::find_if(built.parts.begin(),built.parts.end(),
                [&](const Assembly::Part &candidate){return candidate.id==id;});
            if (part==built.parts.end())
                throw std::runtime_error("Isolated fracture has no selected plank");
            const auto begin=std::ptrdiff_t(part->firstTriangle*3);
            const auto end=std::ptrdiff_t((part->firstTriangle+part->triangleCount)*3);
            auto isolated=built.mesh;
            isolated.mesh.indices.assign(built.mesh.mesh.indices.begin()+begin,
                                         built.mesh.mesh.indices.begin()+end);
            isolated.mesh.triangleRegions.assign(
                built.mesh.mesh.triangleRegions.begin()+
                    std::ptrdiff_t(part->firstTriangle),
                built.mesh.mesh.triangleRegions.begin()+
                    std::ptrdiff_t(part->firstTriangle+part->triangleCount));
            isolated.cornerNormals.assign(built.mesh.cornerNormals.begin()+begin,
                                          built.mesh.cornerNormals.begin()+end);
            const auto &sourceUv=bakeAtlas?bakeAtlas->cornerUv:built.cornerUV;
            const std::vector<std::array<float,2>> uv(sourceUv.begin()+begin,
                                                       sourceUv.begin()+end);
            renderer.upload(isolated,nullptr,nullptr,nullptr,false,&uv);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,
            bakeAtlas?&bakeAtlas->cornerUv:&built.cornerUV);
        auto lo=built.mesh.mesh.positions.front(),hi=lo;
        for (auto p:built.mesh.mesh.positions)
        {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
         hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
        Camera camera;
        camera.yaw=.35f;camera.pitch=-.25f;
        camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
        camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.25f,
                                   2.0f,90.0f);
        if (item.areaSide || caseName=="pier-break-closeup" ||
            caseName=="plank-fracture-isolated" || geometric ||
            !brokenTargetId.empty())
        {
            const bool pipe=geometric && caseName.substr(0,5)=="pipe-";
            const auto id=!brokenTargetId.empty()?brokenTargetId:
                Assembly::stablePartId(item.kind,
                pipe?Assembly::Role::PipeBody:Assembly::Role::DeckPlank,
                pipe?0:7);
            const auto part=std::find_if(built.parts.begin(),built.parts.end(),
                [&](const Assembly::Part &candidate){return candidate.id==id;});
            camera.target={float(part->position.x),float(part->position.y),
                           float(part->position.z)};
            if (geometric)
                camera.target={
                    float(settings.damage.areaRules.back().area.projector.center.x),
                    float(settings.damage.areaRules.back().area.projector.center.y),
                    float(settings.damage.areaRules.back().area.projector.center.z)};
            camera.distance=3.2f;
            camera.pitch=-.65f;
            if (std::string_view(item.name)=="pier-break-closeup")
            {camera.distance=2.4f;camera.yaw=.8f;camera.pitch=-.4f;}
            if (std::string_view(item.name)=="plank-fracture-isolated")
            {camera.distance=1.8f;camera.yaw=.6f;camera.pitch=-.55f;}
            if (geometric && !pipe)
            {camera.distance=1.4f;camera.yaw=.6f;camera.pitch=-.5f;}
            if (pipe) {camera.distance=1.4f;camera.yaw=.5f;camera.pitch=-.4f;}
            if (caseName=="broken-coupling" || caseName=="broken-rail-joint" ||
                caseName=="broken-rail-member")
            {camera.distance=.65f;camera.yaw=.5f;camera.pitch=-.4f;}
            if (caseName=="broken-beam-member")
            {camera.distance=2.5f;camera.yaw=.6f;camera.pitch=-.5f;}
            if (caseName=="broken-cross-beam")
            {camera.distance=1.5f;camera.yaw=.6f;camera.pitch=-.5f;}
        }
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
        nlohmann::json fragments=nlohmann::json::array();
        for (const auto &fragment:built.fragments)
            fragments.push_back({{"id",fragment.id},
                {"parentPartId",fragment.parentPartId},
                {"triangles",fragment.triangles.size()}});
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
            {"fragmentCount",built.fragments.size()},{"fragments",fragments},
            {"repeatStations",built.repeatedStations},
            {"manualDamageCount",settings.damage.manual.size()},
            {"damageAreaSide",item.areaSide},
            {"promotedBodies",built.promotedBodyCount},
            {"constraints",built.constraintCount},{"stateCounts",states},
            {"meshHash",meshHash(built.mesh.mesh)},
            {"semanticHash",semanticHash(built.parts)},
            {"parts",parts},{"image",std::string(item.name)+".png"}});
        if (bakeAtlas)
            proof["cases"].back()["bakeUv"]={{"islands",bakeAtlas->islandCount},
                {"pixelsPerMeter",bakeAtlas->pixelsPerMeter},
                {"activeAreaFraction",bakeAtlas->activeAreaFraction}};
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
void captureAssemblyFragmentPhysics(Renderer &renderer,
                                    const std::filesystem::path &directory)
{
    std::filesystem::create_directories(directory);
    const auto spline=path({{-3,0,0},{-1,0,0},{1,0,0},{3,0,0}});
    Assembly::Settings settings;
    settings.kind=Assembly::Kind::WoodenPier;
    const auto partId=Assembly::stablePartId(settings.kind,
        Assembly::Role::DeckPlank,7);
    settings.damage.manual[partId]=Assembly::State::Broken;
    const auto built=Assembly::build(spline,settings);
    const auto selected=std::find_if(built.fragments.begin(),built.fragments.end(),
        [&](const Assembly::Fragment &fragment)
        {return fragment.parentPartId==partId;});
    if (selected==built.fragments.end())
        throw std::runtime_error("Physical evidence has no broken plank fragment");
    Camera camera;
    camera.yaw=.6f;camera.pitch=-.45f;camera.distance=4.5f;
    camera.target={float(selected->center.x),float(selected->center.y-.3),
                   float(selected->center.z)};
    renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
    renderer.draw(1280,960,camera,0);
    renderer.capture(directory/"fragment-before.png");
    Assembly::FragmentPhysics physics;
    physics.activate(built,*selected);
    const auto before=physics.pose();
    for (unsigned i=0;i<120;++i) physics.step();
    const auto after=physics.pose();
    const auto moved=physics.displacedMesh(built,built.mesh);
    const auto lowest=std::min_element(built.mesh.mesh.positions.begin(),
        built.mesh.mesh.positions.end(),
        [](Vec3d a,Vec3d b){return a.y<b.y;})->y;
    double fragmentBottom=std::numeric_limits<double>::infinity();
    for (const auto face:selected->triangles)
        for (int corner=0;corner<3;++corner)
            fragmentBottom=std::min(fragmentBottom,
                moved.mesh.positions[built.mesh.mesh.indices[face*3+corner]].y);
    for (unsigned i=0;i<120;++i) physics.step();
    const auto settled=physics.pose();
    const auto settlingDrift=std::hypot(
        settled.position.x-after.position.x,
        settled.position.y-after.position.y,
        settled.position.z-after.position.z);
    if (fragmentBottom<lowest-.21 || settlingDrift>=.01)
        throw std::runtime_error("Fragment evidence did not establish floor contact");
    renderer.upload(moved,nullptr,nullptr,nullptr,false,&built.cornerUV);
    renderer.draw(1280,960,camera,0);
    renderer.capture(directory/"fragment-after.png");
    auto closeup=camera;
    closeup.target={float(after.position.x),float(after.position.y),
                    float(after.position.z)};
    closeup.distance=1.6f;
    renderer.draw(1280,960,closeup,0);
    renderer.capture(directory/"fragment-after-closeup.png");
    std::ofstream(directory/"fragment-physics-proof.json")<<nlohmann::json{
        {"fragmentId",selected->id},
        {"parentPartId",selected->parentPartId},
        {"fixedSteps",120},
        {"initialPosition",{before.position.x,before.position.y,before.position.z}},
        {"finalPosition",{after.position.x,after.position.y,after.position.z}},
        {"floorTopY",lowest-.2},
        {"fragmentBottomY",fragmentBottom},
        {"settlingDriftMeters",settlingDrift},
        {"stepCpuMilliseconds",after.stepCpuMilliseconds},
        {"beforeMeshHash",meshHash(built.mesh.mesh)},
        {"renderMeshHash",meshHash(moved.mesh)},
        {"adapter",renderer.adapter()},
        {"resolution",{1280,960}}}.dump(2)<<'\n';
}
}
