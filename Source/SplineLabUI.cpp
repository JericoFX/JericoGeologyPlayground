#include "SplineLabUI.hpp"
#include "SolidGeometry.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <numbers>

namespace Playground
{
namespace
{
constexpr const char *presetNames[] = {
    "Pipe 25 mm", "Pipe 80 mm", "Hollow Pipe", "Cable", "Hose",
    "Round Duct", "Rectangular Duct", "Cable Tray", "Handrail", "I-Beam Run",
    "Corrugated Hose", "Fractured Pipe", "Twisted Duct"};
constexpr const char *profileNames[] = {
    "Circle", "Ring / hollow", "Rectangle", "Rounded Rectangle", "U / Channel", "I-Beam"};
void panel(const char *name, float x, float y, float width, float height)
{
    ImGui::SetNextWindowPos({x,y});
    ImGui::SetNextWindowSize({width,height});
    ImGui::Begin(name, nullptr,
                 ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
}
}
Construction::ConstructionSettings splinePreset(int index)
{
    Construction::ConstructionSettings settings;
    settings.profile=Construction::ProfileKind::Circle;
    settings.radiusMeters=.04;
    settings.widthMeters=.12;
    settings.heightMeters=.12;
    settings.wallThicknessMeters=.008;
    settings.cornerRadiusMeters=.012;
    settings.repeatEnabled=false;
    switch(index)
    {
    case 0: settings.radiusMeters=.0125; break;
    case 1: settings.radiusMeters=.04; break;
    case 2: settings.profile=Construction::ProfileKind::Ring; settings.radiusMeters=.055; settings.wallThicknessMeters=.007; break;
    case 3: settings.radiusMeters=.009; break;
    case 4: settings.radiusMeters=.018; break;
    case 5: settings.profile=Construction::ProfileKind::Ring; settings.radiusMeters=.11; settings.wallThicknessMeters=.012; break;
    case 6: settings.profile=Construction::ProfileKind::Rectangle; settings.widthMeters=.26; settings.heightMeters=.16; break;
    case 7: settings.profile=Construction::ProfileKind::Channel; settings.widthMeters=.36; settings.heightMeters=.10; settings.wallThicknessMeters=.012;
            settings.repeatEnabled=true; settings.repeat.spacingMeters=.35; break;
    case 8: settings.profile=Construction::ProfileKind::RoundedRectangle; settings.widthMeters=.08; settings.heightMeters=.08;
            settings.cornerRadiusMeters=.025; settings.repeatEnabled=true; settings.repeat.spacingMeters=.8; break;
    case 9: settings.profile=Construction::ProfileKind::IBeam; settings.widthMeters=.22; settings.heightMeters=.3;
            settings.wallThicknessMeters=.025; break;
    case 10: settings.radiusMeters=.022;settings.ribAmplitude=.16;
             settings.ribSpacingMeters=.16;break;
    case 11: settings.profile=Construction::ProfileKind::Ring;
             settings.radiusMeters=.055;settings.wallThicknessMeters=.008;
             settings.breakEnabled=true;settings.breakGapMeters=.18;break;
    case 12: settings.profile=Construction::ProfileKind::Rectangle;
             settings.widthMeters=.22;settings.heightMeters=.13;
             settings.twistTurns=.4;settings.taperLengthMeters=.3;
             settings.tipScale=.7;break;
    }
    return settings;
}
SplineLabUI::SplineLabUI()
{
    spline.splineGuid = Solid::region(1901,1);
    spline.name = "Spline Construction";
    spline.interpolation = PA::SplineInterpolation::CatmullRom;
    const Vec3d points[] = {{-1.5,0,-1},{-0.5,0.5,0},{0.5,-0.3,0.6},{1.5,0,1.5}};
    for (unsigned i=0;i<4;++i)
    {
        PA::SplineControlPoint point;
        point.pointGuid = Solid::region(1901,i+2);
        point.position = points[i];
        point.incomingHandle = {-0.25,0,0};
        point.outgoingHandle = {0.25,0,0};
        spline.points.push_back(point);
    }
    choosePreset(1);
    camera.yaw=0.55f;
    camera.pitch=-0.25f;
    camera.distance=6;
}
void SplineLabUI::choosePreset(int index)
{
    preset=index;
    settings=splinePreset(index);
    dirty=true;
}
void SplineLabUI::activate(Renderer &renderer)
{
    dirty=true;
    uploadedView=-1;
    rebuild(renderer);
}
void SplineLabUI::rebuild(Renderer &renderer)
{
    if (!dirty) return;
    try
    {
        auto next=Construction::buildConstruction(spline,settings);
        built=std::move(next);
        uploadedView=-1;
        error.clear();
        if (fitCamera && !built.mesh.mesh.positions.empty())
        {
            auto lo=built.mesh.mesh.positions.front(), hi=lo;
            for (const auto &p:built.mesh.mesh.positions)
            {
                lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};
                hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};
            }
            camera.target={float((lo.x+hi.x)*.5),float((lo.y+hi.y)*.5),float((lo.z+hi.z)*.5)};
            camera.distance=std::clamp(float(std::hypot(hi.x-lo.x,hi.y-lo.y,hi.z-lo.z))*1.4f,1.0f,90.0f);
            fitCamera=false;
        }
    }
    catch (const std::exception &e) {error=e.what();}
    dirty=false;
}
void SplineLabUI::frame(Renderer &renderer, unsigned width, unsigned height)
{
    const float top=54, bottom=112, left=300, right=340;
    panel("Spline path and presets",0,top,left,float(height)-top-bottom);
    ImGui::TextUnformatted("One path, many constructions");
    ImGui::Separator();
    if (ImGui::BeginCombo("Preset",presetNames[preset]))
    {
        for (int i=0;i<int(std::size(presetNames));++i)
            if (ImGui::Selectable(presetNames[i],preset==i)) choosePreset(i);
        ImGui::EndCombo();
    }
    if (ImGui::Button("Pipe 80 mm")) choosePreset(1);
    ImGui::SameLine();
    if (ImGui::Button("Rect Duct")) choosePreset(6);
    if (ImGui::Button("Cable Tray")) choosePreset(7);
    ImGui::SameLine();
    if (ImGui::Button("I-Beam")) choosePreset(9);
    ImGui::Separator();
    int interpolation=int(spline.interpolation);
    const char *interpolationNames[]={"Polyline","Catmull-Rom","Cubic Bezier"};
    if (ImGui::Combo("Interpolation",&interpolation,interpolationNames,3))
    {spline.interpolation=PA::SplineInterpolation(interpolation);dirty=true;}
    if (ImGui::Checkbox("Closed",&spline.closed)) dirty=true;
    if (ImGui::Button("Add Point") && spline.points.size()<256)
    {
        auto point=spline.points.back();
        point.pointGuid=Solid::region(1901,nextPointId++);
        const auto &previous=spline.points[spline.points.size()-2];
        point.position={point.position.x+(point.position.x-previous.position.x),
                        point.position.y+(point.position.y-previous.position.y),
                        point.position.z+(point.position.z-previous.position.z)};
        spline.points.push_back(point);selected=int(spline.points.size()-1);dirty=true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Insert Point") && spline.points.size()<256)
    {
        const auto at=std::size_t(selected+1)<spline.points.size() ?
            std::size_t(selected+1) : std::size_t(selected);
        const auto previous=at-1;
        auto point=spline.points[previous];
        point.pointGuid=Solid::region(1901,nextPointId++);
        const auto &next=spline.points[at];
        point.position={(point.position.x+next.position.x)*.5,
                        (point.position.y+next.position.y)*.5,
                        (point.position.z+next.position.z)*.5};
        spline.points.insert(spline.points.begin()+std::ptrdiff_t(at),point);
        selected=int(at);dirty=true;
    }
    if (ImGui::Button("Delete Point") && spline.points.size()>(spline.closed?3:2))
    {spline.points.erase(spline.points.begin()+selected);selected=std::min(selected,int(spline.points.size()-1));dirty=true;}
    ImGui::SameLine();
    if (ImGui::Button("Reverse"))
    {
        std::reverse(spline.points.begin(),spline.points.end());
        for (auto &point:spline.points) std::swap(point.incomingHandle,point.outgoingHandle);
        selected=int(spline.points.size())-1-selected;dirty=true;
    }
    ImGui::Text("Control points: %zu / 256",spline.points.size());
    for (std::size_t i=0;i<spline.points.size();++i)
    {
        char label[32];std::snprintf(label,sizeof(label),"Point %zu",i);
        if (ImGui::Selectable(label,selected==int(i))) selected=int(i);
    }
    if (!spline.points.empty())
    {
        auto &point=spline.points[selected];
        ImGui::Separator();ImGui::Text("Edit point %d",selected);
        double position[]={point.position.x,point.position.y,point.position.z};
        if (ImGui::InputScalarN("XYZ (m)",ImGuiDataType_Double,position,3,nullptr,nullptr,"%.4f"))
        {point.position={position[0],position[1],position[2]};dirty=true;}
        double in[]={point.incomingHandle.x,point.incomingHandle.y,point.incomingHandle.z};
        if (ImGui::InputScalarN("Bezier in",ImGuiDataType_Double,in,3,nullptr,nullptr,"%.4f"))
        {point.incomingHandle={in[0],in[1],in[2]};dirty=true;}
        double out[]={point.outgoingHandle.x,point.outgoingHandle.y,point.outgoingHandle.z};
        if (ImGui::InputScalarN("Bezier out",ImGuiDataType_Double,out,3,nullptr,nullptr,"%.4f"))
        {point.outgoingHandle={out[0],out[1],out[2]};dirty=true;}
        if (ImGui::InputDouble("Width scale",&point.widthMeters,.01,.1,"%.4f"))dirty=true;
        double rollDegrees=point.rollRadians*180/std::numbers::pi;
        if (ImGui::InputDouble("Roll (deg)",&rollDegrees,1,15,"%.2f"))
        {point.rollRadians=rollDegrees*std::numbers::pi/180;dirty=true;}
    }
    ImGui::End();

    panel("Spline construction controls",float(width)-right,top,right,float(height)-top-bottom);
    ImGui::PushItemWidth(165);
    int profile=int(settings.profile);
    if (ImGui::Combo("Profile",&profile,profileNames,6))
    {settings.profile=Construction::ProfileKind(profile);dirty=true;}
    dirty|=ImGui::InputDouble("Radius (m)",&settings.radiusMeters,.001,.01,"%.4f");
    double diameter=2*settings.radiusMeters;
    if (ImGui::InputDouble("Diameter (m)",&diameter,.002,.02,"%.4f"))
    {settings.radiusMeters=diameter*.5;dirty=true;}
    dirty|=ImGui::InputDouble("Width (m)",&settings.widthMeters,.01,.1,"%.4f");
    dirty|=ImGui::InputDouble("Height (m)",&settings.heightMeters,.01,.1,"%.4f");
    dirty|=ImGui::InputDouble("Wall (m)",&settings.wallThicknessMeters,.001,.01,"%.4f");
    dirty|=ImGui::InputDouble("Corner radius (m)",&settings.cornerRadiusMeters,.001,.01,"%.4f");
    dirty|=ImGui::InputDouble("Profile scale",&settings.profileScale,.1,1,"%.3f");
    dirty|=ImGui::InputDouble("Profile rotation (rad)",&settings.profileRotationRadians,.05,.5,"%.3f");
    dirty|=ImGui::Checkbox("Start cap",&settings.capStart);
    ImGui::SameLine();dirty|=ImGui::Checkbox("End cap",&settings.capEnd);
    ImGui::Separator();ImGui::TextUnformatted("Quality and hard limits");
    const char *qualityNames[]={"Draft","Preview","Production","Hero"};
    if (ImGui::Combo("Quality",&quality,qualityNames,4))
    {
        const double scale[]={4,1,.35,.1};
        settings.pathQuality.maximumDeviationMeters=.01*scale[quality];
        settings.pathQuality.maximumTangentAngleRadians=.18*std::sqrt(scale[quality]);
        settings.pathQuality.maximumStepMeters=.25*std::sqrt(scale[quality]);
        settings.profileChordErrorMeters=.002*scale[quality];
        settings.profileMaximumAngleRadians=.25*std::sqrt(scale[quality]);
        dirty=true;
    }
    dirty|=ImGui::InputDouble("Path error (m)",&settings.pathQuality.maximumDeviationMeters,.001,.01,"%.5f");
    dirty|=ImGui::InputDouble("Path angle (rad)",&settings.pathQuality.maximumTangentAngleRadians,.01,.1,"%.4f");
    dirty|=ImGui::InputDouble("Max step (m)",&settings.pathQuality.maximumStepMeters,.01,.1,"%.4f");
    dirty|=ImGui::InputDouble("Profile error (m)",&settings.profileChordErrorMeters,.001,.01,"%.5f");
    dirty|=ImGui::InputDouble("Profile angle (rad)",&settings.profileMaximumAngleRadians,.01,.1,"%.4f");
    int samples=int(settings.pathQuality.maximumSamples);
    if (ImGui::InputInt("Max path samples",&samples))
    {settings.pathQuality.maximumSamples=std::size_t(std::max(0,samples));dirty=true;}
    int vertices=int(settings.maximumVertices);
    if (ImGui::InputInt("Max vertices",&vertices))
    {settings.maximumVertices=std::size_t(std::max(0,vertices));dirty=true;}
    ImGui::Separator();ImGui::TextUnformatted("Analytic UV");
    int uvMode=int(settings.uvMode);
    const char *uvNames[]={"World Metric","Stretch 0..1","Repeat Count"};
    if (ImGui::Combo("UV mode",&uvMode,uvNames,3))
    {settings.uvMode=Construction::UvMode(uvMode);dirty=true;}
    dirty|=ImGui::InputDouble("Metres / tile U",&settings.metresPerTileU,.1,1,"%.4f");
    dirty|=ImGui::InputDouble("Metres / tile V",&settings.metresPerTileV,.1,1,"%.4f");
    dirty|=ImGui::InputDouble("Repeat count",&settings.repeatCount,1,10,"%.2f");
    dirty|=ImGui::InputDouble("Seam angle (rad)",&settings.seamAngleRadians,.1,.5,"%.3f");
    ImGui::Separator();ImGui::TextUnformatted("Repeat along actual arc length");
    dirty|=ImGui::Checkbox("Supports / crossbars",&settings.repeatEnabled);
    dirty|=ImGui::InputDouble("Spacing (m)",&settings.repeat.spacingMeters,.1,1,"%.3f");
    dirty|=ImGui::InputDouble("Offset (m)",&settings.repeat.offsetMeters,.1,1,"%.3f");
    dirty|=ImGui::InputDouble("Start padding (m)",&settings.repeat.startPaddingMeters,.1,1,"%.3f");
    dirty|=ImGui::InputDouble("End padding (m)",&settings.repeat.endPaddingMeters,.1,1,"%.3f");
    ImGui::Separator();ImGui::TextUnformatted("Spline effects and controlled fracture");
    dirty|=ImGui::InputDouble("Taper length (m)",&settings.taperLengthMeters,.05,.2,"%.3f");
    dirty|=ImGui::InputDouble("Tip scale",&settings.tipScale,.05,.2,"%.3f");
    dirty|=ImGui::InputDouble("Rib amplitude",&settings.ribAmplitude,.02,.1,"%.3f");
    dirty|=ImGui::InputDouble("Rib spacing (m)",&settings.ribSpacingMeters,.02,.1,"%.3f");
    dirty|=ImGui::InputDouble("Twist turns",&settings.twistTurns,.1,.5,"%.3f");
    dirty|=ImGui::Checkbox("Fracture sweep",&settings.breakEnabled);
    dirty|=ImGui::InputDouble("Fracture position 0..1",&settings.breakFraction,.05,.2,"%.3f");
    dirty|=ImGui::InputDouble("Fracture gap (m)",&settings.breakGapMeters,.02,.1,"%.3f");
    dirty|=ImGui::InputDouble("Fracture roughness",&settings.breakRoughness,.05,.2,"%.3f");
    ImGui::PopItemWidth();
    ImGui::End();

    rebuild(renderer);
    panel("Spline Construction viewport",left,top,float(width)-left-right,float(height)-top-bottom);
    const char *views[]={"Lit","Wireframe","Normals","RMF Frames","UV Checker","Seam Debug"};
    ImGui::Combo("View",&view,views,6);
    if (view!=uploadedView && !built.mesh.mesh.positions.empty())
    {
        if (view==3)
        {
            auto debug=Construction::withFrameAxes(built);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
        uploadedView=view;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit")) {fitCamera=true;dirty=true;}
    ImGui::TextUnformatted("Left drag: orbit | Middle/right: pan | Wheel: zoom");
    auto size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,32.0f);size.y=std::max(size.y,32.0f);
    const int rendererMode[]={0,2,1,10,8,9};
    auto texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,rendererMode[view]);
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);
    if (ImGui::IsItemHovered())
    {
        auto &io=ImGui::GetIO();
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Left))
        {camera.yaw-=io.MouseDelta.x*.008f;
         camera.pitch=std::clamp(camera.pitch+io.MouseDelta.y*.008f,-1.4f,1.4f);}
        if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle) || ImGui::IsMouseDragging(ImGuiMouseButton_Right))
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
    panel("Spline Construction metrics",0,float(height)-bottom,float(width),bottom);
    if (!error.empty()) ImGui::TextColored({1,.32,.25,1},"Diagnostic: %s | Previous valid mesh retained",error.c_str());
    else ImGui::TextUnformatted("Ready | Geometry regenerated from current spline and controls");
    ImGui::Text("CPU %.2f ms | Path %.3f m | Samples / frames %zu | Profile segments %zu | Vertices %zu | Triangles %zu | Repeats %zu",
                built.generationCpuMilliseconds,built.path.lengthMeters,built.path.frames.size(),
                built.profileSegments,built.mesh.mesh.positions.size(),built.mesh.mesh.indices.size()/3,
                built.repeats.size());
    ImGui::Text("Measured chord deviation %.6f m | Approx geometry %.2f MiB | GPU %.2f MiB | %s",
                built.path.maximumMeasuredDeviationMeters,
                (built.mesh.mesh.positions.size()*sizeof(Vec3d)+built.mesh.mesh.indices.size()*sizeof(std::uint32_t))/1048576.0,
                renderer.bufferBytes()/1048576.0,renderer.adapter().c_str());
    if (built.bodyVertexCount && built.vertexUV.size()>=built.bodyVertexCount)
    {
        float minimumU=built.vertexUV[0][0],maximumU=minimumU;
        float minimumV=built.vertexUV[0][1],maximumV=minimumV;
        for (std::size_t i=1;i<built.bodyVertexCount;++i)
        {
            minimumU=std::min(minimumU,built.vertexUV[i][0]);
            maximumU=std::max(maximumU,built.vertexUV[i][0]);
            minimumV=std::min(minimumV,built.vertexUV[i][1]);
            maximumV=std::max(maximumV,built.vertexUV[i][1]);
        }
        ImGui::Text("Body UV U %.3f..%.3f | V %.3f..%.3f | Repeat stations %zu",
                    minimumU,maximumU,minimumV,maximumV,built.repeats.size());
    }
    ImGui::End();
}
}
