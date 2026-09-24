#pragma once
#include "Model.hpp"

namespace Playground
{
struct SurfaceFeatureSettings
{
    bool contact{true};
    bool ambient{false};
    bool thickness{false};
    double radius{0.3}, creaseAngleDegrees{30};
    int contactSamples{16};
    double ambientRadius{1};
    int ambientSamples{24};
    double thicknessRadius{10};
    Vec3d exposureDirection{0, 1, 0};
};
struct SurfaceFeatureFields
{
    // Vertex domain, evaluated on the supplied geometry; never silently welded.
    // Sign is relative to authored face orientation: use outward winding for closed solids.
    std::vector<double> signedDihedralCurvature; // positive convex on the front side, inverse object units
    std::vector<double> convexity;               // max(signed curvature, 0)
    std::vector<double> concavity;               // max(-signed curvature, 0)
    std::vector<double> absoluteCurvature;
    std::vector<double> edgeDistance;            // shortest edge-graph distance; infinity if no crease
    std::vector<double> boundaryDistance;        // edge-graph distance to open topology boundary
    std::vector<double> contactOcclusion;        // [0,1], empty when not requested
    std::vector<double> ambientOcclusion;        // larger independent radius; empty when not requested
    std::vector<double> thickness;               // inward first-hit distance; 0 on miss
    std::vector<double> thicknessConfidence;     // 1 hit, 0 miss
    std::vector<double> exposure;                // positive normal alignment with authored direction
    std::vector<double> minimumEdgeLength;
    std::vector<double> minimumTriangleAltitude;
    std::vector<bool> boundary;
};
SurfaceFeatureFields surfaceFeatures(const TriangleMesh &mesh, const SurfaceFeatureSettings &settings = {},
                                     const Jerico::CancellationToken *cancel = nullptr);
} // namespace Playground
