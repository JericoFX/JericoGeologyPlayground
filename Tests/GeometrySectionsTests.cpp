// SPDX-License-Identifier: GPL-3.0-only
#include "GeometrySections.hpp"
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace G=JE::GeometryAnalysis;
namespace {
void require(bool value,const char* message){if(!value)throw std::runtime_error(message);}
template<class Action> void rejects(Action action){
    bool rejected=false;
    try{action();}catch(const std::invalid_argument&){rejected=true;}
    require(rejected,"Invalid section input was accepted");
}
struct Fixture {
    std::vector<G::Vec3> positions;
    std::vector<G::Vertex> indices;
    G::MeshView view() const{return {positions,indices};}
};
Fixture cube(){
    return {{{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},
             {-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
            {0,2,1,0,3,2,4,5,6,4,6,7,0,1,5,0,5,4,
             3,7,6,3,6,2,0,4,7,0,7,3,1,2,6,1,6,5}};
}
double length(const G::PlaneSection& section){
    double sum=0;
    for(const auto& s:section.segments)
        sum+=std::hypot(s.second.x-s.first.x,s.second.y-s.first.y,s.second.z-s.first.z);
    return sum;
}
void squareSection(){
    const auto source=cube();
    const auto section=G::sectionMesh(source.view(),{0,0,0},{0,0,1});
    require(section.segments.size()==8,"Cube mid-plane must intersect eight triangles");
    require(std::abs(length(section)-8)<1e-12,"Cube section must have perimeter eight metres");
    require(section.coplanarTriangles.empty(),"Mid-plane cannot contain a cube face");
    for(const auto& segment:section.segments){
        require(segment.sourceTriangle<12,"Section lost source triangle provenance");
        for(const auto& [p,key]:{std::pair{segment.first,segment.firstEdge},std::pair{segment.second,segment.secondEdge}}){
            require(key[0]<=key[1]&&key[1]<source.positions.size(),"Section source edge identity is invalid");
            for(const auto vertex:key){
                const auto first=source.indices.begin()+segment.sourceTriangle*3;
                require(vertex==first[0]||vertex==first[1]||vertex==first[2],
                    "Section edge belongs to a different source triangle");
            }
            const auto a=source.positions[key[0]],b=source.positions[key[1]];
            const auto distance=[](G::Vec3 x,G::Vec3 y){return std::hypot(x.x-y.x,x.y-y.y,x.z-y.z);};
            require(std::abs(distance(a,p)+distance(p,b)-distance(a,b))<1e-12,
                "Section endpoint does not belong to its reported source edge");
        }
        for(const auto p:{segment.first,segment.second}){
            require(std::abs(p.z)<1e-12,"Section endpoint left the plane");
            require(std::abs(std::abs(p.x)-1)<1e-12||std::abs(std::abs(p.y)-1)<1e-12,
                "Section invented an interior diagonal");
        }
    }
    auto open=source;open.indices.erase(open.indices.begin()+12,open.indices.begin()+18);
    const auto incomplete=G::sectionMesh(open.view(),{},{0,0,1});
    require(incomplete.segments.size()==6&&std::abs(length(incomplete)-6)<1e-12,
        "Open surface section was rejected or silently closed");
    auto disconnected=source;
    for(auto p:source.positions){p.x+=5;disconnected.positions.push_back(p);}
    for(auto i:source.indices)disconnected.indices.push_back(i+8);
    const auto pieces=G::sectionMesh(disconnected.view(),{},{0,0,1});
    require(pieces.segments.size()==16&&std::abs(length(pieces)-16)<1e-12,
        "Disconnected section lost a component or joined unrelated surfaces");
}
void boundaryCases(){
    const auto source=cube();
    const auto top=G::sectionMesh(source.view(),{0,0,1},{0,0,1});
    require(top.coplanarTriangles==std::vector<std::size_t>({2,3}),
        "Coplanar faces must be explicit ambiguity, not triangle diagonals");
    require(top.segments.size()==4&&std::abs(length(top)-8)<1e-12,
        "On-plane mesh edges were lost or duplicated within a triangle");
    require(G::sectionMesh(source.view(),{0,0,2},{0,0,1}).segments.empty(),
        "Plane outside the mesh produced a contour");
    const Fixture tangent{{{0,0,0},{1,0,1},{0,1,1}},{0,1,2}};
    require(G::sectionMesh(tangent.view(),{},{0,0,1}).segments.empty(),
        "A point contact was promoted to a line segment");
    const Fixture degenerate{{{-1,0,0},{0,0,0},{1,0,0}},{0,1,2}};
    const auto zeroArea=G::sectionMesh(degenerate.view(),{},{0,0,1});
    require(zeroArea.segments.empty()&&zeroArea.coplanarTriangles.empty()&&zeroArea.degenerateTriangles==1,
        "Degenerate triangle was promoted to a surface section");
}
void frameAndValidation(){
    auto source=cube();
    const auto diagonal=G::sectionMesh(source.view(),{},{1e300,1e300,1e300});
    require(std::abs(length(diagonal)-6*std::sqrt(2.))<1e-12,
        "Oblique plane must produce the metric hexagon through a cube");
    for(const auto& segment:diagonal.segments)for(const auto p:{segment.first,segment.second})
        require(std::abs(p.x+p.y+p.z)<1e-12,"Oblique intersection left its plane");
    for(auto& p:source.positions){p={p.x*2+3,p.y*.5+2,p.z*1.5-4};}
    const auto section=G::sectionMesh(source.view(),{3,2,-4},{0,0,-2000});
    require(section.segments.size()==8&&std::abs(length(section)-10)<1e-12,
        "Translated nonuniform geometry changed section metric or normal normalization");
    rejects([&]{G::sectionMesh(source.view(),{},{0,0,0});});
    rejects([&]{G::sectionMesh(source.view(),{},{0,std::numeric_limits<double>::quiet_NaN(),1});});
    rejects([&]{G::sectionMesh(source.view(),{std::numeric_limits<double>::infinity(),0,0},{0,0,1});});
    rejects([&]{G::sectionMesh(source.view(),{},{0,0,1},0);});
    rejects([&]{G::sectionMesh(source.view(),{},{0,0,1},std::numeric_limits<double>::infinity());});
    auto invalid=source;invalid.indices[0]=99;
    rejects([&]{G::sectionMesh(invalid.view(),{},{0,0,1});});
    invalid=source;invalid.indices.pop_back();
    rejects([&]{G::sectionMesh(invalid.view(),{},{0,0,1});});
    invalid=source;invalid.positions[0].x=std::numeric_limits<double>::quiet_NaN();
    rejects([&]{G::sectionMesh(invalid.view(),{},{0,0,1});});
    bool cancelled=false;
    try{G::sectionMesh(source.view(),{},{0,0,1},1e-9,[]{throw 42;});}
    catch(int marker){cancelled=marker==42;}
    require(cancelled,"Section analysis swallowed cancellation");
}
}
int main(){
    try{squareSection();boundaryCases();frameAndValidation();
        std::cout<<"PASS surface sections: open/disconnected geometry, metric, provenance, coplanar/degenerate faces, invalid inputs and cancellation\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
