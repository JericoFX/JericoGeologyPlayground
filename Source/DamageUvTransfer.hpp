#pragma once

#include "DamageField.hpp"
#include "Model.hpp"

#include <array>
#include <vector>

namespace Playground
{
// Keeps the source UV on surviving faces and maps freshly exposed faces in projector space.
std::vector<std::array<float,2>> transferDamageCornerUv(
    const TriangleMesh &source,
    const std::vector<std::array<float,2>> &sourceCornerUv,
    const TriangleMesh &cut,Jerico::Guid freshInterior,
    const DamageProjector &projector);
}
