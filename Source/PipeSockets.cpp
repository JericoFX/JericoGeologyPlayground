#include "PipeSockets.hpp"
#include "GeometryAnalysis.hpp"
#include "SolidGeometry.hpp"
#include <nlohmann/json.hpp>
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <numbers>
#include <set>

namespace Playground::Pipes
{
namespace
{
using Json=nlohmann::json;
constexpr double pi=std::numbers::pi;
Vec3d add(Vec3d a,Vec3d b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3d sub(Vec3d a,Vec3d b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
double length(Vec3d p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
double dot(Vec3d a,Vec3d b){return a.x*b.x+a.y*b.y+a.z*b.z;}
bool finite(Vec3d p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
Quaternion multiply(Quaternion a,Quaternion b)
{
    return {a[3]*b[0]+a[0]*b[3]+a[1]*b[2]-a[2]*b[1],
        a[3]*b[1]-a[0]*b[2]+a[1]*b[3]+a[2]*b[0],
        a[3]*b[2]+a[0]*b[1]-a[1]*b[0]+a[2]*b[3],
        a[3]*b[3]-a[0]*b[0]-a[1]*b[1]-a[2]*b[2]};
}
Quaternion inverse(Quaternion q){return {-q[0],-q[1],-q[2],q[3]};}
void validatePose(const Pose& p)
{
    double sum=0;for(auto x:p.rotation)sum+=x*x;
    require(finite(p.position)&&std::isfinite(sum)&&std::abs(sum-1)<1e-6,"Invalid rigid pose");
}
std::string mismatch(const Socket&a,const Socket&b,double tolerance)
{
    validate(a);validate(b);
    if(a.role!=b.role)return "Socket roles differ";
    if(a.role==Role::Effect)return "Effect sockets are markers, not physical connections";
    if(std::abs(a.diameter-b.diameter)>tolerance)return "Diameters differ";
    for(const auto&c:a.capabilities)
        if(std::find(b.capabilities.begin(),b.capabilities.end(),c)!=b.capabilities.end())return {};
    return "No shared capability";
}
std::filesystem::path sidecar(std::filesystem::path asset){asset.replace_extension(".sockets.json");return asset;}
Json vectorJson(Vec3d p){return Json::array({p.x,p.y,p.z});}
Vec3d vectorFrom(const Json& j){auto a=j.get<std::array<double,3>>();return {a[0],a[1],a[2]};}
void validateModule(const Module&m)
{
    require(!m.moduleType.empty(),"Module type cannot be empty");
    std::set<std::string> names;
    for(const auto&s:m.sockets){validate(s);require(names.insert(s.name).second,"Socket names must be unique");}
}
}
Quaternion quaternion(Vec3d degrees)
{
    require(finite(degrees),"Non-finite rotation");
    const double x=degrees.x*pi/360,y=degrees.y*pi/360,z=degrees.z*pi/360;
    return multiply(multiply({0,0,std::sin(z),std::cos(z)},{0,std::sin(y),0,std::cos(y)}),
                    {std::sin(x),0,0,std::cos(x)});
}
Vec3d rotate(Quaternion q,Vec3d p)
{
    auto r=multiply(multiply(q,{p.x,p.y,p.z,0}),inverse(q));return {r[0],r[1],r[2]};
}
Vec3d eulerAngles(Quaternion q)
{
    auto x=rotate(q,{1,0,0}),y=rotate(q,{0,1,0}),z=rotate(q,{0,0,1});
    const auto pitch=std::asin(std::clamp(-x.z,-1.,1.));
    if(std::abs(std::cos(pitch))<1e-7)return {std::atan2(-z.y,y.y)*180/pi,pitch*180/pi,0};
    return {std::atan2(y.z,z.z)*180/pi,pitch*180/pi,std::atan2(x.y,x.x)*180/pi};
}
Quaternion rotateWorld(Quaternion q,int axis,double degrees)
{
    require(axis>=0&&axis<3&&std::isfinite(degrees),"Invalid rotation gizmo axis");
    Vec3d angles{};if(axis==0)angles.x=degrees;else if(axis==1)angles.y=degrees;else angles.z=degrees;
    return multiply(quaternion(angles),q);
}
Vec3d faceDirection(Vec3d forward)
{
    const auto n=length(forward);require(finite(forward)&&n>1e-10,"Invalid socket direction");
    return {-std::asin(std::clamp(forward.y/n,-1.,1.))*180/pi,std::atan2(forward.x,forward.z)*180/pi,0};
}
Vec3d transform(const Pose&p,Vec3d v){return add(p.position,rotate(p.rotation,v));}
Frame worldFrame(const Socket&s,const Pose&p)
{
    validate(s);validatePose(p);
    const auto rotation=multiply(p.rotation,quaternion(s.rotation));
    return {transform(p,s.position),rotate(rotation,{0,0,1}),rotate(rotation,{0,1,0})};
}
void validate(const Socket&s)
{
    require(!s.name.empty()&&finite(s.position)&&finite(s.rotation)&&std::isfinite(s.diameter)&&s.diameter>0,
            "Socket needs a name, finite transform and positive diameter");
    require(int(s.role)>=0&&int(s.role)<=2,"Unknown socket role");
    std::set<std::string> capabilities;
    for(const auto&c:s.capabilities)require(!c.empty()&&capabilities.insert(c).second,"Capabilities must be nonempty and unique");
}
std::vector<std::filesystem::path> discover(const std::filesystem::path& directory)
{
    std::vector<std::filesystem::path> result;
    if(!std::filesystem::exists(directory))return result;
    for(const auto&entry:std::filesystem::directory_iterator(directory))
    {
        auto ext=entry.path().extension().string();
        std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return char(std::tolower(c));});
        if(entry.is_regular_file()&&ext==".glb")result.push_back(entry.path());
    }
    std::sort(result.begin(),result.end());return result;
}
Module loadModule(const std::filesystem::path& asset)
{
    auto path=sidecar(asset);if(!std::filesystem::exists(path))return {};
    Json json;std::ifstream input(path);require(bool(input),"Cannot open socket sidecar");input>>json;
    require(json.at("version")==1,"Unsupported socket sidecar version");
    require(json.at("asset")==asset.filename().string(),"Socket sidecar asset mismatch");
    Module m;m.moduleType=json.at("moduleType").get<std::string>();
    for(const auto&j:json.at("sockets"))
    {
        Socket s;s.name=j.at("name");s.position=vectorFrom(j.at("position"));s.rotation=vectorFrom(j.at("rotation"));
        s.diameter=j.at("diameter");s.capabilities=j.at("capabilities").get<std::vector<std::string>>();
        const std::string role=j.at("role");
        require(role=="Connection"||role=="StructuralAnchor"||role=="Effect","Unknown socket role");
        s.role=role=="Connection"?Role::Connection:role=="StructuralAnchor"?Role::StructuralAnchor:Role::Effect;
        m.sockets.push_back(std::move(s));
    }
    validateModule(m);return m;
}
void saveModule(const std::filesystem::path& asset,const Module&m)
{
    validateModule(m);
    Json json={{"version",1},{"asset",asset.filename().string()},{"moduleType",m.moduleType},{"sockets",Json::array()}};
    for(const auto&s:m.sockets)
    {
        const auto f=worldFrame(s,{});
        json["sockets"].push_back({{"name",s.name},{"position",vectorJson(s.position)},{"rotation",vectorJson(s.rotation)},
            {"forward",vectorJson(f.forward)},{"up",vectorJson(f.up)},{"diameter",s.diameter},
            {"role",s.role==Role::Connection?"Connection":s.role==Role::StructuralAnchor?"StructuralAnchor":"Effect"},
            {"capabilities",s.capabilities}});
    }
    auto path=sidecar(asset),temporary=path;temporary+=".tmp";
    {std::ofstream out(temporary,std::ios::binary|std::ios::trunc);require(bool(out),"Cannot write socket sidecar");out<<json.dump(2)<<'\n';out.flush();require(bool(out),"Socket sidecar write failed");}
    require(MoveFileExW(temporary.c_str(),path.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=0,
            "Cannot replace socket sidecar; temporary file retained");
}
Pose snap(const Socket&a,const Pose&fixed,const Socket&b)
{
    validatePose(fixed);
    const auto reason=mismatch(a,b,.001);if(!reason.empty())throw std::runtime_error(reason);
    // Match up vectors and oppose forwards by a local 180-degree turn around Y.
    Pose result;result.rotation=multiply(multiply(multiply(fixed.rotation,quaternion(a.rotation)),{0,1,0,0}),inverse(quaternion(b.rotation)));
    result.position=sub(transform(fixed,a.position),rotate(result.rotation,b.position));return result;
}
ConnectionCheck connection(const Socket&a,const Pose&pa,const Socket&b,const Pose&pb,
                           double positionTolerance,double angleTolerance,double diameterTolerance)
{
    require(std::isfinite(positionTolerance)&&positionTolerance>0&&std::isfinite(angleTolerance)&&angleTolerance>0&&
            std::isfinite(diameterTolerance)&&diameterTolerance>=0,"Invalid connection tolerance");
    ConnectionCheck result;result.reason=mismatch(a,b,diameterTolerance);
    auto fa=worldFrame(a,pa),fb=worldFrame(b,pb);
    result.positionError=length(sub(fa.position,fb.position));result.diameterError=std::abs(a.diameter-b.diameter);
    result.angleErrorDegrees=std::max(std::acos(std::clamp(-dot(fa.forward,fb.forward),-1.,1.)),
                                    std::acos(std::clamp(dot(fa.up,fb.up),-1.,1.)))*180/pi;
    if(!result.reason.empty())return result;
    if(result.positionError>positionTolerance||result.angleErrorDegrees>angleTolerance)
        result.reason="Outside position/orientation tolerance; Snap can align compatible sockets";
    else if(result.positionError>1e-6||result.angleErrorDegrees>1e-4)
    {result.state=ConnectionState::NeedsCorrection;result.reason="Within tolerance; needs alignment";}
    else {result.state=ConnectionState::Compatible;result.reason="Connected: position, orientation, diameter and capabilities match";}
    return result;
}
Construction::ConstructionResult straight(const StraightSettings&s)
{
    require(finite(s.start)&&finite(s.end)&&length(sub(s.end,s.start))>1e-6,"Straight pipe needs two distinct finite endpoints");
    require(std::isfinite(s.diameter)&&s.diameter>0&&std::isfinite(s.wallThickness)&&s.wallThickness>0&&
        s.wallThickness<s.diameter*.5&&std::isfinite(s.repeatMeters)&&s.repeatMeters>0,"Invalid straight pipe dimensions or UV repeat");
    PA::ProceduralSpline spline;spline.splineGuid=Solid::region(4501,1);spline.name="Socket straight pipe";spline.interpolation=PA::SplineInterpolation::Polyline;
    for(auto p:{s.start,s.end}){PA::SplineControlPoint point;point.pointGuid=Solid::region(4501,unsigned(spline.points.size()+2));point.position=p;spline.points.push_back(point);}
    Construction::ConstructionSettings settings;settings.profile=Construction::ProfileKind::Ring;
    settings.radiusMeters=s.diameter*.5;settings.wallThicknessMeters=s.wallThickness;
    settings.metresPerTileU=s.repeatMeters;settings.metresPerTileV=pi*s.diameter;
    settings.pathQuality.maximumStepMeters=std::clamp(length(sub(s.end,s.start)),.25,10.);
    auto result=Construction::buildConstruction(spline,settings);
    // The existing sweep uses U along the path. Socket pipes expose V along it.
    for(auto&uv:result.cornerUV)std::swap(uv[0],uv[1]);
    for(auto&uv:result.vertexUV)std::swap(uv[0],uv[1]);
    return result;
}
RoutedResult routed(const std::vector<Vec3d>& path,const StraightSettings& s)
{
    require(path.size()>=2,"A routed pipe needs at least two points");
    require(std::isfinite(s.diameter)&&s.diameter>0&&std::isfinite(s.wallThickness)&&s.wallThickness>0&&
        s.wallThickness<s.diameter*.5&&std::isfinite(s.repeatMeters)&&s.repeatMeters>0,"Invalid routed pipe dimensions or UV repeat");
    double total=0;
    for(std::size_t index=1;index<path.size();++index) {
        require(finite(path[index]),"Routed pipe point is not finite");
        total+=length(sub(path[index],path[index-1]));
    }
    require(total>1e-6,"Routed pipe has no length");
    PA::ProceduralSpline spline;spline.splineGuid=Solid::region(4501,1);spline.name="Socket routed pipe";
    spline.interpolation=PA::SplineInterpolation::Polyline;
    for(const auto&p:path){PA::SplineControlPoint point;point.pointGuid=Solid::region(4501,unsigned(spline.points.size()+2));point.position=p;spline.points.push_back(point);}
    Construction::ConstructionSettings settings;settings.profile=Construction::ProfileKind::Ring;
    settings.radiusMeters=s.diameter*.5;settings.wallThicknessMeters=s.wallThickness;
    settings.metresPerTileU=s.repeatMeters;settings.metresPerTileV=pi*s.diameter;
    settings.pathQuality.maximumStepMeters=.25;
    auto result=Construction::buildConstruction(spline,settings);
    for(auto&uv:result.cornerUV)std::swap(uv[0],uv[1]);
    for(auto&uv:result.vertexUV)std::swap(uv[0],uv[1]);
    RoutedResult routedResult;
    routedResult.mesh=std::move(result);
    const auto at=s.diameter*.5;
    Socket startSocket;startSocket.name="Socket 1";startSocket.position=path.front();
    startSocket.diameter=s.diameter;startSocket.role=Role::Connection;
    const auto normalizeVector=[](Vec3d v){const double l=length(v);return l>1e-12?Vec3d{v.x/l,v.y/l,v.z/l}:Vec3d{};};
    const auto startDirection=normalizeVector(sub(path[1],path.front()));
    startSocket.rotation=faceDirection(startDirection);
    Socket endSocket;endSocket.name="Socket 2";endSocket.position=path.back();
    endSocket.diameter=s.diameter;endSocket.role=Role::Connection;
    const auto endDirection=normalizeVector(sub(path[path.size()-1],path[path.size()-2]));
    endSocket.rotation=faceDirection(endDirection);
    routedResult.module.moduleType="Routed";
    routedResult.module.sockets={startSocket,endSocket};
    (void)at;
    return routedResult;
}
std::vector<TriangleMesh> breakPipe(const TriangleMesh& pipe,double fraction,
                                                double knifeThicknessMeters,std::size_t maximumFragments)
{
    require(!pipe.positions.empty()&&!pipe.indices.empty(),"Breaking a pipe needs geometry");
    require(fraction>0.0&&fraction<1.0,"The break fraction must be inside the pipe");
    require(std::isfinite(knifeThicknessMeters)&&knifeThicknessMeters>0,"The knife needs a thickness");
    // Weld first: the sweep leaves duplicate vertices at caps and seams.
    TriangleMesh welded;
    std::map<std::tuple<long long,long long,long long>,std::uint32_t> lookup;
    std::vector<std::uint32_t> remap(pipe.positions.size());
    for(std::size_t index=0;index<pipe.positions.size();++index) {
        const auto&p=pipe.positions[index];
        const auto key=std::make_tuple(std::llround(p.x*100000.0),std::llround(p.y*100000.0),std::llround(p.z*100000.0));
        auto found=lookup.find(key);
        if(found==lookup.end()) {
            found=lookup.emplace(key,std::uint32_t(welded.positions.size())).first;
            welded.positions.push_back(p);
        }
        remap[index]=found->second;
    }
    for(const auto index:pipe.indices)welded.indices.push_back(remap[index]);
    // The damage backend builds its Manifold input from a per-triangle region map; an empty
    // one makes Manifold reject the mesh as not manifold.
    Jerico::Guid pipeRegion=Solid::region(4501,3);
    welded.triangleRegions.assign(welded.indices.size()/3,pipeRegion);

    Vec3d low=welded.positions.front(),high=low;
    for(const auto&p:welded.positions){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};
        high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
    const Vec3d extent{high.x-low.x,high.y-low.y,high.z-low.z};
    const int axis=extent.x>=extent.y?(extent.x>=extent.z?0:2):(extent.y>=extent.z?1:2);
    const double station=(axis==0?low.x:axis==1?low.y:low.z)+(axis==0?extent.x:axis==1?extent.y:extent.z)*fraction;
    Vec3d axisU{0,0,0},axisV{0,0,0};
    double halfU=0,halfV=0;
    if(axis==0){axisU={0,(extent.y*0.75)+0.05,0};axisV={0,0,(extent.z*0.75)+0.05};halfU=extent.y;halfV=extent.z;}
    else if(axis==1){axisU={(extent.x*0.75)+0.05,0,0};axisV={0,0,(extent.z*0.75)+0.05};halfU=extent.x;halfV=extent.z;}
    else {axisU={(extent.x*0.75)+0.05,0,0};axisV={0,(extent.y*0.75)+0.05,0};halfU=extent.x;halfV=extent.y;}
    (void)halfU;(void)halfV;
    const double halfThickness=knifeThicknessMeters*0.5;
    Vec3d centre{low.x+extent.x*0.5,low.y+extent.y*0.5,low.z+extent.z*0.5};
    if(axis==0)centre.x=station;else if(axis==1)centre.y=station;else centre.z=station;
    std::vector<DamageSweepSection> sections(2);
    for(int side=0;side<2;++side) {
        Vec3d offset{centre.x,centre.y,centre.z};
        const double shift=(side==0?-halfThickness:halfThickness);
        if(axis==0)offset.x+=shift;else if(axis==1)offset.y+=shift;else offset.z+=shift;
        sections[std::size_t(side)]={offset,axisU,axisV};
    }
    const auto knife=buildDamageSweepVolume(sections,pipeRegion);
    const auto cut=subtractDamageVolume(welded,knife,240000);

    // Split the result into connected components; each is a fragment.
    std::vector<JE::GeometryAnalysis::Vec3> points;
    for(const auto&p:cut.positions)points.push_back({p.x,p.y,p.z});
    JE::GeometryAnalysis::Settings settings;
    settings.maximumVertices=1000000;
    settings.maximumTriangles=1000000;
    const auto analysis=JE::GeometryAnalysis::analyze({points,cut.indices},settings);
    std::vector<std::size_t> componentOf(points.size(),SIZE_MAX);
    for(std::size_t index=0;index<analysis.components.size();++index)
        for(const auto vertex:analysis.components[index].vertices)componentOf[vertex]=index;
    // Vertices stay shared inside a fragment: duplicating them per triangle would leave every
    // edge used once and the fragment would not be a closed solid.
    std::vector<std::map<std::uint32_t,std::uint32_t>> vertexMap(analysis.components.size());
    std::vector<TriangleMesh> fragments(analysis.components.size());
    for(std::size_t triangle=0;triangle+2<cut.indices.size();triangle+=3) {
        const auto a=cut.indices[triangle],b=cut.indices[triangle+1],c=cut.indices[triangle+2];
        const auto component=componentOf[a];
        if(component==SIZE_MAX||componentOf[b]!=component||componentOf[c]!=component)continue;
        auto& fragment=fragments[component];
        auto& mapping=vertexMap[component];
        auto remapVertex=[&](std::uint32_t vertex) {
            auto found=mapping.find(vertex);
            if(found!=mapping.end())return found->second;
            const auto next=std::uint32_t(fragment.positions.size());
            mapping.emplace(vertex,next);
            fragment.positions.push_back(cut.positions[vertex]);
            return next;
        };
        fragment.indices.push_back(remapVertex(a));
        fragment.indices.push_back(remapVertex(b));
        fragment.indices.push_back(remapVertex(c));
    }
    std::vector<TriangleMesh> result;
    for(auto& fragment:fragments)if(fragment.indices.size()>=3*20)result.push_back(std::move(fragment));
    std::sort(result.begin(),result.end(),[](const auto&a,const auto&b){return a.indices.size()>b.indices.size();});
    if(result.size()>maximumFragments)result.resize(maximumFragments);
    return result;
}
void append(ImportedMesh& out,const ImportedMesh& input,const Pose&pose)
{
    validatePose(pose);const auto offset=std::uint32_t(out.mesh.mesh.positions.size());
    const auto indexOffset=std::uint32_t(out.mesh.mesh.indices.size());
    for(auto p:input.mesh.mesh.positions)out.mesh.mesh.positions.push_back(transform(pose,p));
    for(auto n:input.mesh.normals)out.mesh.normals.push_back(rotate(pose.rotation,n));
    for(auto n:input.mesh.cornerNormals)out.mesh.cornerNormals.push_back(rotate(pose.rotation,n));
    for(auto i:input.mesh.mesh.indices)out.mesh.mesh.indices.push_back(offset+i);
    out.mesh.field.insert(out.mesh.field.end(),input.mesh.field.begin(),input.mesh.field.end());
    out.cornerUV.insert(out.cornerUV.end(),input.cornerUV.begin(),input.cornerUV.end());
    const auto cornerCount=input.mesh.mesh.indices.size();
    if(!out.cornerUV1.empty()||!input.cornerUV1.empty()){
        require(out.cornerUV1.empty()||out.cornerUV1.size()==indexOffset,"Invalid destination UV1 count");
        require(input.cornerUV1.empty()||input.cornerUV1.size()==cornerCount,"Invalid source UV1 count");
        if(out.cornerUV1.empty())out.cornerUV1.resize(indexOffset);
        if(input.cornerUV1.empty())out.cornerUV1.resize(indexOffset+cornerCount);
        else out.cornerUV1.insert(out.cornerUV1.end(),input.cornerUV1.begin(),input.cornerUV1.end());
    }
    if(!out.cornerTangents.empty()||!input.cornerTangents.empty()){
        require(out.cornerTangents.empty()||out.cornerTangents.size()==indexOffset,"Invalid destination tangent count");
        require(input.cornerTangents.empty()||input.cornerTangents.size()==cornerCount,"Invalid source tangent count");
        if(out.cornerTangents.empty())out.cornerTangents.resize(indexOffset,{0,0,0,1});
        if(input.cornerTangents.empty())out.cornerTangents.resize(indexOffset+cornerCount,{0,0,0,1});
        else for(auto t:input.cornerTangents){auto rotated=rotate(pose.rotation,{t[0],t[1],t[2]});out.cornerTangents.push_back({float(rotated.x),float(rotated.y),float(rotated.z),t[3]});}
    }
    for(auto part:input.parts){part.firstIndex+=indexOffset;out.parts.push_back(std::move(part));}
    if(input.materials.empty()&&!input.mesh.mesh.indices.empty())
        out.materials.push_back({indexOffset,std::uint32_t(input.mesh.mesh.indices.size()),straightMaterial()});
    for(auto range:input.materials){range.firstIndex+=indexOffset;out.materials.push_back(std::move(range));}
}
Module straightModule(const StraightSettings&s)
{
    const auto direction=sub(s.end,s.start);
    require(finite(s.start)&&finite(s.end)&&length(direction)>1e-6&&std::isfinite(s.diameter)&&s.diameter>0,"Invalid procedural endpoints");
    Module m;m.moduleType="Straight pipe";
    Socket a;a.name="Start";a.position=s.start;a.diameter=s.diameter;
    a.rotation=faceDirection({-direction.x,-direction.y,-direction.z});
    Socket b=a;b.name="End";b.position=s.end;b.rotation=faceDirection(direction);m.sockets={a,b};return m;
}
std::shared_ptr<const PipeMaterial> straightMaterial()
{
    static const auto material=[]
    {
        auto m=std::make_shared<PipeMaterial>();m->name="Brushed steel / metric UV";m->metallicFactor=.85f;m->roughnessFactor=.48f;m->doubleSided=true;
        auto image=std::make_shared<TextureImage>();image->name="Periodic brushed steel";image->width=image->height=256;image->rgba.resize(256*256*4);
        for(unsigned y=0;y<256;++y)for(unsigned x=0;x<256;++x)
        {
            const double u=2*pi*x/256,v=2*pi*y/256;
            const auto value=std::uint8_t(std::clamp(146+10*std::sin(u*31)+5*std::sin(v*3+std::sin(u*7))+3*std::sin(v*61),0.,255.));
            const auto i=4*(y*256+x);image->rgba[i]=value;image->rgba[i+1]=value;image->rgba[i+2]=std::uint8_t(value+6);image->rgba[i+3]=255;
        }
        m->baseColor=image;return std::shared_ptr<const PipeMaterial>(m);
    }();return material;
}
}
