#pragma once

#include <Jerico/Core/Result.hpp>

#include <array>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

namespace Jerico
{
struct Guid
{
    std::array<std::byte, 16> bytes{};

    [[nodiscard]] bool isZero() const noexcept;

    bool operator==(const Guid&) const = default;
    auto operator<=>(const Guid&) const = default;
};

struct GuidHash
{
    [[nodiscard]] std::size_t operator()(const Guid& guid) const noexcept;
};

[[nodiscard]] std::uint64_t stableGuidHash(const Guid& guid) noexcept;
[[nodiscard]] Result<Guid> parseGuid(std::string_view text);
[[nodiscard]] std::string formatGuid(const Guid& guid);
}
