#pragma once

#include "oros/ai/actor_vision_state.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <cstddef>
#include <span>
#include <type_traits>
#include <vector>

namespace oros::ai
{
    static_assert(
        std::is_nothrow_move_constructible_v<ActorVisionState>);
    static_assert(
        std::is_nothrow_move_assignable_v<ActorVisionState>);

    // One validated vision configuration per actor. The order is
    // canonical by EntityId, regardless of insertion order.
    //
    // The caller is responsible for validating that each EntityId
    // belongs to the authoritative World. Stored references must
    // not outlive registry insertions, removals or destruction.
    class ActorVisionStateRegistry final
    {
    public:
        ActorVisionStateRegistry() = default;

        ActorVisionStateRegistry(
            const ActorVisionStateRegistry&) = default;
        ActorVisionStateRegistry& operator=(
            const ActorVisionStateRegistry&) = default;
        ActorVisionStateRegistry(
            ActorVisionStateRegistry&&) noexcept = default;
        ActorVisionStateRegistry& operator=(
            ActorVisionStateRegistry&&) noexcept = default;

        [[nodiscard]]
        foundation::Status insert(ActorVisionState state);

        [[nodiscard]]
        foundation::Status remove(world::EntityId actor);

        [[nodiscard]]
        ActorVisionState* find(
            world::EntityId actor) noexcept;

        [[nodiscard]]
        const ActorVisionState* find(
            world::EntityId actor) const noexcept;

        [[nodiscard]]
        bool contains(world::EntityId actor) const noexcept;

        [[nodiscard]]
        std::size_t size() const noexcept;

        [[nodiscard]]
        bool empty() const noexcept;

        [[nodiscard]]
        std::span<const ActorVisionState>
        states_in_entity_order() const noexcept;

    private:
        [[nodiscard]]
        std::size_t lower_bound_index(
            world::EntityId actor) const noexcept;

        std::vector<ActorVisionState> states_{};
    };
}
