#pragma once

#include "DamageField.hpp"
#include "Model.hpp"

namespace Playground
{
// Geometric tier of the shared projector. The source mesh is never modified.
TriangleMesh cutDamageCavity(const TriangleMesh &host,const DamageArea &area,
                             Jerico::Guid freshInteriorRegion,
                             std::size_t maximumTriangles=240000,
                             const Jerico::CancellationToken *cancel=nullptr);
}
