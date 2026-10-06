#pragma once
#include "physics.hpp"
#include <cstddef>
#include <optional>

namespace web::physics {
// Immutable rest topology. Build from REST UV and graph connectivity, including
// inactive rest edges. Rebuild only after an explicit node/edge topology change,
// never from current motion. Indices are physical particle indices.
struct CageOptions { std::size_t maxFaces=8400, maxVertices=25200; };
struct CageConstraint { std::array<int,2> particles{}; std::vector<int> sourceEdges; };
class RestCage {
public:
 const std::vector<V2>& uv() const { return uv_; }
 const std::vector<V3>& rest() const { return rest_; }
 const std::vector<std::array<int,3>>& triangles() const { return triangles_; }
 const std::vector<CageConstraint>& constraints() const { return constraints_; }
private:
 std::vector<V2> uv_;
 std::vector<V3> rest_;
 std::vector<std::array<int,3>> triangles_;
 std::vector<CageConstraint> constraints_;
 friend RestCage buildRestCage(const Graph&, const CageOptions&);
};
// WorldFixed preserves analytic rest appearance and shared-edge continuity. A
// rigid rotation of a curved analytic residual is approximate (error <= 2*|offset|).
// TriangleFrame is rigid-exact, but can open seams under nonrigid deformation;
// use it only explicitly. Offset-free queries are affine/isometry-exact.
enum class CageOffsetMode { WorldFixed, TriangleFrame };
struct CageBinding {
 int triangle=-1;
 std::array<double,3> barycentric{};
 // Rest offset in an orthonormal triangle frame. Zero for a
 // piecewise-linear carrier; bindPatchUV explicitly preserves analytic rest shape.
 V3 offsetCoordinates{};
 V3 worldRestOffset{};
 CageOffsetMode offsetMode=CageOffsetMode::WorldFixed;
};
RestCage buildRestCage(const Graph& graph, const CageOptions& options={});
// Explicit-face binding is useful for independently clipped UV triangles. Queries
// outside this triangle return nullopt. Shared edge queries have identical weights.
std::optional<CageBinding> bindCageTriangle(const RestCage&, int triangle, V2 uv,
                                         std::optional<V3> carrierRest=std::nullopt,
                                         CageOffsetMode mode=CageOffsetMode::WorldFixed);
// The default has NO analytic rest correction: it is the authoritative physical
// cage surface. Pass carrierRest or call bindPatchUV explicitly to opt in.
// Deterministic initial point location only. Keep the returned binding for all
// subsequent frames; evaluation never selects nearest particles or switches support.
// A caller-owned zero mask excludes removed/cross-tear faces without changing cage.
std::optional<CageBinding> bindCageUV(const RestCage&, V2 uv,
                                   std::optional<V3> carrierRest=std::nullopt,
                                   const std::vector<unsigned char>* enabledFaces=nullptr,
                                   CageOffsetMode mode=CageOffsetMode::WorldFixed);
std::optional<CageBinding> bindPatchUV(const RestCage&, const Patch&, V2 uv,
                                    const std::vector<unsigned char>* enabledFaces=nullptr,
                                   CageOffsetMode mode=CageOffsetMode::WorldFixed);
// Appended particles are accepted, but existing bindings retain their original
// support indices. Rebuild/rebind explicitly after topology changes to use them.
V3 evaluateCage(const RestCage&, const CageBinding&, const Graph& current);
V3 cageRestPosition(const RestCage&, const CageBinding&);
// -1 means mixed components and permits callers to discard/clip that face. This
// does not infer cuts from distance, fabricate pins, or modify the physical graph.
int cageTriangleComponent(const RestCage&, int triangle, const Graph& current);
}
