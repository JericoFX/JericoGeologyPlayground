#pragma once
#include "scene.hpp"
namespace jerico {
struct MaterialSample { V3 base_color{.5,.5,.5}; double roughness=.8,metallic=0,transmission=0,ior=1.5,coat=0,height=0,bump_strength=0,bump_distance=0,specular=.5; };
// Matches Blender 4.3 normalized 3D Noise Texture (fBM), zero distortion.
double blender_noise(V3 position,double scale=1,double detail=3,double roughness=.65,V3 axis_scale={1,1,1},V3 offset={0,0,0});
MaterialSample sample_material(const Json&part,const std::string&material,V3 position);
// Filtered procedural bump normal in object space. Rotation columns map the
// object to world; the physical footprint is supplied by native UV texel size.
V3 sample_material_normal(const Json&part,const std::string&material,V3 position,
 V3 geometric_normal,V3 shading_normal,double footprint_m,
 const std::array<V3,3>&object_to_world={V3{1,0,0},V3{0,1,0},V3{0,0,1}});
// Bake in C++ from the part's own geometry and metric UVs. No images are input.
Json bake_material_textures(Json&scene,const std::string&output_directory,int resolution=512);
}
