#pragma once
#include "SplineConstruction.hpp"
#include <filesystem>
namespace Playground::Construction {
std::string serializeSplineDamage(const std::vector<PathDamageRule>& paths);
std::vector<PathDamageRule> deserializeSplineDamage(const std::string& text);
void saveSplineDamage(const std::vector<PathDamageRule>& paths,const std::filesystem::path& file);
std::vector<PathDamageRule> loadSplineDamage(const std::filesystem::path& file);
// Commit the replacement only after the existing construction pipeline accepts it.
ConstructionResult loadSplineDamage(const PA::ProceduralSpline& spline,ConstructionSettings& settings,const std::filesystem::path& file);
std::string nextSplineDamageId(const std::vector<PathDamageRule>& paths);
}
