#include "MeshReduction.hpp"
#include "SolidGeometry.hpp"
#include <TriangleBvh.hpp>
#include <algorithm>
#include <manifold/manifold.h>
#include <map>
#include <numeric>
#include <set>
#include <stdexcept>
namespace Playground
{
namespace D = PA::Detail;
namespace
{
void check(const Jerico::CancellationToken *cancel)
{
    if (cancel && cancel->isCancellationRequested())
        throw std::runtime_error("Cancelled");
}
D::TriangleBvh tree(const TriangleMesh &mesh)
{
    std::vector<D::Triangle> triangles;
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        auto a = mesh.positions[mesh.indices[t]], b = mesh.positions[mesh.indices[t + 1]],
             c = mesh.positions[mesh.indices[t + 2]];
        auto bounds = D::emptyBounds();
        D::include(bounds, a);
        D::include(bounds, b);
        D::include(bounds, c);
        triangles.push_back({a, b, c, D::multiply(D::add(D::add(a, b), c), 1.0 / 3), bounds,
                             std::uint32_t(t / 3), std::uint32_t(t / 3)});
    }
    return D::TriangleBvh(std::move(triangles));
}
double directedError(const TriangleMesh &source, const TriangleMesh &target,
                     const Jerico::CancellationToken *cancel)
{
    auto bvh = tree(target);
    double maximum = 0;
    for (std::size_t i = 0; i < source.positions.size(); ++i)
    {
        if ((i & 255) == 0)
            check(cancel);
        maximum = std::max(maximum, bvh.closest(source.positions[i], 1e-12).distance);
    }
    for (std::size_t t = 0; t < source.indices.size(); t += 3)
    {
        if ((t % 768) == 0)
            check(cancel);
        auto a = source.positions[source.indices[t]], b = source.positions[source.indices[t + 1]],
             c = source.positions[source.indices[t + 2]];
        for (auto p : {D::multiply(D::add(D::add(a, b), c), 1.0 / 3), D::multiply(D::add(a, b), 0.5),
                       D::multiply(D::add(b, c), 0.5), D::multiply(D::add(c, a), 0.5)})
            maximum = std::max(maximum, bvh.closest(p, 1e-12).distance);
    }
    return maximum;
}
} // namespace
double sampledBidirectionalMeshError(const TriangleMesh &a, const TriangleMesh &b,
                                     const Jerico::CancellationToken *cancel)
{
    return std::max(directedError(a, b, cancel), directedError(b, a, cancel));
}
TriangleMesh reductionCandidate(const TriangleMesh &source, double tolerance, double &sampledError,
                                const Jerico::CancellationToken *cancel)
{
    check(cancel);
    Solid::validate(source);
    if (source.indices.size() / 3 > 240000)
        throw std::runtime_error("Reduction input exceeds triangle budget");
    // The public engine adapter has no arbitrary-mesh simplification entry point.
    // This small consumer uses its already admitted Manifold installation.
    manifold::MeshGL64 input;
    input.numProp = 3;
    for (auto p : source.positions)
        input.vertProperties.insert(input.vertProperties.end(), {p.x, p.y, p.z});
    std::map<Jerico::Guid, std::vector<std::size_t>> groups;
    for (std::size_t t = 0; t < source.triangleRegions.size(); ++t)
        groups[source.triangleRegions[t]].push_back(t);
    auto first = manifold::Manifold::ReserveIDs(std::uint32_t(groups.size()));
    std::map<std::uint32_t, Jerico::Guid> identities;
    std::uint32_t ordinal = 0;
    for (auto &[id, triangles] : groups)
    {
        input.runIndex.push_back(input.triVerts.size());
        input.runOriginalID.push_back(first + ordinal);
        identities[first + ordinal++] = id;
        for (auto t : triangles)
            for (int k = 0; k < 3; ++k)
                input.triVerts.push_back(source.indices[t * 3 + k]);
    }
    input.runIndex.push_back(input.triVerts.size());
    manifold::Manifold solid(input);
    if (solid.Status() != manifold::Manifold::Error::NoError)
        throw std::runtime_error("Native reduction rejected input solid");
    auto simplified = solid.Simplify(tolerance);
    check(cancel);
    if (simplified.Status() != manifold::Manifold::Error::NoError || simplified.IsEmpty())
        throw std::runtime_error("Native reduction failed or became empty");
    auto output = simplified.GetMeshGL64();
    check(cancel);
    if (output.NumTri() > 240000 || output.NumVert() > 240000)
        throw std::runtime_error("Native reduction output exceeds budget");
    std::vector<std::size_t> parent(output.NumVert());
    std::iota(parent.begin(), parent.end(), 0);
    auto root = [&](std::size_t index)
    {
        while (parent[index] != index)
        {
            parent[index] = parent[parent[index]];
            index = parent[index];
        }
        return index;
    };
    for (std::size_t k = 0; k < output.mergeFromVert.size(); ++k)
        parent[root(output.mergeFromVert[k])] = root(output.mergeToVert[k]);
    std::map<std::size_t, std::uint32_t> remap;
    TriangleMesh result;
    for (std::size_t k = 0; k < output.triVerts.size(); ++k)
    {
        auto index = root(output.triVerts[k]);
        auto [at, inserted] = remap.try_emplace(index, std::uint32_t(result.positions.size()));
        if (inserted)
            result.positions.push_back({output.vertProperties[index * output.numProp],
                                        output.vertProperties[index * output.numProp + 1],
                                        output.vertProperties[index * output.numProp + 2]});
        result.indices.push_back(at->second);
    }
    result.triangleRegions.resize(result.indices.size() / 3);
    for (std::size_t run = 0; run < output.runOriginalID.size(); ++run)
        for (std::size_t t = output.runIndex[run] / 3; t < output.runIndex[run + 1] / 3; ++t)
            result.triangleRegions[t] = identities.at(output.runOriginalID[run]);
    result = Solid::validate(result);
    std::set<Jerico::Guid> before(source.triangleRegions.begin(), source.triangleRegions.end()),
        after(result.triangleRegions.begin(), result.triangleRegions.end());
    if (before != after)
        throw std::runtime_error("Reduction removed a protected surface region");
    sampledError = sampledBidirectionalMeshError(source, result, cancel);
    return result;
}
TriangleMesh reduceClosedMesh(const TriangleMesh &source, double error, double &sampledError,
                              const Jerico::CancellationToken *cancel)
{
    // Native local simplification tolerance is not the measured global error.
    // Tighten the former until the independently measured quality gate passes.
    double tolerance = error * 0.5;
    for (int attempt = 0; attempt < 8; ++attempt)
    {
        auto result = reductionCandidate(source, tolerance, sampledError, cancel);
        if (sampledError <= error)
        {
            if (result.indices.size() < source.indices.size())
                return result;
            sampledError = 0;
            return source;
        }
        tolerance *= 0.5;
    }
    sampledError = 0;
    return source;
}
} // namespace Playground
