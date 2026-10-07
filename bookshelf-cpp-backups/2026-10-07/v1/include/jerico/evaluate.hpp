#pragma once
#include "scene.hpp"
namespace jerico {
Json evaluate_part(const Json&part);
Json evaluate_scene(const Json&scene);
void write_obj(const Json&scene,const std::string&path);
}
