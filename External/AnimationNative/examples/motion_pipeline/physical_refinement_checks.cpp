// SPDX-License-Identifier: MIT
#include "physical_refinement.hpp"
#include <bit>
#include <iostream>
#include <limits>
#include <string_view>

namespace {
using namespace JE::Research::MotionPipeline;
std::size_t checks{}, failures{};
void Check(bool condition, std::string_view label) {
    ++checks;
    if (!condition) { ++failures; std::cerr << "FAIL: " << label << '\n'; }
}
void Near(double actual, double expected, std::string_view label) {
    Check(std::isfinite(actual) && std::abs(actual - expected) <= 1e-9, label);
}
template<class F> void Reject(F&& operation, std::string_view label) {
    bool rejected = false;
    try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, label);
}
void Cases() {
    const std::array<Node, 2> free{{{10, {0, 0, 0}, {1, 1, 1}}, {20, {2, 0, 0}, {1, 1, 1}}}};
    const std::array<Segment, 1> edge{{{0, 1, 1}}};
    const auto corrected = RefinePositions(free, edge, {});
    Check(corrected.converged, "free stretched segment converges");
    Near(corrected.positions[0].x, .5, "equal mobility endpoint A");
    Near(corrected.positions[1].x, 1.5, "equal mobility endpoint B");
    Near(corrected.maxLengthResidual, 0, "length residual recomputed after correction");
    Check(corrected.iterations > 0, "a correction consumes an iteration");
    Near(free[0].position.x, 0, "input A preserved");
    Near(free[1].position.x, 2, "input B preserved");

    auto weighted = free;
    weighted[0].inverse_mobility = {2, 1, 1};
    weighted[1].position.x = 3;
    const auto unequal = RefinePositions(weighted, edge, {});
    Check(unequal.converged, "unequal mobility converges");
    Near(unequal.positions[0].x, 4.0 / 3, "weighted endpoint A");
    Near(unequal.positions[1].x, 7.0 / 3, "weighted endpoint B");

    auto large_mobility = free;
    for (auto& node : large_mobility) node.inverse_mobility = {1e308, 1e308, 1e308};
    const auto scaled = RefinePositions(large_mobility, edge, {});
    Check(scaled.converged, "large finite mobilities do not overflow their sum");
    Near(scaled.positions[0].x, .5, "common mobility scaling preserves correction A");
    Near(scaled.positions[1].x, 1.5, "common mobility scaling preserves correction B");
    auto zero_length = edge;
    zero_length[0].rest_length = 0;
    const auto collapsed = RefinePositions(free, zero_length, {});
    Check(collapsed.converged, "zero rest length may collapse a segment");
    Near(collapsed.positions[0].x, 1, "zero-length endpoint A");
    Near(collapsed.positions[1].x, 1, "zero-length endpoint B");

    auto pinned = free;
    pinned[0].inverse_mobility = {0, 0, 0};
    const auto pin = RefinePositions(pinned, edge, {});
    Check(pin.converged, "one pinned endpoint converges");
    Near(pin.positions[0].x, 0, "pin remains exactly supplied");
    Near(pin.positions[1].x, 1, "free endpoint accommodates pin");
    pinned[1].inverse_mobility = {0, 0, 0};
    const auto impossible = RefinePositions(pinned, edge, {});
    Check(!impossible.converged, "incompatible pins cannot report convergence");
    Near(impossible.maxLengthResidual, 1, "incompatible pins retain their error");
    Near(impossible.positions[1].x, 2, "incompatible pin not moved");

    const std::array<Node, 1> penetrating{{{1, {-0.0, -.5, 3}, {0, 1, 0}}}};
    const std::array<Contact, 1> plane{{{0, {0, 1, 0}, .2, .3}}};
    const auto contact = RefinePositions(penetrating, {}, plane);
    Check(contact.converged, "offset plus radius contact corrected");
    Near(contact.positions[0].y, .5, "contact plane and radius included");
    Check(std::bit_cast<std::uint64_t>(contact.positions[0].x) ==
          std::bit_cast<std::uint64_t>(penetrating[0].position.x), "locked signed zero bit preserved");
    Near(contact.positions[0].z, 3, "contact locked Z preserved");
    Near(contact.maxContactViolation, 0, "contact diagnostic recomputed");
    auto above = penetrating;
    above[0].position.y = 2;
    const auto inactive = RefinePositions(above, {}, plane);
    Near(inactive.positions[0].y, 2, "separated contact remains inactive");
    Check(inactive.converged && inactive.iterations == 0, "already feasible pose consumes no iteration");

    const double inv_sqrt_two = 1 / std::sqrt(2.0);
    const std::array<Node, 1> oblique{{{5, {0, 0, 0}, {1, 1, 1}}}};
    const std::array<Contact, 1> tilted{{{0, {inv_sqrt_two, inv_sqrt_two, 0}, 1, 0}}};
    const auto tilted_result = RefinePositions(oblique, {}, tilted);
    Check(tilted_result.converged, "oblique unit plane converges");
    Near(tilted_result.positions[0].x, inv_sqrt_two, "oblique normal X used");
    Near(tilted_result.positions[0].y, inv_sqrt_two, "oblique normal Y used");

    auto locked_contact = penetrating;
    locked_contact[0].inverse_mobility = {0, 0, 0};
    const auto blocked = RefinePositions(locked_contact, {}, plane);
    Check(!blocked.converged, "locked penetrating contact cannot pass");
    Near(blocked.maxContactViolation, 1, "locked contact violation measured");
    Near(blocked.positions[0].y, -.5, "blocked coordinate preserved");

    const std::array<Node, 2> coupled{{{1, {0, 0, 0}, {0, 0, 0}}, {2, {2, -1, 0}, {1, 1, 1}}}};
    const std::array<Contact, 1> floor{{{1, {0, 1, 0}, .5, 0}}};
    const auto joint_constraints = RefinePositions(coupled, edge, floor, {1e-8, 256});
    Check(joint_constraints.converged, "segment and contact jointly converge");
    Check(joint_constraints.maxLengthResidual <= 1e-8 && joint_constraints.maxContactViolation <= 1e-8,
          "both constraint families checked after full sweep");
    Near(joint_constraints.positions[0].x, 0, "coupled root X pinned");
    Near(joint_constraints.positions[0].y, 0, "coupled root Y pinned");
    const auto limited = RefinePositions(coupled, edge, floor, {1e-8, 1});
    Check(!limited.converged && limited.iterations == 1, "one sweep cannot hide remaining length error");
    Check(limited.maxLengthResidual > 1e-8 && limited.maxContactViolation <= 1e-8,
          "contact projection does not certify other constraints");

    const auto exhausted = RefinePositions(free, edge, {}, {1e-9, 0});
    Check(!exhausted.converged && exhausted.iterations == 0, "zero budget diagnoses without false pass");
    Near(exhausted.positions[1].x, 2, "zero budget does not project");
    auto coincident = free;
    coincident[1].position = coincident[0].position;
    const auto singular = RefinePositions(coincident, edge, {});
    Check(!singular.converged && std::isfinite(singular.maxLengthResidual), "coincident positive edge stays unresolved");
    Near(singular.maxLengthResidual, 1, "coincident edge residual retained");

    auto invalid = free;
    invalid[1].id = invalid[0].id;
    Reject([&] { (void)RefinePositions(invalid, edge, {}); }, "duplicate node IDs rejected");
    invalid = free;
    invalid[0].position.x = std::numeric_limits<double>::quiet_NaN();
    Reject([&] { (void)RefinePositions(invalid, edge, {}); }, "nonfinite position rejected");
    invalid = free;
    invalid[0].inverse_mobility[0] = -1;
    Reject([&] { (void)RefinePositions(invalid, edge, {}); }, "negative mobility rejected");
    invalid = free;
    invalid[0].inverse_mobility[0] = std::numeric_limits<double>::infinity();
    Reject([&] { (void)RefinePositions(invalid, edge, {}); }, "nonfinite mobility rejected");
    auto bad_edge = edge;
    bad_edge[0].b = free.size();
    Reject([&] { (void)RefinePositions(free, bad_edge, {}); }, "segment index rejected");
    bad_edge = edge;
    bad_edge[0].rest_length = -1;
    Reject([&] { (void)RefinePositions(free, bad_edge, {}); }, "negative rest length rejected");
    bad_edge = edge;
    bad_edge[0].a = bad_edge[0].b;
    Reject([&] { (void)RefinePositions(free, bad_edge, {}); }, "self edge rejected");
    auto bad_plane = plane;
    bad_plane[0].normal = {};
    Reject([&] { (void)RefinePositions(penetrating, {}, bad_plane); }, "zero normal rejected");
    bad_plane = plane;
    bad_plane[0].normal = {0, 2, 0};
    Reject([&] { (void)RefinePositions(penetrating, {}, bad_plane); }, "nonunit normal rejected");
    bad_plane = plane;
    bad_plane[0].nodeIndex = penetrating.size();
    Reject([&] { (void)RefinePositions(penetrating, {}, bad_plane); }, "contact index rejected");
    bad_plane = plane;
    bad_plane[0].radius = -1;
    Reject([&] { (void)RefinePositions(penetrating, {}, bad_plane); }, "negative radius rejected");
    Reject([&] { (void)RefinePositions(free, edge, {}, {-1, 8}); }, "negative tolerance rejected");
}
}

int main() {
    try { Cases(); }
    catch (const std::exception& error) { std::cerr << "Unexpected exception: " << error.what() << '\n'; return 1; }
    std::cout << checks << " checks, " << failures << " failures\n";
    return failures ? 1 : 0;
}
