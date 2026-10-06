#pragma once
#include "rocks/mesh.hpp"
namespace rocks {
bool isSurfaceFamily(const std::string&);
void validateSurfaceFamily(const HeightfieldRecipe&);
HeightfieldRecipe surfaceFamilyHostRecipe(const HeightfieldRecipe&);
HeightfieldSamples surfaceFamilySamples(const HeightfieldRecipe&,const HeightfieldSamples* mathematicalHost=nullptr);
}
