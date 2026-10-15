// SPDX-License-Identifier: MIT
#pragma once
#include "../spd_system/spd_system.hpp"
#include <array>
#include <cmath>
#include <optional>

namespace original_coupled {
using JMatrix = std::array<std::array<double,12>,6>;
using WMatrix = std::array<std::array<double,12>,12>;
using RightMatrix = std::array<std::array<double,6>,12>;
using Delta = std::array<double,12>;
struct Products {
    JMatrix left{};
    original_spd::Matrix reduced{};
    RightMatrix right{};
};

// Original row-major standard matrix products. No native packing or descriptors.
[[nodiscard]] inline std::optional<Products> products(const JMatrix& j, const WMatrix& w) {
    for (const auto& row : j) for (double x : row) if (!std::isfinite(x)) return std::nullopt;
    for (const auto& row : w) for (double x : row) if (!std::isfinite(x)) return std::nullopt;
    Products result;
    for (std::size_t i = 0; i < 6; ++i) {
        for (std::size_t c = 0; c < 12; ++c) {
            for (std::size_t k = 0; k < 12; ++k) result.left[i][c] += j[i][k]*w[k][c];
            if (!std::isfinite(result.left[i][c])) return std::nullopt;
        }
        for (std::size_t c = 0; c < 6; ++c) {
            for (std::size_t k = 0; k < 12; ++k) result.reduced[i][c] += result.left[i][k]*j[c][k];
            if (!std::isfinite(result.reduced[i][c])) return std::nullopt;
        }
    }
    for (std::size_t i = 0; i < 12; ++i) {
        for (std::size_t c = 0; c < 6; ++c) {
            for (std::size_t k = 0; k < 12; ++k) result.right[i][c] += w[i][k]*j[c][k];
            if (!std::isfinite(result.right[i][c])) return std::nullopt;
        }
    }
    return result;
}

// Original reduced solve. SPD/exact-symmetry rejection comes from our Cholesky.
[[nodiscard]] inline std::optional<Delta> solve(const JMatrix& j, const WMatrix& w,
        const original_spd::Vector& residual, double regularization = 0) {
    if (!std::isfinite(regularization) || regularization < 0) return std::nullopt;
    auto p = products(j,w);
    if (!p) return std::nullopt;
    for (std::size_t i = 0; i < 6; ++i) p->reduced[i][i] += regularization;
    const auto lambda = original_spd::solve(p->reduced,residual);
    if (!lambda) return std::nullopt;
    Delta result{};
    for (std::size_t i = 0; i < 12; ++i) {
        for (std::size_t k = 0; k < 6; ++k) result[i] += p->right[i][k]*(*lambda)[k];
        if (!std::isfinite(result[i])) return std::nullopt;
    }
    return result;
}
}
