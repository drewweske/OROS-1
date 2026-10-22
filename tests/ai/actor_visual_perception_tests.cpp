#include "oros/ai/actor_visual_perception.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <utility>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nActor visual perception test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }

    [[nodiscard]]
    bool activate_cell(
        oros::physical_world::
            WorldCellColliderRegistry& registry,
        const oros::streaming::
            WorldCellKey cell_key,
        const std::span<
            const oros::physics::
                ColliderGeometry>
            colliders,
        const std::uint64_t revision,
        const std::uint64_t request_id)
    {
        using namespace oros::physical_world;
        using namespace oros::streaming;

        const auto collider_set_result =
            WorldCellColliderSet::create(
                cell_key,
                colliders);

        if (!collider_set_result.has_value())
        {
            return false;
        }

        const auto payload_result =
            serialize_world_cell_collider_payload(
                collider_set_result.value());

        if (!payload_result.has_value())
        {
            return false;
        }

        const auto snapshot_result =
            WorldCellSnapshot::create(
                cell_key,
                revision,
                std::span<const std::byte>{
                    payload_result.value()
                });

        if (!snapshot_result.has_value())
        {
            return false;
        }

        auto residency_result =
            WorldCellResidency::create(
                cell_key);

        if (!residency_result.has_value())
        {
            return false;
        }

        WorldCellResidency residency =
            std::move(
                residency_result.value());

        const WorldCellRevisionId revision_id{
            cell_key,
            revision
        };

        if (!residency.queue_load(
                revision_id,
                request_id).
                has_value())
        {
            return false;
        }

        if (!residency.begin_load(
                request_id).
                has_value())
        {
            return false;
        }

        if (!residency.complete_load(
                request_id,
                snapshot_result.value()).
                has_value())
        {
            return false;
        }

        return
            registry.synchronize(
                residency).
                has_value();
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        !is_valid_actor_visual_perception_state(
            ActorVisualPerceptionState::
                invalid) &&
            is_valid_actor_visual_perception_state(
                ActorVisualPerceptionState::
                    out_of_range) &&
            is_valid_actor_visual_perception_state(
                ActorVisualPerceptionState::
                    outside_field_of_view) &&
            is_valid_actor_visual_perception_state(
                ActorVisualPerceptionState::
                    visible) &&
            is_valid_actor_visual_perception_state(
                ActorVisualPerceptionState::
                    occluded) &&
            is_valid_actor_visual_perception_state(
                ActorVisualPerceptionState::
                    unavailable),
        "Visual perception states expose an explicit invalid sentinel");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000009ULL
        };

    const EntityId observer{
        world_namespace,
        1000ULL
    };

    const EntityId target{
        world_namespace,
        2000ULL
    };

    const EntityId occluder{
        world_namespace,
        3000ULL
    };

    const EntityId foreign_target{
        world_namespace + 1ULL,
        2000ULL
    };

    const WorldCellKey cell_zero{
        world_namespace,
        WorldCell{
            0,
            0,
            0
        }
    };

    const WorldCellKey cell_one{
        world_namespace,
        WorldCell{
            1,
            0,
            0
        }
    };

    const auto origin_result =
        WorldPosition::origin();

    const auto front_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                10.0,
                0.0,
                0.0
            });

    const auto far_front_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                20.0,
                0.0,
                0.0
            });

    const auto side_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                10.0,
                0.0
            });

    const auto forward_result =
        ActorVisionDirection::create(
            WorldDisplacement{
                1.0,
                0.0,
                0.0
            });

    const auto profile_result =
        ActorVisionProfile::create(
            15.0,
            90.0);

    check(
        state,
        origin_result.has_value() &&
            front_result.has_value() &&
            far_front_result.has_value() &&
            side_result.has_value() &&
            forward_result.has_value() &&
            profile_result.has_value(),
        "Visual perception geometric fixtures are created");

    if (
        !origin_result.has_value() ||
        !front_result.has_value() ||
        !far_front_result.has_value() ||
        !side_result.has_value() ||
        !forward_result.has_value() ||
        !profile_result.has_value())
    {
        return finish(state);
    }

    const WorldPosition origin =
        origin_result.value();

    const ActorVisionDirection forward =
        forward_result.value();

    const ActorVisionProfile profile =
        profile_result.value();

    WorldCellColliderRegistry
        missing_registry{};

    const auto invalid_observer_result =
        query_actor_visual_perception(
            missing_registry,
            invalid_entity_id,
            origin,
            forward,
            profile,
            target,
            front_result.value());

    check(
        state,
        !invalid_observer_result.has_value() &&
            invalid_observer_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "Visual perception rejects invalid observer identity");

    const auto invalid_target_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            invalid_entity_id,
            front_result.value());

    check(
        state,
        !invalid_target_result.has_value() &&
            invalid_target_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "Visual perception rejects invalid target identity");

    const auto foreign_target_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            foreign_target,
            front_result.value());

    check(
        state,
        !foreign_target_result.has_value() &&
            foreign_target_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "Visual perception rejects cross-namespace actor identity");

    const auto self_target_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            observer,
            origin);

    check(
        state,
        !self_target_result.has_value() &&
            self_target_result.
                    error().code ==
                ErrorCode::invalid_argument,
        "Visual perception requires distinct observer and target");

    const auto out_of_range_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            far_front_result.value());

    check(
        state,
        out_of_range_result.has_value() &&
            out_of_range_result.value() ==
                ActorVisualPerceptionState::
                    out_of_range,
        "Range rejection occurs before Physical World LOS");

    const auto outside_fov_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            side_result.value());

    check(
        state,
        outside_fov_result.has_value() &&
            outside_fov_result.value() ==
                ActorVisualPerceptionState::
                    outside_field_of_view,
        "FOV rejection occurs before Physical World LOS");

    const auto missing_los_result =
        query_actor_visual_perception(
            missing_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            front_result.value());

    check(
        state,
        missing_los_result.has_value() &&
            missing_los_result.value() ==
                ActorVisualPerceptionState::
                    unavailable,
        "Missing Physical World knowledge becomes unavailable");

    WorldCellColliderRegistry
        empty_resident_registry{};

    check(
        state,
        activate_cell(
            empty_resident_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            1ULL,
            101ULL),
        "Empty resident perception cell activates");

    const auto clear_visibility_result =
        query_actor_visual_perception(
            empty_resident_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            front_result.value());

    check(
        state,
        clear_visibility_result.has_value() &&
            clear_visibility_result.value() ==
                ActorVisualPerceptionState::
                    visible,
        "Fully resident clear segment makes target visible without a target collider");

    const auto actor_shape_result =
        SphereShape::create(
            1.0);

    const auto occluder_shape_result =
        SphereShape::create(
            0.5);

    check(
        state,
        actor_shape_result.has_value() &&
            occluder_shape_result.has_value(),
        "Visual LOS collider shapes are created");

    if (
        !actor_shape_result.has_value() ||
        !occluder_shape_result.has_value())
    {
        return finish(state);
    }

    const ColliderId observer_primary{
        observer,
        1U
    };

    const ColliderId observer_secondary{
        observer,
        2U
    };

    const ColliderId target_collider{
        target,
        1U
    };

    const ColliderId occluder_collider{
        occluder,
        1U
    };

    const auto observer_primary_geometry_result =
        ColliderGeometry::create(
            observer_primary,
            CollisionShape{
                actor_shape_result.value()
            },
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            });

    const auto observer_secondary_geometry_result =
        ColliderGeometry::create(
            observer_secondary,
            CollisionShape{
                actor_shape_result.value()
            },
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    const auto target_geometry_result =
        ColliderGeometry::create(
            target_collider,
            CollisionShape{
                actor_shape_result.value()
            },
            PhysicsVector3{
                10.0,
                0.0,
                0.0
            });

    const auto occluder_geometry_result =
        ColliderGeometry::create(
            occluder_collider,
            CollisionShape{
                occluder_shape_result.value()
            },
            PhysicsVector3{
                5.0,
                0.0,
                0.0
            });

    check(
        state,
        observer_primary_geometry_result.
                has_value() &&
            observer_secondary_geometry_result.
                has_value() &&
            target_geometry_result.has_value() &&
            occluder_geometry_result.has_value(),
        "Visual LOS collider geometry is created");

    if (
        !observer_primary_geometry_result.
            has_value() ||
        !observer_secondary_geometry_result.
            has_value() ||
        !target_geometry_result.has_value() ||
        !occluder_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        3U>
        observer_and_target_colliders{
            observer_primary_geometry_result.value(),
            observer_secondary_geometry_result.value(),
            target_geometry_result.value()
        };

    WorldCellColliderRegistry
        target_hit_registry{};

    check(
        state,
        activate_cell(
            target_hit_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{
                    observer_and_target_colliders
                },
            2ULL,
            102ULL),
        "Observer and target collider registry activates");

    const auto target_hit_visibility_result =
        query_actor_visual_perception(
            target_hit_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            front_result.value());

    check(
        state,
        target_hit_visibility_result.
                has_value() &&
            target_hit_visibility_result.value() ==
                ActorVisualPerceptionState::
                    visible,
        "Observer shape slots are excluded and target-owned first hit is visible");

    const std::array<
        ColliderGeometry,
        4U>
        observer_target_occluder_colliders{
            observer_primary_geometry_result.value(),
            observer_secondary_geometry_result.value(),
            target_geometry_result.value(),
            occluder_geometry_result.value()
        };

    WorldCellColliderRegistry
        occluded_registry{};

    check(
        state,
        activate_cell(
            occluded_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{
                    observer_target_occluder_colliders
                },
            3ULL,
            103ULL),
        "Occluded visual perception registry activates");

    const auto occluded_result =
        query_actor_visual_perception(
            occluded_registry,
            observer,
            origin,
            forward,
            profile,
            target,
            front_result.value());

    check(
        state,
        occluded_result.has_value() &&
            occluded_result.value() ==
                ActorVisualPerceptionState::
                    occluded,
        "Unrelated first collider hit makes target occluded");

    const auto boundary_observer_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                500.0,
                0.0,
                0.0
            });

    const auto boundary_target_result =
        WorldPosition::create(
            WorldCell{
                1,
                0,
                0
            },
            LocalPosition{
                -500.0,
                0.0,
                0.0
            });

    const auto boundary_profile_result =
        ActorVisionProfile::create(
            30.0,
            0.0);

    check(
        state,
        boundary_observer_result.has_value() &&
            boundary_target_result.has_value() &&
            boundary_profile_result.has_value(),
        "Cross-cell visual perception fixtures are created");

    if (
        !boundary_observer_result.has_value() ||
        !boundary_target_result.has_value() ||
        !boundary_profile_result.has_value())
    {
        return finish(state);
    }

    WorldCellColliderRegistry
        cross_cell_registry{};

    check(
        state,
        activate_cell(
            cross_cell_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            4ULL,
            104ULL) &&
            activate_cell(
                cross_cell_registry,
                cell_one,
                std::span<
                    const ColliderGeometry>{},
                5ULL,
                105ULL),
        "Both cross-cell LOS residency cells activate");

    const auto cross_cell_visible_result =
        query_actor_visual_perception(
            cross_cell_registry,
            observer,
            boundary_observer_result.value(),
            forward,
            boundary_profile_result.value(),
            target,
            boundary_target_result.value());

    check(
        state,
        cross_cell_visible_result.has_value() &&
            cross_cell_visible_result.value() ==
                ActorVisualPerceptionState::
                    visible,
        "Fully resident cross-cell LOS is visible");

    WorldCellColliderRegistry
        cross_cell_missing_registry{};

    check(
        state,
        activate_cell(
            cross_cell_missing_registry,
            cell_zero,
            std::span<
                const ColliderGeometry>{},
            6ULL,
            106ULL),
        "Cross-cell missing registry activates only start cell");

    const auto cross_cell_unavailable_result =
        query_actor_visual_perception(
            cross_cell_missing_registry,
            observer,
            boundary_observer_result.value(),
            forward,
            boundary_profile_result.value(),
            target,
            boundary_target_result.value());

    check(
        state,
        cross_cell_unavailable_result.
                has_value() &&
            cross_cell_unavailable_result.value() ==
                ActorVisualPerceptionState::
                    unavailable,
        "Missing crossed cell remains epistemically unavailable");

    return finish(state);
}