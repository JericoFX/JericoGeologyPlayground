#pragma once

#include "ProceduralAssemblies.hpp"

#include <filesystem>
#include <string>

namespace Playground::Assembly
{
std::string serializeAssemblyDamage(const DamageSettings &settings);
DamageSettings deserializeAssemblyDamage(const std::string &text);
void saveAssemblyDamage(const DamageSettings &settings,const std::filesystem::path &path);
DamageSettings loadAssemblyDamage(const std::filesystem::path &path);
}
