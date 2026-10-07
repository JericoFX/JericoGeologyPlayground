#pragma once
#include "jerico/damage.hpp"
namespace jerico {
double smootherstep(double value);
double selection_weight(const std::string&part_id,const std::string&side,V3 point,const Json&selection);
double edge_distance(const Json&part,V3 point,std::string side="");
double edge_wear_weight(const Json&part,const std::string&side,V3 point,const Json&selection);
// Base operation seeds, like recipe seeds, must fit signed 64-bit integers.
// Out-of-range unsigned JSON seeds are rejected before any mutation.
Json apply_operations(const Json&scene,const Json&operations);
// FineWear checkpoint validation is opt-in; historical R2 accepts extra metadata.
Json apply_operations(const Json&scene,const Json&operations,bool fine_breakup_validation);
}
