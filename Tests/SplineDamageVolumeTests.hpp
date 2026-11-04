#pragma once
#include "DamageGeometry.hpp"
#include "SolidGeometry.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
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
    require(std::all_of(cutter.triangleRegions.begin(),cutter.triangleRegions.end(),[&](auto id){return id==fresh;}),"all sweep faces retain operation identity");
    const auto cut=subtractDamageVolume(host,cutter);
    require(PA::validateClosedMesh(cut).hasValue(),"spatial cut must retain a closed host");
    require(volume(cut)<7.99&&volume(cut)>7.2,"spatial cut must remove localized real volume");
    require(meshHash(host)==original,"spatial cut must preserve its source");
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
    auto reject=[](auto action){bool failed=false;try{action();}catch(const std::exception&){failed=true;}require(failed,"invalid spatial damage input must be rejected");};
    reject([&]{buildDamageSweepVolume({},fresh);});
    auto invalid=sections;invalid[1].axisV=invalid[1].axisU;reject([&]{buildDamageSweepVolume(invalid,fresh);});
    invalid=sections;invalid[1].center.x=std::numeric_limits<double>::infinity();reject([&]{buildDamageSweepVolume(invalid,fresh);});
    reject([&]{buildDamageSweepVolume(sections,fresh,8);});
    auto open=host;open.indices.resize(open.indices.size()-3);open.triangleRegions.pop_back();
    reject([&]{subtractDamageVolume(open,cutter);});
    std::cout<<"PASS continuous spatial damage on two cube faces: volume "<<volume(cut)<<", cutter triangles "<<cutter.indices.size()/3<<'\n';
}
}
