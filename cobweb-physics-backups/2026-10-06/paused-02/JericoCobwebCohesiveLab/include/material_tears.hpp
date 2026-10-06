#pragma once
#include "physics.hpp"
#include <array>
#include <cstddef>
#include <iosfwd>
#include <optional>

namespace web::physics {
struct MaterialFailure { int edge=-1; std::string cause="manual"; };
struct MaterialTearResult { int regions=0,primaryCuts=0,secondaryCuts=0,particles=0,edges=0; };
// Opt-in artistic material cuts; reserve is bounded by the builder's 128 particles.
// Existing whole-edge damage remains existing damage, not new cut polygons.
// ExactAuthoringUV uses immutable pre-cluster cohesive UVs when available.
// ParticleUV explicitly selects the solver's post-cluster particle chart, so
// child endpoints remain on its parent segments. Neither mode changes cohesive
// authoring metadata; the chart choice is retained in checkpoints.
void enableMaterialTears(Graph&, double radius=.0075, int particleReserve=128,
                         MaterialChartSource chartSource=MaterialChartSource::ExactAuthoringUV);
// Atomic topology commit. Failures are sorted by edge ID; crossing secondary
// cuts use the SAME polygon and never recursively emit another polygon.
// New ends inherit interpolated rest/position/velocity. Their finite mass is
// transferred from the retained endpoint's edge-weighted lump; cut-away material
// remains in those lumps. Free-particle mass is conserved, but interpolation is
// not momentum/energy conserving. Anchored donors stay exact and supply a counted
// finite reservoir mass (anchorSuppliedMass), never a pinned new endpoint.
// Budgets include inactive stored constraints plus current contact scalars.
// Failures leave the entire input graph unchanged; solver prediction preceding
// this end-of-substep topology commit is not rolled back by step().
MaterialTearResult applyMaterialTear(Graph&, int edge, const std::string& cause="manual", int substep=0);
MaterialTearResult applyMaterialTearBatch(Graph&, const std::vector<MaterialFailure>&, int substep=0, std::size_t contactScalars=0);
std::array<V2,2> materialEdgeUV(const Graph&, const Edge&);
// Boundary-only tangencies have no interior interval. Polygons are convex CCW.
std::optional<std::array<double,2>> materialCutInterval(const MaterialCutRegion&, V2 a, V2 b);
bool pointInMaterialCut(const MaterialCutRegion&, V2 uv, double tolerance=0);
bool pointInMaterialCut(const Graph&, V2 uv, double tolerance=0);
void validateMaterialTears(const Graph&);
// Extension follows the legacy END marker. Old strict readers reject it.
void writeMaterialTearState(std::ostream&, const Graph&);
void readMaterialTearState(std::istream&, Graph&);
}
