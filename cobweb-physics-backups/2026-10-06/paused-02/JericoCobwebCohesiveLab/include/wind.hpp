#pragma once
#include "physics.hpp"
#include <cstdint>

namespace web::physics {
// Artistic world-space air velocity and bounded drag, not calibrated silk data.
struct WindSettings {
    V3 velocity{};
    double gust=.25,frequency=.4,drag=.02,maxParticleForce=.02;
    std::uint64_t seed=42;
};

// Evaluate once before a physical step. This does not modify the graph or settings.
std::vector<Force> windForces(const Graph& graph,const WindSettings& settings);
}
