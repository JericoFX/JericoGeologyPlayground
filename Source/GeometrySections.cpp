// SPDX-License-Identifier: GPL-3.0-only
#include "GeometrySections.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <stdexcept>

namespace JE::GeometryAnalysis {
namespace {
bool finite(Vec3 p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
Vec3 subtract(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
double magnitude(Vec3 p){return std::hypot(p.x,p.y,p.z);}
double scale(Vec3 p){return std::max({std::abs(p.x),std::abs(p.y),std::abs(p.z)});}
Vec3 divide(Vec3 p,double d){return {p.x/d,p.y/d,p.z/d};}
double dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
}
PlaneSection sectionMesh(MeshView mesh,Vec3 origin,Vec3 normal,double tolerance,const Checkpoint& checkpoint)
{
    if(checkpoint)checkpoint();
    if(!finite(origin)||!finite(normal)||scale(normal)==0||!std::isfinite(tolerance)||tolerance<=0)
        throw std::invalid_argument("Plane section needs finite origin, nonzero normal and positive metric tolerance");
    if(mesh.indices.size()%3!=0)throw std::invalid_argument("Section mesh indices must describe triangles");
    normal=divide(normal,scale(normal));normal=divide(normal,magnitude(normal));
    for(std::size_t i=0;i<mesh.positions.size();++i){
        if((i&255)==0&&checkpoint)checkpoint();
        if(!finite(mesh.positions[i]))throw std::invalid_argument("Section mesh contains a non-finite position");
    }
    PlaneSection result;
    for(std::size_t face=0;face<mesh.indices.size()/3;++face){
        if((face&255)==0&&checkpoint)checkpoint();
        std::array<Vec3,3> p;
        for(std::size_t i=0;i<3;++i){
            const auto vertex=mesh.indices[face*3+i];
            if(vertex>=mesh.positions.size())throw std::invalid_argument("Section mesh index is outside its positions");
            p[i]=mesh.positions[vertex];
        }
        auto a=subtract(p[1],p[0]),b=subtract(p[2],p[0]);
        if(!finite(a)||!finite(b))throw std::invalid_argument("Section mesh edge exceeds finite coordinate range");
        const auto edgeScale=std::max(scale(a),scale(b));
        if(edgeScale==0||magnitude(cross(divide(a,edgeScale),divide(b,edgeScale)))==0){
            ++result.degenerateTriangles;continue;
        }
        std::array<double,3> distance;
        std::array<int,3> side;
        for(std::size_t i=0;i<3;++i){
            const auto local=subtract(p[i],origin);
            distance[i]=dot(normal,local);
            if(!finite(local)||!std::isfinite(distance[i]))throw std::invalid_argument("Section plane distance exceeds finite range");
            side[i]=distance[i]>tolerance?1:distance[i]<-tolerance?-1:0;
        }
        if(side[0]==0&&side[1]==0&&side[2]==0){result.coplanarTriangles.push_back(face);continue;}
        std::array<Vec3,3> hits;
        std::array<std::array<Vertex,2>,3> edges;
        std::size_t count=0;
        for(std::size_t i=0;i<3;++i)if(side[i]==0){
            const auto vertex=mesh.indices[face*3+i];
            edges[count]={vertex,vertex};hits[count++]=p[i];
        }
        for(std::size_t i=0;i<3;++i){
            const auto j=(i+1)%3;
            if(side[i]*side[j]!=-1)continue;
            const auto dScale=std::max(std::abs(distance[i]),std::abs(distance[j]));
            const auto first=std::abs(distance[i])/dScale,second=std::abs(distance[j])/dScale;
            const auto t=first/(first+second);
            const auto edgeA=mesh.indices[face*3+i],edgeB=mesh.indices[face*3+j];
            edges[count]={std::min(edgeA,edgeB),std::max(edgeA,edgeB)};
            hits[count++]={std::lerp(p[i].x,p[j].x,t),std::lerp(p[i].y,p[j].y,t),std::lerp(p[i].z,p[j].z,t)};
        }
        if(count==2&&magnitude(subtract(hits[1],hits[0]))>tolerance)
            result.segments.push_back({hits[0],hits[1],face,edges[0],edges[1]});
    }
    return result;
}
}
