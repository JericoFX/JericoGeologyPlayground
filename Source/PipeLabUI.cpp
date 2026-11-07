#include "PipeLabUI.hpp"
#include <fstream>
#include "PipeConnector.hpp"
#include "SocketInference.hpp"
#include "ViewportProjection.hpp"
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <set>
#include <Windows.h>
#include <commdlg.h>

namespace Playground
{
namespace
{
using namespace Pipes;
void panel(const char* title,float x,float y,float w,float h)
{
    ImGui::SetNextWindowPos({x,y});ImGui::SetNextWindowSize({w,h});
    ImGui::Begin(title,nullptr,ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoCollapse);
}
bool textField(const char* label,std::string& value)
{
    std::vector<char> buffer(std::max(std::size_t(256),value.size()+128));std::copy(value.begin(),value.end(),buffer.begin());
    if(!ImGui::InputText(label,buffer.data(),buffer.size()))return false;value=buffer.data();return true;
}
std::pair<Vec3d,Vec3d> bounds(const MeshResult&m)
{
    Vec3d low{1e30,1e30,1e30},high{-1e30,-1e30,-1e30};
    for(auto p:m.mesh.positions){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}return {low,high};
}
ImU32 stateColor(ConnectionState state)
{
    return state==ConnectionState::Compatible?IM_COL32(70,225,130,255):state==ConnectionState::NeedsCorrection?IM_COL32(255,202,65,255):IM_COL32(250,80,80,255);
}
Socket newSocket(const Module&m)
{
    Socket s;for(unsigned i=1;;++i){s.name="Socket "+std::to_string(i);if(std::none_of(m.sockets.begin(),m.sockets.end(),[&](const auto&v){return v.name==s.name;}))return s;}
}
std::filesystem::path chooseMeshFile()
{
    wchar_t filename[32768]{};OPENFILENAMEW dialog{};dialog.lStructSize=sizeof(dialog);
    dialog.hwndOwner=GetActiveWindow();dialog.lpstrFilter=L"GLB meshes (*.glb)\0*.glb\0\0";
    dialog.lpstrFile=filename;dialog.nMaxFile=32768;dialog.Flags=OFN_NOCHANGEDIR|OFN_PATHMUSTEXIST|OFN_FILEMUSTEXIST;
    const bool accepted=GetOpenFileNameW(&dialog);ImGui::GetIO().ClearInputMouse();
    return accepted?std::filesystem::path(filename):std::filesystem::path{};
}
}
PipeLabUI::PipeLabUI():directory(std::filesystem::path(PLAYGROUND_ROOT)/"models")
{
    for(auto&c:orthographic){c.distance=100;c.yaw=0;c.pitch=0;c.orthographicHeight=3;}
    orthographic[0].pitch=-float(std::numbers::pi/2); // Top: XZ.
    orthographic[2].yaw=float(std::numbers::pi/2); // Side: ZY.
}
Pipes::Module& PipeLabUI::moduleFor(const Instance&i){return i.generated>=0?pipes.at(i.generated).module:loadAsset(i.asset).module;}
const Pipes::ImportedMesh& PipeLabUI::meshFor(const Instance&i){return i.generated>=0?pipes.at(i.generated).mesh:loadAsset(i.asset).mesh;}
std::string PipeLabUI::instanceName(int i) const
{
    return std::to_string(i+1)+": "+(instances.at(i).generated>=0?"Procedural pipe":instances.at(i).asset.filename().string());
}
void PipeLabUI::openMesh(const std::filesystem::path&path)
{
    if(path.empty())return;
    const auto absolute=std::filesystem::weakly_canonical(path);loadAsset(absolute);
    auto it=std::find(files.begin(),files.end(),absolute);
    if(it==files.end()){files.push_back(absolute);externalFiles.push_back(absolute);it=files.end()-1;}
    select(int(it-files.begin()));status="Mesh loaded. Author its sockets, then Add to test scene.";
}
void PipeLabUI::placeSelected()
{
    if(selected<0)throw std::runtime_error("Select or open a GLB mesh first");
    Instance i;i.asset=files.at(selected);i.pose.position.x=double(instances.size())*1.5;instances.push_back(i);
    sceneSelection=int(instances.size())-1;secondInstance=sceneSelection;secondSocket=0;
    if(instances.size()==1){firstInstance=0;firstSocket=0;}
    scene=true;dirty=true;fitAfterRebuild=true;requestedTab=1;
    status=moduleFor(i).sockets.empty()?"This mesh has no sockets. Use Edit sockets for this module before connecting.":"Choose socket A and B, then Connect B to A.";
}
PipeLabUI::Asset& PipeLabUI::loadAsset(const std::filesystem::path&path)
{
    if(auto it=assets.find(path);it!=assets.end())return it->second;
    Asset asset;asset.mesh=Pipes::loadGlb(path);asset.module=Pipes::loadModule(path);
    if(asset.module.sockets.empty())
    {
        // No sidecar: propose sockets from the mesh itself so the module can be connected and
        // routed. Proposals are named as such and are never written back to a sidecar.
        std::vector<JE::SocketInference::Vec3> points;
        points.reserve(asset.mesh.mesh.mesh.positions.size());
        for(const auto&p:asset.mesh.mesh.mesh.positions)points.push_back({p.x,p.y,p.z});
        const auto inferred=JE::SocketInference::infer(points,asset.mesh.mesh.mesh.indices);
        // The valve's own body reads as a metre-wide ring, so ranking by diameter proposes
        // the envelope instead of the bores. Keep bores (a small fraction of the diagonal)
        // and rank by how circular each rim is.
        double lowX=1e30,lowY=1e30,lowZ=1e30,highX=-1e30,highY=-1e30,highZ=-1e30;
        for(const auto&p:points){lowX=std::min(lowX,p.x);lowY=std::min(lowY,p.y);lowZ=std::min(lowZ,p.z);
            highX=std::max(highX,p.x);highY=std::max(highY,p.y);highZ=std::max(highZ,p.z);}
        const double diagonal=std::sqrt((highX-lowX)*(highX-lowX)+(highY-lowY)*(highY-lowY)+(highZ-lowZ)*(highZ-lowZ));
        std::vector<const JE::SocketInference::SocketCandidate*> ranked;
        for(const auto& socket:inferred.sockets)
            if(socket.diameter>=0.02&&socket.diameter<=0.45*diagonal)ranked.push_back(&socket);
        std::sort(ranked.begin(),ranked.end(),[](const auto*a,const auto*b) {
            return a->radialErrorMeters<b->radialErrorMeters;
        });
        if(ranked.size()>4)ranked.resize(4);
        for(std::size_t index=0;index<ranked.size();++index)
        {
            Pipes::Socket socket;
            socket.name="Proposed "+std::to_string(index+1);
            socket.position={ranked[index]->position.x,ranked[index]->position.y,ranked[index]->position.z};
            socket.diameter=ranked[index]->diameter;
            socket.role=Pipes::Role::Connection;
            socket.capabilities={"Pipe","Proposed"};
            socket.rotation=Pipes::faceDirection({ranked[index]->forward.x,ranked[index]->forward.y,ranked[index]->forward.z});
            asset.module.sockets.push_back(std::move(socket));
        }
        if(!asset.module.sockets.empty())asset.module.moduleType="Proposed sockets";
    }
    return assets.emplace(path,std::move(asset)).first->second;
}
void PipeLabUI::reload()
{
    for(const auto&[path,a]:assets)if(a.edited)throw std::runtime_error("Save sockets before reloading assets");
    auto next=Pipes::discover(directory);
    for(const auto&path:externalFiles)if(std::filesystem::exists(path)&&std::find(next.begin(),next.end(),path)==next.end())next.push_back(path);
    files=std::move(next);assets.clear();instances.clear();pipes.clear();selected=-1;socketIndex=-1;sceneSelection=-1;dirty=true;
    if(!files.empty())select(0);
}
void PipeLabUI::select(int index)
{
    auto&asset=loadAsset(files.at(index));selected=index;socketIndex=asset.module.sockets.empty()?-1:0;dragging=false;dragView=-1;dirty=true;scene=false;
    fitAfterRebuild=true;
}
void PipeLabUI::saveAll()
{
    for(auto&[path,asset]:assets)if(asset.edited){Pipes::saveModule(path,asset.module);asset.edited=false;}
    status="Socket sidecars saved";
}
void PipeLabUI::fit()
{
    if(displayed.mesh.mesh.positions.empty())return;
    auto [low,high]=bounds(displayed.mesh);camera.target={float((low.x+high.x)*.5),float((low.y+high.y)*.5),float((low.z+high.z)*.5)};
    const auto basis=ViewportDetail::basis(camera);double distance=.1;
    for(auto point:displayed.mesh.mesh.positions)
    {
        auto relative=ViewportDetail::sub(point,basis.target);const auto depth=ViewportDetail::dot(relative,basis.forward);
        distance=std::max({distance,2.4*std::abs(ViewportDetail::dot(relative,basis.right))/(viewportAspect*.86)-depth,
            2.4*std::abs(ViewportDetail::dot(relative,basis.up))/.86-depth,.1-depth});
    }
    camera.distance=float(distance);
    fitViews();
}
void PipeLabUI::fitViews()
{
    for(auto&c:orthographic)
    {
        c.target=camera.target;const auto basis=ViewportDetail::basis(c);double span=.1;
        for(auto p:displayed.mesh.mesh.positions){auto d=ViewportDetail::sub(p,basis.target);span=std::max({span,2*std::abs(ViewportDetail::dot(d,basis.right))/viewportAspect,2*std::abs(ViewportDetail::dot(d,basis.up))});}
        c.orthographicHeight=float(span/.82);
    }
}
void PipeLabUI::rebuild(Renderer&renderer)
{
    displayed={};
    if(scene)
    {
        for(const auto&i:instances)Pipes::append(displayed,meshFor(i),i.pose);
    }
    else if(selected>=0)Pipes::append(displayed,loadAsset(files.at(selected)).mesh,{});
    if(!displayed.mesh.mesh.indices.empty())renderer.uploadPipes(displayed);
    else renderer.clearMesh();
    renderStats=renderer.pipeStats();
    if(fitAfterRebuild){fit();fitAfterRebuild=false;}
    dirty=false;
}
std::optional<Pipes::ConnectionCheck> PipeLabUI::checkSelected()
{
    if(firstInstance<0||secondInstance<0||firstInstance>=int(instances.size())||secondInstance>=int(instances.size())||firstInstance==secondInstance)return {};
    const auto&a=instances[firstInstance],&b=instances[secondInstance];
    const auto&sa=moduleFor(a).sockets,&sb=moduleFor(b).sockets;
    if(firstSocket<0||secondSocket<0||firstSocket>=int(sa.size())||secondSocket>=int(sb.size()))return {};
    try{return Pipes::connection(sa[firstSocket],a.pose,sb[secondSocket],b.pose);}
    catch(const std::exception&e){error=e.what();return {};}
}
void PipeLabUI::snapSelected()
{
    if(!checkSelected())throw std::runtime_error("Choose two different instances and valid sockets");
    auto&a=instances[firstInstance];auto&b=instances[secondInstance];
    b.pose=Pipes::snap(moduleFor(a).sockets.at(firstSocket),a.pose,moduleFor(b).sockets.at(secondSocket));
    dirty=true;fitAfterRebuild=true;status="Connected B to A";
}
void PipeLabUI::addStraight()
{
    auto built=Pipes::straight(pipeSettings);pipes.push_back({{std::move(built.mesh),std::move(built.cornerUV)},Pipes::straightModule(pipeSettings)});
    instances.push_back({{}, {},int(pipes.size())-1});sceneSelection=int(instances.size())-1;
    secondInstance=sceneSelection;secondSocket=0;if(instances.size()==1){firstInstance=0;firstSocket=1;}
    scene=true;dirty=true;fitAfterRebuild=true;status="Pipe added with Start / End sockets. Choose A and B in Connect.";
}
void PipeLabUI::showAllModels()
{
    // Build first so a broken asset does not destroy a previous test scene.
    // One asset that is not a static GLB (animated valve, FBX, ...) used to abort the whole
    // gallery, which is why the test scene never appeared. Skip the unusable ones instead.
    std::vector<std::filesystem::path> usable;
    std::string skipped;
    for(const auto&file:files) {
        try { loadAsset(file); usable.push_back(file); }
        catch(const std::exception&e) { if(!skipped.empty())skipped+=", ";skipped+=file.filename().string(); }
    }
    if(usable.empty())throw std::runtime_error("No static GLB module could be loaded"+std::string(skipped.empty()?"":": "+skipped));
    std::vector<Instance> gallery;
    double spacing=1.5;
    for(const auto&file:usable){auto [low,high]=bounds(loadAsset(file).mesh.mesh);spacing=std::max(spacing,std::max(high.x-low.x,high.z-low.z)+.7);}
    for(std::size_t i=0;i<usable.size();++i)
    {
        auto [low,high]=bounds(loadAsset(usable[i]).mesh.mesh);
        Instance instance;instance.asset=usable[i];
        instance.pose.position={double(i%4)*spacing-(low.x+high.x)*.5,-low.y,double(i/4)*spacing-(low.z+high.z)*.5};
        gallery.push_back(instance);
    }
    Pipes::StraightSettings longPipe;longPipe.start={-1.5,0,-2};longPipe.end={18.5,0,-2};longPipe.diameter=.3;longPipe.repeatMeters=1;
    instances=std::move(gallery);pipes.clear();pipeSettings=longPipe;addStraight();view=0;showSockets=true;
    camera.yaw=.35f;camera.pitch=-.65f;status="All "+std::to_string(files.size())+" models + 20 m textured pipe";
}
void PipeLabUI::frame(Renderer&renderer,unsigned width,unsigned height)
{
    using namespace Pipes;
    viewportAspect=std::max(32.f,float(width)-636)/std::max(32.f,float(height)-91);
    auto action=[&](auto function,bool clearError=true){try{function();if(clearError)error.clear();}catch(const std::exception&e){error=e.what();}};
    if(files.empty()&&selected==-1){action([&]{reload();});if(files.empty())selected=-2;}
    constexpr float left=290,right=330,top=54;
    panel("PIPE ASSETS",0,top,left,float(height)-top);
    ImGui::TextUnformatted("models / GLB");
    if(ImGui::Button("Open GLB mesh..."))action([&]{openMesh(chooseMeshFile());});
    if(ImGui::Button("Reload Assets"))action([&]{reload();});
    ImGui::SameLine();if(ImGui::Button("Save sockets"))action([&]{saveAll();});
    ImGui::BeginChild("Asset files",{0,std::min(275.f,float(height)*.32f)},true);
    for(int i=0;i<int(files.size());++i)
    {
        ImGui::PushID(i);const auto label=files[i].filename().string();
        if(ImGui::Selectable(label.c_str(),selected==i))action([&]{select(i);});
        if(ImGui::IsItemHovered())ImGui::SetTooltip("%s\nType: GLB",label.c_str());ImGui::PopID();
    }
    ImGui::EndChild();ImGui::Text("%zu assets",files.size());
    if(ImGui::Button("Test all models + 20 m pipe"))action([&]{showAllModels();});
    if(ImGui::Checkbox("Test scene",&scene)){dirty=true;dragging=false;dragView=-1;fitAfterRebuild=true;}
    if(ImGui::Button("Add selected mesh to scene"))action([&]{placeSelected();});
    if(ImGui::Button("Clear test scene")){instances.clear();pipes.clear();firstInstance=0;secondInstance=1;dirty=true;}
    ImGui::Separator();if(ImGui::Button("Fit"))fit();ImGui::SameLine();ImGui::Checkbox("Sockets",&showSockets);
    if(ImGui::Combo("Layout",&layout,"4 views\0Perspective\0Top (XZ)\0Front (XY)\0Side (ZY)\0"))fitAfterRebuild=dirty=true;
    ImGui::Combo("View",&view,"Textured PBR\0Base color\0Mapped normals\0Metal / roughness\0Wireframe\0UV checker\0Neutral\0");
    ImGui::Text("%zu materials / %zu textures",renderStats.materials,renderStats.textures);
    ImGui::TextWrapped("Place socket: click surface in 3D or a working plane in 2D. Move: drag arrows or center. Rotate: drag rings. Right/middle drag pans; wheel zooms. 3D left drag orbits. Esc cancels a drag.");
    ImGui::TextWrapped("Imported geometry keeps its original scale. Socket units and diameter use that same scale. Test scene is temporary.");
    if(!status.empty())ImGui::TextWrapped("%s",status.c_str());
    if(!error.empty())ImGui::TextColored({1,.35f,.3f,1},"%s",error.c_str());
    ImGui::End();

    panel("SOCKETS / TEST",float(width)-right,top,right,float(height)-top);
    ImGui::PushItemWidth(170);
    const int selectedTab=requestedTab;requestedTab=-1;
    if(ImGui::BeginTabBar("Socket tools"))
    {
        if(ImGui::BeginTabItem("Author",nullptr,selectedTab==0?ImGuiTabItemFlags_SetSelected:0))
        {
            if(selected>=0)
            {
                auto&a=loadAsset(files.at(selected));auto&m=a.module;
                a.edited|=textField("Module type",m.moduleType);
                if(ImGui::Button("Place socket")){tool=2;scene=false;dirty=true;showSockets=true;}
                ImGui::SameLine();if(ImGui::Button("At center")){m.sockets.push_back(newSocket(m));socketIndex=int(m.sockets.size()-1);a.edited=true;tool=0;}
                ImGui::SameLine();ImGui::TextUnformatted(a.edited?"Unsaved":"Saved");
                if(m.sockets.empty())ImGui::TextWrapped("No sockets yet. Place one on each opening using the 2D planes or 3D surface. Blue arrow points out of the pipe.");
                ImGui::BeginChild("Socket list",{0,110},true);
                for(int i=0;i<int(m.sockets.size());++i){ImGui::PushID(i);if(ImGui::Selectable(m.sockets[i].name.c_str(),socketIndex==i))socketIndex=i;ImGui::PopID();}ImGui::EndChild();
                if(socketIndex>=0&&socketIndex<int(m.sockets.size()))
                {
                    auto&s=m.sockets[socketIndex];a.edited|=textField("Name",s.name);
                    a.edited|=ImGui::InputScalarN("Position",ImGuiDataType_Double,&s.position.x,3,nullptr,nullptr,"%.4f");
                    a.edited|=ImGui::InputScalarN("Rotation XYZ",ImGuiDataType_Double,&s.rotation.x,3,nullptr,nullptr,"%.2f");
                    a.edited|=ImGui::InputDouble("Diameter",&s.diameter,.01,.1,"%.4f");
                    ImGui::TextUnformatted("Point socket outward:");
                    const char*faces[]={"+X","-X","+Y","-Y","+Z","-Z"};
                    const Vec3d directions[]={{1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}};
                    for(int k=0;k<6;++k){if(k)ImGui::SameLine();if(ImGui::SmallButton(faces[k])){s.rotation=faceDirection(directions[k]);a.edited=true;}}
                    int role=int(s.role);if(ImGui::Combo("Role",&role,"Connection\0StructuralAnchor\0Effect\0")){s.role=Role(role);a.edited=true;}
                    ImGui::TextUnformatted("Capabilities (multiple)");
                    std::set<std::string> names={"Pipe","Elbow","Tee","Valve","Reducer","BrokenPipe","PipeSupport","Ceiling","Floor","Steam","WaterLeak","Sparks"};
                    for(const auto&socket:m.sockets)names.insert(socket.capabilities.begin(),socket.capabilities.end());
                    ImGui::BeginChild("Capabilities",{0,100},true);
                    for(const auto&name:names)
                    {
                        auto it=std::find(s.capabilities.begin(),s.capabilities.end(),name);bool enabled=it!=s.capabilities.end();
                        if(ImGui::Checkbox(name.c_str(),&enabled)){if(enabled)s.capabilities.push_back(name);else s.capabilities.erase(it);a.edited=true;}
                    }ImGui::EndChild();
                    ImGui::InputText("Custom",newCapability.data(),newCapability.size());
                    if(ImGui::Button("Add capability")&&newCapability[0]){std::string value=newCapability.data();if(std::find(s.capabilities.begin(),s.capabilities.end(),value)==s.capabilities.end()){s.capabilities.push_back(value);a.edited=true;}newCapability[0]=0;}
                    if(ImGui::Button("Duplicate")){auto copy=s;copy.name=newSocket(m).name;m.sockets.push_back(copy);socketIndex=int(m.sockets.size()-1);a.edited=true;}
                    ImGui::SameLine();if(ImGui::Button("Delete"))deleteSelectedSocket();
                }
            }
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Connect",nullptr,selectedTab==1?ImGuiTabItemFlags_SetSelected:0))
        {
            ImGui::TextWrapped("1. Add two meshes or generate pipes.\n2. Pick a socket on A and B.\n3. Connect B to A.");
            ImGui::RadioButton("Pick A",&pickEnd,0);ImGui::SameLine();ImGui::RadioButton("Pick B",&pickEnd,1);
            if(!scene&&ImGui::Button("Show test scene")){scene=true;dirty=true;fitAfterRebuild=true;}
            auto selector=[&](const char*label,int&instance,int&socket)
            {
                ImGui::PushID(label);ImGui::TextUnformatted(label);
                std::string preview=instance>=0&&instance<int(instances.size())?instanceName(instance):"Choose module";
                if(ImGui::BeginCombo("Module",preview.c_str())){for(int i=0;i<int(instances.size());++i){const auto name=instanceName(i);if(ImGui::Selectable(name.c_str(),instance==i)){instance=i;socket=0;sceneSelection=i;}}ImGui::EndCombo();}
                if(instance>=0&&instance<int(instances.size()))
                {
                    auto&i=instances[instance];const auto&sockets=moduleFor(i).sockets;
                    if(ImGui::BeginCombo("Socket",socket>=0&&socket<int(sockets.size())?sockets[socket].name.c_str():"No socket")){for(int j=0;j<int(sockets.size());++j)if(ImGui::Selectable(sockets[j].name.c_str(),socket==j))socket=j;ImGui::EndCombo();}
                    dirty|=ImGui::InputScalarN("Translation",ImGuiDataType_Double,&i.pose.position.x,3,nullptr,nullptr,"%.4f");
                    if(i.generated<0&&ImGui::Button("Edit sockets for this module"))
                    {auto found=std::find(files.begin(),files.end(),i.asset);if(found!=files.end()){select(int(found-files.begin()));requestedTab=0;}}
                    if(sockets.empty())ImGui::TextColored({1,.65f,.25f,1},"Add sockets to this mesh first.");
                }ImGui::PopID();
            };
            selector("A / fixed",firstInstance,firstSocket);selector("B / moving",secondInstance,secondSocket);
            ImGui::BeginDisabled(!checkSelected().has_value());
            if(ImGui::Button("Connect B to A"))action([&]{snapSelected();scene=true;});
            ImGui::EndDisabled();
            action([&]{if(auto check=checkSelected()){const auto color=ImGui::ColorConvertU32ToFloat4(stateColor(check->state));ImGui::TextColored(color,"%s",check->state==ConnectionState::Compatible?"GREEN / Compatible":check->state==ConnectionState::NeedsCorrection?"YELLOW / Needs correction":"RED / Incompatible");ImGui::TextWrapped("%s",check->reason.c_str());ImGui::Text("Offset %.6f | angle %.3f deg",check->positionError,check->angleErrorDegrees);ImGui::Text("Diameter delta %.5f",check->diameterError);}},false);
            ImGui::TextWrapped("Tolerance: position 0.01, angle 5 degrees, diameter 0.001. Snap aligns compatible metadata from any starting position.");
            ImGui::EndTabItem();
        }
        if(ImGui::BeginTabItem("Straight",nullptr,selectedTab==2?ImGuiTabItemFlags_SetSelected:0))
        {
            ImGui::InputScalarN("Start",ImGuiDataType_Double,&pipeSettings.start.x,3,nullptr,nullptr,"%.3f");
            ImGui::InputScalarN("End",ImGuiDataType_Double,&pipeSettings.end.x,3,nullptr,nullptr,"%.3f");
            ImGui::InputDouble("Diameter",&pipeSettings.diameter,.01,.1,"%.3f");
            ImGui::InputDouble("Wall thickness",&pipeSettings.wallThickness,.005,.01,"%.3f");
            ImGui::InputDouble("UV repeat (m)",&pipeSettings.repeatMeters,.1,1,"%.3f");
            int material=view==5?1:0;
            if(ImGui::Combo("Preview material",&material,"Brushed steel\0UV checker\0"))view=material==1?5:0;
            if(ImGui::Button("Endpoints from A / B"))action([&]{
                if(!checkSelected())throw std::runtime_error("Select A / B sockets in Connect");
                const auto&a=instances[firstInstance],&b=instances[secondInstance];const auto&sa=moduleFor(a).sockets.at(firstSocket),&sb=moduleFor(b).sockets.at(secondSocket);
                auto fa=worldFrame(sa,a.pose),fb=worldFrame(sb,b.pose);auto candidate=snap(sa,a.pose,sb);(void)candidate;
                auto delta=ViewportDetail::sub(fb.position,fa.position);const auto distance=std::sqrt(ViewportDetail::dot(delta,delta));
                if(distance<1e-6)throw std::runtime_error("Sockets must be separated for a straight section");
                auto direction=ViewportDetail::mul(delta,1/distance);
                if(ViewportDetail::dot(fa.forward,direction)<std::cos(5*std::numbers::pi/180)||ViewportDetail::dot(fb.forward,direction)>-std::cos(5*std::numbers::pi/180))throw std::runtime_error("Straight endpoints must face each other along their connecting line");
                pipeSettings.start=fa.position;pipeSettings.end=fb.position;pipeSettings.diameter=sa.diameter;
            });
            if(ImGui::Button("Generate straight pipe"))action([&]{addStraight();});
            ImGui::BeginDisabled(selected<0||selected>=int(files.size())||loadAsset(files.at(std::size_t(selected))).module.sockets.size()<2);
            if(ImGui::Button("Route this module's own sockets"))action([&]{
                const auto&module=loadAsset(files.at(std::size_t(selected))).module;
                if(module.sockets.size()<2)throw std::runtime_error("This module needs two sockets");
                Pipes::Pose pose;pose.position={0,0,0};
                const auto fa=worldFrame(module.sockets[0],pose);
                const auto fb=worldFrame(module.sockets[1],pose);
                const auto fit=connect(fa,fb);
                if(fit.state==ConnectorState::Unconnectable)throw std::runtime_error("Route unavailable: "+fit.reason);
                auto settings=pipeSettings;
                settings.diameter=module.sockets[0].diameter;
                settings.start=fit.path.front();settings.end=fit.path.back();
                auto built=routed(fit.path,settings);
                const auto triangles=built.mesh.mesh.mesh.indices.size()/3;
                pipes.push_back({{std::move(built.mesh.mesh),std::move(built.mesh.cornerUV)},std::move(built.module)});
                instances.push_back({{}, {},int(pipes.size())-1});
                sceneSelection=int(instances.size())-1;scene=true;dirty=true;fitAfterRebuild=true;
                status=std::string("Own-socket route: ")+name(fit.state)+", "+std::to_string(fit.lengthMeters)+" m, "
                    +std::to_string(triangles)+" triangles";
            });
            ImGui::EndDisabled();
            if(ImGui::Button("Fit route between A / B"))action([&]{
                if(!checkSelected())throw std::runtime_error("Select A / B sockets in Connect");
                const auto&a=instances.at(firstInstance),&b=instances.at(secondInstance);
                const auto fa=worldFrame(moduleFor(a).sockets.at(firstSocket),a.pose);
                const auto fb=worldFrame(moduleFor(b).sockets.at(secondSocket),b.pose);
                const auto fit=connect(fa,fb);
                if(fit.state==ConnectorState::Unconnectable)throw std::runtime_error("Route unavailable: "+fit.reason);
                auto settings=pipeSettings;
                settings.diameter=moduleFor(a).sockets.at(firstSocket).diameter;
                settings.start=fit.path.front();settings.end=fit.path.back();
                auto built=routed(fit.path,settings);
                const auto triangles=built.mesh.mesh.mesh.indices.size()/3;
                pipes.push_back({{std::move(built.mesh.mesh),std::move(built.mesh.cornerUV)},std::move(built.module)});
                instances.push_back({{}, {},int(pipes.size())-1});
                sceneSelection=int(instances.size())-1;secondInstance=sceneSelection;secondSocket=0;
                if(instances.size()==1){firstInstance=0;firstSocket=1;}
                scene=true;dirty=true;fitAfterRebuild=true;
                status=std::string("Routed pipe added: ")+name(fit.state)+", "+std::to_string(fit.path.size())+" points, "
                    +std::to_string(fit.lengthMeters)+" m, "+std::to_string(triangles)+" triangles";
            });
            ImGui::BeginDisabled(pipes.empty());
            if(ImGui::Button("Break last generated pipe"))action([&]{
                if(pipes.empty())throw std::runtime_error("Generate or route a pipe first");
                const auto fragments=breakPipe(pipes.back().mesh.mesh.mesh,0.5,0.02);
                if(fragments.size()<2)throw std::runtime_error("The break produced fewer than two fragments");
                std::string sizes;
                for(const auto& fragment:fragments)
                {
                    Pipes::ImportedMesh mesh;
                    mesh.mesh.mesh=fragment;
                    // The renderer keeps a per-vertex diagnostic field; a fragment without one
                    // is rejected with "field count must match".
                    mesh.mesh.field.assign(fragment.positions.size(),0.0f);
                    // The renderer also wants one UV per mesh corner; fragments are untextured.
                    mesh.cornerUV.assign(fragment.indices.size(),{0.0f,0.0f});
                    pipes.push_back({std::move(mesh),Pipes::Module{}});
                    instances.push_back({{}, {},int(pipes.size())-1});
                    if(!sizes.empty())sizes+=", ";
                    sizes+=std::to_string(fragment.indices.size()/3);
                }
                dirty=true;fitAfterRebuild=true;scene=true;
                status="Broken into "+std::to_string(fragments.size())+" fragments: "+sizes+" triangles";
            });
            ImGui::EndDisabled();
            ImGui::TextWrapped("Break cuts the last generated pipe in half with the shared damage backend and adds the closed fragments to the scene; the source pipe stays.");
            ImGui::TextWrapped("Fit route draws the pipe between the selected sockets with the fewest elbows; it refuses when the axes cannot be joined.");
            ImGui::TextWrapped("Hollow ring profile. U follows the circumference; V = distance / repeat meters. UV checker uses the same density at every length.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::PopItemWidth();ImGui::End();

    if(dirty)
    {
        try{rebuild(renderer);}catch(const std::exception&e){error=e.what();renderer.clearMesh();displayed={};renderStats={};dirty=false;}
    }
    panel("Pipe viewport",left,top,float(width)-left-right,float(height)-top);
    ImGui::RadioButton("Move",&tool,0);ImGui::SameLine();ImGui::RadioButton("Rotate",&tool,1);
    ImGui::SameLine();ImGui::BeginDisabled(scene);ImGui::RadioButton("Place socket",&tool,2);ImGui::EndDisabled();
    ImGui::SameLine();ImGui::Checkbox("Snap",&gridSnap);ImGui::SameLine();ImGui::SetNextItemWidth(70);
    ImGui::InputDouble("Grid m",&gridStep,0,0,"%.3f");gridStep=std::clamp(gridStep,.001,10.);
    ImGui::SameLine();ImGui::SetNextItemWidth(60);ImGui::InputDouble("Angle",&angleStep,0,0,"%.0f");angleStep=std::clamp(angleStep,1.,90.);
    auto available=ImGui::GetContentRegionAvail();
    if(layout==0)
    {
        const float w=std::max(32.f,(available.x-ImGui::GetStyle().ItemSpacing.x)*.5f),h=std::max(64.f,(available.y-ImGui::GetStyle().ItemSpacing.y)*.5f);
        drawViewport(renderer,0,w,h);ImGui::SameLine();drawViewport(renderer,1,w,h);
        drawViewport(renderer,2,w,h);ImGui::SameLine();drawViewport(renderer,3,w,h);
    }
    else drawViewport(renderer,layout-1,available.x,available.y);
    ImGui::End();
}
void PipeLabUI::evidenceStage(int stage)
{
    using namespace Pipes;
    if(stage==0)
    {
        layout=1;
        reload();if(files.empty())throw std::runtime_error("No models for pipe evidence");
        auto&a=loadAsset(files[0]);a.module={};a.module.moduleType="Authored fixture";
        auto [low,high]=bounds(a.mesh.mesh);
        // A measured fixture on this captured asset, never inferred module metadata.
        auto rimCenter=[&](bool positive){Vec3d center{};std::size_t count=0;const auto extreme=positive?high.x:low.x;
            for(auto p:a.mesh.mesh.mesh.positions)if(std::abs(p.x-extreme)<(high.x-low.x)*.005){center.y+=p.y;center.z+=p.z;++count;}
            center.x=extreme;if(count){center.y/=count;center.z/=count;}return center;};
        Socket s;s.name="A";s.position=rimCenter(false);s.rotation={0,-90,0};s.diameter=.30;s.capabilities={"Pipe","Valve","Custom"};a.module.sockets.push_back(s);
        s.name="B";s.position=rimCenter(true);s.rotation={0,90,0};a.module.sockets.push_back(s);socketIndex=0;scene=false;requestedTab=0;
        a.edited=true;status="Evidence fixture: sockets authored in memory";
    }
    if(stage==1)
    {
        instances={{files[0],{}},{files[0],{}}};firstInstance=0;secondInstance=1;firstSocket=1;secondSocket=0;scene=true;snapSelected();
        requestedTab=1;camera.target[0]=float(instances[1].pose.position.x*.5);camera.distance*=1.5f;
    }
    if(stage==2){instances[1].pose.position.x+=.005;requestedTab=1;}
    if(stage==3)
    {
        auto&socket=loadAsset(instances[1].asset).module.sockets.at(secondSocket);
        socket.role=Role::StructuralAnchor;socket.capabilities={"Ceiling"};
        bool rejected=false;
        try{snapSelected();}catch(const std::exception&e){rejected=true;status=std::string("Snap rejected: ")+e.what();}
        if(!rejected)throw std::runtime_error("Evidence: incompatible anchor was accepted");
        requestedTab=1;
    }
    if(stage==4)
    {
        scene=true;instances.clear();pipes.clear();pipeSettings.start={-1,0,0};pipeSettings.end={-1,0,1};pipeSettings.diameter=.3;pipeSettings.repeatMeters=.5;addStraight();
        pipeSettings.start={1,0,0};pipeSettings.end={1,0,8};addStraight();view=5;
        camera.target={0,0,4};camera.distance=12;camera.yaw=.9f;camera.pitch=-.65f;
        requestedTab=2;status="1 m and 8 m pipes: same 0.5 m UV repeat";
    }
    dirty=true;
}
void PipeLabUI::textureEvidenceStage(int stage)
{
    layout=1;
    if(files.empty())reload();
    if(stage<int(files.size())){select(stage);view=0;status=files.at(stage).filename().string();}
    else if(stage==int(files.size()))showAllModels();
    else if(stage==int(files.size())+1){view=5;status="Same 20 m pipe / metric UV diagnostic";}
    else if(stage==int(files.size())+2){select(0);view=1;status="Base color / original UV diagnostic";}
    else {view=2;status="Tangent normal / original UV diagnostic";}
    requestedTab=0;dirty=true;
}
void PipeLabUI::deleteSelectedSocket()
{
    auto&a=loadAsset(files.at(selected));
    if(socketIndex<0||socketIndex>=int(a.module.sockets.size()))return;
    a.module.sockets.erase(a.module.sockets.begin()+socketIndex);
    socketIndex=a.module.sockets.empty()?-1:0;a.edited=true;
    auto usesEditedModule=[&](int index)
    {
        return index>=0&&index<int(instances.size())&&instances[index].generated<0&&
               instances[index].asset==files[selected];
    };
    if(usesEditedModule(firstInstance))firstSocket=-1;
    if(usesEditedModule(secondInstance))secondSocket=-1;
    dragging=false;dragView=-1;
}
void PipeLabUI::editorEvidenceStage(int stage)
{
    if(stage==0){evidenceStage(0);layout=0;fitAfterRebuild=true;status="Four independent views / authored socket fixture";}
    if(stage==1)
    {
        auto&s=loadAsset(files[0]).module.sockets[0];s.position.y+=.1;
        s.rotation=eulerAngles(rotateWorld(quaternion(s.rotation),2,30));tool=1;
        status="Socket translation and world rotation visible in all four views";
    }
    if(stage==2)
    {
        instances.clear();pipes.clear();pipeSettings={};pipeSettings.end={0,0,2};addStraight();addStraight();
        firstInstance=0;firstSocket=1;secondInstance=1;secondSocket=0;snapSelected();requestedTab=1;tool=0;
        if(!checkSelected()||checkSelected()->state!=ConnectionState::Compatible)throw std::runtime_error("Procedural pipe to pipe failed");
    }
    if(stage==3)
    {
        openMesh(std::filesystem::path(PLAYGROUND_ROOT)/"Evidence/pipe-editor/external fixture.glb");
        loadAsset(files[selected]).module=loadAsset(files[0]).module;requestedTab=0;status="Loaded another GLB from outside models";
    }
    if(stage==4)
    {
        instances.clear();pipes.clear();placeSelected();pipeSettings={};pipeSettings.end={0,0,20};pipeSettings.diameter=.3;addStraight();
        firstInstance=0;firstSocket=1;secondInstance=1;secondSocket=0;snapSelected();requestedTab=1;
        if(!checkSelected()||checkSelected()->state!=ConnectionState::Compatible)throw std::runtime_error("Imported mesh to long pipe failed");
    }
    if(stage==5)
    {
        moduleFor(instances[1]).sockets[0].diameter=.7;bool rejected=false;
        try{snapSelected();}catch(const std::exception&){rejected=true;}
        if(!rejected)throw std::runtime_error("Mismatched generated endpoint accepted");
        status="Rejected incompatible diameter without moving pipe";requestedTab=1;
    }
    if(stage==6)
    {
        auto&m=loadAsset(files[selected]).module;
        auto extra=m.sockets.back();extra.name="Extra";m.sockets.push_back(extra);
        firstSocket=1;socketIndex=0;deleteSelectedSocket();
        if(firstSocket!=-1)throw std::runtime_error("Deleted socket left A pointing to another endpoint");
        status="Deleted socket clears A selection; select an endpoint again";requestedTab=1;
    }
    dirty=true;
}
void PipeLabUI::writeEvidence(const std::filesystem::path&path,const std::string&adapter)
{
    if(!error.empty())throw std::runtime_error(error);
    nlohmann::json j={{"adapter",adapter},{"assetCount",files.size()},{"instances",std::count_if(instances.begin(),instances.end(),[](const auto&i){return i.generated<0;})},{"proceduralPipes",pipes.size()},
        {"sceneInstances",instances.size()},{"layout",layout},
        {"triangles",displayed.mesh.mesh.indices.size()/3},{"status",status},{"view",view},
        {"gpuMaterials",renderStats.materials},{"gpuTextures",renderStats.textures},{"textureBytes",renderStats.textureBytes},{"draws",renderStats.draws},
        {"uploadMilliseconds",renderStats.uploadMilliseconds}};
    j["materials"]=nlohmann::json::array();
    std::set<const Pipes::PipeMaterial*> recorded;
    for(const auto&range:displayed.materials)if(recorded.insert(range.material.get()).second)
    {
        const auto&m=*range.material;nlohmann::json entry={{"name",m.name},{"metallicFactor",m.metallicFactor},{"roughnessFactor",m.roughnessFactor}};
        const char*names[]={"baseColor","normal","metallicRoughness","occlusion","emissive"};int index=0;
        for(const auto&image:{m.baseColor,m.normal,m.metallicRoughness,m.occlusion,m.emissive}){if(image)entry[names[index]]={{"width",image->width},{"height",image->height},{"bytes",image->encoded.size()},{"wrapS",image->wrapS},{"wrapT",image->wrapT}};++index;}
        j["materials"].push_back(entry);
    }
    if(auto check=checkSelected())j["connection"]={{"state",int(check->state)},{"reason",check->reason},{"positionError",check->positionError},{"angleErrorDegrees",check->angleErrorDegrees}};
    std::ofstream(path)<<j.dump(2)<<'\n';
}
}
