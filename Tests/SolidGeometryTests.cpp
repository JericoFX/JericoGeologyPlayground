#include "SolidGeometry.hpp"
#include "SurfaceFeatures.hpp"
#include <Jerico/Jobs/Cancellation.hpp>
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
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
        auto shear = defaultLayer(Kind::Planes);
        shear.direction = {0.8f, 0, 0.6f};
        shear.amplitude = 0.12f;
        auto groundedPieces = Solid::fractureShear(mesh, shear, nullptr);
        require(PA::validateClosedMesh(groundedPieces).hasValue(),
                "sheared fracture must retain two closed solids");
        require(std::abs(volume(groundedPieces) - 8) < 1e-8,
                "sheared fracture conserves solid volume");
        require(groundedPieces.positions.size() > mesh.positions.size() &&
                    groundedPieces.triangleRegions.size() == groundedPieces.indices.size() / 3,
                "sheared fracture retains both pieces and classified faces");
        std::vector<std::uint32_t> parents(groundedPieces.positions.size());
        for (std::uint32_t i = 0; i < parents.size(); ++i)
            parents[i] = i;
        auto root = [&](std::uint32_t i)
        {
            while (parents[i] != i)
                i = parents[i];
            return i;
        };
        for (std::size_t t = 0; t < groundedPieces.indices.size(); t += 3)
        {
            const auto a = root(groundedPieces.indices[t]);
            parents[root(groundedPieces.indices[t + 1])] = a;
            parents[root(groundedPieces.indices[t + 2])] = a;
        }
        struct Piece
        {
            double floor{1e20}, minPlane{1e20}, maxPlane{-1e20};
            double supportArea{};
            unsigned cutFaces{};
        };
        std::map<std::uint32_t, Piece> pieces;
        const auto shearNormalLength = std::hypot(double(shear.direction[0]), double(shear.direction[2]));
        const auto shearNormalX = double(shear.direction[0]) / shearNormalLength;
        const auto shearNormalZ = double(shear.direction[2]) / shearNormalLength;
        const auto shearCutId = Solid::region(shear.seed, PA::hashTriangleMesh(mesh) ^ 0x50000U);
        double shearPlaneOffset = 0;
        bool foundShearCut = false;
        for (std::size_t face = 0; face < groundedPieces.triangleRegions.size(); ++face)
            if (groundedPieces.triangleRegions[face] == shearCutId)
            {
                const auto p = groundedPieces.positions[groundedPieces.indices[3 * face]];
                shearPlaneOffset = shearNormalX * p.x + shearNormalZ * p.z;
                foundShearCut = true;
                break;
            }
        require(foundShearCut, "fracture plane remains identifiable");
        for (std::uint32_t i = 0; i < groundedPieces.positions.size(); ++i)
        {
            const auto p = groundedPieces.positions[i];
            auto &piece = pieces[root(i)];
            piece.floor = std::min(piece.floor, p.y);
            const auto signedPlane = shearNormalX * p.x + shearNormalZ * p.z - shearPlaneOffset;
            piece.minPlane = std::min(piece.minPlane, signedPlane);
            piece.maxPlane = std::max(piece.maxPlane, signedPlane);
        }
        for (std::size_t face = 0; face < groundedPieces.triangleRegions.size(); ++face)
        {
            auto &piece = pieces[root(groundedPieces.indices[3 * face])];
            if (groundedPieces.triangleRegions[face] == shearCutId)
                ++piece.cutFaces;
            const auto a = groundedPieces.positions[groundedPieces.indices[3 * face]];
            const auto b = groundedPieces.positions[groundedPieces.indices[3 * face + 1]];
            const auto c = groundedPieces.positions[groundedPieces.indices[3 * face + 2]];
            if (std::abs(a.y + 1) < 1e-7 && std::abs(b.y + 1) < 1e-7 &&
                std::abs(c.y + 1) < 1e-7)
                piece.supportArea += 0.5 * std::abs((b.x - a.x) * (c.z - a.z) -
                                                    (b.z - a.z) * (c.x - a.x));
        }
        require(pieces.size() == 2, "fracture has exactly two disconnected solids");
        unsigned negativeParts = 0, positiveParts = 0;
        for (const auto &[id, piece] : pieces)
        {
            require(std::abs(piece.floor + 1) < 1e-10 && piece.supportArea > 1e-5 &&
                        piece.cutFaces > 0,
                    "both fracture pieces have floor support area and retain a cut face");
            negativeParts += piece.maxPlane <= 1e-9;
            positiveParts += piece.minPlane >= -1e-9;
        }
        require(negativeParts == 1 && positiveParts == 1,
                "the cut pieces remain in opposite halfspaces without volume overlap");
        require(PA::hashTriangleMesh(groundedPieces) ==
                    PA::hashTriangleMesh(Solid::fractureShear(mesh, shear, nullptr)),
                "sheared fracture is deterministic");
        Recipe shearRecipe;
        shearRecipe.version = 11;
        shearRecipe.layers = {defaultLayer(Kind::FractureShear)};
        auto recipeOutput = evaluate(shearRecipe, 4);
        require(PA::validateClosedMesh(recipeOutput.mesh).hasValue(),
                "recipe fracture remains a closed source");
        bool hasFractureRegion = false;
        for (const auto &region : recipeOutput.regions)
            hasFractureRegion |= region.role == SurfaceRole::FractureCut;
        require(hasFractureRegion &&
                    PA::hashTriangleMesh(recipeOutput.mesh) ==
                        PA::hashTriangleMesh(evaluate(deserialize(serialize(shearRecipe)), 4).mesh),
                "recipe v11 preserves the fracture cut and deterministic geometry");
        shearRecipe.version = 10;
        bool rejectedOldVersion = false;
        try { validate(shearRecipe); } catch (const std::exception &) { rejectedOldVersion = true; }
        require(rejectedOldVersion, "fracture shear cannot be encoded in recipe v10");
        shearRecipe.version = 11;
        shearRecipe.layers[0].amplitude = 0;
        bool rejectedZeroSlip = false;
        try { validate(shearRecipe); } catch (const std::exception &) { rejectedZeroSlip = true; }
        require(rejectedZeroSlip, "fracture shear rejects coincident caps at zero slip");
        shear.amplitude = 0;
        rejectedZeroSlip = false;
        try { Solid::fractureShear(mesh, shear, nullptr); }
        catch (const std::exception &) { rejectedZeroSlip = true; }
        require(rejectedZeroSlip, "direct fracture shear rejects zero slip");
        shear.amplitude = 0.12f;
        auto fourBlocks = Solid::fractureQuadrants(mesh, shear, nullptr);
        require(PA::validateClosedMesh(fourBlocks).hasValue(),
                "four-block fracture remains closed");
        require(surfaceFeatures(fourBlocks, {.contact = false}).signedDihedralCurvature.size() ==
                    fourBlocks.positions.size(),
                "four-block mass has manifold feature edges");
        require(volume(fourBlocks) > 4 && volume(fourBlocks) < 7.9,
                "four-block fracture removes bounded upper material");
        std::vector<std::uint32_t> blockParents(fourBlocks.positions.size());
        for (std::uint32_t i = 0; i < blockParents.size(); ++i)
            blockParents[i] = i;
        auto blockRoot = [&](std::uint32_t i)
        {
            while (blockParents[i] != i)
                i = blockParents[i];
            return i;
        };
        for (std::size_t t = 0; t < fourBlocks.indices.size(); t += 3)
        {
            const auto a = blockRoot(fourBlocks.indices[t]);
            blockParents[blockRoot(fourBlocks.indices[t + 1])] = a;
            blockParents[blockRoot(fourBlocks.indices[t + 2])] = a;
        }
        std::map<std::uint32_t, double> blockAreas;
        for (std::size_t t = 0; t < fourBlocks.indices.size(); t += 3)
        {
            const auto a = fourBlocks.positions[fourBlocks.indices[t]];
            const auto b = fourBlocks.positions[fourBlocks.indices[t + 1]];
            const auto c = fourBlocks.positions[fourBlocks.indices[t + 2]];
            auto &area = blockAreas[blockRoot(fourBlocks.indices[t])];
            if (std::abs(a.y + 1) < 1e-7 && std::abs(b.y + 1) < 1e-7 &&
                std::abs(c.y + 1) < 1e-7)
                area += 0.5 * std::abs((b.x - a.x) * (c.z - a.z) -
                                       (b.z - a.z) * (c.x - a.x));
        }
        require(blockAreas.size() == 1, "four chiseled blocks join into one closed mass");
        for (const auto &[id, area] : blockAreas)
            require(area > 1e-5, "every block has positive ground support area");
        const auto blockSourceHash = PA::hashTriangleMesh(mesh);
        for (std::uint64_t block = 0; block < 4; ++block)
        {
            const auto capId = Solid::region(shear.seed, blockSourceHash ^ (0x62000U + block));
            require(std::find(fourBlocks.triangleRegions.begin(), fourBlocks.triangleRegions.end(),
                              capId) != fourBlocks.triangleRegions.end(),
                    "all four independently chiseled top faces remain exposed");
        }
        require(PA::hashTriangleMesh(fourBlocks) ==
                    PA::hashTriangleMesh(Solid::fractureQuadrants(mesh, shear, nullptr)),
                "four-block fracture is deterministic");
        Jerico::CancellationSource stoppedBlocks;
        stoppedBlocks.requestCancellation();
        bool cancelledBlocks = false;
        auto stoppedToken = stoppedBlocks.token();
        try { Solid::fractureQuadrants(mesh, shear, &stoppedToken); }
        catch (const std::exception &) { cancelledBlocks = true; }
        require(cancelledBlocks, "four-block fracture never publishes after cancellation");
        Recipe blockRecipe;
        blockRecipe.version = 12;
        blockRecipe.layers = {defaultLayer(Kind::FractureQuadrants)};
        auto blockOutput = evaluate(blockRecipe, 4);
        require(PA::hashTriangleMesh(blockOutput.mesh) ==
                    PA::hashTriangleMesh(evaluate(deserialize(serialize(blockRecipe)), 4).mesh),
                "recipe v12 round-trips deterministic four-block geometry");
        unsigned chiselRegions = 0;
        for (const auto &region : blockOutput.regions)
            chiselRegions += region.role == SurfaceRole::BlockChisel;
        require(chiselRegions == 4, "four chiseled faces have classified recipe identity");
        blockRecipe.version = 11;
        bool rejectedOldBlockVersion = false;
        try { validate(blockRecipe); }
        catch (const std::exception &) { rejectedOldBlockVersion = true; }
        require(rejectedOldBlockVersion, "four-block recipe requires v12");
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
