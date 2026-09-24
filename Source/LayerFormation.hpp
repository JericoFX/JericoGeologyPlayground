#pragma once
#include "Model.hpp"
namespace Playground
{
MeshResult coherentLayerFormation(const MeshResult &source, const Layer &layer, bool showThickness,
                                  const std::vector<Vec3d> &sourceWarp, std::vector<Vec3d> &outputWarp,
                                  const Jerico::CancellationToken *cancel);
void layerEdgeDistances(MeshResult &mesh, const Layer &layer, const Jerico::CancellationToken *cancel);
std::vector<double> settleLayerStack(MeshResult &mesh, Vec3d up,
                                     const Jerico::CancellationToken *cancel);
} // namespace Playground
