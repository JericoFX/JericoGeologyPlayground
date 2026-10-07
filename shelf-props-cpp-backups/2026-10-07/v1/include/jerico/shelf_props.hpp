#pragma once
#include "scene.hpp"
namespace jerico {
Json default_shelf_props_recipe();
Json build_shelf_props(const Json& recipe);
Json evaluate_shelf_props(const Json& scene);
void bake_shelf_props(Json& scene,const std::string& directory,int resolution);
Json plan_shelf_prop_placement(const Json& bookshelf,const Json& props);
}
