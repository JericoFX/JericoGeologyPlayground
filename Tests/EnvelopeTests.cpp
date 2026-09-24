#include "Model.hpp"
#include <Jerico/ProceduralAuthoring/SolidEvaluator.hpp>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace Playground;
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
int main()
{
    try
    {
        Recipe recipe;
        Layer envelope;
        envelope.kind = Kind::Envelope;
        envelope.scale = {0.8f, 0.75f, 0.7f};
        for (int shape = 0; shape < 4; ++shape)
        {
            envelope.envelopeShape = shape;
            recipe.layers = {envelope};
            auto mesh = evaluate(recipe, 8);
            require(PA::validateClosedMesh(mesh.mesh).hasValue(), "envelope must produce closed solid");
            require(meshHash(mesh.mesh) != meshHash(evaluate(Recipe{}, 8).mesh),
                    "envelope changes macroform");
            require(mesh.cornerNormals.size() == mesh.mesh.indices.size(),
                    "cut regions expose separate render normals");
            if (shape == 1)
                for (auto n : mesh.cornerNormals)
                    require(std::max({std::abs(n.x), std::abs(n.y), std::abs(n.z)}) > 0.999999,
                            "box face normals are not blended across cut boundaries");
            for (auto p : mesh.mesh.positions)
                require(std::abs(p.x) <= 0.800001 && std::abs(p.y) <= 0.750001 && std::abs(p.z) <= 0.700001,
                        "envelope axial bounds");
            require(PA::hashTriangleMesh(mesh.mesh) ==
                        PA::hashTriangleMesh(evaluate(deserialize(serialize(recipe)), 8).mesh),
                    "envelope recipe round trip preserves geometry and regions");
        }
        envelope.scale = {3, 3, 3};
        recipe.layers = {envelope};
        require(meshHash(evaluate(recipe, 8).mesh) == meshHash(evaluate(Recipe{}, 8).mesh),
                "enclosing envelope is identity");
        envelope.translation = {10, 0, 0};
        recipe.layers = {envelope};
        bool rejected = false;
        try
        {
            evaluate(recipe, 8);
        }
        catch (const std::exception &)
        {
            rejected = true;
        }
        require(rejected, "empty envelope intersection reports error");
        std::cout << "PASS four envelopes: closed topology, bounds, macroform, recipe identity, enclosing "
                     "and empty cases\n";
        return 0;
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
