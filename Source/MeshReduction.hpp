#pragma once
#include "Model.hpp"
namespace Playground
{
double sampledBidirectionalMeshError(const TriangleMesh &a, const TriangleMesh &b,
                                     const Jerico::CancellationToken *cancel = nullptr);
TriangleMesh reduceClosedMesh(const TriangleMesh &source, double error, double &sampledError,
                              const Jerico::CancellationToken *cancel);
}
