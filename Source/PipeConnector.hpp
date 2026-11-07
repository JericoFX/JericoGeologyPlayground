// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "PipeSockets.hpp"
#include <string>
#include <vector>

// Automatic socket-to-socket routing: given two sockets (frame = position, forward, up),
// compute the pipe centreline that leaves the first along its forward and arrives at the
// second against its forward, with the fewest elbows. Pure geometry.
namespace Playground::Pipes {
struct ConnectorSettings {
    double tangentToleranceRadians{0.001};
    double minimumLegMeters{0.02};
};
enum class ConnectorState { Straight, SingleElbow, DoubleElbow, Unconnectable };
struct ConnectorResult {
    ConnectorState state{ConnectorState::Unconnectable};
    std::vector<Vec3d> path;   // from.position ... to.position.
    double lengthMeters{};
    double startTangentError{}, endTangentError{};
    std::string reason;
};
ConnectorResult connect(const Frame& from, const Frame& to, const ConnectorSettings& = {});
const char* name(ConnectorState);

// Inline valve placement: cut the routed centreline at a fraction of its length and leave
// a body of the requested length between the two remaining runs. The valve frame carries
// the axis and the body length; the caller sweeps the body with a larger radius.
struct ValvePlacement {
    bool placed{};
    std::vector<Vec3d> before, after; // Trimmed centrelines.
    Frame valve{};                    // position = centre, forward = run direction.
    double lengthMeters{}, radiusMeters{};
    std::string reason;
};
ValvePlacement insertValve(const ConnectorResult& route,double fraction,double lengthMeters,double radiusMeters);
}
