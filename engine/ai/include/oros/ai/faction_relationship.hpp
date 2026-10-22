#pragma once

#include "oros/ai/faction_key.hpp"
#include "oros/foundation/result.hpp"

#include <cstdint>

namespace oros::ai
{
    enum class FactionRelationshipDisposition
        : std::uint8_t
    {
        invalid = 0,
        neutral = 1,
        allied = 2,
        hostile = 3
    };

    class FactionRelationship final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            FactionRelationship>
        create(
            const FactionKey& source,
            const FactionKey& target,
            FactionRelationshipDisposition disposition);

        FactionRelationship(
            const FactionRelationship&) =
                default;

        FactionRelationship&
        operator=(
            const FactionRelationship&) =
                default;

        FactionRelationship(
            FactionRelationship&&)
            noexcept = default;

        FactionRelationship&
        operator=(
            FactionRelationship&&)
            noexcept = default;

        [[nodiscard]]
        const FactionKey&
        source() const noexcept;

        [[nodiscard]]
        const FactionKey&
        target() const noexcept;

        [[nodiscard]]
        FactionRelationshipDisposition
        disposition() const noexcept;

        bool operator==(
            const FactionRelationship&)
            const = default;

    private:
        FactionRelationship(
            FactionKey source,
            FactionKey target,
            FactionRelationshipDisposition disposition)
            noexcept;

        FactionKey
            source_;

        FactionKey
            target_;

        FactionRelationshipDisposition
            disposition_;
    };
}
