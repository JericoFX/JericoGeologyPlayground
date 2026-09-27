#include "WebLabEvidence.hpp"
#include "Renderer.hpp"
#include "WebLod.hpp"
#include "WebPhysics.hpp"
#include "SolidGeometry.hpp"

#include <nlohmann/json.hpp>
#include <stb_image_write.h>
#include <fstream>
#include <algorithm>
#include <stdexcept>

namespace Playground
{
namespace
{
void save(const std::filesystem::path &path,const Web::TextureLevel &level,
          const std::vector<std::uint8_t> &pixels)
{
    if (!stbi_write_png(path.string().c_str(),int(level.width),int(level.height),4,
                        pixels.data(),int(level.width*4)))
        throw std::runtime_error("Web texture PNG export failed");
}
std::vector<std::uint8_t> averaged(const Web::TextureLevel &base,
                                   const Web::TextureLevel &target)
{
    std::vector<std::uint8_t> output(std::size_t(target.width)*target.height*4);
    for (unsigned y=0;y<target.height;++y)
        for (unsigned x=0;x<target.width;++x)
        {
            unsigned sum{};
            for (unsigned dy=0;dy<2;++dy)
                for (unsigned dx=0;dx<2;++dx)
                    sum+=base.coverageRgba[4*(std::size_t(2*y+dy)*base.width+2*x+dx)+3];
            const auto value=std::uint8_t(sum/4);
            const auto index=4*(std::size_t(y)*target.width+x);
            output[index]=output[index+1]=output[index+2]=output[index+3]=value;
        }
    return output;
}
MeshResult corridorBackdrop()
{
    MeshResult room;
    auto quad=[&](Vec3d a,Vec3d b,Vec3d c,Vec3d d,Vec3d normal)
    {
        const auto base=std::uint32_t(room.mesh.positions.size());
        room.mesh.positions.insert(room.mesh.positions.end(),{a,b,c,d});
        room.normals.insert(room.normals.end(),4,normal);
        room.mesh.indices.insert(room.mesh.indices.end(),
            {base,base+1,base+2,base,base+2,base+3});
    };
    quad({-3,-2,-5},{3,-2,-5},{3,-2,9},{-3,-2,9},{0,1,0});
    quad({-3,2,-5},{3,2,-5},{3,2,9},{-3,2,9},{0,-1,0});
    quad({-3,-2,-5},{-3,2,-5},{-3,2,9},{-3,-2,9},{1,0,0});
    quad({3,-2,-5},{3,2,-5},{3,2,9},{3,-2,9},{-1,0,0});
    quad({-3,-2,9},{3,-2,9},{3,2,9},{-3,2,9},{0,0,-1});
    return room;
}
}
void captureWebSurface(Renderer &renderer,const std::filesystem::path &directory)
{
    std::filesystem::create_directories(directory);
    nlohmann::json proof;
    proof["adapter"]=renderer.adapter();
    proof["captureResolution"]={1280,960};
    proof["cases"]=nlohmann::json::array();
    proof["recipes"]=nlohmann::json::array();
    Web::Settings settings;
    settings.resolution=256;
    settings.radiusMeters=.018;
    struct Case {const char *name;Web::Preset preset;Web::Authoring authoring;
                 bool broken{};};
    const Case cases[]={{"plane-orb",Web::Preset::Orb,Web::Authoring::Plane},
        {"anchor-web",Web::Preset::Orb,Web::Authoring::Anchors},
        {"area-web",Web::Preset::Orb,Web::Authoring::Area},
        {"messy-cave",Web::Preset::Messy,Web::Authoring::Plane},
        {"corner",Web::Preset::Corner,Web::Authoring::Plane},
        {"broken-old",Web::Preset::Old,Web::Authoring::Plane},
        {"dense-nest",Web::Preset::Dense,Web::Authoring::Plane},
        {"trap-strands",Web::Preset::Trap,Web::Authoring::Plane},
        {"broken-edge",Web::Preset::Orb,Web::Authoring::Plane,true}};
    const auto graph=Web::makeGraph(settings);
    const auto selected=std::find_if(graph.edges.begin(),graph.edges.end(),
        [](const Web::Edge &edge){return edge.id=="Web/CornerA/Spoke_4_0";});
    if (selected==graph.edges.end()) throw std::runtime_error("Evidence damage edge missing");
    const auto firstId=selected->id;
    for (const auto &item:cases)
    {
        settings.preset=item.preset;
        settings.authoring=item.authoring;
        settings.anchors={{-1.4,-1.2,0},{1.5,-1.1,.002},
                          {1.25,1.4,0},{-1.35,1.2,-.001}};
        settings.area={{-1.4,-1.2,0},{1.4,-1.2,0},{.9,.8,0},
                       {0,1.4,0},{-1.3,.6,0}};
        settings.brokenEdges.clear();
        if (item.broken) settings.brokenEdges.insert(firstId);
        const auto web=Web::build(settings);
        if (!web.cardAccepted) throw std::runtime_error(std::string(item.name)+
            ": "+web.fallbackReason);
        Camera camera;
        camera.yaw=item.preset==Web::Preset::Corner?.55f:.1f;
        camera.pitch=-.1f;camera.distance=5.5f;
        renderer.uploadWeb(web);
        renderer.draw(1280,960,camera,11);
        renderer.capture(directory/(std::string(item.name)+"-card.png"));
        if (item.broken)
        {
            const auto geometry=Web::build(settings,true);
            renderer.upload(geometry.source);
            renderer.draw(1280,960,camera,0);
            renderer.capture(directory/"broken-edge-hilos-3d.png");
            renderer.uploadWeb(web);
        }
        double cardGpuMs=-1;
        if (item.preset==Web::Preset::Orb && !item.broken)
        {
            std::vector<double> samples;
            for (unsigned sample=0;sample<5;++sample)
            {
                const auto milliseconds=renderer.measureDrawGpu(1280,960,camera,11);
                if (milliseconds>=0) samples.push_back(milliseconds);
            }
            if (!samples.empty())
            {std::sort(samples.begin(),samples.end());cardGpuMs=samples[samples.size()/2];}
            renderer.uploadWeb(web,true);
            renderer.draw(1280,960,camera,11);
            renderer.capture(directory/"plane-orb-average-mips-card.png");
            renderer.uploadWeb(web);
            constexpr const char *names[]={"coverage","distance","normal"};
            for (int i=0;i<3;++i)
            {
                renderer.draw(1280,960,camera,12+i);
                renderer.capture(directory/(std::string("plane-orb-")+names[i]+".png"));
            }
            renderer.draw(1280,960,camera,15);
            renderer.capture(directory/"plane-orb-distance-render.png");
            save(directory/"plane-orb-coverage-texture.png",web.mips.front(),
                 web.mips.front().coverageRgba);
            save(directory/"plane-orb-distance-texture.png",web.mips.front(),
                 web.mips.front().distanceRgba);
            save(directory/"plane-orb-normal-texture.png",web.mips.front(),
                 web.mips.front().normalRgba);
            save(directory/"plane-orb-source-mip1.png",web.mips[1],
                 web.mips[1].coverageRgba);
            save(directory/"plane-orb-average-mip1.png",web.mips[1],
                 averaged(web.mips[0],web.mips[1]));
            proof["supersampling"]=nlohmann::json::array();
            for (unsigned sampleCount:{1u,4u,8u})
            {
                auto sampleSettings=settings;
                sampleSettings.supersamples=sampleCount;
                const auto sampleWeb=sampleCount==settings.supersamples?
                    Web::Result{}:Web::build(sampleSettings);
                const auto &comparison=sampleCount==settings.supersamples?web:sampleWeb;
                save(directory/("plane-orb-coverage-"+std::to_string(sampleCount)+"x.png"),
                     comparison.mips.front(),comparison.mips.front().coverageRgba);
                proof["supersampling"].push_back({{"samplesPerAxis",sampleCount},
                    {"bakeCpuMs",comparison.bakeCpuMilliseconds},
                    {"occupiedFraction",comparison.mips.front().occupiedFraction},
                    {"hash",Web::textureHash(comparison.mips.front())}});
            }
            proof["mips"]=nlohmann::json::array();
            auto averagedPixels=web.mips.front().coverageRgba;
            for (std::size_t level=0;level<web.mips.size();++level)
            {
                const auto &mip=web.mips[level];
                if (level)
                {
                    auto previous=web.mips[level-1];
                    previous.coverageRgba=std::move(averagedPixels);
                    averagedPixels=averaged(previous,mip);
                }
                std::uint64_t directAlpha{},averageAlpha{};
                std::size_t directOccupied{},averageOccupied{};
                for (std::size_t pixel=0;pixel<
                     std::size_t(mip.width)*mip.height;++pixel)
                {
                    const auto direct=mip.coverageRgba[pixel*4+3];
                    const auto averageValue=averagedPixels[pixel*4+3];
                    directAlpha+=direct;averageAlpha+=averageValue;
                    directOccupied+=direct>0;averageOccupied+=averageValue>0;
                }
                proof["mips"].push_back({{"level",level},
                    {"size",{mip.width,mip.height}},
                    {"sourceAlphaSum",directAlpha},{"averageAlphaSum",averageAlpha},
                    {"sourceOccupiedPixels",directOccupied},
                    {"averageOccupiedPixels",averageOccupied},
                    {"minimumWidthPixels",mip.minimumRepresentedWidthPixels}});
            }
            const auto debug=Web::build(settings,true);
            renderer.upload(debug.source);
            renderer.draw(1280,960,camera,0);
            renderer.capture(directory/"plane-orb-geometry-debug.png");
            samples.clear();
            for (unsigned sample=0;sample<5;++sample)
            {
                const auto milliseconds=renderer.measureDrawGpu(1280,960,camera,0);
                if (milliseconds>=0) samples.push_back(milliseconds);
            }
            double geometryGpuMs=-1;
            if (!samples.empty())
            {std::sort(samples.begin(),samples.end());geometryGpuMs=samples[samples.size()/2];}
            renderer.upload(debug.ribbons,nullptr,nullptr,nullptr,false,&debug.ribbonUV);
            renderer.draw(1280,960,camera,0);
            renderer.capture(directory/"plane-orb-ribbons-debug.png");
            renderer.upload(debug.coarsePhysical,nullptr,nullptr,nullptr,false,
                            &debug.coarsePhysicalUV);
            renderer.draw(1280,960,camera,0);
            renderer.capture(directory/"plane-orb-coarse-physical-graph.png");
            proof["geometryDebug"]={{"triangles",debug.source.mesh.indices.size()/3},
                {"ribbonTriangles",debug.ribbons.mesh.indices.size()/3},
                {"sourceRenderedEdges",debug.sourceRenderedEdges},
                {"sourceTruncated",debug.sourceTruncated},
                {"coarseCandidateEdges",debug.physicalEdgeIds.size()},
                {"joltBodies",0},
                {"generationCpuMs",debug.bakeCpuMilliseconds},
                {"gpuDrawMsMedian5",geometryGpuMs}};
        }
        nlohmann::json anchors=nlohmann::json::array(),area=nlohmann::json::array();
        for (auto p:settings.anchors) anchors.push_back({p.x,p.y,p.z});
        for (auto p:settings.area) area.push_back({p.x,p.y,p.z});
        nlohmann::json edgeIds=nlohmann::json::array();
        for (const auto &edge:web.graph.edges) edgeIds.push_back(edge.id);
        std::vector<std::string> broken(settings.brokenEdges.begin(),
                                        settings.brokenEdges.end());
        std::sort(broken.begin(),broken.end());
        proof["recipes"].push_back({{"name",item.name},{"preset",int(settings.preset)},
            {"authoring",int(settings.authoring)},{"seed",settings.seed},
            {"widthMeters",settings.widthMeters},{"heightMeters",settings.heightMeters},
            {"center",{settings.center.x,settings.center.y,settings.center.z}},
            {"axisU",{settings.axisU.x,settings.axisU.y,settings.axisU.z}},
            {"axisV",{settings.axisV.x,settings.axisV.y,settings.axisV.z}},
            {"anchors",anchors},{"area",area},{"spokes",settings.spokes},
            {"rings",settings.rings},{"density",settings.density},
            {"secondaryStrands",settings.secondaryStrands},
            {"irregularity",settings.irregularity},
            {"missingFraction",settings.missingFraction},
            {"sagMeters",settings.sagMeters},{"hubOffsetU",settings.hubOffsetU},
            {"hubOffsetV",settings.hubOffsetV},{"radiusMeters",settings.radiusMeters},
            {"warpMeters",settings.warpMeters},
            {"normalStrength",settings.normalStrength},
            {"resolution",settings.resolution},{"supersamples",settings.supersamples},
            {"maximumCards",settings.maximumCards},
            {"physicalEdgeBudget",settings.physicalEdgeBudget},
            {"brokenEdges",broken},
            {"nodeIds",web.graph.nodeIds},{"edgeIds",edgeIds}});
        proof["cases"].push_back({{"name",item.name},{"preset",int(item.preset)},
            {"authoring",int(item.authoring)},{"widthMeters",settings.widthMeters},
            {"heightMeters",settings.heightMeters},{"anchors",anchors},{"area",area},
            {"seed",settings.seed},{"spokes",settings.spokes},{"rings",settings.rings},
            {"radiusMeters",settings.radiusMeters},{"resolution",settings.resolution},
            {"bakeResolution",web.bakeResolution},
            {"bakeSupersamples",web.bakeSupersamples},
            {"bakeBudgetReduced",web.bakeBudgetReduced},
            {"supersamples",settings.supersamples},{"brokenEdge",item.broken?firstId:""},
            {"nodes",web.graph.nodes.size()},{"edges",web.graph.edges.size()},
            {"activeEdges",web.activeEdgeCount},{"cards",web.patches.size()},
            {"triangles",web.card.mesh.indices.size()/3},
            {"ordinaryTubeTriangles",web.source.mesh.indices.size()/3},
            {"coverageFraction",web.mips.front().occupiedFraction},
            {"emptyCardFraction",1-web.mips.front().occupiedFraction},
            {"textureBytes",web.textureBytes},{"generationCpuMs",web.bakeCpuMilliseconds},
            {"gpuDrawMsMedian5",cardGpuMs},
            {"textureHash",Web::textureHash(web.mips.front())},
            {"cardMeshHash",meshHash(web.card.mesh)}});
    }
    Web::Settings subtle;
    subtle.style=Web::Style::Ghost;
    subtle.radiusMeters=.006;
    subtle.microStrandDensity=.8;
    subtle.supersamples=8;
    subtle.optics.baseVisibility=.025f;
    subtle.optics.specularResponse=1.5f;
    subtle.optics.lightRevealStrength=1.8f;
    subtle.optics.viewAngleResponse=.5f;
    const auto ghost=Web::build(subtle);
    if (!ghost.cardAccepted) throw std::runtime_error("Ghost Web card rejected");
    Camera opticalCamera;
    opticalCamera.yaw=.1f;opticalCamera.pitch=-.1f;opticalCamera.distance=5.5f;
    proof["opticalCases"]=nlohmann::json::array();
    auto opticalCapture=[&](const char *name,const Web::Result &result,
                            const Web::Optics &optics,const Camera &camera,
                            const Web::Result *structure=nullptr,
                            Web::Style style=Web::Style::Ghost,
                            const MeshResult *backdrop=nullptr)
    {
        renderer.setWebAppearance(optics);
        renderer.uploadWeb(result,false,structure,backdrop);
        renderer.draw(1280,960,camera,11);
        renderer.capture(directory/(std::string(name)+".png"));
        std::vector<double> samples;
        for (unsigned i=0;i<3;++i)
        {
            const auto ms=renderer.measureDrawGpu(1280,960,camera,11);
            if (ms>=0) samples.push_back(ms);
        }
        double median=-1;
        if (!samples.empty())
        {std::sort(samples.begin(),samples.end());median=samples[samples.size()/2];}
        proof["opticalCases"].push_back({{"name",name},
            {"style",int(style)},
            {"vertices",result.card.mesh.positions.size()},
            {"triangles",result.card.mesh.indices.size()/3},
            {"drawCalls",1+int(structure!=nullptr)+int(backdrop!=nullptr)},
            {"instances",1},
            {"textureBytes",result.textureBytes},
            {"generationCpuMs",result.bakeCpuMilliseconds},
            {"gpuDrawMsMedian3",median},
            {"lightXYZ",optics.lightPosition},
            {"baseVisibility",optics.baseVisibility},
            {"wetness",optics.wetness},
            {"dropletDensity",optics.dropletDensity}});
    };
    auto optics=subtle.optics;
    optics.lightPosition={0,0,20};optics.lightReach=1;
    opticalCapture("ghost-unlit",ghost,optics,opticalCamera);
    optics.lightPosition={-1.3f,1.0f,-2.0f};optics.lightReach=5;
    opticalCapture("ghost-light-left",ghost,optics,opticalCamera);
    optics.lightPosition={1.3f,1.0f,-2.0f};
    opticalCapture("ghost-light-right",ghost,optics,opticalCamera);
    optics.wetness=.9f;optics.roughness=.25f;
    optics.dropletDensity=.18f;optics.dropletSizeMin=.008f;
    optics.dropletSizeMax=.025f;
    opticalCapture("ghost-wet-droplets",ghost,optics,opticalCamera);
    const auto corridor=corridorBackdrop();
    auto corridorOptics=subtle.optics;
    corridorOptics.lightReach=5;
    corridorOptics.lightPosition={0,0,20};
    opticalCapture("corridor-ghost-unlit",ghost,corridorOptics,opticalCamera,
                   nullptr,Web::Style::Ghost,&corridor);
    for (float lightX:{-2.0f,-1.0f,0.0f,1.0f,2.0f})
    {
        corridorOptics.lightPosition={lightX,.7f,-2.0f};
        const auto name="corridor-ghost-light-"+std::to_string(int(lightX+2));
        opticalCapture(name.c_str(),ghost,corridorOptics,opticalCamera,
                       nullptr,Web::Style::Ghost,&corridor);
    }
    corridorOptics.wetness=.9f;
    corridorOptics.roughness=.25f;
    corridorOptics.dropletDensity=.18f;
    corridorOptics.dropletSizeMin=.008f;
    corridorOptics.dropletSizeMax=.025f;
    opticalCapture("corridor-ghost-wet",ghost,corridorOptics,opticalCamera,
                   nullptr,Web::Style::Ghost,&corridor);
    for (float distance:{2.5f,6.0f,20.0f,40.0f})
    {
        auto camera=opticalCamera;camera.distance=distance;
        const auto name="ghost-distance-"+std::to_string(int(distance));
        opticalCapture(name.c_str(),ghost,optics,camera);
    }
    Web::Result batch=ghost;
    batch.card={};
    batch.cardUV.clear();
    for (unsigned row=0;row<10;++row)
        for (unsigned column=0;column<10;++column)
        {
            const auto base=std::uint32_t(batch.card.mesh.positions.size());
            for (const auto position:ghost.card.mesh.positions)
                batch.card.mesh.positions.push_back({
                    position.x+(double(column)-4.5)*3.2,
                    position.y+(double(row)-4.5)*3.2,position.z});
            batch.card.normals.insert(batch.card.normals.end(),
                ghost.card.normals.begin(),ghost.card.normals.end());
            batch.card.field.insert(batch.card.field.end(),
                ghost.card.field.begin(),ghost.card.field.end());
            for (const auto index:ghost.card.mesh.indices)
                batch.card.mesh.indices.push_back(base+index);
            batch.card.cornerNormals.insert(batch.card.cornerNormals.end(),
                ghost.card.cornerNormals.begin(),ghost.card.cornerNormals.end());
            batch.cardUV.insert(batch.cardUV.end(),
                ghost.cardUV.begin(),ghost.cardUV.end());
        }
    auto batchOptics=optics;
    batchOptics.cullDistance=100;
    batchOptics.shaderOnlyDistance=80;
    batchOptics.baseVisibility=.25f;
    batchOptics.lightPosition={0,0,-25};
    batchOptics.lightReach=50;
    batchOptics.dropletDensity=0;
    auto batchCamera=opticalCamera;
    batchCamera.distance=42;
    opticalCapture("ghost-100-shared-cards",batch,batchOptics,batchCamera);
    auto &batchCase=proof["opticalCases"].back();
    batchCase["cardCount"]=100;
    batchCase["instances"]=1;
    batchCase["sharedTextureBytes"]=ghost.textureBytes;
    Web::Settings mediumSettings=subtle;
    mediumSettings.style=Web::Style::Medium;
    mediumSettings.microStrandDensity=.35;
    mediumSettings.radiusMeters=.012;
    mediumSettings.optics.baseVisibility=.18f;
    mediumSettings.optics.wetness=.65f;
    mediumSettings.optics.lightRevealStrength=1;
    mediumSettings.optics.dropletDensity=.08f;
    mediumSettings.structuralStrandCount=8;
    const auto medium=Web::build(mediumSettings);
    const auto mediumDebug=Web::build(mediumSettings,true);
    opticalCapture("medium-wet-hybrid",medium,mediumSettings.optics,
                   opticalCamera,&mediumDebug,Web::Style::Medium);
    Web::WebPhysics physical;
    physical.reset(mediumSettings,medium);
    const auto initialNetwork=physical.network();
    renderer.upload(Web::makePhysicalMesh(initialNetwork,{0,0,1}));
    renderer.draw(1280,960,opticalCamera,0);
    renderer.capture(directory/"web-jolt-network-initial.png");
    for (unsigned i=0;i<120;++i) physical.step();
    const auto finalNetwork=physical.network();
    renderer.upload(Web::makePhysicalMesh(finalNetwork,{0,0,1}));
    renderer.draw(1280,960,opticalCamera,0);
    renderer.capture(directory/"web-jolt-network-after-120-steps.png");
    const auto hubDisplacement=Vec3d{
        finalNetwork.positions.front().x-initialNetwork.positions.front().x,
        finalNetwork.positions.front().y-initialNetwork.positions.front().y,
        finalNetwork.positions.front().z-initialNetwork.positions.front().z};
    renderer.setWebAppearance(mediumSettings.optics);
    renderer.uploadWeb(medium,false,&mediumDebug);
    renderer.setWebDisplacement(hubDisplacement);
    renderer.draw(1280,960,opticalCamera,11);
    renderer.capture(directory/"medium-hybrid-after-physics.png");
    renderer.setWebDisplacement({});
    proof["physics"]={{"backend","Jolt"},{"fixedHz",60},{"steps",120},
        {"vertices",finalNetwork.positions.size()},
        {"constraints",finalNetwork.edges.size()},
        {"anchors",finalNetwork.anchors.size()},
        {"stepCpuMsLast",finalNetwork.stepCpuMilliseconds},
        {"hubDisplacement",{hubDisplacement.x,hubDisplacement.y,hubDisplacement.z}},
        {"visualStrands",medium.activeEdgeCount},
        {"hybridDrawCalls",2}};
    Web::Settings removed;
    for (const auto &edge:Web::makeGraph(removed).edges)
        removed.brokenEdges.insert(edge.id);
    const auto noStrands=Web::build(removed);
    if (noStrands.activeEdgeCount || noStrands.cardAccepted)
        throw std::runtime_error("All-broken web retained visible strands");
    renderer.clearMesh();
    Camera emptyCamera;
    renderer.draw(1280,960,emptyCamera,0);
    renderer.capture(directory/"all-broken-empty-web.png");
    proof["allBroken"]={{"activeEdges",noStrands.activeEdgeCount},
        {"cardTriangles",noStrands.card.mesh.indices.size()/3},
        {"ribbonTriangles",noStrands.ribbons.mesh.indices.size()/3}};
    std::ofstream stream(directory/"metrics.json",std::ios::binary);
    stream<<proof.dump(2)<<'\n';
    if (!stream) throw std::runtime_error("Web metrics export failed");
    std::ofstream recipes(directory/"recipes.json",std::ios::binary);
    recipes<<proof["recipes"].dump(2)<<'\n';
    if (!recipes) throw std::runtime_error("Web recipe export failed");
}
}
