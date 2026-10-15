// SPDX-License-Identifier: MIT
#include "motion_chain.hpp"
#include <bit>
#include <iostream>
#include <limits>

namespace MP = JE::Research::MotionPipeline;
namespace {
std::size_t checks{};
void Require(bool value) {
    ++checks;
    if (!value) throw std::runtime_error("motion chain check failed");
}
template<class F> void Reject(F&& operation) {
    bool rejected{};
    try { operation(); } catch (const std::invalid_argument&) { rejected = true; }
    Require(rejected);
}
}
int main() {
    try {
        // Stored bindings deliberately differ from row order. A proposed free
        // endpoint penetrates a plane and stretches a segment from a fixed root.
        std::vector<MP::RowBinding> bindings{
            {1, "limb", "data-a", MP::PositionChannel::GlobalPosition},
            {0, "root", "data-z", MP::PositionChannel::GlobalPosition},
            {2, "other", "data-b", MP::PositionChannel::GlobalPosition}};
        MP::CurrentRigFrame current{0, "scene-revision", "topology-revision", "own/world/units",
            {{"data-a", {1, 0, -0.0}}, {"data-z", {0, 0, 0}}, {"data-b", {3, 4, 5}}},
            {{"data-a", MP::AxisLockMask{4}}, {"data-z", MP::AxisLockMask{0}}, {"data-b", MP::AxisLockMask{0}}}};
        MP::PositionProposal proposal{std::string(64, 'a'), std::string(64, 'b'), 0,
            current.scene_fingerprint, current.topology_fingerprint, current.coordinate_contract,
            {{20, 20, 20}, {2, -1, 9}, {50, 50, 50}}, {"data-a"}};
        const std::array<std::array<double, 3>, 3> weights{{{1, 1, 1}, {1, 1, 1}, {1, 1, 1}}};
        const std::array<MP::Segment, 1> segments{{{0, 1, 1}}};
        const std::array<MP::Contact, 1> contacts{{{1, {0, 1, 0}, .5, 0}}};
        const auto solved = MP::RefinePositionApplication(bindings, current, proposal,
            weights, segments, contacts, {1e-9, 256});
        Require(solved.refinement.converged && solved.application.has_value());
        Require(solved.application->changes.size() == 1);
        const auto& change = solved.application->changes[0];
        Require(change.object_id == "limb" && change.data_id == "data-a" && change.row == 1);
        Require(change.after.y >= .5 && std::abs(std::hypot(change.after.x, change.after.y) - 1) <= 1e-9);
        Require(change.blocked_axes == MP::AxisLockMask{4});
        Require(std::bit_cast<std::uint64_t>(change.after.z) == std::bit_cast<std::uint64_t>(-0.0));
        Require(solved.refinement.positions[0].x == 0 && solved.refinement.positions[0].y == 0);
        Require(solved.refinement.positions[2].x == 3 && solved.refinement.positions[2].y == 4);
        Require(current.values_by_data_id.at("data-a").x == 1 && proposal.values_by_row[1].x == 2);
        auto permuted = bindings;
        std::swap(permuted[0], permuted[2]);
        const auto reordered = MP::RefinePositionApplication(permuted, current, proposal,
            weights, segments, contacts, {1e-9, 256});
        Require(reordered.application && reordered.application->changes[0].after.x == change.after.x);
        Require(reordered.application->changes[0].after.y == change.after.y);
        auto impossible = proposal;
        impossible.selected_data_ids.clear();
        const auto blocked = MP::RefinePositionApplication(bindings, current, impossible,
            weights, segments, contacts);
        Require(!blocked.refinement.converged && !blocked.application);
        Require(blocked.refinement.maxContactViolation == .5);
        const auto exhausted = MP::RefinePositionApplication(bindings, current, proposal,
            weights, segments, contacts, {1e-9, 0});
        Require(!exhausted.application && exhausted.refinement.iterations == 0);
        auto invalid_weights = weights;
        invalid_weights[2][0] = std::numeric_limits<double>::quiet_NaN();
        Reject([&] { (void)MP::RefinePositionApplication(bindings, current, proposal,
            invalid_weights, segments, contacts); });
        Reject([&] { (void)MP::RefinePositionApplication(bindings, current, proposal,
            std::span(weights).first(2), segments, contacts); });
        auto stale = proposal;
        stale.topology_fingerprint = "old-topology";
        Reject([&] { (void)MP::RefinePositionApplication(bindings, current, stale,
            weights, segments, contacts); });
        std::cout << "{\"status\":\"pass\",\"checks\":" << checks
                  << ",\"nativeTargetStarted\":false,\"modelExecuted\":false}\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
