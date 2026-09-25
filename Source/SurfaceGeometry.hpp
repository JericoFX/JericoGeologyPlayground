#pragma once
#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/BspDocument.hpp>
#include <span>
namespace Playground
{
enum class CaveForm { Arch, SquareTunnel };
MeshResult caveSource(CaveForm form, int subdivisions,
                      const Jerico::CancellationToken *cancel = nullptr);
MeshResult geologicalCaveSource(CaveForm form, int subdivisions,
                                const Jerico::CancellationToken *cancel = nullptr);
MeshResult axialCaveSource(CaveForm form, int subdivisions,
                           const Jerico::CancellationToken *cancel = nullptr);
MeshResult planeSource(std::array<float, 2> size, int resolution);
MeshResult bspWallSource(int subdivisions, const Jerico::CancellationToken *cancel = nullptr);
MeshResult bspSource(const PA::BspDocument &document, std::span<const Jerico::Guid> selected,
                     int subdivisions, const Jerico::CancellationToken *cancel = nullptr);
MeshResult solidifySurface(const MeshResult &source, const Layer &layer,
                           const Jerico::CancellationToken *cancel = nullptr);
void cliffCutLoop(MeshResult &surface, std::vector<Vec3d> &warp, const Layer &layer, int operation,
                  bool showField, bool softEdgeProfile,
                  const Jerico::CancellationToken *cancel = nullptr);
void cliffDrainage(MeshResult &surface, const Layer &layer, bool showField,
                   const Jerico::CancellationToken *cancel = nullptr);
} // namespace Playground
