#include "AutoUv.hpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <limits>
#include <map>
#include <numbers>
#include <queue>
#include <stdexcept>
#include <string>
using namespace Playground;
namespace
{
void require(bool value, const char *message)
{
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action, const char *message)
{
    bool rejected = false;
    try { action(); }
    catch (const std::exception &) { rejected = true; }
    require(rejected, message);
}
void requireConnectedCharts(const TriangleMesh &mesh, const AutoUvPlan &plan)
{
    const auto count = mesh.indices.size() / 3;
    require(plan.triangleIsland.size() == count, "chart labels cover every triangle");
    std::map<std::pair<std::uint32_t, std::uint32_t>, std::vector<std::size_t>> edges;
    for (std::size_t face = 0; face < count; ++face)
        for (int k = 0; k < 3; ++k)
        {
            auto a = mesh.indices[3 * face + k], b = mesh.indices[3 * face + (k + 1) % 3];
            edges[{std::min(a, b), std::max(a, b)}].push_back(face);
        }
    std::vector<std::vector<std::size_t>> neighbors(count);
    for (const auto &[edge, faces] : edges)
        if (faces.size() == 2 && plan.triangleIsland[faces[0]] == plan.triangleIsland[faces[1]])
        {
            neighbors[faces[0]].push_back(faces[1]);
            neighbors[faces[1]].push_back(faces[0]);
        }
    std::vector<std::size_t> sizes(plan.islandCount), first(plan.islandCount, count);
    for (std::size_t face = 0; face < count; ++face)
    {
        auto island = plan.triangleIsland[face];
        require(island < plan.islandCount, "chart label in range");
        ++sizes[island];
        first[island] = std::min(first[island], face);
    }
    std::vector<bool> seen(count);
    for (std::size_t island = 0; island < plan.islandCount; ++island)
    {
        require(sizes[island] > 0, "no empty charts");
        std::queue<std::size_t> queue;
        queue.push(first[island]);
        seen[first[island]] = true;
        std::size_t reached = 0;
        while (!queue.empty())
        {
            auto face = queue.front();
            queue.pop();
            ++reached;
            for (auto adjacent : neighbors[face])
                if (!seen[adjacent])
                {
                    seen[adjacent] = true;
                    queue.push(adjacent);
                }
        }
        require(reached == sizes[island], "UV chart remains edge-connected after splitting");
    }
}
} // namespace
int main(int argc, char **argv)
{
    try
    {
        if (argc == 2 && (std::string(argv[1]) == "--family-probe" ||
                          std::string(argv[1]) == "--family-probe-full"))
        {
            const bool full = std::string(argv[1]) == "--family-probe-full";
            int failures = 0;
            for (int family : {5, 7, 8, 9, 10, 11, 12})
            {
                auto recipe = preset(family);
                auto resolution = full ? recipe.finalResolution : std::min(recipe.finalResolution, 16);
                auto low = evaluate(recipe, resolution);
                auto start = std::chrono::steady_clock::now();
                try
                {
                    auto candidate = planAutomaticUv(low.mesh);
                    auto milliseconds = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start).count();
                    require(candidate.cornerUv.size() == low.mesh.indices.size() &&
                                candidate.activeAreaFraction > 0 && candidate.activeAreaFraction < 1,
                            "family chart result incomplete");
                    requireConnectedCharts(low.mesh, candidate);
                    std::cout << family << " resolution=" << resolution
                              << " triangles=" << low.mesh.indices.size() / 3
                              << " charts=" << candidate.islandCount
                              << " occupancy=" << candidate.activeAreaFraction
                              << " subpixel=" << candidate.subpixelTriangles
                              << " uvMs=" << milliseconds << " ACCEPTED\n";
                }
                catch (const std::exception &error)
                {
                    ++failures;
                    auto milliseconds = std::chrono::duration<double, std::milli>(
                        std::chrono::steady_clock::now() - start).count();
                    std::cout << family << " resolution=" << resolution
                              << " triangles=" << low.mesh.indices.size() / 3
                              << " uvMs=" << milliseconds << " REJECTED " << error.what() << '\n';
                }
            }
            return failures ? 2 : 0;
        }
        Recipe recipe;
        recipe.base = Base::Plane;
        recipe.planeSize = {2, 2};
        auto plane = evaluate(recipe, 8);
        AutoUvSettings settings;
        settings.resolution = 512;
        require(requiredUvMarginPixels(settings) == 8,
                "four mip levels need eight pixels of safety margin");
        auto plan = planAutomaticUv(plane.mesh, settings);
        requireConnectedCharts(plane.mesh, plan);
        require(plan.islandCount == 1 && plan.marginPixels == 8 &&
                    plan.cornerUv.size() == plane.mesh.indices.size() &&
                    plan.triangleIsland.size() == plane.mesh.indices.size() / 3,
                "connected flat source creates one automatic UV island");
        require(plan.activeAreaFraction > 0.7 && plan.activeAreaFraction < 1,
                "planar unwrap uses atlas area efficiently without filling the safety border");
        for (auto uv : plan.cornerUv)
            require(std::isfinite(uv[0]) && std::isfinite(uv[1]) &&
                        uv[0] >= 0 && uv[0] <= 1 && uv[1] >= 0 && uv[1] <= 1,
                    "automatic UV corners are finite inside UV0");
        auto replay = planAutomaticUv(plane.mesh, settings);
        require(plan.cornerUv == replay.cornerUv && plan.triangleIsland == replay.triangleIsland &&
                    plan.pixelsPerMeter == replay.pixelsPerMeter,
                "automatic UV plan repeats deterministically");
        auto tinyPlane = plane.mesh;
        for (auto &position : tinyPlane.positions)
        {
            position.x *= 1e-7;
            position.y *= 1e-7;
            position.z *= 1e-7;
        }
        auto tinyPlan = planAutomaticUv(tinyPlane, settings);
        require(tinyPlan.islandCount == plan.islandCount &&
                    tinyPlan.cornerUv.size() == plan.cornerUv.size(),
                "valid small-scale mesh is not rejected by an absolute area threshold");
        auto cube = evaluate(Recipe{}, 8);
        auto cubePlan = planAutomaticUv(cube.mesh, settings);
        requireConnectedCharts(cube.mesh, cubePlan);
        require(cubePlan.islandCount >= 6 && cubePlan.islandCount < cube.mesh.indices.size() / 3 &&
                    cubePlan.activeAreaFraction > 0 && cubePlan.activeAreaFraction < 1,
                "closed cube groups connected faces while preserving orientation seams");
        TriangleMesh overlappingRamp;
        constexpr int steps = 48;
        for (int step = 0; step <= steps; ++step)
        {
            auto angle = 4 * std::numbers::pi * step / steps;
            for (double radius : {1.0, 2.0})
                overlappingRamp.positions.push_back({radius * std::cos(angle), radius * std::sin(angle),
                                                     0.02 * angle});
        }
        for (int step = 0; step < steps; ++step)
        {
            auto base = std::uint32_t(2 * step);
            overlappingRamp.indices.insert(overlappingRamp.indices.end(),
                                           {base, base + 1, base + 2,
                                            base + 1, base + 3, base + 2});
            overlappingRamp.triangleRegions.resize(overlappingRamp.triangleRegions.size() + 2);
        }
        auto rampPlan = planAutomaticUv(overlappingRamp, settings);
        requireConnectedCharts(overlappingRamp, rampPlan);
        require(rampPlan.islandCount >= 2 && rampPlan.activeAreaFraction > 0,
                "two-turn ramp is split into non-overlapping UV charts");
        auto rampReplay = planAutomaticUv(overlappingRamp, settings);
        require(rampPlan.cornerUv == rampReplay.cornerUv &&
                    rampPlan.triangleIsland == rampReplay.triangleIsland,
                "overlap-driven chart splitting is deterministic");
        auto translatedRamp = overlappingRamp;
        for (auto &position : translatedRamp.positions)
        {
            position.x += 1e8;
            position.y += 1e8;
            position.z += 1e8;
        }
        auto translatedPlan = planAutomaticUv(translatedRamp, settings);
        require(translatedPlan.triangleIsland == rampPlan.triangleIsland,
                "overlap-driven splitting is invariant under a large rigid translation");
        settings.mipCount = 5;
        require(requiredUvMarginPixels(settings) == 16,
                "mip margin follows target level count");
        settings.resolution = 64;
        settings.mipCount = 12;
        rejects([&] { requiredUvMarginPixels(settings); },
                "impossible high-mip margin rejected before packing");
        settings = {};
        auto invalid = plane.mesh;
        invalid.positions[0].x = std::numeric_limits<double>::quiet_NaN();
        rejects([&] { planAutomaticUv(invalid, settings); }, "non-finite source rejected");
        Jerico::CancellationSource cancellation;
        cancellation.requestCancellation();
        auto token = cancellation.token();
        rejects([&] { planAutomaticUv(plane.mesh, settings, &token); }, "planning cancels");
        std::cout << "PASS automatic planar UV contract\n";
        return 0;
    }
    catch (const std::exception &error)
    {
        std::cerr << "FAIL " << error.what() << '\n';
        return 1;
    }
}
