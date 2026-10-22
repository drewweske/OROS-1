#pragma once

#include "oros/ai/actor_vision_profile.hpp"
#include "oros/ai/actor_visual_observation.hpp"
#include "oros/foundation/result.hpp"
#include "oros/world/entity_id.hpp"

#include <utility>

namespace oros::ai
{
    // Validated, persistent visual capabilities for one actor.
    // The application does not own this NPC's vision settings.
    class ActorVisionState final
    {
    public:
        [[nodiscard]]
        static foundation::Result<ActorVisionState>
        create(
            world::EntityId actor,
            ActorVisionDirection forward,
            ActorVisionProfile profile,
            ActorVisualObservationConfidence confidence)
        {
            if (
                !actor.is_valid() ||
                !forward.is_valid() ||
                !profile.is_valid() ||
                !confidence.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Actor vision state requires a valid "
                    "actor, direction, profile and confidence.");
            }

            return ActorVisionState{
                actor,
                std::move(forward),
                std::move(profile),
                std::move(confidence)
            };
        }

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                actor_.is_valid() &&
                forward_.is_valid() &&
                profile_.is_valid() &&
                confidence_.is_valid();
        }

        [[nodiscard]]
        world::EntityId actor() const noexcept
        {
            return actor_;
        }

        [[nodiscard]]
        const ActorVisionDirection&
        forward() const noexcept
        {
            return forward_;
        }

        [[nodiscard]]
        const ActorVisionProfile&
        profile() const noexcept
        {
            return profile_;
        }

        [[nodiscard]]
        const ActorVisualObservationConfidence&
        confidence() const noexcept
        {
            return confidence_;
        }

        // Turning preserves identity and visual memory.
        [[nodiscard]]
        foundation::Status face(
            ActorVisionDirection new_forward)
        {
            if (!new_forward.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Actor facing direction must be valid.");
            }

            forward_ = std::move(new_forward);
            return {};
        }

        [[nodiscard]]
        foundation::Status set_profile(
            ActorVisionProfile new_profile)
        {
            if (!new_profile.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Actor vision profile must be valid.");
            }

            profile_ = std::move(new_profile);
            return {};
        }

        [[nodiscard]]
        foundation::Status set_confidence(
            ActorVisualObservationConfidence new_confidence)
        {
            if (!new_confidence.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::invalid_argument,
                    "Actor observation confidence must be valid.");
            }

            confidence_ = std::move(new_confidence);
            return {};
        }

    private:
        ActorVisionState(
            world::EntityId actor,
            ActorVisionDirection forward,
            ActorVisionProfile profile,
            ActorVisualObservationConfidence confidence)
            : actor_{actor},
              forward_{std::move(forward)},
              profile_{std::move(profile)},
              confidence_{std::move(confidence)}
        {
        }

        world::EntityId actor_{};
        ActorVisionDirection forward_;
        ActorVisionProfile profile_;
        ActorVisualObservationConfidence confidence_;
    };
}
