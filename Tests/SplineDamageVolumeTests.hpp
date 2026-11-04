#pragma once
#include "DamageGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <stdexcept>

namespace SplineDamageVolumeTests
{
using namespace Playground;
inline void require(bool value,const char *message)
{if(!value)throw std::runtime_error(message);}
inline Vec3d sub(Vec3d a,Vec3d b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3d mul(Vec3d a,double k){return {a.x*k,a.y*k,a.z*k};}
inline double dot(Vec3d a,Vec3d b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3d cross(Vec3d a,Vec3d b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline Vec3d unit(Vec3d a){return mul(a,1/std::sqrt(dot(a,a)));}
inline double volume(const TriangleMesh &mesh)
{
    double result=0;
    for(std::size_t i=0;i<mesh.indices.size();i+=3)
        result+=dot(mesh.positions[mesh.indices[i]],cross(mesh.positions[mesh.indices[i+1]],mesh.positions[mesh.indices[i+2]]))/6;
    return result;
}
inline TriangleMesh cube()
{
    TriangleMesh mesh;
    mesh.positions={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
                    {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    mesh.indices={0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,
                  3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5};
    mesh.triangleRegions.assign(12,Solid::region(9107,1));return mesh;
}
inline bool connected(const TriangleMesh &mesh)
{
    if(mesh.indices.empty())return false;
    std::vector<std::vector<std::uint32_t>> adjacent(mesh.positions.size());
    for(std::size_t i=0;i<mesh.indices.size();i+=3)
        for(unsigned c=0;c<3;++c){auto a=mesh.indices[i+c],b=mesh.indices[i+(c+1)%3];adjacent[a].push_back(b);adjacent[b].push_back(a);}
    std::vector<bool> seen(mesh.positions.size());std::vector<std::uint32_t> pending{mesh.indices.front()};seen[pending.front()]=true;
    while(!pending.empty()){auto a=pending.back();pending.pop_back();for(auto b:adjacent[a])if(!seen[b]){seen[b]=true;pending.push_back(b);}}
    const bool all=std::all_of(mesh.indices.begin(),mesh.indices.end(),[&](auto vertex){return seen[vertex];});
    if(!all)
    {
        double minimum=std::numeric_limits<double>::infinity();Vec3d first{},second{};
        for(auto a:mesh.indices)if(seen[a])for(auto b:mesh.indices)if(!seen[b])
        {
            const auto p=mesh.positions[a],q=mesh.positions[b];const auto distance=std::hypot(p.x-q.x,p.y-q.y,p.z-q.z);
            if(distance<minimum){minimum=distance;first=p;second=q;}
        }
        std::cerr<<std::setprecision(17)<<"Disconnected sweep diagnostic: vertices="<<mesh.positions.size()<<" triangles="<<mesh.indices.size()/3
            <<" first component="<<std::count(seen.begin(),seen.end(),true)<<" closest vertices="<<minimum
            <<" a="<<first.x<<','<<first.y<<','<<first.z<<" b="<<second.x<<','<<second.y<<','<<second.z<<'\n';
        std::vector<int> component(mesh.positions.size(),-1);int count=0;
        for(auto start:mesh.indices)if(component[start]<0){pending={start};component[start]=count;
            while(!pending.empty()){auto a=pending.back();pending.pop_back();for(auto b:adjacent[a])if(component[b]<0){component[b]=count;pending.push_back(b);}}++count;}
        for(int id=0;id<count;++id){TriangleMesh part;part.positions=mesh.positions;
            for(std::size_t face=0;face<mesh.indices.size();face+=3)if(component[mesh.indices[face]]==id)
                part.indices.insert(part.indices.end(),mesh.indices.begin()+face,mesh.indices.begin()+face+3);
            std::cerr<<"component "<<id<<" vertices="<<std::count(component.begin(),component.end(),id)<<" triangles="<<part.indices.size()/3<<" signed-volume="<<volume(part)<<'\n';}
    }
    return all;
}
inline void run()
{
    const auto host=cube();const auto original=meshHash(host);const auto fresh=Solid::region(9107,2);
    require(PA::validateClosedMesh(host).hasValue()&&std::abs(volume(host)-8)<1e-12,"spatial damage fixture is a real closed eight-vertex cube");
    const std::array<Vec3d,5> path{{{-.7,1,.3},{.7,1,.3},{1,.7,.3},{1,0,.8},{1,-.7,-.3}}};
    require(std::abs(dot(cross(sub(path[1],path[0]),sub(path[2],path[0])),sub(path[3],path[0])))>.1,"spatial fixture must not be coplanar");
    std::vector<DamageSweepSection> sections;
    for(std::size_t i=0;i<path.size();++i)
    {
        const auto tangent=unit(sub(path[std::min(i+1,path.size()-1)],path[i?i-1:0]));
        const auto u=unit(cross(tangent,{0,0,1}));
        sections.push_back({path[i],mul(u,.12),mul(cross(tangent,u),.07)});
    }
    const auto cutter=buildDamageSweepVolume(sections,fresh);
    require(PA::validateClosedMesh(cutter).hasValue()&&volume(cutter)>0,"noncoplanar sweep must make a closed positive volume");
    require(connected(cutter),"spatial sweep must be one continuous connected volume");
    require(std::all_of(cutter.triangleRegions.begin(),cutter.triangleRegions.end(),[&](auto id){return id==fresh;}),"all sweep faces retain operation identity");
    const auto cut=subtractDamageVolume(host,cutter);
    require(PA::validateClosedMesh(cut).hasValue(),"spatial cut must retain a closed host");
    require(volume(cut)<7.99&&volume(cut)>7.2,"spatial cut must remove localized real volume");
    require(meshHash(host)==original,"spatial cut must preserve its source");
    const auto raised=applyDamageVolume(host,cutter,DamageVolumeOperation::Union);
    require(PA::validateClosedMesh(raised).hasValue()&&volume(raised)>8.01&&volume(raised)<8.2,
        "raised spatial decal must add bounded real volume to the closed cube");
    require(meshHash(host)==original,"raised decal must preserve the source mesh");
    require(connected(raised),"raised decal must be fused to its receiver, without floating components");
    require(std::abs(volume(raised)-volume(cut)-volume(cutter))<1e-10,
        "union and difference must account for the same complete authored volume");
    require(std::find(raised.triangleRegions.begin(),raised.triangleRegions.end(),fresh)!=raised.triangleRegions.end(),
        "raised decal must keep its own surface identity");
    require(meshHash(applyDamageVolume(host,cutter,DamageVolumeOperation::Union))==meshHash(raised),
        "raised decal topology and IDs must replay exactly");
    bool top=false,side=false;
    for(std::size_t t=0;t<cut.triangleRegions.size();++t)
    {
        if(cut.triangleRegions[t]!=fresh)continue;
        for(unsigned c=0;c<3;++c)
        {
            const auto p=cut.positions[cut.indices[3*t+c]];
            double distance=1e30;
            for(std::size_t i=1;i<path.size();++i)
            {
                const auto d=sub(path[i],path[i-1]),q=sub(p,path[i-1]);
                const auto delta=sub(q,mul(d,std::clamp(dot(q,d)/dot(d,d),0.,1.)));
                distance=std::min(distance,std::sqrt(dot(delta,delta)));
            }
            require(distance<=.121,"sweep must follow consecutive spans without cutting across distant control points");
            top|=p.y>.85&&p.x<.6;side|=p.x>.85&&p.y<.3;
        }
    }
    require(top&&side,"one operation must create fresh cut faces on different cube sides");
    require(meshHash(buildDamageSweepVolume(sections,fresh))==meshHash(cutter)&&meshHash(subtractDamageVolume(host,cutter))==meshHash(cut),"spatial geometry and IDs must replay exactly");
    auto moved=cutter;for(auto &p:moved.positions)p.x+=10;
    require(meshHash(subtractDamageVolume(host,moved))==original,"disjoint volume must leave host byte-identical");
    require(meshHash(applyDamageVolume(host,moved,DamageVolumeOperation::Union))==original,"disjoint decal must not create a floating island");
    auto inside=host;for(auto&p:inside.positions)p=mul(p,.1);inside.triangleRegions.assign(12,fresh);
    require(meshHash(applyDamageVolume(host,inside,DamageVolumeOperation::Union))==original,"fully buried decal must preserve the exact host");
    auto reject=[](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}require(failed,"invalid spatial damage input must be rejected");};
    reject([&]{buildDamageSweepVolume({},fresh);});
    reject([&]{applyDamageVolume(host,cutter,DamageVolumeOperation(9));});
    auto invalid=sections;invalid[1].axisV=invalid[1].axisU;reject([&]{buildDamageSweepVolume(invalid,fresh);});
    invalid=sections;invalid[1].center.x=std::numeric_limits<double>::infinity();reject([&]{buildDamageSweepVolume(invalid,fresh);});
    reject([&]{buildDamageSweepVolume(sections,fresh,8);});
    auto open=host;open.indices.resize(open.indices.size()-3);open.triangleRegions.pop_back();
    reject([&]{subtractDamageVolume(open,cutter);});
    std::cout<<std::setprecision(17)<<"PASS connected spatial cut and decal on two cube faces: cut volume "<<volume(cut)<<", added volume "<<volume(raised)<<", cutter volume "<<volume(cutter)<<", cutter triangles "<<cutter.indices.size()/3<<'\n';
}
}
