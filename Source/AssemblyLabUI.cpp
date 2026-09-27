#include "AssemblyLabUI.hpp"
#include "AssemblyDamageIO.hpp"
#include "SolidGeometry.hpp"
#include "ViewportProjection.hpp"

#include <Windows.h>
#include <commdlg.h>
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
    "Semantic Roles","RMF Frames","Shared surface field"};
constexpr const char *surfaceFields="Signed curvature\0Convexity\0Concavity\0Cavity/contact\0"
    "Crease distance\0Boundary distance\0Exposure\0Thickness\0";
void panel(const char *title,float x,float y,float width,float height)
{
    ImGui::SetNextWindowPos({x,y});
    ImGui::SetNextWindowSize({width,height});
    ImGui::Begin(title,nullptr,
        ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
}
std::filesystem::path chooseDamageFile(bool saveFile)
{
    wchar_t filename[MAX_PATH]=L"AssemblyDamage.json";
    OPENFILENAMEW dialog{};
    dialog.lStructSize=sizeof(dialog);
    dialog.hwndOwner=GetActiveWindow();
    dialog.lpstrFilter=L"Assembly damage (*.json)\0*.json\0\0";
    dialog.lpstrFile=filename;
    dialog.nMaxFile=MAX_PATH;
    dialog.lpstrDefExt=L"json";
    dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|
        (saveFile?OFN_OVERWRITEPROMPT:OFN_FILEMUSTEXIST);
    const bool accepted=saveFile?GetSaveFileNameW(&dialog):GetOpenFileNameW(&dialog);
    ImGui::GetIO().ClearInputMouse();
    return accepted?std::filesystem::path(filename):std::filesystem::path{};
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
    selectedId.clear();selectedArea=-1;selectedPath=-1;
    dirty=true;fitCamera=true;movingDamageArea=false;
    damageUndo.clear();damageRedo.clear();pendingDamageEdit.reset();
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
    std::optional<Assembly::DamageSettings> damageBefore;
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) || ImGui::IsAnyItemActive() ||
        ImGui::IsKeyDown(ImGuiKey_Enter) || ImGui::IsKeyDown(ImGuiKey_Space))
        damageBefore=settings.damage;
    bool suppressDamageHistory=false;
    auto commitDamageEdit=[&]
    {
        if (!pendingDamageEdit) return;
        if (damageUndo.size()>=64) damageUndo.erase(damageUndo.begin());
        damageUndo.push_back(std::move(*pendingDamageEdit));
        pendingDamageEdit.reset();
        damageRedo.clear();
    };
    constexpr float top=54,bottom=112,left=315,right=390;
    panel("Assembly path and presets",0,top,left,float(height)-top-bottom);
    ImGui::TextUnformatted("Reusable path + semantic parts");
    const auto preset=int(settings.kind);
    if (ImGui::BeginCombo("Preset",presets[preset]))
    {
        for (int i=0;i<int(std::size(presets));++i)
            if (ImGui::Selectable(presets[i],i==preset))
            {choosePreset(i);suppressDamageHistory=true;}
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
    ImGui::BeginChild("Assembly controls scroll",{0,0},false);
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
    ImGui::Separator();ImGui::TextUnformatted("Movable damage areas");
    if (ImGui::Button("Save damage"))
    {
        try
        {
            const auto file=chooseDamageFile(true);
            if (!file.empty()) {Assembly::saveAssemblyDamage(settings.damage,file);error.clear();}
        }
        catch (const std::exception &exception) {error=exception.what();}
    }
    ImGui::SameLine();
    if (ImGui::Button("Load damage"))
    {
        try
        {
            const auto file=chooseDamageFile(false);
            if (!file.empty())
            {
                auto loaded=Assembly::loadAssemblyDamage(file);
                settings.damage=std::move(loaded);
                selectedArea=-1;selectedPath=-1;dirty=true;error.clear();
            }
        }
        catch (const std::exception &exception) {error=exception.what();}
    }
    if (ImGui::Button("Undo damage") && (pendingDamageEdit || !damageUndo.empty()))
    {
        commitDamageEdit();
        damageRedo.push_back(settings.damage);
        settings.damage=std::move(damageUndo.back());damageUndo.pop_back();
        selectedArea=std::min(selectedArea,int(settings.damage.areaRules.size())-1);
        selectedPath=std::min(selectedPath,int(settings.damage.pathRules.size())-1);
        dirty=true;suppressDamageHistory=true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Redo damage") && !pendingDamageEdit && !damageRedo.empty())
    {
        damageUndo.push_back(settings.damage);
        settings.damage=std::move(damageRedo.back());damageRedo.pop_back();
        selectedArea=std::min(selectedArea,int(settings.damage.areaRules.size())-1);
        selectedPath=std::min(selectedPath,int(settings.damage.pathRules.size())-1);
        dirty=true;suppressDamageHistory=true;
    }
    if (settings.damage.areaRules.size()>=Assembly::maximumDamageAreas)
        ImGui::TextUnformatted("Damage area limit reached (64)");
    else if (ImGui::Button("Add area at selected part"))
    {
        Assembly::AreaDamageRule rule;
        do {rule.area.id="area-"+std::to_string(nextAreaId++);}
        while (std::any_of(settings.damage.areaRules.begin(),settings.damage.areaRules.end(),
            [&](const auto &existing){return existing.area.id==rule.area.id;}));
        const auto *part=selectedPart();
        const auto station=part?part->stationMeters:built.path.lengthMeters*.5;
        const auto frame=Construction::frameAtDistance(built.path,station);
        rule.area.projector.center=part?part->position:frame.position;
        rule.area.projector.tangent=frame.tangent;
        rule.area.projector.bitangent=frame.binormal;
        rule.area.projector.normal=frame.normal;
        rule.area.projector.scale.z=.35;
        rule.area.footprint.radius={.3,.3,1};
        if (part) rule.area.semanticPartId=part->id;
        settings.damage.areaRules.push_back(std::move(rule));
        selectedArea=int(settings.damage.areaRules.size())-1;
        selectedPath=-1;
        dirty=true;
    }
    for (std::size_t i=0;i<settings.damage.areaRules.size();++i)
    {
        const auto &rule=settings.damage.areaRules[i];
        const auto label=rule.area.id+"  ["+Assembly::name(rule.state)+"]";
        if (ImGui::Selectable(label.c_str(),selectedArea==int(i)))
        {selectedArea=int(i);selectedPath=-1;}
    }
    if (selectedArea>=0 && selectedArea<int(settings.damage.areaRules.size()))
    {
        auto &rule=settings.damage.areaRules[std::size_t(selectedArea)];
        if (ImGui::Button("Remove selected area"))
        {
            settings.damage.areaRules.erase(settings.damage.areaRules.begin()+selectedArea);
            selectedArea=-1;dirty=true;
        }
        else
        {
            dirty|=ImGui::Checkbox("Area enabled",&rule.area.enabled);
            constexpr double minimumAmount=0,maximumAmount=1;
            dirty|=ImGui::SliderScalar("Area amount",ImGuiDataType_Double,
                &rule.area.amount,&minimumAmount,&maximumAmount,"%.2f");
            int areaState=int(rule.state);
            if (ImGui::Combo("Area state",&areaState,
                             "Intact\0Missing\0Broken\0Bent\0Loose\0Detached\0Decayed\0Collapsed\0"))
            {rule.state=Assembly::State(areaState);dirty=true;}
            double center[]={rule.area.projector.center.x,rule.area.projector.center.y,
                             rule.area.projector.center.z};
            if (ImGui::InputScalarN("Area center XYZ",ImGuiDataType_Double,center,3,
                                    nullptr,nullptr,"%.3f"))
            {rule.area.projector.center={center[0],center[1],center[2]};dirty=true;}
            double radius[]={rule.area.footprint.radius.x,rule.area.footprint.radius.y};
            if (ImGui::InputScalarN("Area radii XY",ImGuiDataType_Double,radius,2,
                                    nullptr,nullptr,"%.3f"))
            {rule.area.footprint.radius.x=radius[0];
             rule.area.footprint.radius.y=radius[1];dirty=true;}
            dirty|=ImGui::InputDouble("Area depth band",&rule.area.projector.scale.z,
                                      .01,.1,"%.3f");
            dirty|=ImGui::InputDouble("Area rotation rad",
                                      &rule.area.projector.rotationRadians,.05,.5,"%.3f");
            constexpr double minimumBoundary=0,maximumBoundary=.25;
            dirty|=ImGui::SliderScalar("Area edge irregularity",ImGuiDataType_Double,
                &rule.area.boundary.irregularity,&minimumBoundary,&maximumBoundary,"%.3f");
            constexpr double minimumFrequency=.1,maximumFrequency=20;
            dirty|=ImGui::SliderScalar("Area edge frequency",ImGuiDataType_Double,
                &rule.area.boundary.frequency,&minimumFrequency,&maximumFrequency,"%.2f");
            if (ImGui::Button("Move area to selected part"))
                if (const auto *part=selectedPart())
                {rule.area.projector.center=part->position;dirty=true;}
            if (ImGui::Button("Restrict area to selected part"))
                if (const auto *part=selectedPart())
                {rule.area.semanticPartId=part->id;dirty=true;}
            ImGui::SameLine();
            if (ImGui::Button("All parts"))
            {rule.area.semanticPartId.clear();dirty=true;}
            ImGui::TextWrapped("Part filter: %s",
                rule.area.semanticPartId.empty()?"All":rule.area.semanticPartId.c_str());
            if (ImGui::Button("Restrict role to selected part"))
                if (const auto *part=selectedPart())
                {rule.area.semanticRole=Assembly::name(part->role);dirty=true;}
            ImGui::SameLine();
            if (ImGui::Button("All roles"))
            {rule.area.semanticRole.clear();dirty=true;}
            bool indexRange=rule.area.indexRange.has_value();
            if (ImGui::Checkbox("Limit index range",&indexRange))
            {
                rule.area.indexRange=indexRange?
                    std::optional<std::pair<std::uint32_t,std::uint32_t>>{{0,0}}:
                    std::nullopt;
                if (indexRange)
                    if (const auto *part=selectedPart())
                        rule.area.indexRange={{part->index,part->index}};
                dirty=true;
            }
            if (rule.area.indexRange)
            {
                int range[]={int(rule.area.indexRange->first),
                             int(rule.area.indexRange->second)};
                if (ImGui::InputInt2("Index first/last",range))
                {rule.area.indexRange={{std::uint32_t(std::max(0,range[0])),
                                        std::uint32_t(std::max(0,range[1]))}};dirty=true;}
            }
            bool pathRange=rule.area.pathRangeMeters.has_value();
            if (ImGui::Checkbox("Limit path distance",&pathRange))
            {
                rule.area.pathRangeMeters=pathRange?
                    std::optional<std::pair<double,double>>{{0,built.path.lengthMeters}}:
                    std::nullopt;
                dirty=true;
            }
            if (rule.area.pathRangeMeters)
            {
                double range[]={rule.area.pathRangeMeters->first,
                                rule.area.pathRangeMeters->second};
                if (ImGui::InputScalarN("Path start/end (m)",ImGuiDataType_Double,
                                        range,2,nullptr,nullptr,"%.3f"))
                {rule.area.pathRangeMeters={{range[0],range[1]}};dirty=true;}
            }
        }
    }
    ImGui::Separator();ImGui::TextUnformatted("Selected geometric crack paths");
    const auto *crackPart=selectedPart();
    const bool crackRole=crackPart &&
        (crackPart->role==Assembly::Role::DeckPlank ||
         crackPart->role==Assembly::Role::Sleeper ||
         crackPart->role==Assembly::Role::Beam);
    const bool partHasCrack=crackPart && std::any_of(
        settings.damage.pathRules.begin(),settings.damage.pathRules.end(),
        [&](const auto &rule){return rule.semanticPartId==crackPart->id;});
    if (crackRole && !partHasCrack &&
        settings.damage.pathRules.size()<Assembly::maximumDamageAreas &&
        ImGui::Button("Add crack to selected solid member"))
    {
        Assembly::PathDamageRule rule;
        do {rule.id="path-"+std::to_string(nextPathId++);}
        while (std::any_of(settings.damage.pathRules.begin(),settings.damage.pathRules.end(),
            [&](const auto &existing){return existing.id==rule.id;}));
        rule.semanticPartId=crackPart->id;
        const auto frame=Construction::frameAtDistance(built.path,crackPart->stationMeters);
        const auto height=crackPart->role==Assembly::Role::Sleeper?
            settings.sleeperHeightMeters:crackPart->role==Assembly::Role::Beam?
            settings.beamHeightMeters:settings.boardThicknessMeters;
        rule.cutter.projector.center={
            crackPart->position.x+frame.normal.x*height*.5,
            crackPart->position.y+frame.normal.y*height*.5,
            crackPart->position.z+frame.normal.z*height*.5};
        rule.cutter.projector.tangent=frame.tangent;
        rule.cutter.projector.normal=frame.normal;
        rule.cutter.projector.bitangent={
            frame.normal.y*frame.tangent.z-frame.normal.z*frame.tangent.y,
            frame.normal.z*frame.tangent.x-frame.normal.x*frame.tangent.z,
            frame.normal.x*frame.tangent.y-frame.normal.y*frame.tangent.x};
        rule.cutter.path={{0,-.35,0},{0,.35,0}};
        rule.cutter.radius=.025;
        rule.cutter.depth=std::min(height*.6,.08);
        settings.damage.pathRules.push_back(std::move(rule));
        selectedPath=int(settings.damage.pathRules.size())-1;
        selectedArea=-1;dirty=true;
    }
    for (std::size_t i=0;i<settings.damage.pathRules.size();++i)
    {
        const auto &rule=settings.damage.pathRules[i];
        const auto label=rule.id+"  ["+rule.semanticPartId+"]";
        if (ImGui::Selectable(label.c_str(),selectedPath==int(i)))
        {selectedPath=int(i);selectedArea=-1;}
    }
    if (selectedPath>=0 && selectedPath<int(settings.damage.pathRules.size()))
    {
        auto &rule=settings.damage.pathRules[std::size_t(selectedPath)];
        if (ImGui::Button("Remove selected crack"))
        {
            settings.damage.pathRules.erase(settings.damage.pathRules.begin()+selectedPath);
            selectedPath=-1;dirty=true;
        }
        else
        {
            dirty|=ImGui::Checkbox("Crack enabled",&rule.cutter.enabled);
            constexpr double zero=0,one=1;
            dirty|=ImGui::SliderScalar("Crack amount",ImGuiDataType_Double,
                &rule.cutter.amount,&zero,&one,"%.2f");
            double center[]={rule.cutter.projector.center.x,
                rule.cutter.projector.center.y,rule.cutter.projector.center.z};
            if (ImGui::InputScalarN("Crack center XYZ",ImGuiDataType_Double,
                                    center,3,nullptr,nullptr,"%.3f"))
            {rule.cutter.projector.center={center[0],center[1],center[2]};dirty=true;}
            dirty|=ImGui::InputDouble("Crack rotation rad",
                &rule.cutter.projector.rotationRadians,.05,.5,"%.3f");
            dirty|=ImGui::InputDouble("Crack radius",&rule.cutter.radius,.005,.02,"%.3f");
            dirty|=ImGui::InputDouble("Crack depth",&rule.cutter.depth,.005,.02,"%.3f");
            for (std::size_t i=0;i<rule.cutter.path.size();++i)
            {
                ImGui::PushID(int(i));
                double xy[]={rule.cutter.path[i].x,rule.cutter.path[i].y};
                if (ImGui::InputScalarN("Crack point XY",ImGuiDataType_Double,
                                        xy,2,nullptr,nullptr,"%.3f"))
                {rule.cutter.path[i]={xy[0],xy[1],0};dirty=true;}
                ImGui::PopID();
            }
            if (rule.cutter.path.size()<maximumDamagePathPoints &&
                ImGui::Button("Add crack point"))
            {
                auto point=rule.cutter.path.back();
                point.x+=point.x<9.8?.2:-.2;
                rule.cutter.path.push_back(point);dirty=true;
            }
            if (rule.cutter.path.size()>2 && ImGui::Button("Remove last crack point"))
            {rule.cutter.path.pop_back();dirty=true;}
            if (ImGui::Button("Move crack to selected part"))
                if (const auto *part=selectedPart();part &&
                    (part->role==Assembly::Role::DeckPlank ||
                     part->role==Assembly::Role::Sleeper ||
                     part->role==Assembly::Role::Beam))
                {
                    const auto assigned=std::any_of(settings.damage.pathRules.begin(),
                        settings.damage.pathRules.end(),[&](const auto &other)
                        {return &other!=&rule && other.semanticPartId==part->id;});
                    if (!assigned)
                    {
                        const auto frame=Construction::frameAtDistance(built.path,
                            part->stationMeters);
                        const auto height=part->role==Assembly::Role::Sleeper?
                            settings.sleeperHeightMeters:part->role==Assembly::Role::Beam?
                            settings.beamHeightMeters:settings.boardThicknessMeters;
                        rule.semanticPartId=part->id;
                        rule.cutter.projector.center={
                            part->position.x+frame.normal.x*height*.5,
                            part->position.y+frame.normal.y*height*.5,
                            part->position.z+frame.normal.z*height*.5};
                        rule.cutter.projector.tangent=frame.tangent;
                        rule.cutter.projector.normal=frame.normal;
                        rule.cutter.projector.bitangent={
                            frame.normal.y*frame.tangent.z-frame.normal.z*frame.tangent.y,
                            frame.normal.z*frame.tangent.x-frame.normal.x*frame.tangent.z,
                            frame.normal.x*frame.tangent.y-frame.normal.y*frame.tangent.x};
                        dirty=true;
                    }
                }
            ImGui::TextWrapped("Part filter: %s",rule.semanticPartId.c_str());
        }
    }
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
    ImGui::PopItemWidth();ImGui::EndChild();ImGui::End();

    rebuild();
    panel("Procedural Assemblies viewport",left,top,
          float(width)-left-right,float(height)-top-bottom);
    ImGui::Combo("View",&view,views,int(std::size(views)));
    if (view==6)
    {
        if (ImGui::Combo("Shared field",&surfaceDebugField,surfaceFields))
            uploadedView=-1;
        ImGui::Text("Feature cache: %zu hits / %zu misses | %.1f ms compute",
            surfaceFeatureCache.hits(),surfaceFeatureCache.misses(),
            surfaceFeatureCache.computeMilliseconds());
    }
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
        else if (view==6)
        {
            try
            {
                SurfaceFeatureSettings analysis;
                analysis.contact=surfaceDebugField==3;
                analysis.thickness=surfaceDebugField==7;
                const auto &features=surfaceFeatureCache.get(built.mesh.mesh,analysis);
                auto debug=built.mesh;
                debug.field=surfaceDebugValues(features,
                    static_cast<SurfaceDebugField>(surfaceDebugField));
                renderer.upload(debug,nullptr,nullptr,nullptr,false,&built.cornerUV);
            }
            catch (const std::exception &exception)
            {
                error=exception.what();
                renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
            }
        }
        else renderer.upload(built.mesh,nullptr,nullptr,nullptr,false,&built.cornerUV);
        uploadedView=view;
    }
    auto selectedProjector=[&]() -> DamageProjector *
    {
        if (selectedPath>=0 && selectedPath<int(settings.damage.pathRules.size()))
            return &settings.damage.pathRules[std::size_t(selectedPath)].cutter.projector;
        if (selectedArea>=0 && selectedArea<int(settings.damage.areaRules.size()))
            return &settings.damage.areaRules[std::size_t(selectedArea)].area.projector;
        return nullptr;
    };
    if (selectedProjector())
    {
        ImGui::SameLine();
        ImGui::Checkbox("Move selected damage",&moveDamageAreaMode);
        ImGui::SameLine();ImGui::TextUnformatted("Shift+drag also moves");
    }
    else
    {
        ImGui::SameLine();
        ImGui::TextUnformatted("Click part; Shift+drag selected damage area");
    }
    auto size=ImGui::GetContentRegionAvail();
    size.x=std::max(size.x,32.0f);size.y=std::max(size.y,32.0f);
    constexpr int modes[]={0,2,1,8,3,10,3};
    auto texture=renderer.draw(unsigned(size.x),unsigned(size.y),camera,modes[view]);
    ImGui::Image(reinterpret_cast<ImTextureID>(texture),size);
    if (const auto *projector=selectedProjector())
    {
        if (const auto point=projectToViewport(projector->center,camera,
                                               unsigned(size.x),unsigned(size.y)))
        {
            const auto origin=ImGui::GetItemRectMin();
            const ImVec2 at{origin.x+float(point->x),origin.y+float(point->y)};
            auto *draw=ImGui::GetWindowDrawList();
            draw->AddCircle(at,9,IM_COL32(255,195,55,255),24,2);
            draw->AddLine({at.x-13,at.y},{at.x+13,at.y},
                          IM_COL32(255,195,55,255),1);
            draw->AddLine({at.x,at.y-13},{at.x,at.y+13},
                          IM_COL32(255,195,55,255),1);
        }
    }
    if (ImGui::IsItemHovered())
    {
        auto &io=ImGui::GetIO();
        const bool moveArea=(io.KeyShift || moveDamageAreaMode) && selectedProjector() &&
            ImGui::IsMouseDown(ImGuiMouseButton_Left);
        if (moveArea)
        {
            auto &projector=*selectedProjector();
            const auto origin=ImGui::GetItemRectMin();
            if (const auto hit=pointOnPlaneAtPixel(camera,unsigned(size.x),unsigned(size.y),
                    io.MousePos.x-origin.x,io.MousePos.y-origin.y,
                    projector.center,projector.normal))
            {
                if (!movingDamageArea)
                {
                    damageGrabOffset={projector.center.x-hit->x,
                        projector.center.y-hit->y,projector.center.z-hit->z};
                    movingDamageArea=true;
                }
                const Vec3d next{hit->x+damageGrabOffset.x,hit->y+damageGrabOffset.y,
                    hit->z+damageGrabOffset.z};
                if (next.x!=projector.center.x || next.y!=projector.center.y ||
                    next.z!=projector.center.z)
                {projector.center=next;dirty=true;}
            }
        }
        if (!moveArea && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && view!=5)
        {
            const auto minimum=ImGui::GetItemRectMin();
            const auto x=unsigned(std::clamp(io.MousePos.x-minimum.x,0.0f,size.x-1));
            const auto y=unsigned(std::clamp(io.MousePos.y-minimum.y,0.0f,size.y-1));
            if (const auto triangle=renderer.pickTriangle(unsigned(size.x),unsigned(size.y),camera,x,y))
                if (*triangle<built.mesh.mesh.triangleRegions.size())
                    for (const auto &part:built.parts)
                        if (part.guid==built.mesh.mesh.triangleRegions[*triangle] ||
                            std::find(part.freshInteriorRegions.begin(),
                                      part.freshInteriorRegions.end(),
                                      built.mesh.mesh.triangleRegions[*triangle])!=
                                part.freshInteriorRegions.end())
                        {selectedId=part.id;break;}
            renderer.draw(unsigned(size.x),unsigned(size.y),camera,modes[view]);
        }
        if (!moveArea && ImGui::IsMouseDragging(ImGuiMouseButton_Left))
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
    if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) movingDamageArea=false;
    ImGui::End();
    if (!suppressDamageHistory)
    {
        bool changed=false;
        if (damageBefore)
        {
            try {changed=Assembly::serializeAssemblyDamage(*damageBefore)!=
                         Assembly::serializeAssemblyDamage(settings.damage);}
            catch (const std::exception &) {changed=true;}
        }
        if (changed && !pendingDamageEdit) pendingDamageEdit=std::move(*damageBefore);
        if (pendingDamageEdit && !ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
            !ImGui::IsAnyItemActive()) commitDamageEdit();
    }
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
