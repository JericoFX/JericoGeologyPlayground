// SPDX-License-Identifier: GPL-3.0-only
#include "GeometryAnalysis.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>
#include <tuple>

namespace JE::GeometryAnalysis {
namespace {
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
double length(Vec3 p){return std::sqrt(p.x*p.x+p.y*p.y+p.z*p.z);}
void checkpoint(const Checkpoint& check){if(check)check();}
void include(Vec3& low,Vec3& high,Vec3 p){low={std::min(low.x,p.x),std::min(low.y,p.y),std::min(low.z,p.z)};high={std::max(high.x,p.x),std::max(high.y,p.y),std::max(high.z,p.z)};}
std::vector<Component> components(const Graph& graph,std::vector<std::size_t>& labels,const Checkpoint& check) {
    labels.assign(graph.points.size(),noParent);std::vector<Component> result;
    for(std::size_t seed=0;seed<labels.size();++seed)if(labels[seed]==noParent) {
        Component region;region.vertices.push_back(seed);region.minimum=region.maximum=graph.points[seed];labels[seed]=result.size();
        for(std::size_t i=0;i<region.vertices.size();++i) {
            if((i&255)==0)checkpoint(check);const auto vertex=region.vertices[i];include(region.minimum,region.maximum,graph.points[vertex]);
            for(const auto [next,distance]:graph.adjacent[vertex])if(labels[next]==noParent){labels[next]=result.size();region.vertices.push_back(next);}
        }
        result.push_back(std::move(region));
    }
    return result;
}
}
Distances shortestPaths(const Adjacency& adjacent,std::span<const Seed> seeds,const Checkpoint& check) {
    checkpoint(check);
    if(adjacent.size()>std::numeric_limits<Vertex>::max())throw std::invalid_argument("Geometry graph exceeds its index range");
    for(std::size_t i=0;i<adjacent.size();++i) {
        if((i&255)==0)checkpoint(check);
        for(const auto [next,weight]:adjacent[i])if(next>=adjacent.size()||!std::isfinite(weight)||weight<0)
            throw std::invalid_argument("Geometry graph has an invalid edge or distance");
    }
    Distances result{std::vector<double>(adjacent.size(),std::numeric_limits<double>::infinity()),std::vector<std::size_t>(adjacent.size(),noParent)};
    using Entry=std::pair<double,Vertex>;std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> queue;
    for(const auto seed:seeds) {
        if(seed.vertex>=adjacent.size()||!std::isfinite(seed.distance)||seed.distance<0)throw std::invalid_argument("Geometry graph seed is invalid");
        if(seed.distance<result.distance[seed.vertex]){result.distance[seed.vertex]=seed.distance;queue.push({seed.distance,seed.vertex});}
    }
    std::size_t visits=0;
    while(!queue.empty()) {
        if((visits++&255)==0)checkpoint(check);const auto [current,vertex]=queue.top();queue.pop();
        if(current>result.distance[vertex])continue;
        for(const auto [next,weight]:adjacent[vertex]) {
            const auto candidate=current+weight;if(!std::isfinite(candidate))throw std::overflow_error("Geometry graph distance overflow");
            if(candidate<result.distance[next]){result.distance[next]=candidate;result.parent[next]=vertex;queue.push({candidate,next});}
        }
    }
    return result;
}
Analysis analyze(MeshView mesh,const Settings& settings,const Checkpoint& check) {
    checkpoint(check);
    if(mesh.positions.empty()||mesh.positions.size()>settings.maximumVertices||mesh.positions.size()>std::numeric_limits<Vertex>::max()||
       mesh.indices.empty()||mesh.indices.size()%3||mesh.indices.size()/3>settings.maximumTriangles)
        throw std::invalid_argument("Geometry analysis requires bounded indexed triangles");
    if(!std::isfinite(settings.weldRelative)||settings.weldRelative<0||settings.weldRelative>.01||
       (settings.weldRelative>0&&settings.weldRelative<1e-15)||!std::isfinite(settings.connectionRelative)||
       settings.connectionRelative<0||settings.connectionRelative>1||(settings.connectionRelative>0&&settings.connectionRelative<1e-15)||!std::isfinite(settings.minimumComponentAreaRelative)||
       settings.minimumComponentAreaRelative<0||settings.minimumComponentAreaRelative>1)
        throw std::invalid_argument("Geometry analysis tolerances are invalid");
    Vec3 low=mesh.positions.front(),high=low;
    for(std::size_t i=0;i<mesh.positions.size();++i) {
        if((i&255)==0)checkpoint(check);const auto p=mesh.positions[i];
        if(!std::isfinite(p.x)||!std::isfinite(p.y)||!std::isfinite(p.z))throw std::invalid_argument("Geometry analysis position is non-finite");
        include(low,high,p);
    }
    Analysis result;result.settings=settings;result.diagonal=length(sub(high,low));
    if(!std::isfinite(result.diagonal)||!(result.diagonal>0))throw std::invalid_argument("Geometry analysis requires finite nonzero extent");
    auto& graph=result.graph;result.sourceToGraph.reserve(mesh.positions.size());
    std::map<std::tuple<long long,long long,long long>,std::size_t> welded;
    const auto quantum=result.diagonal*settings.weldRelative;
    if(settings.weldRelative>0&&!(quantum>0))throw std::invalid_argument("Geometry welding precision underflow");
    for(std::size_t i=0;i<mesh.positions.size();++i) {
        if((i&255)==0)checkpoint(check);const auto p=mesh.positions[i];std::size_t vertex=graph.points.size();bool inserted=true;
        if(settings.weldRelative>0) {
            const auto key=std::tuple{std::llround((p.x-low.x)/quantum),std::llround((p.y-low.y)/quantum),std::llround((p.z-low.z)/quantum)};
            const auto entry=welded.emplace(key,vertex);vertex=entry.first->second;inserted=entry.second;
        }
        if(inserted){graph.points.push_back(p);graph.adjacent.emplace_back();}
        result.sourceToGraph.push_back(vertex);
    }
    for(std::size_t t=0;t<mesh.indices.size();t+=3) {
        if((t%768)==0)checkpoint(check);
        for(std::size_t corner=0;corner<3;++corner) {
            if(mesh.indices[t+corner]>=mesh.positions.size()||mesh.indices[t+(corner+1)%3]>=mesh.positions.size())throw std::invalid_argument("Geometry analysis index out of range");
            const auto a=result.sourceToGraph[mesh.indices[t+corner]],b=result.sourceToGraph[mesh.indices[t+(corner+1)%3]];
            if(a!=b){const auto distance=length(sub(graph.points[a],graph.points[b]));graph.adjacent[a].push_back({Vertex(b),distance});graph.adjacent[b].push_back({Vertex(a),distance});}
        }
    }
    for(auto& edges:graph.adjacent){std::sort(edges.begin(),edges.end());edges.erase(std::unique(edges.begin(),edges.end()),edges.end());}
    std::vector<std::size_t> observedLabels;auto observed=components(graph,observedLabels,check);result.componentCountBeforeConnections=observed.size();
    for(std::size_t t=0;t<mesh.indices.size();t+=3) {
        if((t%768)==0)checkpoint(check);
        const auto a=result.sourceToGraph[mesh.indices[t]],b=result.sourceToGraph[mesh.indices[t+1]],c=result.sourceToGraph[mesh.indices[t+2]];
        const auto area=length(cross(sub(graph.points[b],graph.points[a]),sub(graph.points[c],graph.points[a])))*.5;
        if(!std::isfinite(area))throw std::overflow_error("Geometry triangle area overflow");
        if(area==0)++result.degenerateTriangles;observed[observedLabels[a]].area+=area;
    }
    if(settings.connectionRelative>0&&observed.size()>1) {
        double maximumArea=0;for(const auto& region:observed)maximumArea=std::max(maximumArea,region.area);
        const auto radius=result.diagonal*settings.connectionRelative;
        using Cell=std::array<long long,3>;std::map<Cell,std::vector<std::size_t>> cells;
        struct Bridge {double squared;Vertex first,second;};
        std::map<std::pair<std::size_t,std::size_t>,Bridge> nearest;
        std::size_t comparisons=0;
        for(std::size_t i=0;i<graph.points.size();++i) {
            if((i&255)==0)checkpoint(check);const auto& region=observed[observedLabels[i]];
            if(region.vertices.size()<settings.minimumComponentVertices||region.area<maximumArea*settings.minimumComponentAreaRelative)continue;
            const auto p=sub(graph.points[i],low);const Cell cell{static_cast<long long>(std::floor(p.x/radius)),static_cast<long long>(std::floor(p.y/radius)),static_cast<long long>(std::floor(p.z/radius))};
            for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)for(int z=-1;z<=1;++z) {
                const auto found=cells.find({cell[0]+x,cell[1]+y,cell[2]+z});if(found==cells.end())continue;
                for(const auto j:found->second) {
                    if(++comparisons>settings.maximumProximityComparisons)throw std::length_error("Geometry proximity search exceeds its comparison budget");
                    if(observedLabels[i]==observedLabels[j])continue;
                    const auto delta=sub(graph.points[i],graph.points[j]);const auto squared=delta.x*delta.x+delta.y*delta.y+delta.z*delta.z;if(squared>radius*radius)continue;
                    const std::pair<std::size_t,std::size_t> pair=std::minmax(observedLabels[i],observedLabels[j]);const auto old=nearest.find(pair);
                    if(old==nearest.end()&&nearest.size()>=settings.maximumConnectionCandidates)throw std::length_error("Geometry proximity search exceeds its connection budget");
                    if(old==nearest.end()||squared<old->second.squared)nearest[pair]={squared,Vertex(i),Vertex(j)};
                }
            }
            cells[cell].push_back(i);
        }
        std::vector<Bridge> links;for(const auto& [pair,link]:nearest)links.push_back(link);
        std::sort(links.begin(),links.end(),[](const auto& a,const auto& b){return std::tie(a.squared,a.first,a.second)<std::tie(b.squared,b.first,b.second);});
        std::vector<std::size_t> parent(observed.size());std::iota(parent.begin(),parent.end(),0);
        auto root=[&](std::size_t i){while(parent[i]!=i){parent[i]=parent[parent[i]];i=parent[i];}return i;};
        for(const auto link:links) {
            checkpoint(check);const auto a=root(observedLabels[link.first]),b=root(observedLabels[link.second]);if(a==b)continue;
            const auto distance=std::sqrt(link.squared);parent[b]=a;graph.adjacent[link.first].push_back({link.second,distance});graph.adjacent[link.second].push_back({link.first,distance});result.inferredConnections.push_back({link.first,link.second,distance});
        }
    }
    if(result.inferredConnections.empty())result.components=std::move(observed);
    else {
        std::vector<std::size_t> connectedLabels;result.components=components(graph,connectedLabels,check);
        for(const auto& original:observed)result.components[connectedLabels[original.vertices.front()]].area+=original.area;
    }
    return result;
}
}
