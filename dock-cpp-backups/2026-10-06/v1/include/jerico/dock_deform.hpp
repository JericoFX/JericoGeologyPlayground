#pragma once
#include "jerico/scene.hpp"
namespace jerico {
// Immutable, pre-evaluation operation on a closed triangulated timber mesh.
// timber_frame is an orthonormal right-handed frame; u is metres along grain,
// v spans [-width/2,width/2], w is thickness. Only v changes, preserving levels.
// Config: amount_m, signed64 seed, edge_band_m, start_protection_m,
// end_protection_m, protected_intervals (closed [u0,u1] intervals in metres).
// Defaults protect both ends. protected_support=true protects the whole part.
// UVs and connectivity are preserved exactly. Excessive amplitude and invalid
// or inverted geometry are rejected, never silently repaired or clamped.
Json deform_timber_edges(const Json& part,const Json& operation);
}
