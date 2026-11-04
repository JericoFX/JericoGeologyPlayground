// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "GeometryAnalysis.hpp"
#include <array>

namespace JE::GeometryAnalysis {
struct PlaneSectionSegment {
    Vec3 first,second;
    std::size_t sourceTriangle{};
    // Canonical source-edge endpoints; {v,v} denotes a vertex on the plane.
    // Consumers can join incident segments without a second position-welding rule.
    std::array<Vertex,2> firstEdge{},secondEdge{};
};
struct PlaneSection {
    std::vector<PlaneSectionSegment> segments;
    std::vector<std::size_t> coplanarTriangles;
    std::size_t degenerateTriangles{};
};

// Triangle/plane intersections for analysis of open or disconnected surfaces.
// Segments retain source triangle identity; they are not welded or ordered here.
// Coplanar faces are reported explicitly rather than inventing a contour across
// their interior. This is not a solid cutter or a proof of a closed cross-section.
// Normal magnitude is irrelevant. Tolerance is finite, positive and in metres.
PlaneSection sectionMesh(MeshView,Vec3 planeOrigin,Vec3 planeNormal,
    double toleranceMeters=1e-9,const Checkpoint& = {});
}
