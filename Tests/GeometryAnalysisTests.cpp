#include "GeometryAnalysis.hpp"
#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>

namespace G=JE::GeometryAnalysis;
namespace {
void require(bool condition,const char* text){if(!condition)throw std::runtime_error(text);}
template<class F> void rejects(F call){bool caught=false;try{call();}catch(const std::exception&){caught=true;}require(caught,"Invalid geometry request was accepted");}
void propagation() {
    G::Adjacency graph{{{1,1},{2,5}},{{0,1},{2,2}},{{0,5},{1,2}}, {}};
    const std::array<G::Seed,1> seed{{{0,0}}};const auto result=G::shortestPaths(graph,seed);
    require(result.distance[0]==0&&result.distance[1]==1&&result.distance[2]==3&&std::isinf(result.distance[3]),"Shortest distances or disconnected state are wrong");
    require(result.parent[2]==1&&result.parent[1]==0&&result.parent[0]==G::noParent&&result.parent[3]==G::noParent,"Geodesic paths are not traceable to their source");
    const std::array<G::Seed,3> multiple{{{0,5},{2,0},{0,0}}};const auto nearest=G::shortestPaths(graph,multiple);
    require(nearest.distance[0]==0&&nearest.distance[1]==1&&nearest.distance[2]==0,"Multiple sources or duplicate seed priority changed");
    const auto empty=G::shortestPaths(graph,{});for(const auto value:empty.distance)require(std::isinf(value),"No feature seeds must yield infinite distances");
    auto bad=graph;bad[3].push_back({99,1});rejects([&]{G::shortestPaths(bad,seed);});
    bad=graph;bad[3].push_back({0,-1});rejects([&]{G::shortestPaths(bad,seed);});
    bad=graph;bad[0][0].second=std::numeric_limits<double>::quiet_NaN();rejects([&]{G::shortestPaths(bad,seed);});
    const std::array<G::Seed,1> outside{{{99,0}}};rejects([&]{G::shortestPaths(graph,outside);});
    const std::array<G::Seed,1> invalid{{{0,-1}}};rejects([&]{G::shortestPaths(graph,invalid);});
    int checkpoints=0;rejects([&]{G::shortestPaths(graph,seed,[&]{++checkpoints;throw std::runtime_error("cancelled");});});require(checkpoints==1,"Cancellation did not stop propagation immediately");
    const auto repeated=G::shortestPaths(graph,seed);require(repeated.distance==result.distance&&repeated.parent==result.parent,"Propagation is not deterministic");
}
void topology() {
    const std::vector<G::Vec3> points{{0,0,0},{1,0,0},{0,1,0},{1,0,0},{1,1,0},{0,1,0}};
    const std::vector<G::Vertex> triangles{0,1,2,3,4,5};
    const auto strict=G::analyze({points,triangles});require(strict.graph.points.size()==6&&strict.components.size()==2&&strict.inferredConnections.empty(),"Default analysis silently welded or connected a mesh");
    G::Settings weld;weld.weldRelative=1e-6;const auto joined=G::analyze({points,triangles},weld);
    require(joined.graph.points.size()==4&&joined.components.size()==1&&std::abs(joined.components[0].area-1)<1e-12,"Explicit geometric welding did not preserve quad area");
    require(joined.sourceToGraph[1]==joined.sourceToGraph[3]&&joined.sourceToGraph[2]==joined.sourceToGraph[5],"Analysis lost original vertex correspondence");
    require(points[3].x==1&&triangles==std::vector<G::Vertex>{0,1,2,3,4,5},"Analysis modified its source");
    const auto repeat=G::analyze({points,triangles},weld);require(repeat.sourceToGraph==joined.sourceToGraph&&repeat.graph.adjacent==joined.graph.adjacent,"Topology analysis is not deterministic");
    auto invalid=triangles;invalid[4]=6;rejects([&]{G::analyze({points,invalid});});
    invalid.pop_back();rejects([&]{G::analyze({points,invalid});});
    auto nonfinite=points;nonfinite[0].x=std::numeric_limits<double>::infinity();rejects([&]{G::analyze({nonfinite,triangles});});
    auto limited=weld;limited.maximumVertices=5;rejects([&]{G::analyze({points,triangles},limited);});
    limited=weld;limited.maximumTriangles=1;rejects([&]{G::analyze({points,triangles},limited);});
    limited=weld;limited.weldRelative=1e-30;rejects([&]{G::analyze({points,triangles},limited);});
    limited=weld;limited.connectionRelative=1e-30;rejects([&]{G::analyze({points,triangles},limited);});
    int calls=0;rejects([&]{G::analyze({points,triangles},weld,[&]{if(++calls==3)throw std::runtime_error("cancelled");});});require(calls==3,"Analysis ignored cancellation");
    const auto intact=G::analyze({points,triangles},weld);require(intact.graph.adjacent==joined.graph.adjacent,"Cancelled analysis left mutable global state");
}
void inferredConnections() {
    const std::vector<G::Vec3> points{{0,0,0},{1,0,0},{0,1,0},{1.02,0,0},{2.02,0,0},{1.02,1,0}};
    const std::vector<G::Vertex> triangles{0,1,2,3,4,5};G::Settings settings;settings.connectionRelative=.02;settings.minimumComponentVertices=3;
    const auto result=G::analyze({points,triangles},settings);
    require(result.componentCountBeforeConnections==2&&result.components.size()==1&&result.inferredConnections.size()==1,"Nearby shells were not explicitly distinguished from observed connectivity");
    require(std::abs(result.inferredConnections.front().distance-.02)<1e-12&&std::abs(result.components.front().area-1)<1e-12,"Connection changed geometric area or gap");
    const std::array<G::Seed,1> seeds{{{0,0}}};const auto distance=G::shortestPaths(result.graph.adjacent,seeds);
    require(std::abs(distance.distance[4]-2.02)<1e-12,"Geodesic traversal did not include the inferred bridge");
    auto bounded=settings;bounded.maximumProximityComparisons=0;rejects([&]{G::analyze({points,triangles},bounded);});
    bounded=settings;bounded.maximumConnectionCandidates=0;rejects([&]{G::analyze({points,triangles},bounded);});
    settings.minimumComponentVertices=4;require(G::analyze({points,triangles},settings).components.size()==2,"Small-component exclusion was ignored");
    settings.minimumComponentVertices=3;settings.connectionRelative=.001;require(G::analyze({points,triangles},settings).components.size()==2,"Connection escaped its geometric budget");
    auto transformed=points;for(auto& p:transformed){p.x=p.x*3+7;p.y=p.y*3-4;p.z=p.z*3+2;}
    settings.connectionRelative=.02;const auto scaled=G::analyze({transformed,triangles},settings);
    require(scaled.components.size()==1&&std::abs(scaled.inferredConnections.front().distance-.06)<1e-12,"Relative connection policy depends on origin or unit scale");
    const auto next=G::shortestPaths(scaled.graph.adjacent,seeds);require(std::abs(next.distance[4]-6.06)<1e-12,"Scaled path distances are inconsistent");
}
}
int main(){try{propagation();topology();inferredConnections();std::cout<<"Geometry analysis: topology, provenance, shortest paths, bounds and cancellation PASS\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
