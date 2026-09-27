#include "AssemblyLabUI.hpp"
#include "SolidGeometry.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>

namespace Playground
{
namespace
{
constexpr const char *presets[]={"Pipe + Couplings","Railway","Broken Railway",
    "Metal Railing","Broken Railing","Wooden Pier","Damaged Wooden Pier",
    "Catwalk","Beam Bridge","Warren Truss","Pratt Truss","Howe Truss",
    "Suspension Bridge","Rope / Plank Bridge"};
constexpr const char *views[]={"Lit","Wireframe","Normals","UV Checker",
    "Semantic Roles","RMF Frames"};
void panel(const char *title,float x,float y,float width,float height)
{
    ImGui::SetNextWindowPos({x,y});
    ImGui::SetNextWindowSize({width,height});
    ImGui::Begin(title,nullptr,
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
}
}
AssemblyLabUI::AssemblyLabUI()
{
    spline.splineGuid=Solid::region(2301,1);
    spline.name="Procedural assembly path";
    spline.interpolation=PA::SplineInterpolation::CatmullRom;
    const Vec3d positions[]={{-3,0,0},{-1,0,.10},{1,0,-.10},{3,0,0}};
    for (unsigned i=0;i<4;++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid=Solid::region(2301,i+2);
        point.position=positions[i];
        point.incomingHandle={-.4,0,0};
        point.outgoingHandle={.4,0,0};
        spline.points.push_back(point);
    }
    camera.yaw=.4f;camera.pitch=-.35f;camera.distance=12;
}
void AssemblyLabUI::choosePreset(int preset)
{
    settings=Assembly::Settings{};
    settings.kind=Assembly::Kind(preset);
    if (settings.kind==Assembly::Kind::PipeCouplings)
    {settings.couplingSpacingMeters=.8;settings.pipeRadiusMeters=.09;}
    if (settings.kind==Assembly::Kind::RopePlankBridge)
    {settings.widthMeters=1.4;settings.sagMeters=1.0;settings.boardWidthMeters=.22;}
    if (settings.kind==Assembly::Kind::SuspensionBridge)
    {settings.spanMeters=12;settings.hangerSpacingMeters=.8;settings.towerHeightMeters=3.5;}
    selectedId.clear();dirty=true;fitCamera=true;
}
void AssemblyLabUI::activate(Renderer &renderer)
{dirty=true;uploadedView=-1;rebuild();}
void AssemblyLabUI::rebuild()
{
    if (!dirty) return;
    try
    {
        auto next=Assembly::build(spline,settings);
        built=std::move(next);
        error.clear();uploadedView=-1;
        if (fitCamera && !built.mesh.mesh.positions.empty())
        {
            auto lo=built.mesh.mesh.positions.front(),hi=lo;
            for (auto p:built.mesh.mesh.positions)
            {lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
             hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
            camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
            camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.5f,
                                       2.0f,90.0f);
            fitCamera=false;
        }
    }
    catch (const std::exception &exception) {error=exception.what();}
    dirty=false;
}
const Assembly::Part *AssemblyLabUI::selectedPart() const
{
    for (const auto &part:built.parts)
        if (part.id==selectedId) return &part;
    return nullptr;
}
void AssemblyLabUI::frame(Renderer &renderer,unsigned width,unsigned height)
{
    constexpr float top=54,bottom=112,left=315,right=390;
    panel("Assembly path and presets",0,top,left,float(height)-top-bottom);
    ImGui::TextUnformatted("Reusable path + semantic parts");
    const auto preset=int(settings.kind);
    if (ImGui::BeginCombo("Preset",presets[preset]))
    {
        for (int i=0;i<int(std::size(presets));++i)
            if (ImGui::Selectable(presets[i],i==preset)) choosePreset(i);
        ImGui::EndCombo();
    }
    ImGui::Separator();
    int interpolation=int(spline.interpolation);
    if (ImGui::Combo("Path",&interpolation,"Polyline\0Catmull-Rom\0Cubic Bezier\0"))
    {spline.interpolation=PA::SplineInterpolation(interpolation);dirty=true;}
    if (ImGui::Button("Add point") && spline.points.size()<256)
    {
        auto point=spline.points.back();
        const auto previous=spline.points[spline.points.size()-2];
        point.pointGuid=Solid::region(2301,nextPointId++);
        point.position={2*point.position.x-previous.position.x,
                        2*point.position.y-previous.position.y,
                        2*point.position.z-previous.position.z};
        spline.points.push_back(point);selectedPoint=int(spline.points.size()-1);dirty=true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove point") && spline.points.size()>2)
    {spline.points.erase(spline.points.begin()+selectedPoint);
     selectedPoint=std::min(selectedPoint,int(spline.points.size()-1));dirty=true;}
    for (std::size_t i=0;i<spline.points.size();++i)
    {
        char label[32];std::snprintf(label,sizeof(label),"Point %zu",i);
        if (ImGui::Selectable(label,selectedPoint==int(i))) selectedPoint=int(i);
    }
    if (!spline.points.empty())
    {
        auto &point=spline.points[selectedPoint];
        double xyz[]={point.position.x,point.position.y,point.position.z};
        if (ImGui::InputScalarN("XYZ (m)",ImGuiDataType_Double,xyz,3,nullptr,nullptr,"%.3f"))
        {point.position={xyz[0],xyz[1],xyz[2]};dirty=true;}
        double in[]={point.incomingHandle.x,point.incomingHandle.y,point.incomingHandle.z};
        if (ImGui::InputScalarN("Bezier in",ImGuiDataType_Double,in,3,nullptr,nullptr,"%.3f"))
        {point.incomingHandle={in[0],in[1],in[2]};dirty=true;}
        double out[]={point.outgoingHandle.x,point.outgoingHandle.y,point.outgoingHandle.z};
        if (ImGui::InputScalarN("Bezier out",ImGuiDataType_Double,out,3,nullptr,nullptr,"%.3f"))
        {point.outgoingHandle={out[0],out[1],out[2]};dirty=true;}
        dirty|=ImGui::InputDouble("Roll (rad)",&point.rollRadians,.05,.25,"%.3f");
    }
    ImGui::Separator();ImGui::TextUnformatted("Construction quality");
    dirty|=ImGui::InputDouble("Path step (m)",&settings.quality.maximumStepMeters,.02,.1,"%.3f");
    dirty|=ImGui::InputDouble("Path chord error (m)",&settings.quality.maximumDeviationMeters,.001,.01,"%.4f");
    dirty|=ImGui::InputDouble("Path angle (rad)",&settings.quality.maximumTangentAngleRadians,.01,.1,"%.3f");
    int lod=settings.lod;
    if (ImGui::Combo("LOD",&lod,"LOD 0\0LOD 1\0LOD 2\0"))
    {settings.lod=lod;dirty=true;}
    ImGui::End();

    panel("Assembly controls and parts",float(width)-right,top,right,float(height)-top-bottom);
    ImGui::PushItemWidth(145);
    auto edit=[&](const char *label,double &value,double step=.05)
    {dirty|=ImGui::InputDouble(label,&value,step,step*10,"%.3f");};
    edit("Width (m)",settings.widthMeters);
    edit("Height (m)",settings.heightMeters);
    if (settings.kind==Assembly::Kind::PipeCouplings)
    {
        edit("Pipe radius",settings.pipeRadiusMeters,.005);
        dirty|=ImGui::Checkbox("Couplings",&settings.couplingsEnabled);
        edit("Coupling spacing",settings.couplingSpacingMeters);
        edit("Coupling width",settings.couplingWidthMeters,.01);
        edit("Radial thickness",settings.couplingRadialThicknessMeters,.005);
        edit("Start padding",settings.couplingStartPaddingMeters);
        edit("End padding",settings.couplingEndPaddingMeters);
        edit("Radius variation",settings.couplingVariationMeters,.002);
        int variationSeed=int(settings.variationSeed);
        if (ImGui::InputInt("Variation seed",&variationSeed))
        {settings.variationSeed=std::uint32_t(std::max(0,variationSeed));dirty=true;}
    }
    else if (settings.kind==Assembly::Kind::Railway ||
             settings.kind==Assembly::Kind::BrokenRailway)
    {
        edit("Gauge",settings.gaugeMeters);
        edit("Sleeper spacing",settings.sleeperSpacingMeters);
        edit("Sleeper length",settings.sleeperLengthMeters);
        edit("Sleeper width",settings.sleeperWidthMeters,.01);
        edit("Sleeper height",settings.sleeperHeightMeters,.01);
        edit("Rail width",settings.railWidthMeters,.005);
        edit("Rail height",settings.railHeightMeters,.005);
        dirty|=ImGui::Checkbox("Rail joints",&settings.railJointsEnabled);
    }
    else
    {
        edit("Board width",settings.boardWidthMeters,.01);
        edit("Board gap",settings.boardGapMeters,.005);
        edit("Board thickness",settings.boardThicknessMeters,.005);
        edit("Beam width",settings.beamWidthMeters,.01);
        edit("Beam height",settings.beamHeightMeters,.01);
        edit("Support spacing",settings.supportSpacingMeters);
        edit("Bay spacing",settings.baySpacingMeters);
        if (settings.kind==Assembly::Kind::Railing ||
            settings.kind==Assembly::Kind::BrokenRailing ||
            settings.kind==Assembly::Kind::Catwalk)
            dirty|=ImGui::Checkbox("Lower rail",&settings.lowerRailEnabled);
        if (int(settings.kind)>=int(Assembly::Kind::BeamBridge))
        {
            edit("Span",settings.spanMeters,.5);
            edit("Sag",settings.sagMeters,.1);
            edit("Tower height",settings.towerHeightMeters,.1);
            edit("Hanger spacing",settings.hangerSpacingMeters,.1);
            edit("Cable radius",settings.cableRadiusMeters,.005);
            edit("Plank irregularity",settings.irregularityMeters,.005);
        }
    }
    ImGui::Separator();ImGui::TextUnformatted("Procedural damage");
    dirty|=ImGui::Checkbox("Enable damage",&settings.damage.proceduralEnabled);
    int seed=int(settings.damage.seed);
    if (ImGui::InputInt("Seed",&seed))
    {settings.damage.seed=std::uint32_t(std::max(0,seed));dirty=true;}
    edit("Missing %",settings.damage.missingProbability,.01);
    edit("Broken %",settings.damage.brokenProbability,.01);
    edit("Loose %",settings.damage.looseProbability,.01);
    edit("Bent %",settings.damage.bentProbability,.01);
    edit("Cluster center",settings.damage.clusterCenterMeters,.1);
    edit("Cluster radius",settings.damage.clusterRadiusMeters,.1);
    edit("Cluster strength",settings.damage.clusterStrength,.05);
    ImGui::Separator();ImGui::Text("Semantic parts: %zu",built.parts.size());
    if (ImGui::BeginChild("Part list",{0,145},true))
    {
        for (const auto &part:built.parts)
        {
            const auto label=part.id+"  ["+Assembly::name(part.state)+"]";
            if (ImGui::Selectable(label.c_str(),part.id==selectedId)) selectedId=part.id;
        }
    }
    ImGui::EndChild();
    if (const auto *part=selectedPart())
    {
        ImGui::TextWrapped("%s | %s | station %.3f m",part->id.c_str(),
                           Assembly::name(part->role),part->stationMeters);
        if (ImGui::BeginCombo("State",Assembly::name(part->state)))
        {
            for (int i=0;i<=int(Assembly::State::Collapsed);++i)
            {
                auto state=Assembly::State(i);
                if (Assembly::stateAllowed(part->role,state) &&
                    ImGui::Selectable(Assembly::name(state),part->state==state))
                {settings.damage.manual[part->id]=state;dirty=true;}
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Reset manual state"))
        {settings.damage.manual.erase(part->id);dirty=true;}
        ImGui::SameLine();
        bool locked=settings.damage.protectedIds.contains(part->id);
        if (ImGui::Checkbox("Lock",&locked))
        {if (locked) settings.damage.protectedIds.insert(part->id);
         else settings.damage.protectedIds.erase(part->id);dirty=true;}
        ImGui::Text("Triangles %zu | LOD %d | Physical bodies %zu / constraints %zu",
            part->triangleCount,settings.lod,built.promotedBodyCount,built.constraintCount);
    }
    ImGui::PopItemWidth();ImGui::End();

    rebuild();
    panel("Procedural Assemblies viewport",left,top,
          float(width)-left-right,float(height)-top-bottom);
    ImGui::Combo("View",&view,views,int(std::size(views)));
    if (ImGui::Button("Fit")) {fitCamera=true;dirty=true;rebuild();}
    if (view!=uploadedView)
    {
        if (view==5)
        {
            Construction::ConstructionResult axes;
            axes.path=built.path;axes.mesh=built.mesh;axes.cornerUV=built.cornerUV;
            auto debug=Construction::withFrameAxes(axes);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
        uploadedView=view;
    }
    ImGui::SameLine();ImGui::TextUnformatted("Click a visible member to select it");
    auto size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,32.0f);size.y=std::max(size.y,32.0f);
    constexpr int modes[]={0,2,1,8,3,10};
    auto texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,modes[view]);
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);
    if (ImGui::IsItemHovered())
    {
        auto &io=ImGui::GetIO();
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && view!=5)
        {
            const auto minimum=ImGui::GetItemRectMin();
            const auto x=unsigned(std::clamp(io.MousePos.x-minimum.x,0.0f,size.x-1));
            const auto y=unsigned(std::clamp(io.MousePos.y-minimum.y,0.0f,size.y-1));
            if (const auto triangle=renderer.pickTriangle(unsigned(size.x),unsigned(size.y),camera,x,y))
                if (*triangle<built.mesh.mesh.triangleRegions.size())
                    for (const auto &part:built.parts)
                        if (part.guid==built.mesh.mesh.triangleRegions[*triangle])
                        {selectedId=part.id;break;}
            renderer.draw(unsigned(size.x),unsigned(size.y),camera,modes[view]);
        }
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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
    panel("Assembly metrics",0,float(height)-bottom,float(width),bottom);
    if (!error.empty())
        ImGui::TextColored({1,.35f,.3f,1},"Diagnostic: %s | Previous valid mesh retained",error.c_str());
    else ImGui::TextUnformatted("Ready | Static geometry; no physics bodies promoted");
    ImGui::Text("CPU %.2f ms | Path %.3f m | Vertices %zu | Triangles %zu | Parts %zu | Repeat stations %zu | LOD %d",
        built.generationCpuMilliseconds,built.path.lengthMeters,built.mesh.mesh.positions.size(),
        built.mesh.mesh.indices.size()/3,built.parts.size(),built.repeatedStations,settings.lod);
    ImGui::Text("Manual overrides %zu | Protected %zu | Bodies %zu | Constraints %zu | GPU %.2f MiB | %s",
        settings.damage.manual.size(),settings.damage.protectedIds.size(),
        built.promotedBodyCount,built.constraintCount,renderer.bufferBytes()/1048576.0,
        renderer.adapter().c_str());
    ImGui::End();
}
}
