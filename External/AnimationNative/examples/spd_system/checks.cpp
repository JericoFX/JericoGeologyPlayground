// SPDX-License-Identifier: MIT
#include "spd_system.hpp"
#include "native_fixtures_v1.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
std::size_t checks = 0;
double maxError = 0;
void require(bool value) {
    ++checks;
    if (!value) throw std::runtime_error("Original SPD check failed");
}
void near(double actual, double expected, double tolerance) {
    maxError = std::max(maxError, std::abs(actual - expected));
    require(std::isfinite(actual) && std::abs(actual - expected) <= tolerance);
}
}

int main() {
    try {
        constexpr original_spd::Vector known{1,-2,3,-4,5,-6};
        for (const auto& fixture : fixtures) {
            const auto result = original_spd::solve(fixture.matrix, fixture.residual);
            require(result.has_value());
            for (std::size_t i = 0; i < 6; ++i) {
                near((*result)[i], known[i], 1e-11);
                near((*result)[i], fixture.nativeOutput[i], 1e-11);
                double residual = fixture.residual[i];
                for (std::size_t j = 0; j < 6; ++j) residual += fixture.matrix[i][j]*(*result)[j];
                near(residual, 0, 1e-10);
            }
            auto doubled = fixture.residual;
            for (auto& value : doubled) value *= 2;
            const auto twice = original_spd::solve(fixture.matrix, doubled);
            require(twice.has_value());
            for (std::size_t i = 0; i < 6; ++i) near((*twice)[i], 2*known[i], 2e-11);
        }
        auto matrix = fixtures[0].matrix;
        auto residual = fixtures[0].residual;
        matrix[0][1] = 1;
        require(!original_spd::solve(matrix, residual));
        matrix = fixtures[0].matrix;
        matrix[0][0] = 0;
        require(!original_spd::solve(matrix, residual));
        matrix[0][0] = -1;
        require(!original_spd::solve(matrix, residual));
        matrix[0][0] = std::numeric_limits<double>::quiet_NaN();
        require(!original_spd::solve(matrix, residual));
        matrix = fixtures[0].matrix;
        residual[0] = std::numeric_limits<double>::infinity();
        require(!original_spd::solve(matrix, residual));
        std::cout << "fixtures=" << fixtures.size() << " checks=" << checks << " maxError=" << maxError << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
