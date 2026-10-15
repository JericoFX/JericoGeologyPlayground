// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Contributors to cascadeur-engine-research
// Original positions-only planning contract; no private ABI or native invocation.
#pragma once
#include "../../playground/animation_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace JE::Research::MotionPipeline {
// Only the numeric type is reused. Space, units and normalization come from the
// explicit coordinate contract, not animation_core's algorithm conventions.
using Vec3 = JE::AnimationTools::Playground::Vec3;
enum class PositionChannel { LocalPosition, GlobalPosition };
using AxisLockMask = std::uint8_t; // Own contract: X=1, Y=2, Z=4; no native enum.
struct RowBinding {
    std::size_t row{};
    std::string object_id, data_id;
    PositionChannel channel{PositionChannel::LocalPosition};
};
struct CurrentRigFrame {
    std::int64_t frame{};
    std::string scene_fingerprint, topology_fingerprint, coordinate_contract;
    std::map<std::string, Vec3> values_by_data_id;
    std::map<std::string, AxisLockMask> axis_locks_by_data_id;
};
struct PositionProposal {
    std::string model_sha256, input_sha256;
    std::int64_t frame{};
    std::string scene_fingerprint, topology_fingerprint, coordinate_contract;
    std::vector<Vec3> values_by_row;
    std::set<std::string> selected_data_ids;
};
struct PositionChange {
    std::size_t row{};
    std::string object_id, data_id;
    PositionChannel channel{PositionChannel::LocalPosition};
    Vec3 before{}, after{};
    AxisLockMask axis_lock_mask{}, blocked_axes{};
};
struct PositionApplicationPlan {
    std::int64_t frame{};
    std::string scene_fingerprint, topology_fingerprint, coordinate_contract;
    std::string model_sha256, input_sha256;
    // Includes selected no-ops/fully blocked requests so the caller gets a report.
    // DataId order is stable. This plan neither mutates a rig nor opens a session.
    std::vector<PositionChange> changes;
};

// Validate every bound row, including unselected/locked rows, before planning.
// Fingerprints/contracts are caller declarations, not proof of native parity.
// No normalization, unit conversion, model-state decoding or rig mutation occurs.
inline PositionApplicationPlan PlanPositionApplication(std::span<const RowBinding> bindings,
        const CurrentRigFrame& current, const PositionProposal& proposal) {
    const auto valid_sha = [](const std::string& value) {
        return value.size() == 64 && std::all_of(value.begin(), value.end(), [](char c) {
            return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        });
    };
    const auto finite = [](Vec3 value) {
        return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
    };
    if (!valid_sha(proposal.model_sha256) || !valid_sha(proposal.input_sha256))
        throw std::invalid_argument("position proposal requires model/input SHA-256 identities");
    if (current.frame < 0 || proposal.frame != current.frame)
        throw std::invalid_argument("position proposal frame differs from current frame");
    if (current.scene_fingerprint.empty() || current.topology_fingerprint.empty() ||
        proposal.scene_fingerprint != current.scene_fingerprint ||
        proposal.topology_fingerprint != current.topology_fingerprint)
        throw std::invalid_argument("position proposal revision is missing or stale");
    if (current.coordinate_contract.empty() || proposal.coordinate_contract != current.coordinate_contract)
        throw std::invalid_argument("position proposal coordinate contract is missing or different");
    if (bindings.empty() || bindings.size() != proposal.values_by_row.size())
        throw std::invalid_argument("position proposal requires one XYZ value for every bound row");

    std::vector<bool> seen_rows(bindings.size(), false);
    std::set<std::string> object_ids;
    std::map<std::string, const RowBinding*> by_data_id;
    for (const auto& binding : bindings) {
        if (binding.row >= bindings.size() || seen_rows[binding.row])
            throw std::invalid_argument("position row is out of range or duplicated");
        seen_rows[binding.row] = true;
        if (binding.object_id.empty() || binding.data_id.empty() ||
            !object_ids.insert(binding.object_id).second ||
            !by_data_id.emplace(binding.data_id, &binding).second)
            throw std::invalid_argument("position destination identity is missing or duplicated");
        if (binding.channel != PositionChannel::LocalPosition && binding.channel != PositionChannel::GlobalPosition)
            throw std::invalid_argument("position destination is not a supported float3 channel");
        const auto value = current.values_by_data_id.find(binding.data_id);
        const auto locks = current.axis_locks_by_data_id.find(binding.data_id);
        if (value == current.values_by_data_id.end() || locks == current.axis_locks_by_data_id.end())
            throw std::invalid_argument("current position or axis lock mask is unknown");
        if (locks->second > AxisLockMask{7})
            throw std::invalid_argument("axis lock mask has unsupported bits");
        if (!finite(value->second) || !finite(proposal.values_by_row[binding.row]))
            throw std::invalid_argument("current/proposed position contains a nonfinite component");
    }
    for (const auto& data_id : proposal.selected_data_ids)
        if (!by_data_id.contains(data_id))
            throw std::invalid_argument("selected position DataId has no row binding");

    PositionApplicationPlan result{current.frame, current.scene_fingerprint, current.topology_fingerprint,
        current.coordinate_contract, proposal.model_sha256, proposal.input_sha256, {}};
    result.changes.reserve(proposal.selected_data_ids.size());
    for (const auto& data_id : proposal.selected_data_ids) {
        const auto& binding = *by_data_id.at(data_id);
        const Vec3 before = current.values_by_data_id.at(data_id);
        Vec3 after = proposal.values_by_row[binding.row];
        const AxisLockMask locks = current.axis_locks_by_data_id.at(data_id);
        AxisLockMask blocked{0};
        const auto preserve_axis = [&](double baseline, double& requested, AxisLockMask axis) {
            if ((locks & axis) == 0) return;
            // Signed-zero requests are changes too; preserve the captured value.
            if (requested != baseline || std::signbit(requested) != std::signbit(baseline))
                blocked = static_cast<AxisLockMask>(blocked | axis);
            requested = baseline;
        };
        preserve_axis(before.x, after.x, AxisLockMask{1});
        preserve_axis(before.y, after.y, AxisLockMask{2});
        preserve_axis(before.z, after.z, AxisLockMask{4});
        result.changes.push_back({binding.row, binding.object_id, binding.data_id,
            binding.channel, before, after, locks, blocked});
    }
    return result;
}
} // namespace JE::Research::MotionPipeline
