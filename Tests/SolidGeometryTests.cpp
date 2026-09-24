#include "SolidGeometry.hpp"
#include <Jerico/Jobs/Cancellation.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <set>
#include <stdexcept>

using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
double volume(const TriangleMesh &mesh)
{
    double sum = 0;
    for (std::size_t t = 0; t < mesh.indices.size(); t += 3)
    {
        auto a = mesh.positions[mesh.indices[t]], b = mesh.positions[mesh.indices[t + 1]],
             c = mesh.positions[mesh.indices[t + 2]];
        sum += a.x * (b.y * c.z - b.z * c.y) + a.y * (b.z * c.x - b.x * c.z) + a.z * (b.x * c.y - b.y * c.x);
    }
    return sum / 6;
}
int main()
{
    try
    {
        auto mesh = evaluate(Recipe{}, 4).mesh;
        require(PA::validateClosedMesh(mesh).hasValue(), "engine must accept generated cube");
        require(std::abs(volume(mesh) - 8) < 1e-10, "cube volume is eight cubic units");
        Jerico::CancellationSource cancellation;
        auto cutId = Solid::region(987, 50);
        PA::MeshPlaneCut cut{{0, 1, 0}, 0, cutId, 10000};
        auto halves = PA::splitClosedMeshByPlane(mesh, cut, cancellation.token());
        require(halves.hasValue(), "engine plane split must succeed");
        require(std::abs(volume(halves.value().positive) - 4) < 1e-9, "upper half volume");
        require(std::abs(volume(halves.value().negative) - 4) < 1e-9, "lower half volume");
        std::set<Jerico::Guid> sourceRegions(mesh.triangleRegions.begin(), mesh.triangleRegions.end());
        for (auto *part : {&halves.value().positive, &halves.value().negative})
        {
            require(PA::validateClosedMesh(*part).hasValue(), "closed halves");
            bool foundCut = false;
            for (std::size_t t = 0; t < part->triangleRegions.size(); ++t)
            {
                auto id = part->triangleRegions[t];
                if (id == cutId)
                {
                    foundCut = true;
                    for (int j = 0; j < 3; ++j)
                        require(std::abs(part->positions[part->indices[t * 3 + j]].y) < 1e-10,
                                "cap lies on cut plane");
                }
                else
                    require(sourceRegions.contains(id), "source surface identity preserved");
            }
            require(foundCut, "interior cap region present");
        }
        auto subdivided = PA::subdivideClosedMesh(halves.value().negative, 3, 10000, cancellation.token());
        require(subdivided.hasValue(), "native subdivision");
        require(std::abs(volume(subdivided.value()) - 4) < 1e-9, "subdivision preserves volume");
        Recipe faceted;
        auto planes = defaultLayer(Kind::Planes);
        planes.cutSubdivisions = 1;
        faceted.layers = {planes};
        auto coarse = evaluate(faceted, 8);
        auto inspected = evaluate(faceted, 8, 0, 0);
        require(*std::max_element(inspected.field.begin(), inspected.field.end()) == 1 &&
                    *std::min_element(inspected.field.begin(), inspected.field.end()) == 0,
                "structural field marks new cut faces and preserves source area");
        faceted.layers[0].cutSubdivisions = 3;
        auto dense = evaluate(faceted, 8);
        require(dense.mesh.indices.size() == coarse.mesh.indices.size() * 9,
                "cut tessellation supplies geometry for later deformation");
        require(std::abs(volume(dense.mesh) - volume(coarse.mesh)) < 1e-8,
                "cut tessellation preserves surface volume");
        auto repeated = PA::splitClosedMeshByPlane(mesh, cut, cancellation.token());
        require(repeated.hasValue() && PA::hashTriangleMesh(repeated.value().negative) ==
                                           PA::hashTriangleMesh(halves.value().negative),
                "native split deterministic including regions");
        cut.maximumTriangleCount = 1;
        require(!PA::splitClosedMeshByPlane(mesh, cut, cancellation.token()).hasValue(),
                "native budget rejected");
        cancellation.requestCancellation();
        require(!PA::subdivideClosedMesh(mesh, 3, 10000, cancellation.token()).hasValue(),
                "native cancellation");
        mesh.indices.resize(mesh.indices.size() - 3);
        mesh.triangleRegions.pop_back();
        require(!PA::validateClosedMesh(mesh).hasValue(), "open mesh rejected by engine");
        std::cout << "PASS native engine mesh, closed halves, volume, cap regions, subdivision, determinism, "
                     "limits, cancellation, open input\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
