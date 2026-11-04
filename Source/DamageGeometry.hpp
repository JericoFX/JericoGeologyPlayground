#pragma once

#include "DamageField.hpp"
#include "Model.hpp"
#include <span>

namespace Playground
{
enum class DamageVolumeOperation { Difference, Union };
// Metric sections shared by spline cuts and volumetric decal consumers.
// Axes are half-extents in world space; adjacent sections form one continuous volume.
struct DamageSweepSection
{
    Vec3d center, axisU, axisV;
};
TriangleMesh buildDamageSweepVolume(std::span<const DamageSweepSection> sections,
    Jerico::Guid region, std::size_t maximumTriangles=240000,
    const Jerico::CancellationToken *cancel=nullptr);
TriangleMesh subtractDamageVolume(const TriangleMesh &host,const TriangleMesh &volume,
    std::size_t maximumTriangles=240000,
    const Jerico::CancellationToken *cancel=nullptr);
TriangleMesh applyDamageVolume(const TriangleMesh &host,const TriangleMesh &volume,
    DamageVolumeOperation operation,std::size_t maximumTriangles=240000,
    const Jerico::CancellationToken *cancel=nullptr);
// Geometric tier of the shared projector. The source mesh is never modified.
TriangleMesh cutDamageCavity(const TriangleMesh &host,const DamageArea &area,
                             Jerico::Guid freshInteriorRegion,
                             std::size_t maximumTriangles=240000,
                             const Jerico::CancellationToken *cancel=nullptr);
TriangleMesh cutDamagePath(const TriangleMesh &host,const DamagePathCutter &cutter,
                           Jerico::Guid freshInteriorRegion,
                           std::size_t maximumTriangles=240000,
                           const Jerico::CancellationToken *cancel=nullptr);
// Per-corner semantic mask avoids bleed at shared source/cut vertices; empty if no fresh role.
std::vector<std::uint8_t> freshInteriorCornerMask(const MeshResult &mesh);
}
