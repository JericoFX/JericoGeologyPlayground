#pragma once
#include "scene.hpp"
namespace jerico {
Json normalize_bed_recipe(const Json& recipe);
Json build_bed(const Json& recipe);
Json evaluate_bed(const Json& scene);
Json validate_bed(const Json& scene);
void bake_bed_materials(Json& scene,const std::string& directory,int resolution=512);
}
