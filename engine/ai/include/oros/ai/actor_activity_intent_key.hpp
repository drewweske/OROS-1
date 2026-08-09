#pragma once

#include "oros/foundation/result.hpp"

#include <string>
#include <string_view>

namespace oros::ai
{
    class ActorActivityIntentKey final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            ActorActivityIntentKey>
        create(
            std::string_view intent_namespace,
            std::string_view intent_name);

        ActorActivityIntentKey(
            const ActorActivityIntentKey&) =
                default;

        ActorActivityIntentKey&
        operator=(
            const ActorActivityIntentKey&) =
                default;

        ActorActivityIntentKey(
            ActorActivityIntentKey&&)
            noexcept = default;

        ActorActivityIntentKey&
        operator=(
            ActorActivityIntentKey&&)
            noexcept = default;

        [[nodiscard]]
        std::string_view
        intent_namespace() const noexcept;

        [[nodiscard]]
        std::string_view
        intent_name() const noexcept;

        bool operator==(
            const ActorActivityIntentKey&)
            const = default;

    private:
        ActorActivityIntentKey(
            std::string intent_namespace,
            std::string intent_name)
            noexcept;

        std::string
            intent_namespace_{};

        std::string
            intent_name_{};
    };
}