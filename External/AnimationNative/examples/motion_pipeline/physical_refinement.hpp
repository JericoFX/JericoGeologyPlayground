// SPDX-License-Identifier: MIT
#pragma once
#include "../../playground/animation_core.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <unordered_set>
#include <vector>

namespace JE::Research::MotionPipeline {
using Vec3 = JE::AnimationTools::Playground::Vec3;
struct Node {
    std::uint64_t id{};
    Vec3 position{};
    std::array<double, 3> inverse_mobility{1, 1, 1};
};
struct Segment { std::size_t a{}, b{}; double rest_length{}; };
struct Contact { std::size_t nodeIndex{}; Vec3 normal{}; double offset{}, radius{}; };
struct RefinementSettings { double tolerance{1e-6}; std::size_t iteration_budget{128}; };
struct RefinementResult {
    std::vector<Vec3> positions;
    std::size_t iterations{};
    double maxLengthResidual{}, maxContactViolation{};
    bool converged{};
};

namespace RefinementDetail {
namespace Math = JE::AnimationTools::Playground;
inline void Validate(std::span<const Node> nodes, std::span<const Segment> segments,
        std::span<const Contact> contacts, const RefinementSettings& settings) {
    Math::Finite(settings.tolerance);
    if (settings.tolerance < 0) throw std::invalid_argument("negative refinement tolerance");
    std::unordered_set<std::uint64_t> ids;
    for (const auto& node : nodes) {
        if (!ids.insert(node.id).second) throw std::invalid_argument("duplicate refinement node ID");
        Math::Finite(node.position);
        for (double mobility : node.inverse_mobility) {
            Math::Finite(mobility);
            if (mobility < 0) throw std::invalid_argument("negative inverse mobility");
        }
    }
    for (const auto& segment : segments) {
        Math::Finite(segment.rest_length);
        if (segment.a >= nodes.size() || segment.b >= nodes.size() || segment.a == segment.b ||
                segment.rest_length < 0) throw std::invalid_argument("invalid refinement segment");
    }
    for (const auto& contact : contacts) {
        Math::Finite(contact.normal); Math::Finite(contact.offset); Math::Finite(contact.radius);
        const double norm = Math::Length(contact.normal);
        if (contact.nodeIndex >= nodes.size() || contact.radius < 0 ||
                !std::isfinite(norm) || std::abs(norm - 1) > 1e-8)
            throw std::invalid_argument("contact requires an in-range node and unit normal");
    }
}
inline void Measure(RefinementResult& result, std::span<const Segment> segments,
        std::span<const Contact> contacts, double tolerance) {
    result.maxLengthResidual = 0;
    result.maxContactViolation = 0;
    for (const auto& segment : segments) {
        const double length = Math::Length(result.positions[segment.b] - result.positions[segment.a]);
        Math::Finite(length);
        result.maxLengthResidual = std::max(result.maxLengthResidual, std::abs(length - segment.rest_length));
    }
    for (const auto& contact : contacts) {
        const double gap = Math::Dot(contact.normal, result.positions[contact.nodeIndex]) - contact.offset - contact.radius;
        Math::Finite(gap);
        result.maxContactViolation = std::max(result.maxContactViolation, std::max(0.0, -gap));
    }
    result.converged = result.maxLengthResidual <= tolerance && result.maxContactViolation <= tolerance;
}
inline double MaximumMobility(const std::array<double, 3>& mobility) {
    return std::max({mobility[0], mobility[1], mobility[2]});
}
inline Vec3 WeightedGradient(Vec3 gradient, const std::array<double, 3>& mobility, double maximum) {
    // A common positive scale cancels in W*J^T/(J*W*J^T).
    return {gradient.x * (mobility[0] / maximum), gradient.y * (mobility[1] / maximum),
            gradient.z * (mobility[2] / maximum)};
}
inline Vec3 Correction(Vec3 weighted_gradient, double denominator, double residual) {
    const Vec3 delta{(weighted_gradient.x / denominator) * residual,
                     (weighted_gradient.y / denominator) * residual,
                     (weighted_gradient.z / denominator) * residual};
    Math::Finite(delta);
    return delta;
}
inline bool Apply(Vec3& position, const std::array<double, 3>& mobility, Vec3 delta) {
    Vec3 candidate = position;
    if (mobility[0] > 0) candidate.x += delta.x;
    if (mobility[1] > 0) candidate.y += delta.y;
    if (mobility[2] > 0) candidate.z += delta.z;
    Math::Finite(candidate);
    const bool changed = candidate.x != position.x || candidate.y != position.y || candidate.z != position.z;
    position = candidate;
    return changed;
}
}

// Own geometric constraints in one caller-declared space. No native solver ABI.
// Zero mobility preserves the corresponding input coordinate exactly.
// A zero iteration budget measures the supplied positions without correcting them.
[[nodiscard]] inline RefinementResult RefinePositions(std::span<const Node> nodes,
        std::span<const Segment> segments, std::span<const Contact> contacts,
        const RefinementSettings& settings = {}) {
    RefinementDetail::Validate(nodes, segments, contacts, settings);
    RefinementResult result;
    result.positions.reserve(nodes.size());
    for (const auto& node : nodes) result.positions.push_back(node.position);
    RefinementDetail::Measure(result, segments, contacts, settings.tolerance);
    if (result.converged) return result;

    namespace Detail = RefinementDetail;
    for (std::size_t iteration = 0; iteration < settings.iteration_budget; ++iteration) {
        bool changed = false;
        for (const auto& segment : segments) {
            const Vec3 difference = result.positions[segment.b] - result.positions[segment.a];
            const double length = Detail::Math::Length(difference);
            Detail::Math::Finite(length);
            const double residual = length - segment.rest_length;
            if (std::abs(residual) <= settings.tolerance || length == 0) continue;
            const auto& a_mobility = nodes[segment.a].inverse_mobility;
            const auto& b_mobility = nodes[segment.b].inverse_mobility;
            const double maximum = std::max(Detail::MaximumMobility(a_mobility), Detail::MaximumMobility(b_mobility));
            if (maximum == 0) continue;
            // c=|b-a|-L; J_a=-n and J_b=n. Avoid reciprocal overflow for tiny lengths.
            const Vec3 normal{difference.x / length, difference.y / length, difference.z / length};
            const Vec3 a_gradient = Detail::WeightedGradient(normal, a_mobility, maximum);
            const Vec3 b_gradient = Detail::WeightedGradient(normal, b_mobility, maximum);
            const double denominator = Detail::Math::Dot(normal, a_gradient + b_gradient);
            Detail::Math::Finite(denominator);
            if (denominator <= 0) continue;
            const Vec3 a_delta = Detail::Correction(a_gradient, denominator, residual);
            const Vec3 b_delta = Detail::Correction(b_gradient, denominator, -residual);
            const bool a_changed = Detail::Apply(result.positions[segment.a], a_mobility, a_delta);
            const bool b_changed = Detail::Apply(result.positions[segment.b], b_mobility, b_delta);
            changed = changed || a_changed || b_changed;
        }
        for (const auto& contact : contacts) {
            const double gap = Detail::Math::Dot(contact.normal, result.positions[contact.nodeIndex]) -
                               contact.offset - contact.radius;
            Detail::Math::Finite(gap);
            if (gap >= 0) continue;
            const auto& mobility = nodes[contact.nodeIndex].inverse_mobility;
            const double maximum = Detail::MaximumMobility(mobility);
            if (maximum == 0) continue;
            const Vec3 gradient = Detail::WeightedGradient(contact.normal, mobility, maximum);
            const double denominator = Detail::Math::Dot(contact.normal, gradient);
            Detail::Math::Finite(denominator);
            if (denominator <= 0) continue;
            // Active c=n.p-offset-radius<0, J=n; delta=-W*J^T*c/(J*W*J^T).
            const Vec3 delta = Detail::Correction(gradient, denominator, -gap);
            const bool contact_changed = Detail::Apply(result.positions[contact.nodeIndex], mobility, delta);
            changed = changed || contact_changed;
        }
        result.iterations = iteration + 1;
        Detail::Measure(result, segments, contacts, settings.tolerance);
        if (result.converged || !changed) break;
    }
    return result;
}
}
