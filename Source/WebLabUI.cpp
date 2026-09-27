#include "WebLabUI.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Playground
{
namespace
{
void applyStyle(Web::Settings &settings,Web::Style style)
{
    settings.style=style;
    if (style==Web::Style::Ghost)
    {
        settings.radiusMeters=.004;
        settings.microStrandDensity=.65;
        settings.optics.baseVisibility=.025f;
        settings.optics.specularResponse=1.5f;
        settings.optics.lightRevealStrength=1.8f;
        settings.optics.viewAngleResponse=.5f;
    }
    else if (style==Web::Style::Medium)
    {
        settings.radiusMeters=.009;
        settings.microStrandDensity=.35;
        settings.optics.baseVisibility=.18f;
        settings.optics.specularResponse=1.0f;
        settings.optics.lightRevealStrength=.8f;
        settings.optics.viewAngleResponse=.3f;
    }
    else
    {
        settings.radiusMeters=.015;
        settings.microStrandDensity=0;
        settings.optics.baseVisibility=1.0f;
        settings.optics.specularResponse=.6f;
        settings.optics.lightRevealStrength=0;
        settings.optics.viewAngleResponse=0;
    }
}
constexpr const char *presets[]={"Orb Web","Corner Cobweb","Messy Cave Web",
    "Broken Old Web","Dense Nest","Trap Strands"};
constexpr const char *views[]={"Card / lit","Coverage","Distance field",
    "Distance render","Pseudo normals","Ribbons","Hilos 3D",
    "Coarse physical graph"};
void panel(const char *name,float x,float y,float width,float height)
{
    ImGui::SetNextWindowPos({x,y});
    ImGui::SetNextWindowSize({width,height});
    ImGui::Begin(name,nullptr,ImGuiWindowFlags_NoMove|
        ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
}
bool editPoint(const char *name,Vec3d &point)
{
    double xyz[]={point.x,point.y,point.z};
    if (!ImGui::InputScalarN(name,ImGuiDataType_Double,xyz,3,nullptr,nullptr,"%.3f"))
        return false;
    point={xyz[0],xyz[1],xyz[2]};
    return true;
}
Vec3d add(Vec3d a,Vec3d b) {return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3d mul(Vec3d a,double s) {return {a.x*s,a.y*s,a.z*s};}
double dot(Vec3d a,Vec3d b) {return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3d cross(Vec3d a,Vec3d b)
{return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
Vec3d unit(Vec3d p)
{const auto n=std::sqrt(dot(p,p));return n>1e-9?mul(p,1/n):Vec3d{};}
struct ViewAxes {Vec3d right,forward,up,eye;};
ViewAxes axes(const Camera &camera)
{
    const double cy=std::cos(camera.yaw),sy=std::sin(camera.yaw);
    const double cp=std::cos(camera.pitch),sp=std::sin(camera.pitch);
    const Vec3d right{cy,0,-sy},forward{sy*cp,sp,cy*cp};
    const auto target=Vec3d{camera.target[0],camera.target[1],camera.target[2]};
    return {right,forward,cross(forward,right),sub(target,mul(forward,camera.distance))};
}
ImVec2 pointScreen(Vec3d point,const Camera &camera,ImVec2 origin,ImVec2 size)
{
    const auto basis=axes(camera);
    const auto q=sub(point,basis.eye);
    const auto depth=dot(q,basis.forward);
    if (depth<=.05) return {-10000,-10000};
    const auto ndcX=dot(q,basis.right)*2.4/(depth*(size.x/size.y));
    const auto ndcY=dot(q,basis.up)*2.4/depth;
    return {origin.x+float((ndcX*.5+.5)*size.x),
            origin.y+float((.5-ndcY*.5)*size.y)};
}
bool onPlacementPlane(ImVec2 mouse,const Camera &camera,ImVec2 origin,ImVec2 size,
                      const Web::Settings &settings,Vec3d &point)
{
    if (size.x<1 || size.y<1) return false;
    const auto basis=axes(camera);
    const auto normal=unit(cross(settings.axisU,settings.axisV));
    if (dot(normal,normal)<.5) return false;
    const double x=(2*(mouse.x-origin.x)/size.x-1)*(size.x/size.y)/2.4;
    const double y=(1-2*(mouse.y-origin.y)/size.y)/2.4;
    const auto direction=unit(add(basis.forward,add(mul(basis.right,x),mul(basis.up,y))));
    const auto denominator=dot(direction,normal);
    if (std::abs(denominator)<1e-7) return false;
    const auto distance=dot(sub(settings.center,basis.eye),normal)/denominator;
    if (distance<=.05 || distance>1000) return false;
    point=add(basis.eye,mul(direction,distance));
    return true;
}
}
WebLabUI::WebLabUI()
{
    applyStyle(settings,Web::Style::Ghost);
    camera.yaw=.3f;camera.pitch=-.12f;camera.distance=7;
    settings.anchors={{-1.5,-1.5,0},{1.5,-1.5,0},{1.5,1.5,0},{-1.5,1.5,0}};
    settings.area=settings.anchors;
    edited=std::chrono::steady_clock::now();
}
void WebLabUI::activate()
{if (pendingCancellation) pendingCancellation->store(true);
 ++revision;uploadedView=-1;dirty=true;
 debugRevision=physicalRevision=~0u;
 edited=std::chrono::steady_clock::now()-std::chrono::seconds(1);}
void WebLabUI::changed()
{if (pendingCancellation) pendingCancellation->store(true);
 ++revision;dirty=true;debugRevision=physicalRevision=~0u;
 edited=std::chrono::steady_clock::now();}
void WebLabUI::process()
{
    const auto now=std::chrono::steady_clock::now();
    if (pending.valid() && pending.wait_for(std::chrono::seconds(0))==
        std::future_status::ready)
    {
        try
        {
            auto result=pending.get();
            if (pendingRevision==revision)
            {
                if (pendingDebug) {debug=std::move(result);debugRevision=revision;}
                else
                {
                    built=std::move(result);builtRevision=revision;
                    const auto &fitMesh=built.cardAccepted?built.card:built.ribbons;
                    if (fitCamera && !fitMesh.mesh.positions.empty())
                    {
                        auto lo=fitMesh.mesh.positions.front(),hi=lo;
                        for (auto p:fitMesh.mesh.positions)
                        {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
                         hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
                        camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),
                            float((lo.z+hi.z)*.5)};
                        camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,
                            hi.y-lo.y,hi.z-lo.z))*1.7f,2.0f,90.0f);
                        fitCamera=false;
                    }
                    dirty=false;
                }
                error.clear();uploadedView=-1;
            }
        }
        catch (const std::exception &exception)
        {if (pendingRevision==revision)
         {error=exception.what();dirty=false;breakStarted.reset();}}
    }
    if (!pending.valid() && dirty && now-edited>std::chrono::milliseconds(250))
    {
        const auto snapshot=settings;
        pendingCancellation=std::make_shared<std::atomic_bool>(false);
        const auto cancellation=pendingCancellation;
        pendingRevision=revision;pendingDebug=false;
        pending=std::async(std::launch::async,[snapshot,cancellation]
            {return Web::build(snapshot,false,cancellation.get());});
    }
    else if (!pending.valid() && !dirty &&
             (view>=5 || (autoLod && settings.style!=Web::Style::Ghost &&
                           camera.distance<settings.hybridDistance)) &&
             debugRevision!=revision &&
             builtRevision==revision)
    {
        const auto snapshot=settings;
        pendingCancellation=std::make_shared<std::atomic_bool>(false);
        const auto cancellation=pendingCancellation;
        pendingRevision=revision;pendingDebug=true;
        pending=std::async(std::launch::async,[snapshot,cancellation]
            {return Web::build(snapshot,true,cancellation.get());});
    }
}
void WebLabUI::frame(Renderer &renderer,unsigned width,unsigned height)
{
    constexpr float top=54,bottom=112,left=315,right=345;
    bool changedAny=false,generateNow=false;
    panel("Spider Web Surface - authoring",0,top,left,float(height)-top-bottom);
    int authoring=int(settings.authoring);
    if (ImGui::Combo("Create",&authoring,"Plane Web\0Anchor Web\0Area Web\0"))
    {settings.authoring=Web::Authoring(authoring);changedAny=true;}
    if (settings.authoring==Web::Authoring::Plane)
    {
        changedAny|=ImGui::InputDouble("Width (m)",&settings.widthMeters,.1,.5,"%.3f");
        changedAny|=ImGui::InputDouble("Height (m)",&settings.heightMeters,.1,.5,"%.3f");
        changedAny|=editPoint("Center XYZ",settings.center);
        changedAny|=editPoint("Axis U",settings.axisU);
        changedAny|=editPoint("Axis V",settings.axisV);
    }
    else
    {
        auto &points=settings.authoring==Web::Authoring::Anchors?
            settings.anchors:settings.area;
        ImGui::TextUnformatted(settings.authoring==Web::Authoring::Anchors?
            "Place at least 3 anchors":"Mark polygon area (3-32 points)");
        if (ImGui::Button(placePoint?"Cancel placement":"Place point in viewport"))
            placePoint=!placePoint;
        ImGui::SameLine();
        if (ImGui::Button("Remove point") && points.size()>3)
        {points.erase(points.begin()+selectedPoint);
         selectedPoint=std::min(selectedPoint,int(points.size()-1));changedAny=true;}
        if (!points.empty())
        {
            selectedPoint=std::clamp(selectedPoint,0,int(points.size()-1));
            if (ImGui::BeginChild("Anchor list",{0,135},true))
                for (std::size_t i=0;i<points.size();++i)
                {
                    const auto label="Anchor "+std::to_string(i);
                    if (ImGui::Selectable(label.c_str(),selectedPoint==int(i)))
                        selectedPoint=int(i);
                }
            ImGui::EndChild();
            changedAny|=editPoint("Anchor XYZ",points[selectedPoint]);
        }
        ImGui::TextUnformatted("Click a marker to select; drag to move it on the plane.");
        changedAny|=editPoint("Placement center",settings.center);
        changedAny|=editPoint("Placement U",settings.axisU);
        changedAny|=editPoint("Placement V",settings.axisV);
    }
    if (ImGui::Button("Generate")) {changedAny=true;generateNow=true;}
    ImGui::SameLine();
    if (ImGui::Button("Reset"))
    {settings=Web::Settings{};applyStyle(settings,Web::Style::Ghost);
     settings.anchors={{-1.5,-1.5,0},{1.5,-1.5,0},
        {1.5,1.5,0},{-1.5,1.5,0}};settings.area=settings.anchors;
     selectedEdge.clear();changedAny=true;fitCamera=true;}
    ImGui::Separator();
    int preset=int(settings.preset);
    if (ImGui::Combo("Preset",&preset,presets,int(std::size(presets))))
    {settings.preset=Web::Preset(preset);changedAny=true;}
    int style=int(settings.style);
    if (ImGui::Combo("Web Field style",&style,"Heavy\0Medium\0Ghost\0"))
    {
        applyStyle(settings,Web::Style(style));
        changedAny=true;
    }
    int seed=int(settings.seed);
    if (ImGui::InputInt("Seed",&seed))
    {settings.seed=std::uint32_t(std::max(0,seed));changedAny=true;}
    int spokes=int(settings.spokes),rings=int(settings.rings);
    if (ImGui::SliderInt("Spokes",&spokes,4,64))
    {settings.spokes=unsigned(spokes);changedAny=true;}
    if (ImGui::SliderInt("Rings",&rings,1,12))
    {settings.rings=unsigned(rings);changedAny=true;}
    ImGui::End();

    panel("Spider Web Surface - render and damage",float(width)-right,top,
          right,float(height)-top-bottom);
    auto &appearance=settings.optics;
    ImGui::TextUnformatted("Visual recipe / static surface");
    auto slider=[&](const char *label,double &value,double low,double high)
    {
        float current=float(value);
        if (ImGui::SliderFloat(label,&current,float(low),float(high),"%.3f"))
        {value=current;changedAny=true;}
    };
    slider("Density",settings.density,0,1);
    slider("Microstrand density",settings.microStrandDensity,0,1);
    slider("Secondary strands",settings.secondaryStrands,0,1);
    slider("Irregularity",settings.irregularity,0,.4);
    slider("Missing %",settings.missingFraction,0,1);
    slider("Radius (m)",settings.radiusMeters,.001,.05);
    slider("Sag (m)",settings.sagMeters,0,.25);
    slider("Hub U",settings.hubOffsetU,-.2,.2);
    slider("Hub V",settings.hubOffsetV,-.2,.2);
    slider("Normal strength",settings.normalStrength,0,1);
    int structural=int(settings.structuralStrandCount);
    if (ImGui::SliderInt("Structural strands",&structural,0,64))
    {settings.structuralStrandCount=unsigned(structural);changedAny=true;}
    ImGui::SliderFloat("Roughness",&appearance.roughness,0,1);
    ImGui::SliderFloat("Brightness",&appearance.brightness,0,2);
    ImGui::SliderFloat("Wetness",&appearance.wetness,0,1);
    ImGui::SliderFloat("Frost",&appearance.frost,0,1);
    ImGui::SliderFloat("Dust",&appearance.dust,0,1);
    ImGui::SliderFloat("Distance softness",&appearance.distanceSoftness,.25,2);
    ImGui::SliderFloat("Base visibility",&appearance.baseVisibility,0,1);
    ImGui::SliderFloat("Specular response",&appearance.specularResponse,0,2);
    ImGui::SliderFloat("Light reveal",&appearance.lightRevealStrength,0,2);
    ImGui::SliderFloat("View angle response",&appearance.viewAngleResponse,0,2);
    ImGui::SliderFloat("Droplet density",&appearance.dropletDensity,0,1);
    ImGui::SliderFloat("Droplet size min",&appearance.dropletSizeMin,.002,.05);
    ImGui::SliderFloat("Droplet size max",&appearance.dropletSizeMax,.002,.05);
    int dropletSeed=int(appearance.dropletSeed);
    if (ImGui::InputInt("Droplet seed",&dropletSeed))
        appearance.dropletSeed=unsigned(std::max(0,dropletSeed));
    ImGui::InputFloat3("Light XYZ",appearance.lightPosition.data());
    ImGui::SliderFloat("Light reach (m)",&appearance.lightReach,.1,20);
    ImGui::Separator();
    ImGui::Checkbox("Adaptive distance LOD",&autoLod);
    slider("Geometry distance",settings.geometryDistance,0,20);
    slider("Hybrid distance",settings.hybridDistance,0,40);
    slider("Shader only distance",settings.shaderOnlyDistance,0,80);
    slider("Cull distance",settings.cullDistance,0,120);
    settings.hybridDistance=std::max(settings.hybridDistance,settings.geometryDistance);
    settings.shaderOnlyDistance=std::max(settings.shaderOnlyDistance,settings.hybridDistance);
    settings.cullDistance=std::max(settings.cullDistance,settings.shaderOnlyDistance+.1);
    appearance.shaderOnlyDistance=float(settings.shaderOnlyDistance);
    appearance.cullDistance=float(settings.cullDistance);
    appearance.dropletSizeMax=std::max(appearance.dropletSizeMax,appearance.dropletSizeMin);
    renderer.setWebAppearance(appearance);
    int resolution=settings.resolution==64?0:settings.resolution==128?1:
        settings.resolution==256?2:3;
    if (ImGui::Combo("Texture resolution",&resolution,"64\0 128\0 256\0 512\0"))
    {settings.resolution=64u<<resolution;changedAny=true;}
    int samples=settings.supersamples==1?0:settings.supersamples==2?1:
        settings.supersamples==4?2:3;
    if (ImGui::Combo("Supersampling",&samples,"1x\0 2x2\0 4x4\0 8x8\0"))
    {settings.supersamples=1u<<samples;changedAny=true;}
    if (ImGui::Checkbox("Averaged coverage mips",&averagedMips)) uploadedView=-1;
    ImGui::Separator();
    ImGui::Text("Edges: %zu | active: %zu",built.graph.edges.size(),built.activeEdgeCount);
    if (ImGui::BeginChild("Web edges",{0,140},true))
        for (const auto &edge:built.graph.edges)
            if (ImGui::Selectable(edge.id.c_str(),selectedEdge==edge.id))
                selectedEdge=edge.id;
    ImGui::EndChild();
    if (!selectedEdge.empty())
    {
        ImGui::TextWrapped("%s",selectedEdge.c_str());
        if (ImGui::Button("Break Edge"))
        {settings.brokenEdges.insert(selectedEdge);changedAny=true;
         breakStarted=std::chrono::steady_clock::now();breakLatencyMs=-1;}
        ImGui::SameLine();
        if (ImGui::Button("Restore Edge"))
        {settings.brokenEdges.erase(selectedEdge);changedAny=true;
         breakStarted=std::chrono::steady_clock::now();breakLatencyMs=-1;}
    }
    int physicalBudget=int(settings.physicalEdgeBudget);
    if (ImGui::SliderInt("Physical edge budget",&physicalBudget,0,50))
    {settings.physicalEdgeBudget=unsigned(physicalBudget);changedAny=true;}
    slider("Gravity influence",settings.gravityInfluence,0,4);
    ImGui::Checkbox("Simulate coarse Jolt network",&simulatePhysics);
    ImGui::SameLine();
    if (ImGui::Button("Reset network")) physicalRevision=~0u;
    ImGui::TextUnformatted("Fixed 60 Hz; authored points remain pinned.");
    ImGui::End();

    if (changedAny) changed();
    if (generateNow) edited=std::chrono::steady_clock::now()-std::chrono::seconds(1);
    process();
    if (builtRevision==revision && physicalRevision!=revision)
    {
        try
        {physics.reset(settings,built);physicalRevision=revision;
         physicsTick=std::chrono::steady_clock::now();physicsAccumulator=0;}
        catch (const std::exception &exception)
        {error=exception.what();physicalRevision=revision;simulatePhysics=false;}
    }
    if (physicalRevision==revision && simulatePhysics &&
        !physics.network().edges.empty())
    {
        const auto now=std::chrono::steady_clock::now();
        physicsAccumulator+=std::min(.05,std::chrono::duration<double>(
            now-physicsTick).count());
        physicsTick=now;
        try
        {
            for (unsigned step=0;step<3 && physicsAccumulator>=1.0/60;++step)
            {physics.step();physicsAccumulator-=1.0/60;}
        }
        catch (const std::exception &exception)
        {error=exception.what();simulatePhysics=false;}
    }
    else physicsTick=std::chrono::steady_clock::now();
    panel("Spider Web Surface viewport",left,top,
          float(width)-left-right,float(height)-top-bottom);
    ImGui::Combo("View",&view,views,int(std::size(views)));
    ImGui::SameLine();
    if (ImGui::Button("Fit")) {fitCamera=true;changed();}
    const int displayView=autoLod && camera.distance>=settings.cullDistance?10:
        autoLod && settings.style==Web::Style::Ghost?0:
        autoLod && camera.distance<settings.geometryDistance &&
            settings.style==Web::Style::Heavy?6:
        autoLod && camera.distance<settings.hybridDistance?9:view;
    Vec3d displacement{};
    if (displayView==9 && physicalRevision==revision &&
        !physics.network().positions.empty() && !built.graph.nodes.empty())
        displacement=sub(physics.network().positions.front(),built.graph.nodes.front());
    renderer.setWebDisplacement(displacement);
    if (builtRevision==revision && !built.graph.edges.empty() &&
        uploadedView!=displayView)
    {
        if (built.activeEdgeCount==0 || displayView==10)
        {renderer.clearMesh();uploadedView=displayView;}
        else if (displayView==9 && built.cardAccepted && debugRevision==revision)
            renderer.uploadWeb(built,averagedMips,&debug);
        else if (displayView<5 && built.cardAccepted)
            renderer.uploadWeb(built,averagedMips);
        else if (displayView<5 && !built.ribbons.mesh.indices.empty())
            renderer.upload(built.ribbons,nullptr,nullptr,nullptr,false,&built.ribbonUV);
        else if (debugRevision==revision)
        {
            const auto &mesh=displayView==5?debug.ribbons:
                displayView==6?debug.source:debug.coarsePhysical;
            const auto &uv=displayView==5?debug.ribbonUV:
                displayView==6?debug.sourceUV:debug.coarsePhysicalUV;
            renderer.upload(mesh,nullptr,nullptr,nullptr,false,&uv);
        }
        if (built.activeEdgeCount==0 || displayView==10 ||
            (displayView<5 && (built.cardAccepted || !built.ribbons.mesh.indices.empty())) ||
            debugRevision==revision) uploadedView=displayView;
    }
    if (displayView==7 && physicalRevision==revision &&
        !physics.network().edges.empty())
    {
        const auto mesh=Web::makePhysicalMesh(physics.network(),
            cross(settings.axisU,settings.axisV));
        if (!mesh.mesh.indices.empty())
        {renderer.upload(mesh);uploadedView=displayView;}
    }
    auto size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,32.0f);size.y=std::max(size.y,32.0f);
    constexpr int modes[]={11,12,13,15,14,0,0,0,0,11,0};
    const auto begin=std::chrono::steady_clock::now();
    auto texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,
        built.cardAccepted?modes[displayView]:0);
    if (breakStarted && builtRevision==revision && uploadedView==displayView)
    {
        breakLatencyMs=std::chrono::duration<double,std::milli>(
            std::chrono::steady_clock::now()-*breakStarted).count();
        breakStarted.reset();
    }
    drawCpuMs=std::chrono::duration<double,std::milli>(
        std::chrono::steady_clock::now()-begin).count();
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);
    const auto imageOrigin=ImGui::GetItemRectMin();
    if (settings.authoring!=Web::Authoring::Plane)
    {
        auto &points=settings.authoring==Web::Authoring::Anchors?
            settings.anchors:settings.area;
        const auto mouse=ImGui::GetIO().MousePos;
        const auto drawable=ImGui::GetWindowDrawList();
        for (std::size_t i=0;i<points.size();++i)
        {
            const auto at=pointScreen(points[i],camera,imageOrigin,size);
            drawable->AddCircleFilled(at,selectedPoint==int(i)?7:5,
                selectedPoint==int(i)?IM_COL32(255,190,48,255):IM_COL32(72,220,255,255));
            drawable->AddCircle(at,selectedPoint==int(i)?7:5,IM_COL32(10,15,20,255));
        }
        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
        {
            int nearest=-1;
            float distanceSquared=144;
            for (std::size_t i=0;i<points.size();++i)
            {
                const auto at=pointScreen(points[i],camera,imageOrigin,size);
                const auto dx=at.x-mouse.x,dy=at.y-mouse.y;
                if (dx*dx+dy*dy<distanceSquared)
                {nearest=int(i);distanceSquared=dx*dx+dy*dy;}
            }
            if (nearest>=0) {selectedPoint=nearest;dragPoint=true;placePoint=false;}
            else if (placePoint && points.size()<32)
            {
                Vec3d point;
                if (onPlacementPlane(mouse,camera,imageOrigin,size,settings,point))
                {points.push_back(point);selectedPoint=int(points.size()-1);
                 changed();placePoint=false;}
            }
        }
        if (dragPoint && ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            Vec3d point;
            if (onPlacementPlane(mouse,camera,imageOrigin,size,settings,point))
            {points[selectedPoint]=point;changed();}
        }
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) dragPoint=false;
    }
    if (ImGui::IsItemHovered())
    {
        auto &io=ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !dragPoint && !placePoint)
        {camera.yaw-=io.MouseDelta.x*.008f;
         camera.pitch=std::clamp(camera.pitch+io.MouseDelta.y*.008f,-1.4f,1.4f);}
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) ||
            ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            const float dx=-io.MouseDelta.x*camera.distance*.0018f;
            const float dy=io.MouseDelta.y*camera.distance*.0018f;
            const float cy=std::cos(camera.yaw),sy=std::sin(camera.yaw);
            const float cp=std::cos(camera.pitch),sp=std::sin(camera.pitch);
            camera.target[0]+=dx*cy-dy*sp*sy;camera.target[1]+=dy*cp;
            camera.target[2]+=-dx*sy-dy*sp*cy;
        }
        camera.distance=std::clamp(camera.distance*std::exp(-io.MouseWheel*.1f),1.0f,90.0f);
    }
    ImGui::End();
    panel("Web surface metrics",0,float(height)-bottom,float(width),bottom);
    if (!error.empty()) ImGui::TextColored({1,.35f,.3f,1},"Diagnostic: %s",error.c_str());
    else if (!built.cardAccepted && !built.fallbackReason.empty())
        ImGui::Text("Ribbon fallback: %s",built.fallbackReason.c_str());
    else ImGui::TextUnformatted(pending.valid()?"Regenerating baked texture...":"Ready");
    ImGui::Text("Nodes %zu | Edges %zu | Active %zu | Cards %zu | Card triangles %zu | Geometry debug triangles %zu",
        built.graph.nodes.size(),built.graph.edges.size(),built.activeEdgeCount,
        built.patches.size(),built.card.mesh.indices.size()/3,debug.source.mesh.indices.size()/3);
    if (built.bakeBudgetReduced)
        ImGui::Text("CPU bake budget: %u -> %u px/patch, %u -> %u samples/axis",
            settings.resolution,built.bakeResolution,
            settings.supersamples,built.bakeSupersamples);
    if (debug.sourceTruncated)
        ImGui::Text("3D debug cap: %zu of %zu visible strands",
            debug.sourceRenderedEdges,debug.activeEdgeCount);
    ImGui::Text("Bake CPU %.1f ms | draw CPU %.2f ms | FPS %.1f | GPU time unavailable | coverage %.1f%% | empty %.1f%%",
        built.bakeCpuMilliseconds,drawCpuMs,ImGui::GetIO().Framerate,
        built.mips.empty()?0:100*built.mips.front().occupiedFraction,
        built.mips.empty()?0:100*(1-built.mips.front().occupiedFraction));
    if (breakLatencyMs>=0)
        ImGui::Text("Last Break/Restore bake + upload + draw CPU latency %.1f ms",
            breakLatencyMs);
    ImGui::Text("Texture %u x %u | all maps + mips %.2f MiB | adapter %s | Jolt nodes %zu / edges %zu / step %.3f ms",
        built.mips.empty()?0:built.mips.front().width,
        built.mips.empty()?0:built.mips.front().height,
        built.textureBytes/1048576.0,renderer.adapter().c_str(),
        physics.network().positions.size(),physics.network().edges.size(),
        physics.network().stepCpuMilliseconds);
    ImGui::End();
}
}
