#pragma once
#include "scene.hpp"
namespace jerico {
// Export evaluated C++ geometry and its native baked PNG textures as GLB 2.0.
// No Blender, Python, external image converter or engine integration is used.
void write_glb(const Json& scene, const std::string& path);
// Flatten native evaluated geometry into world-space Z-up metres. Writes OBJ,
// sibling MTL, relative native PNG textures and an explicit format-limit README.
void write_obj_mtl(const Json& scene, const std::string& obj_path);
}
