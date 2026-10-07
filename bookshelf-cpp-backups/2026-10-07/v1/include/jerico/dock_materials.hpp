#pragma once
#include "scene.hpp"

namespace jerico {
// Bake bounded shared dock PBR families from native procedures. Geometry, metric
// UVs and evaluated normals are preserved. Timber frame axes are world-space,
// orthonormal columns {u: grain, v: width, w: depth}; origin is world-space.
// Six wood variants each contain longitudinal, exposed and end-grain regions;
// iron uses one separate family. Assignment is deterministic by part ID/seed.
// Generates PNG16 maps with the existing native GLB/OBJ texture key contract.
// resolution must be in [32, 4096]. Missing/invalid timber frames are rejected.
void bake_dock_materials(Json& evaluatedScene, const std::string& directory,
                         int resolution = 512);
}
