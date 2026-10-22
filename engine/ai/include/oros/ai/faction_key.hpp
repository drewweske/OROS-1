#pragma once

#include "oros/foundation/result.hpp"

#include <string>
#include <string_view>

namespace oros::ai
{
    class FactionKey final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            FactionKey>
        create(
            std::string_view faction_namespace,
            std::string_view faction_name);

        FactionKey(
            const FactionKey&) =
                default;

        FactionKey&
        operator=(
            const FactionKey&) =
                default;

        FactionKey(
            FactionKey&&)
            noexcept = default;

        FactionKey&
        operator=(
            FactionKey&&)
            noexcept = default;

        [[nodiscard]]
        std::string_view
        faction_namespace() const noexcept;

        [[nodiscard]]
        std::string_view
        faction_name() const noexcept;

        bool operator==(
            const FactionKey&)
            const = default;

    private:
        FactionKey(
            std::string faction_namespace,
            std::string faction_name)
            noexcept;

        std::string
            faction_namespace_{};

        std::string
            faction_name_{};
    };
}
