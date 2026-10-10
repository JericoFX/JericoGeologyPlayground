#pragma once
#include "rocks/mesh.hpp"
namespace rocks {
bool isReferenceStudy(const std::string&);
void validateReferenceStudy(const HeightfieldRecipe&);
HeightfieldSamples referenceStudySamples(const HeightfieldRecipe&);
}
