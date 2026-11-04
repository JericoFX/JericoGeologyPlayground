#include "SplineLabUI.hpp"
#include "SplineDamageIO.hpp"
#include <Windows.h>
#include <commdlg.h>
#include "SolidGeometry.hpp"
#include "ViewportProjection.hpp"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace Playground
{
namespace
{
std::filesystem::path chooseSplineDamageFile(bool saveFile)
{
    wchar_t filename[MAX_PATH]=L"SplineDamage.json";
    OPENFILENAMEW dialog{};
    dialog.lStructSize=sizeof(dialog);dialog.hwndOwner=GetActiveWindow();
    dialog.lpstrFilter=L"Spline damage (*.json)\0*.json\0\0";
    dialog.lpstrFile=filename;dialog.nMaxFile=MAX_PATH;dialog.lpstrDefExt=L"json";
    dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|(saveFile?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    const bool accepted=saveFile?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog);
    ImGui::GetIO().ClearInputMouse();
    return accepted?std::filesystem::path(filename):std::filesystem::path{};
}
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
    selectedDamage=-1;
    movingDamage=false;
    status.clear();
    error.clear();
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
    int breakSeed=int(settings.breakSeed);
    if (ImGui::InputInt("Fracture seed",&breakSeed))
    {settings.breakSeed=std::uint32_t(std::max(0,breakSeed));dirty=true;}
    ImGui::Separator();ImGui::TextUnformatted("Shared path damage");
    ImGui::TextWrapped("Save/load damage operations on the current spline and profile.");
    if (ImGui::Button("Save damage"))
    {
        const auto file=chooseSplineDamageFile(true);
        if (!file.empty()) try
        {
            Construction::buildConstruction(spline,settings);
            Construction::saveSplineDamage(settings.damagePaths,file);
            error.clear();status="Damage operations saved.";
        }
        catch(const std::exception &e){status=std::string("Could not save damage: ")+e.what();}
    }
    ImGui::SameLine();
    if (ImGui::Button("Load damage"))
    {
        const auto file=chooseSplineDamageFile(false);
        if (!file.empty()) try
        {
            auto next=Construction::loadSplineDamage(spline,settings,file);
            built=std::move(next);selectedDamage=settings.damagePaths.empty()?-1:0;
            movingDamage=moveDamageMode=false;damageGrabOffset={};
            dirty=false;uploadedView=-1;error.clear();status="Damage operations loaded.";
        }
        catch(const std::exception &e){status=std::string("Could not load damage: ")+e.what();}
    }
    if (!status.empty()) ImGui::TextWrapped("%s",status.c_str());
    if (ImGui::Button("Add crack / gouge"))
    {
        try
        {
            if (settings.damagePaths.size()>=64)
                throw std::runtime_error("The 64 damage area limit has been reached");
            const auto current=Construction::buildConstruction(spline,settings);
            auto next=settings;
            next.damagePaths.push_back(Construction::proposeSplineDamage(current,settings,
                Construction::nextSplineDamageId(settings.damagePaths),nextDamageId+1));
            auto candidate=Construction::buildConstruction(spline,next);
            if (meshHash(candidate.mesh.mesh)==meshHash(current.mesh.mesh))
                throw std::runtime_error("The proposed crack does not remove material from this surface");
            settings=std::move(next);built=std::move(candidate);++nextDamageId;
            selectedDamage=int(settings.damagePaths.size())-1;
            movingDamage=false;dirty=false;uploadedView=-1;error.clear();
            status="Crack fitted to the surface and local wall thickness.";
        }
        catch(const std::exception &e){status=std::string("Crack not added: ")+e.what();}
    }
    ImGui::SameLine();ImGui::Text("Areas %zu / 64",settings.damagePaths.size());
    for (std::size_t i=0;i<settings.damagePaths.size();++i)
    {
        auto &rule=settings.damagePaths[i];
        if (ImGui::Selectable(rule.id.c_str(),selectedDamage==int(i)))
            selectedDamage=int(i);
    }
    if (selectedDamage>=0 && selectedDamage<int(settings.damagePaths.size()))
    {
        auto &rule=settings.damagePaths[std::size_t(selectedDamage)];
        auto &cutter=rule.cutter;
        dirty|=ImGui::Checkbox("Damage enabled",&cutter.enabled);
        double center[]={cutter.projector.center.x,cutter.projector.center.y,
                         cutter.projector.center.z};
        if (ImGui::InputScalarN("Damage XYZ (m)",ImGuiDataType_Double,center,3,
                               nullptr,nullptr,"%.4f"))
        {cutter.projector.center={center[0],center[1],center[2]};dirty=true;}
        dirty|=ImGui::InputDouble("Crack radius (m)",&cutter.radius,.005,.02,"%.4f");
        dirty|=ImGui::InputDouble(rule.spatialPath?"Depth radius (m)":"Crack depth (m)",&cutter.depth,.01,.05,"%.4f");
        dirty|=ImGui::InputDouble("Damage amount",&cutter.amount,.05,.2,"%.3f");
        dirty|=ImGui::InputDouble("Edge irregularity",&cutter.irregularity,.05,.2,"%.3f");
        if(!rule.spatialPath)
            dirty|=ImGui::InputDouble("Separate chunks (m)",&cutter.separationMeters,.01,.05,"%.3f");
        if (!rule.spatialPath && cutter.path.size()==2)
        {
            double ends[]={cutter.path[0].x,cutter.path[1].x};
            if (ImGui::InputScalarN("Crack ends (m)",ImGuiDataType_Double,ends,2,
                                   nullptr,nullptr,"%.4f"))
            {cutter.path[0].x=ends[0];cutter.path[1].x=ends[1];dirty=true;}
        }
        if(!rule.spatialPath && ImGui::Button("Convert to 3D spline"))
        {
            try
            {
                auto next=settings;auto &candidate=next.damagePaths[std::size_t(selectedDamage)];
                if(candidate.cutter.separationMeters!=0)
                    throw std::runtime_error("Set chunk separation to zero before converting");
                PA::ProceduralSpline path;path.name=rule.id;
                path.splineGuid=Solid::region(2410,Jerico::stableGuidHash(spline.splineGuid)^cutter.seed);
                for(const auto position:cutter.path)
                {
                    PA::SplineControlPoint point;point.position=position;
                    point.pointGuid=Solid::region(2411,path.points.size()+1);path.points.push_back(point);
                }
                candidate.spatialPath=std::move(path);
                auto preview=Construction::buildConstruction(spline,next);
                // Keep the selected vector entry alive while this editor holds references to it.
                rule.spatialPath=std::move(candidate.spatialPath);built=std::move(preview);
                dirty=false;uploadedView=-1;error.clear();status="3D path ready. Edit its local XYZ points below.";
            }
            catch(const std::exception &e){status=std::string("3D conversion failed: ")+e.what();}
        }
        if(rule.spatialPath)
        {
            int operation=int(rule.operation);
            if(ImGui::Combo("Surface operation",&operation,"Cut / groove\0Raised decal\0"))
            {rule.operation=DamageVolumeOperation(operation);dirty=true;}
            auto &path=*rule.spatialPath;int interpolation=int(path.interpolation);
            if(ImGui::Combo("Damage curve",&interpolation,"Polyline\0Catmull-Rom\0Bezier\0"))
            {path.interpolation=PA::SplineInterpolation(interpolation);dirty=true;}
            ImGui::TextWrapped("Point XYZ is relative to the damage area. Width and depth set the swept cross-section.");
            for(std::size_t i=0;i<path.points.size();++i)
            {
                ImGui::PushID(int(i));auto &point=path.points[i];
                double xyz[]{point.position.x,point.position.y,point.position.z};
                ImGui::Text("Point %zu",i+1);
                if(ImGui::InputScalarN("XYZ (m)",ImGuiDataType_Double,xyz,3,nullptr,nullptr,"%.4f"))
                {point.position={xyz[0],xyz[1],xyz[2]};dirty=true;}
                dirty|=ImGui::InputDouble("Width scale",&point.widthMeters,.05,.2,"%.3f");
                dirty|=ImGui::InputDouble("Roll (rad)",&point.rollRadians,.05,.2,"%.3f");
                if(path.interpolation==PA::SplineInterpolation::CubicBezier)
                {
                    for(unsigned h=0;h<2;++h)
                    {
                        auto &handle=h?point.outgoingHandle:point.incomingHandle;
                        double values[]{handle.x,handle.y,handle.z};
                        if(ImGui::InputScalarN(h?"Outgoing (m)":"Incoming (m)",ImGuiDataType_Double,values,3,nullptr,nullptr,"%.4f"))
                        {handle={values[0],values[1],values[2]};dirty=true;}
                    }
                }
                const bool remove=path.points.size()>2&&ImGui::Button("Remove point");
                ImGui::PopID();
                if(remove){path.points.erase(path.points.begin()+i);dirty=true;break;}
            }
            if(path.points.size()<64&&ImGui::Button("Split last segment"))
            {
                PA::SplineControlPoint point;const auto &a=path.points[path.points.size()-2],&b=path.points.back();
                point.position={(a.position.x+b.position.x)*.5,(a.position.y+b.position.y)*.5,(a.position.z+b.position.z)*.5};
                point.widthMeters=(a.widthMeters+b.widthMeters)*.5;point.rollRadians=(a.rollRadians+b.rollRadians)*.5;
                do{point.pointGuid=Solid::region(2411,++nextPointId);}while(std::any_of(path.points.begin(),path.points.end(),[&](const auto&p){return p.pointGuid==point.pointGuid;}));
                path.points.insert(path.points.end()-1,point);dirty=true;
            }
        }
        dirty|=ImGui::InputScalar("Damage seed",ImGuiDataType_U64,&cutter.seed);
        ImGui::Checkbox("Move selected area",&moveDamageMode);
        ImGui::SameLine();ImGui::TextUnformatted("or Shift+drag");
        if (ImGui::Button("Remove selected damage"))
        {
            settings.damagePaths.erase(settings.damagePaths.begin()+selectedDamage);
            selectedDamage=std::min(selectedDamage,int(settings.damagePaths.size())-1);
            movingDamage=false;
            dirty=true;
        }
    }
    ImGui::PopItemWidth();
    ImGui::End();

    rebuild(renderer);
    panel("Spline Construction viewport",left,top,float(width)-left-right,float(height)-top-bottom);
    const char *views[]={"Lit","Wireframe","Normals","RMF Frames","UV Checker",
                         "Seam Debug","Bake UV Checker"};
    ImGui::Combo("View",&view,views,7);
    if (view!=uploadedView && !built.mesh.mesh.positions.empty())
    {
        if (view==3)
        {
            auto debug=Construction::withFrameAxes(built);
            renderer.upload(debug.mesh,nullptr,nullptr,nullptr,false,&debug.cornerUV);
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,
            view==6 && !built.bakeCornerUV.empty() ? &built.bakeCornerUV : &built.cornerUV);
        uploadedView=view;
    }
    ImGui::SameLine();
    if (ImGui::Button("Fit")) {fitCamera=true;dirty=true;}
    ImGui::TextUnformatted("Left drag: orbit | Shift+drag damage | Middle/right: pan | Wheel: zoom");
    auto size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,32.0f);size.y=std::max(size.y,32.0f);
    const int rendererMode[]={0,2,1,10,8,9,8};
    auto texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,rendererMode[view]);
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);
    if (selectedDamage>=0 && selectedDamage<int(settings.damagePaths.size()))
        if (const auto point=projectToViewport(
                settings.damagePaths[std::size_t(selectedDamage)].cutter.projector.center,
                camera,unsigned(size.x),unsigned(size.y)))
        {
            const auto origin=ImGui::GetItemRectMin();
            const ImVec2 at{origin.x+float(point->x),origin.y+float(point->y)};
            auto *draw=ImGui::GetWindowDrawList();
            draw->AddCircle(at,9,IM_COL32(255,195,55,255),24,2);
            draw->AddLine({at.x-13,at.y},{at.x+13,at.y},IM_COL32(255,195,55,255),1);
            draw->AddLine({at.x,at.y-13},{at.x,at.y+13},IM_COL32(255,195,55,255),1);
        }
    if (ImGui::IsItemHovered())
    {
        auto &io=ImGui::GetIO();
        const bool moveArea=(io.KeyShift || moveDamageMode) &&
            selectedDamage>=0 && selectedDamage<int(settings.damagePaths.size()) &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (moveArea)
        {
            auto &projector=settings.damagePaths[std::size_t(selectedDamage)].cutter.projector;
            const auto origin=ImGui::GetItemRectMin();
            if (const auto hit=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),
                    io.MousePos.x-origin.x,io.MousePos.y-origin.y,
                    projector.center,projector.normal))
            {
                if (!movingDamage)
                {
                    damageGrabOffset={projector.center.x-hit->x,
                        projector.center.y-hit->y,projector.center.z-hit->z};
                    movingDamage=true;
                }
                const Vec3d next{hit->x+damageGrabOffset.x,hit->y+damageGrabOffset.y,
                    hit->z+damageGrabOffset.z};
                if (next.x!=projector.center.x || next.y!=projector.center.y ||
                    next.z!=projector.center.z)
                {projector.center=next;dirty=true;}
            }
        }
        if (!moveArea && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) movingDamage=false;
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
    if (built.bakeIslandCount)
        ImGui::Text("Unique bake UV islands %u | Damage/decal areas %zu",
                    built.bakeIslandCount,settings.damagePaths.size());
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
