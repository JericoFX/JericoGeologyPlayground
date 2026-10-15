// SPDX-License-Identifier: MIT
#include "products.hpp"
#include "native_fixtures_v1.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
std::size_t checks = 0;
double maximumError = 0;
void require(bool condition) {
    ++checks;
    if (!condition) throw std::runtime_error("Original coupled-system check failed");
}
void near(double actual, double expected) {
    maximumError = std::max(maximumError, std::abs(actual-expected));
    require(std::isfinite(actual) && std::abs(actual-expected) <= 1e-10);
}
}

int main() {
    try {
        for (const auto& fixture : fixtures) {
            const auto actual = original_coupled::products(fixture.j, fixture.w);
            require(actual.has_value());
            for (std::size_t i = 0; i < 6; ++i) {
                for (std::size_t k = 0; k < 12; ++k) near(actual->left[i][k], fixture.f[i][k]);
                for (std::size_t k = 0; k < 6; ++k) near(actual->reduced[i][k], fixture.a[i][k]);
            }
            for (std::size_t i = 0; i < 12; ++i) {
                for (std::size_t k = 0; k < 6; ++k) near(actual->right[i][k], fixture.r[i][k]);
            }
        }
        const original_spd::Vector residual{-29,27,-21,44,-65,102};
        const auto delta = original_coupled::solve(fixtures[0].j, fixtures[0].w, residual);
        require(delta.has_value());
        constexpr std::array<double,12> known{-5,0,21,-44,65,-102,0,17,0,0,0,0};
        for (std::size_t i = 0; i < 12; ++i) near((*delta)[i], known[i]);
        for (std::size_t i = 0; i < 6; ++i) {
            double error = residual[i];
            for (std::size_t k = 0; k < 12; ++k) error += fixtures[0].j[i][k]*(*delta)[k];
            near(error, 0);
        }
        require(!original_coupled::solve(fixtures[4].j, fixtures[4].w, residual));
        auto j = fixtures[0].j;
        j[0][0] = std::numeric_limits<double>::infinity();
        require(!original_coupled::products(j, fixtures[0].w));
        auto w = fixtures[0].w;
        w[0][0] = std::numeric_limits<double>::quiet_NaN();
        require(!original_coupled::products(fixtures[0].j, w));
        j = fixtures[0].j;
        j[0][0] = std::numeric_limits<double>::max();
        require(!original_coupled::products(j, fixtures[0].w));
        require(!original_coupled::solve(fixtures[0].j, fixtures[0].w, residual, -1));
        std::cout << "fixtures=" << fixtures.size() << " checks=" << checks << " maxError=" << maximumError << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
