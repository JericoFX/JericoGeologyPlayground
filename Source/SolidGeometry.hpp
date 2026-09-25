#pragma once
#include "Model.hpp"

namespace Playground::Solid
{
Jerico::Guid region(std::uint32_t seed, std::uint64_t id);
TriangleMesh validate(const TriangleMesh &mesh);
TriangleMesh planes(const TriangleMesh &mesh, const Layer &layer, const Jerico::CancellationToken *cancel);
TriangleMesh fractureShear(const TriangleMesh &mesh, const Layer &layer,
                           const Jerico::CancellationToken *cancel);
TriangleMesh envelope(const TriangleMesh &mesh, const Layer &layer, const Jerico::CancellationToken *cancel);
void transferSamples(const MeshResult &source, const std::vector<Vec3d> &sourceWarp, MeshResult &destination,
                     std::vector<Vec3d> &destinationWarp, const Jerico::CancellationToken *cancel);
} // namespace Playground::Solid
