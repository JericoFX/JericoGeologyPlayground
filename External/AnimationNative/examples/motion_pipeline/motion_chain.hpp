// SPDX-License-Identifier: MIT
#pragma once
#include "physical_refinement.hpp"
#include "rig_application.hpp"
#include <optional>

namespace JE::Research::MotionPipeline {
struct RefinedApplication {
    RefinementResult refinement;
    std::optional<PositionApplicationPlan> application;
};

// Original positions-only pipeline. All positions/constraints must already be
// in the proposal's explicitly declared coordinate contract. No native decoder.
// Constraints index explicit rows, never the storage order of RowBinding.
// Unselected channels and locked axes anchor to the captured current values.
// Selected zero-mobility axes anchor to the requested (lock-filtered) values.
[[nodiscard]] inline RefinedApplication RefinePositionApplication(
        std::span<const RowBinding> bindings, const CurrentRigFrame& current,
        const PositionProposal& proposal,
        std::span<const std::array<double, 3>> inverse_mobility_by_row,
        std::span<const Segment> segments, std::span<const Contact> contacts,
        const RefinementSettings& settings = {}) {
    const auto preliminary = PlanPositionApplication(bindings, current, proposal);
    if (inverse_mobility_by_row.size() != bindings.size())
        throw std::invalid_argument("one mobility vector required per explicit row");
    for (const auto& mobility : inverse_mobility_by_row)
        for (double value : mobility)
            if (!std::isfinite(value) || value < 0)
                throw std::invalid_argument("invalid declared inverse mobility");
    std::vector<Node> nodes(bindings.size());
    for (const auto& binding : bindings) {
        // Solve-local unique indices; persistent ObjectId/DataId remain in the
        // binding/application plan and are not replaced by these temporary IDs.
        nodes[binding.row] = {static_cast<std::uint64_t>(binding.row) + 1,
                             current.values_by_data_id.at(binding.data_id), {0, 0, 0}};
    }
    for (const auto& change : preliminary.changes) {
        auto& node = nodes[change.row];
        node.position = change.after;
        node.inverse_mobility = inverse_mobility_by_row[change.row];
        for (std::size_t axis = 0; axis < 3; ++axis)
            if ((change.axis_lock_mask & (1U << axis)) != 0)
                node.inverse_mobility[axis] = 0;
    }
    RefinedApplication result{RefinePositions(nodes, segments, contacts, settings), std::nullopt};
    if (!result.refinement.converged)
        return result; // Diagnostics retained; no application of infeasible data.
    auto refined = proposal;
    refined.values_by_row = result.refinement.positions;
    result.application = PlanPositionApplication(bindings, current, refined);
    for (std::size_t i = 0; i < result.application->changes.size(); ++i)
        result.application->changes[i].blocked_axes = static_cast<AxisLockMask>(
            result.application->changes[i].blocked_axes | preliminary.changes[i].blocked_axes);
    return result;
}
}
