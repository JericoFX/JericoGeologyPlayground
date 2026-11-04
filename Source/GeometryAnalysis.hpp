// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <span>
#include <utility>
#include <vector>

namespace JE::GeometryAnalysis {
struct Vec3 {double x{},y{},z{};};
using Vertex=std::uint32_t;
using Adjacency=std::vector<std::vector<std::pair<Vertex,double>>>;
using Checkpoint=std::function<void()>; // Optional cancellation; exceptions propagate unchanged.
inline constexpr auto noParent=std::numeric_limits<std::size_t>::max();
struct MeshView {std::span<const Vec3> positions;std::span<const Vertex> indices;};
struct Graph {std::vector<Vec3> points;Adjacency adjacent;};
struct Component {std::vector<std::size_t> vertices;Vec3 minimum,maximum;double area{};};
struct Connection {Vertex first{},second{};double distance{};};
struct Settings {
    // Defaults preserve input topology. Welding quantizes coordinates into cells
    // of diagonal*weldRelative; proximity uses a Euclidean radius. Both are opt-in
    // analysis hypotheses: neither changes the caller's mesh or its semantic IDs.
    double weldRelative{};
    double connectionRelative{};
    double minimumComponentAreaRelative{};
    std::size_t minimumComponentVertices{1};
    std::size_t maximumVertices{1'000'000},maximumTriangles{2'000'000};
    std::size_t maximumProximityComparisons{20'000'000},maximumConnectionCandidates{100'000};
};
struct Analysis {
    Settings settings;
    Graph graph;
    std::vector<std::size_t> sourceToGraph;
    std::vector<Component> components;
    std::vector<Connection> inferredConnections;
    std::size_t componentCountBeforeConnections{},degenerateTriangles{};
    double diagonal{};
};
struct Seed {Vertex vertex{};double distance{};};
struct Distances {std::vector<double> distance;std::vector<std::size_t> parent;};

// Pure C++20 support for the existing shared surface backend and its consumers.
// Component indices are analysis-local, not persistent Part/Region identifiers.
// Inputs remain immutable throughout the call, including during checkpoints.
Analysis analyze(MeshView,const Settings& = {},const Checkpoint& = {});
// Shared multi-source propagation. Existing surface fields supply their edge
// weights directly; anatomical/mesh consumers can use the graph built above.
Distances shortestPaths(const Adjacency&,std::span<const Seed>,const Checkpoint& = {});
}
