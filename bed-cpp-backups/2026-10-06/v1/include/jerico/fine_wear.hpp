#pragma once
#include "jerico/localized_damage.hpp"
namespace jerico {
Json contour_segments(const Json&config);
Json apply_fine_wear_operations(const Json&scene,const Json&operations);
Json build_fine_wear(const Json&recipe);
}
