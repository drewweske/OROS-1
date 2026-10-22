#pragma once

#include "oros/ai/faction_key.hpp"
#include "oros/foundation/result.hpp"

#include <cstdint>

namespace oros::ai
{
    class FactionTrust final
    {
    public:
        static constexpr std::int32_t
            minimum_score{-10000};

        static constexpr std::int32_t
            neutral_score{0};

        static constexpr std::int32_t
            maximum_score{10000};

        [[nodiscard]]
        static foundation::Result<
            FactionTrust>
        create(
            const FactionKey& source,
            const FactionKey& target,
            std::int32_t score);

        FactionTrust(
            const FactionTrust&) =
                default;

        FactionTrust&
        operator=(
            const FactionTrust&) =
                default;

        FactionTrust(
            FactionTrust&&)
            noexcept = default;

        FactionTrust&
        operator=(
            FactionTrust&&)
            noexcept = default;

        [[nodiscard]]
        const FactionKey&
        source() const noexcept;

        [[nodiscard]]
        const FactionKey&
        target() const noexcept;

        [[nodiscard]]
        std::int32_t
        score() const noexcept;

        bool operator==(
            const FactionTrust&)
            const = default;

    private:
        FactionTrust(
            FactionKey source,
            FactionKey target,
            std::int32_t score)
            noexcept;

        FactionKey
            source_;

        FactionKey
            target_;

        std::int32_t
            score_;
    };
}
