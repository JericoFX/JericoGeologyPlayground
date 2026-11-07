// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

// Infers pipe sockets from the mesh itself: an open boundary loop that fits a
// circle is a connection. Pure C++20 geometry, with no renderer, UI or asset-path
// dependency, so the Engine can reuse it without the Playground.
namespace JE::SocketInference {
struct Vec3 {double x{},y{},z{};};
struct SocketCandidate {
    Vec3 position{},forward{};
    double diameter{};
    double radialErrorMeters{};   // Max deviation from the fitted circle.
    double planeErrorMeters{};    // Max out-of-plane deviation.
    std::size_t loopVertices{};
    bool closedLoop{};
};
struct Settings {
    double weldRelative{1e-5};
    std::size_t minimumLoopVertices{8};
    double maximumRadialErrorRelative{0.08};
    double maximumPlaneErrorRelative{0.05};
};
struct ClusterInfo {
    Vec3 position{},normal{};
    double area{},radius{},maximumRadial{},averageRadial{};
    std::size_t triangles{};
};

struct LoopInfo {
    Vec3 centre{};
    double maximumRadial{},averageRadial{};
    std::size_t vertices{};
    bool closed{};
};

struct Report {
    std::size_t boundaryLoops{},rejectedLoops{},planarRegions{};
    std::size_t boundaryEdges{},vertices{};
    std::vector<std::size_t> loopSizes; // Border component sizes, for diagnosis.
    std::vector<LoopInfo> loops; // Largest border loops, for diagnosis.
    double bestFill{},bestRadial{}; // Diagnostics for the planar-cap stage.
    std::vector<ClusterInfo> clusters; // Largest planar clusters, for diagnosis.
    std::vector<SocketCandidate> sockets;
};
Report infer(std::span<const Vec3>,std::span<const std::uint32_t>,const Settings& = {});
}
