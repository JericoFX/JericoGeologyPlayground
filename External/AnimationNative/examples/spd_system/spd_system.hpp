// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include <optional>

namespace original_spd {
using Vector = std::array<double, 6>;
using Matrix = std::array<Vector, 6>;

// Original square-root Cholesky example for A*x = -residual.
// Contract: finite, exactly symmetric, positive definite A; finite solution.
// No native layouts, pivot policy, tolerance or status codes are reused.
[[nodiscard]] inline std::optional<Vector> solve(const Matrix& a, const Vector& residual) {
    for (std::size_t i = 0; i < 6; ++i) {
        if (!std::isfinite(residual[i])) return std::nullopt;
        for (std::size_t j = 0; j < 6; ++j) {
            if (!std::isfinite(a[i][j]) || a[i][j] != a[j][i]) return std::nullopt;
        }
    }
    Matrix lower{};
    for (std::size_t i = 0; i < 6; ++i) {
        for (std::size_t j = 0; j <= i; ++j) {
            double value = a[i][j];
            for (std::size_t k = 0; k < j; ++k) value -= lower[i][k]*lower[j][k];
            if (i == j) {
                if (!(value > 0) || !std::isfinite(value)) return std::nullopt;
                lower[i][j] = std::sqrt(value);
            } else {
                lower[i][j] = value / lower[j][j];
                if (!std::isfinite(lower[i][j])) return std::nullopt;
            }
        }
    }
    Vector intermediate{}, output{};
    for (std::size_t i = 0; i < 6; ++i) {
        double value = -residual[i];
        for (std::size_t j = 0; j < i; ++j) value -= lower[i][j]*intermediate[j];
        intermediate[i] = value / lower[i][i];
        if (!std::isfinite(intermediate[i])) return std::nullopt;
    }
    for (std::size_t remaining = 6; remaining > 0; --remaining) {
        const auto i = remaining - 1;
        double value = intermediate[i];
        for (std::size_t j = i + 1; j < 6; ++j) value -= lower[j][i]*output[j];
        output[i] = value / lower[i][i];
        if (!std::isfinite(output[i])) return std::nullopt;
    }
    return output;
}
}
