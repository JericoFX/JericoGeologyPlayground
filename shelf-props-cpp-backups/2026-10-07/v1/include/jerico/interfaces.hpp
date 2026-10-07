#pragma once
#include "jerico/scene.hpp"
namespace jerico {
Json handle_interface(const std::string&style,const std::string&role="HandleFront");
// Pure authoring compatibility check. Does not connect an engine mechanism.
bool can_connect(const Json&socket,const Json&component_interface);
double grip_surface_distance(const Json&scene,const Json&grip);
// Additional authoring-contract checks, separate from mesh/engine validation.
Json validate_interfaces(const Json&scene);
}
