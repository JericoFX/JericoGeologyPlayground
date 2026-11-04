#include "SplineDamageIO.hpp"
#include "SplineLabEvidence.hpp"
#include "SplineLabUI.hpp"
#include "SolidGeometry.hpp"

#include <fstream>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <algorithm>
#include <cmath>

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
void captureSpatialSplineDamage(Renderer &renderer,const std::filesystem::path &directory)
{
    if(std::filesystem::exists(directory)&&!std::filesystem::is_empty(directory))
        throw std::runtime_error("Spatial damage evidence directory must be empty");
    std::filesystem::create_directories(directory);
    nlohmann::json proof;proof["adapter"]=renderer.adapter();proof["resolution"]={1280,960};
    proof["interactive"]=false;proof["cases"]=nlohmann::json::array();
    auto volume=[](const TriangleMesh &mesh)
    {
        double sum=0;
        for(std::size_t i=0;i<mesh.indices.size();i+=3)
        {
            const auto a=mesh.positions[mesh.indices[i]],b=mesh.positions[mesh.indices[i+1]],c=mesh.positions[mesh.indices[i+2]];
            sum+=(a.x*(b.y*c.z-b.z*c.y)+a.y*(b.z*c.x-b.x*c.z)+a.z*(b.x*c.y-b.y*c.x))/6;
        }
        return sum;
    };
    for(unsigned fixture=0;fixture<8;++fixture)
    {
        const auto base=fixture%4;
        const bool curved=base%2!=0,raised=base>=2,follow=fixture>=4;
        const std::string name=std::string(curved?"curved-pipe":"cube")+(raised?"-decal":"")+(follow?"-surface":"");
        const auto host=curved?path({{-1.5,0,-.6},{-.6,.55,.2},{.4,-.5,-.15},{1.5,0,.65}},PA::SplineInterpolation::CatmullRom):
                                path({{0,0,-1},{0,0,1}},PA::SplineInterpolation::Polyline);
        auto settings=splinePreset(1);
        if(!curved){settings.profile=Construction::ProfileKind::Rectangle;settings.widthMeters=2;settings.heightMeters=2;}
        const auto before=Construction::buildConstruction(host,settings);
        Construction::PathDamageRule rule;rule.id="continuous-spatial-crack";
        rule.cutter.projector.center={};rule.cutter.projector.tangent={1,0,0};
        rule.cutter.projector.bitangent={0,1,0};rule.cutter.projector.normal={0,0,1};
        rule.cutter.path={{-.5,0,0},{.5,0,0}};rule.cutter.seed=719;
        rule.cutter.radius=curved?.016:.12;rule.cutter.depth=curved?.014:.07;rule.cutter.irregularity=.25;
        rule.operation=raised?DamageVolumeOperation::Union:DamageVolumeOperation::Difference;
        rule.followSurface=follow;rule.surfaceDistanceMeters=curved?.05:.3;
        std::vector<Vec3d> points;
        if(!curved)points={{-.7,1,.3},{.7,1,.3},{1,.7,.3},{1,0,.8},{1,-.7,-.3}};
        else for(unsigned i=0;i<9;++i)
        {
            const auto frame=Construction::frameAtDistance(before.path,before.path.lengthMeters*(.2+.6*i/8));
            points.push_back({frame.position.x+frame.normal.x*settings.radiusMeters,
                frame.position.y+frame.normal.y*settings.radiusMeters,frame.position.z+frame.normal.z*settings.radiusMeters});
        }
        rule.spatialPath=path(points,PA::SplineInterpolation::Polyline);
        settings.damagePaths.push_back(rule);
        const auto after=Construction::buildConstruction(host,settings);
        if((raised?volume(after.mesh.mesh)<=volume(before.mesh.mesh):volume(after.mesh.mesh)>=volume(before.mesh.mesh))||
            meshHash(after.mesh.mesh)==meshHash(before.mesh.mesh))
            throw std::runtime_error("Spatial evidence has the wrong volume response");
        if(raised&&!freshInteriorCornerMask(after.mesh).empty())
            throw std::runtime_error("Raised decal was mislabeled as fresh interior");
        const auto recipe=directory/(name+"-damage.json");Construction::saveSplineDamage(settings.damagePaths,recipe);
        auto restored=settings;restored.damagePaths.clear();
        const auto replay=Construction::loadSplineDamage(host,restored,recipe);
        if(meshHash(replay.mesh.mesh)!=meshHash(after.mesh.mesh)||replay.cornerUV!=after.cornerUV)
            throw std::runtime_error("Spatial native replay mismatch");
        restored.damagePaths[0].cutter.enabled=false;const auto off=Construction::buildConstruction(host,restored);
        if(meshHash(off.mesh.mesh)!=meshHash(before.mesh.mesh))throw std::runtime_error("Spatial native off mismatch");
        nlohmann::json images=nlohmann::json::array();
        const Construction::ConstructionResult *states[]={&before,&after,&off,&replay};
        const char *labels[]={"before","after","off","replay"};
        for(unsigned stage=0;stage<4;++stage)
        {
            const auto &built=*states[stage];renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
            for(unsigned view=0;view<3;++view)
            {
                Camera camera;camera.yaw=view==1?3.69f:.55f;camera.pitch=view==1?.28f:-.28f;
                camera.target={0,0,0};camera.distance=curved?4.4f:4.9f;
                if(view==2)
                {
                    const auto p=points[points.size()/2];camera.target={float(p.x),float(p.y),float(p.z)};
                    camera.distance=curved?.8f:1.5f;camera.yaw=curved?.55f:1.2f;camera.pitch=-.6f;
                }
                const auto image=name+"-"+labels[stage]+"-"+std::to_string(view)+".png";
                renderer.draw(1280,960,camera,0);renderer.capture(directory/image);
                images.push_back({{"file",image},{"yaw",camera.yaw},{"pitch",camera.pitch},{"distance",camera.distance},{"target",camera.target}});
            }
        }
        proof["cases"].push_back({{"name",name},{"operation",raised?"VolumeUnion":"VolumeDifference"},
            {"followSurface",follow},{"maximumProjectionMeters",after.maximumDamageProjectionMeters},
            {"maximumSampledSurfaceErrorMeters",after.maximumDamageSurfaceErrorMeters},
            {"beforeHash",meshHash(before.mesh.mesh)},{"afterHash",meshHash(after.mesh.mesh)},
            {"beforeVolume",volume(before.mesh.mesh)},{"afterVolume",volume(after.mesh.mesh)},
            {"beforeTriangles",before.mesh.mesh.indices.size()/3},{"afterTriangles",after.mesh.mesh.indices.size()/3},
            {"replayExact",true},{"offExact",true},{"recipe",recipe.filename().string()},{"images",images}});
    }
    std::ofstream(directory/"spatial-damage-proof.json")<<proof.dump(2)<<'\n';
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
    struct Case {const char *name;int preset;const PA::ProceduralSpline *spline;int mode;
                 bool sharedDamage{},bakeUv{};int fittedDamage{};double profileScale{1};};
    const Case cases[]={
        {"pipe-straight",1,&line,0},{"pipe-s-curve",1,&s,0},
        {"pipe-uv-checker",1,&s,8},{"pipe-vertical-rmf",1,&vertical,10},
        {"pipe-seam",1,&s,9},{"rectangular-duct",6,&s,0},
        {"cable-tray-crossbars",7,&s,0},{"i-beam-run",9,&s,0},
        {"high-curvature-bend",2,&bend,2},{"cable",3,&s,0},
        {"corrugated-hose",10,&s,0},
        {"fractured-pipe",11,&s,0},
        {"twisted-duct",12,&s,0},
        {"shared-crack-lit",1,&line,0,true,false},
        {"shared-crack-wire",1,&line,2,true,false},
        {"shared-crack-bake-uv",1,&line,8,true,true},
        {"fitted-small-before",1,&line,0,true,false,2,.15},
        {"fitted-small-after",1,&line,0,true,false,1,.15},
        {"fitted-large-before",1,&line,0,true,false,2,3},
        {"fitted-large-after",1,&line,0,true,false,1,3},
        {"fitted-hollow-before",2,&line,0,true,false,2},
        {"fitted-hollow-after",2,&line,0,true,false,1},
        {"fitted-beam-before",9,&line,0,true,false,2},
        {"fitted-beam-after",9,&line,0,true,false,1},
        {"fitted-ribs-before",10,&line,0,true,false,2},
        {"fitted-ribs-after",10,&line,0,true,false,1},
        {"fitted-twist-before",12,&line,0,true,false,2},
        {"fitted-twist-after",12,&line,0,true,false,1},
        {"fitted-curve-before",1,&s,0,true,false,2},
        {"fitted-curve-after",1,&s,0,true,false,1}};
    nlohmann::json proof;
    proof["adapter"]=renderer.adapter();
    proof["resolution"]={1280,960};
    proof["cases"]=nlohmann::json::array();
    for (const auto &item:cases)
    {
        auto settings=splinePreset(item.preset);
        settings.profileScale=item.profileScale;
        std::optional<Construction::PathDamageRule> proposal;
        std::uint64_t sourceHash{};
        if (item.fittedDamage)
        {
            const auto source=Construction::buildConstruction(*item.spline,settings);
            sourceHash=meshHash(source.mesh.mesh);
            proposal=Construction::proposeSplineDamage(source,settings,"fitted-evidence-crack",777);
            if (item.fittedDamage==1) settings.damagePaths.push_back(*proposal);
        }
        else if (item.sharedDamage)
        {
            const auto intact=Construction::buildConstruction(*item.spline,settings);
            const auto frame=Construction::frameAtDistance(intact.path,
                                                           intact.path.lengthMeters*.5);
            Construction::PathDamageRule rule;
            rule.id="spline-evidence-crack";
            rule.cutter.projector.center={
                frame.position.x+frame.normal.x*settings.radiusMeters,
                frame.position.y+frame.normal.y*settings.radiusMeters,
                frame.position.z+frame.normal.z*settings.radiusMeters};
            rule.cutter.projector.normal=frame.normal;
            rule.cutter.projector.tangent=frame.tangent;
            rule.cutter.projector.bitangent=frame.binormal;
            rule.cutter.path={{-.3,0,0},{.3,0,0}};
            rule.cutter.radius=.02;
            rule.cutter.depth=.05;
            rule.cutter.irregularity=.3;
            rule.cutter.seed=777;
            settings.damagePaths.push_back(std::move(rule));
        }
        const auto built=Construction::buildConstruction(*item.spline,settings);
        if (item.fittedDamage==1 && meshHash(built.mesh.mesh)==sourceHash)
            throw std::runtime_error(std::string("Fitted damage made no change: ")+item.name);
        auto restored=settings;
        const auto damageFile=directory/(std::string(item.name)+"-damage.json");
        if (item.sharedDamage) {
            Construction::saveSplineDamage(settings.damagePaths,damageFile);
            restored.damagePaths.clear();
        }
        const auto replay=item.sharedDamage ?
            Construction::loadSplineDamage(*item.spline,restored,damageFile) :
            Construction::buildConstruction(*item.spline,restored);
        if (built.bakeCornerUV!=replay.bakeCornerUV)
            throw std::runtime_error("Spline damage bake UV persistence mismatch");
        if (meshHash(built.mesh.mesh)!=meshHash(replay.mesh.mesh) ||
            uvHash(built.cornerUV)!=uvHash(replay.cornerUV))
            throw std::runtime_error(std::string("Nondeterministic spline construction: ")+item.name);
        if (item.mode==10)
        {
            const auto debug=Construction::withFrameAxes(built);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(replay.mesh,nullptr,nullptr,nullptr,false,
            item.bakeUv ? &replay.bakeCornerUV : &replay.cornerUV);
        auto lo=built.mesh.mesh.positions.front(),hi=lo;
        for (auto p:built.mesh.mesh.positions)
        {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
         hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
        Camera camera;
        camera.yaw=.52f;camera.pitch=-.22f;
        camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
        camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.35f,1.0f,90.0f);
        if (item.sharedDamage && !item.fittedDamage)
        {camera.pitch=-.45f;camera.distance=1.0f;}
        renderer.draw(1280,960,camera,item.mode);
        renderer.capture(directory/(std::string(item.name)+".png"));
        nlohmann::json fitted=nullptr;
        if (proposal)
        {
            const auto &cutter=proposal->cutter;
            const auto p=cutter.projector.center;
            camera.target={float(p.x),float(p.y),float(p.z)};
            camera.pitch=-1.1f;
            camera.distance=float(std::clamp(cutter.path.back().x-cutter.path.front().x,.12,.8));
            renderer.draw(1280,960,camera,item.mode);
            const auto detail=std::string(item.name)+"-detail.png";
            renderer.capture(directory/detail);
            fitted={{"enabled",item.fittedDamage==1},{"sourceMeshHash",sourceHash},
                    {"proposal",nlohmann::json::parse(Construction::serializeSplineDamage({*proposal}))},
                    {"detailImage",detail},{"detailDistance",camera.distance},
                    {"detailTarget",camera.target},{"detailYaw",camera.yaw},{"detailPitch",camera.pitch}};
        }
        nlohmann::json points=nlohmann::json::array();
        for (const auto &point:item.spline->points)
            points.push_back({point.position.x,point.position.y,point.position.z});
        proof["cases"].push_back({
            {"name",item.name},{"preset",item.preset},{"viewMode",item.mode},
            {"controlPoints",points},{"interpolation",int(item.spline->interpolation)},
            {"profile",int(settings.profile)},{"radiusMeters",settings.radiusMeters},
            {"profileScale",settings.profileScale},{"profileRotationRadians",settings.profileRotationRadians},
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
            {"sharedDamage",item.sharedDamage},{"damageFileRoundtrip",item.sharedDamage},{"bakeUv",item.bakeUv},
            {"fittedDamage",fitted},
            {"bakeIslandCount",built.bakeIslandCount},
            {"bakeUvHash",uvHash(built.bakeCornerUV)},
            {"geometryBytesApprox",built.mesh.mesh.positions.size()*sizeof(Vec3d)+
                built.mesh.mesh.indices.size()*sizeof(std::uint32_t)},
            {"image",std::string(item.name)+".png"}});
    }
    std::ofstream(directory/"spline-construction-proof.json")<<proof.dump(2)<<'\n';
}
}
