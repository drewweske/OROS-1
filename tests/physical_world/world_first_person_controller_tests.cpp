#include "oros/physical_world/world_first_person_controller.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_capsule_ground_query.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/world/entity_id.hpp"

#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
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
            << "\nWorld first-person controller "
            << "test summary: "
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
    bool make_resident(
        oros::streaming::WorldCellResidency&
            residency,
        oros::streaming::WorldCellSnapshot
            snapshot,
        const std::uint64_t request_id)
    {
        const oros::streaming::
            WorldCellRevisionId
            revision{
                snapshot.key(),
                snapshot.revision()
            };

        if (!residency.queue_load(
                revision,
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

        return
            residency.complete_load(
                request_id,
                std::move(snapshot)).
                has_value();
    }
}

int main()
{
    using namespace std::chrono_literals;

    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000008ULL
        };

    const ColliderId y_capsule_id{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const ColliderId x_capsule_id{
        EntityId{
            world_namespace,
            901ULL
        },
        1U
    };

    const ColliderId y_floor_id{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId x_floor_id{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const auto y_capsule_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::positive_y());

    const auto x_capsule_result =
        CapsuleShape::create(
            1.0,
            1.0,
            PhysicsUnitVector3::positive_x());

    const auto y_floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                10.0,
                0.5,
                10.0
            });

    const auto x_floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                0.5,
                10.0,
                10.0
            });

    check(
        state,
        y_capsule_result.has_value(),
        "Positive-Y controller capsule is created");

    check(
        state,
        x_capsule_result.has_value(),
        "Positive-X controller capsule is created");

    check(
        state,
        y_floor_shape_result.has_value(),
        "Positive-Y controller floor is created");

    check(
        state,
        x_floor_shape_result.has_value(),
        "Positive-X controller floor is created");

    if (!y_capsule_result.has_value() ||
        !x_capsule_result.has_value() ||
        !y_floor_shape_result.has_value() ||
        !x_floor_shape_result.has_value())
    {
        return finish(state);
    }

    const auto y_traversal_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_y(),
            0.70,
            0.5,
            0.25);

    const auto x_traversal_result =
        WorldCapsuleTraversalSettings::create(
            PhysicsUnitVector3::positive_x(),
            1.0,
            0.5,
            0.25);

    check(
        state,
        y_traversal_result.has_value(),
        "Positive-Y traversal settings are created");

    check(
        state,
        x_traversal_result.has_value(),
        "Positive-X traversal settings are created");

    if (!y_traversal_result.has_value() ||
        !x_traversal_result.has_value())
    {
        return finish(state);
    }

    const auto controller_settings_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.1,
            64U,
            4U);

    check(
        state,
        controller_settings_result.has_value(),
        "Controller settings are created");

    if (!controller_settings_result.has_value())
    {
        return finish(state);
    }

    const WorldFirstPersonControllerSettings&
        controller_settings =
            controller_settings_result.value();

    check(
        state,
        nearly_equal(
            controller_settings.
                maximum_ground_speed(),
            6.0) &&
            nearly_equal(
                controller_settings.
                    maximum_substep_distance(),
                0.1) &&
            controller_settings.
                maximum_substeps() == 64U &&
            controller_settings.
                maximum_depenetration_iterations() ==
                    4U,
        "Controller settings preserve their configuration");

    const auto zero_speed_settings_result =
        WorldFirstPersonControllerSettings::create(
            0.0,
            0.1,
            64U,
            4U);

    check(
        state,
        zero_speed_settings_result.has_value(),
        "Zero controller speed is a valid disabled-motion configuration");

    const auto non_finite_speed_result =
        WorldFirstPersonControllerSettings::create(
            std::numeric_limits<double>::
                quiet_NaN(),
            0.1,
            64U,
            4U);

    check(
        state,
        !non_finite_speed_result.has_value() &&
            non_finite_speed_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects non-finite ground speed");

    const auto negative_speed_result =
        WorldFirstPersonControllerSettings::create(
            -1.0,
            0.1,
            64U,
            4U);

    check(
        state,
        !negative_speed_result.has_value() &&
            negative_speed_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects negative ground speed");

    const auto zero_substep_distance_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.0,
            64U,
            4U);

    check(
        state,
        !zero_substep_distance_result.has_value() &&
            zero_substep_distance_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects zero substep distance");

    const auto zero_substeps_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.1,
            0U,
            4U);

    check(
        state,
        !zero_substeps_result.has_value() &&
            zero_substeps_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects zero motion-substep budget");

    const auto zero_depenetration_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.1,
            64U,
            0U);

    check(
        state,
        !zero_depenetration_result.has_value() &&
            zero_depenetration_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects zero depenetration budget");

    const auto y_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                0.0,
                0.0
            });

    const auto x_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                41.0,
                0.0,
                0.0
            });

    check(
        state,
        y_start_result.has_value() &&
            x_start_result.has_value(),
        "Controller start positions are created");

    if (!y_start_result.has_value() ||
        !x_start_result.has_value())
    {
        return finish(state);
    }

    const auto y_floor_geometry_result =
        ColliderGeometry::create(
            y_floor_id,
            y_floor_shape_result.value(),
            PhysicsVector3{
                0.0,
                -2.5,
                0.0
            });

    const auto x_floor_geometry_result =
        ColliderGeometry::create(
            x_floor_id,
            x_floor_shape_result.value(),
            PhysicsVector3{
                38.5,
                0.0,
                0.0
            });

    check(
        state,
        y_floor_geometry_result.has_value() &&
            x_floor_geometry_result.has_value(),
        "Controller support geometry is created");

    if (!y_floor_geometry_result.has_value() ||
        !x_floor_geometry_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{}
    };

    const std::array<
        ColliderGeometry,
        2U>
        colliders{
            y_floor_geometry_result.value(),
            x_floor_geometry_result.value()
        };

    const auto collider_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    colliders
                });

    check(
        state,
        collider_set_result.has_value(),
        "Controller collider set is created");

    if (!collider_set_result.has_value())
    {
        return finish(state);
    }

    const auto payload_result =
        serialize_world_cell_collider_payload(
            collider_set_result.value());

    check(
        state,
        payload_result.has_value(),
        "Controller collider payload serializes");

    if (!payload_result.has_value())
    {
        return finish(state);
    }

    const auto snapshot_result =
        WorldCellSnapshot::create(
            cell_key,
            1ULL,
            std::span<const std::byte>{
                payload_result.value()
            });

    check(
        state,
        snapshot_result.has_value(),
        "Controller world snapshot is created");

    if (!snapshot_result.has_value())
    {
        return finish(state);
    }

    auto residency_result =
        WorldCellResidency::create(
            cell_key);

    check(
        state,
        residency_result.has_value(),
        "Controller residency is created");

    if (!residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency residency =
        std::move(
            residency_result.value());

    check(
        state,
        make_resident(
            residency,
            snapshot_result.value(),
            101ULL),
        "Controller test cell becomes resident");

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.synchronize(
            residency).
            has_value(),
        "Controller colliders activate");

    check(
        state,
        registry.is_valid() &&
            registry.collider_count() == 2U,
        "Controller collider registry is valid");

    const auto non_finite_command_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    std::numeric_limits<double>::
                        quiet_NaN(),
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        !non_finite_command_result.has_value() &&
            non_finite_command_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects non-finite movement intent");

    const auto zero_step_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            Nanoseconds::zero());

    check(
        state,
        !zero_step_result.has_value() &&
            zero_step_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects zero simulation step");

    const auto negative_step_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            Nanoseconds{-1});

    check(
        state,
        !negative_step_result.has_value() &&
            negative_step_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Controller rejects negative simulation step");

    const auto full_speed_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        full_speed_result.has_value(),
        "Full-speed fixed controller step resolves");

    if (!full_speed_result.has_value())
    {
        return finish(state);
    }

    const auto full_speed_displacement_result =
        y_start_result.
            value().
            displacement_to(
                full_speed_result.value());

    check(
        state,
        full_speed_displacement_result.has_value() &&
            nearly_equal(
                full_speed_displacement_result.
                    value().
                    x,
                0.6) &&
            nearly_equal(
                full_speed_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                full_speed_displacement_result.
                    value().
                    z,
                0.0),
        "Six meters per second integrates to 0.6 meters over 100 ms");

    const auto half_speed_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    0.5,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        half_speed_result.has_value(),
        "Analog half-speed controller step resolves");

    const auto half_speed_displacement_result =
        half_speed_result.has_value()
            ? y_start_result.
                  value().
                  displacement_to(
                      half_speed_result.value())
            : Result<WorldDisplacement>{
                  std::unexpected{
                      half_speed_result.error()
                  }
              };

    check(
        state,
        half_speed_displacement_result.
                has_value() &&
            nearly_equal(
                half_speed_displacement_result.
                    value().
                    x,
                0.3),
        "Analog movement magnitude scales controller speed");

    const auto oversized_intent_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    2.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        oversized_intent_result.has_value(),
        "Oversized movement intent resolves");

    const auto oversized_displacement_result =
        oversized_intent_result.has_value()
            ? y_start_result.
                  value().
                  displacement_to(
                      oversized_intent_result.value())
            : Result<WorldDisplacement>{
                  std::unexpected{
                      oversized_intent_result.error()
                  }
              };

    check(
        state,
        oversized_displacement_result.
                has_value() &&
            nearly_equal(
                oversized_displacement_result.
                    value().
                    x,
                0.6),
        "Oversized movement intent clamps to unit magnitude");

    const auto diagonal_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    1.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        diagonal_result.has_value(),
        "Diagonal controller movement resolves");

    const auto diagonal_displacement_result =
        diagonal_result.has_value()
            ? y_start_result.
                  value().
                  displacement_to(
                      diagonal_result.value())
            : Result<WorldDisplacement>{
                  std::unexpected{
                      diagonal_result.error()
                  }
              };

    const double diagonal_component =
        0.6 / std::sqrt(2.0);

    check(
        state,
        diagonal_displacement_result.
                has_value() &&
            nearly_equal(
                diagonal_displacement_result.
                    value().
                    x,
                diagonal_component) &&
            nearly_equal(
                diagonal_displacement_result.
                    value().
                    z,
                diagonal_component),
        "Diagonal input cannot exceed maximum controller speed");

    const auto contaminated_up_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    50.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        contaminated_up_result.has_value(),
        "Controller removes movement along the traversal up axis");

    const auto contaminated_displacement_result =
        contaminated_up_result.has_value()
            ? y_start_result.
                  value().
                  displacement_to(
                      contaminated_up_result.value())
            : Result<WorldDisplacement>{
                  std::unexpected{
                      contaminated_up_result.error()
                  }
              };

    check(
        state,
        contaminated_displacement_result.
                has_value() &&
            nearly_equal(
                contaminated_displacement_result.
                    value().
                    x,
                0.6) &&
            nearly_equal(
                contaminated_displacement_result.
                    value().
                    y,
                0.0),
        "Up-axis contamination cannot alter planar controller speed");

    const auto zero_command_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{},
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        zero_command_result.has_value(),
        "Zero movement command still resolves grounded state");

    check(
        state,
        zero_command_result.has_value() &&
            zero_command_result.value() ==
                y_start_result.value(),
        "Zero movement command preserves an already grounded position");

    const auto half_tick_first_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            50ms);

    check(
        state,
        half_tick_first_result.has_value(),
        "First partitioned fixed step resolves");

    const auto half_tick_second_result =
        half_tick_first_result.has_value()
            ? step_world_first_person_controller(
                  registry,
                  world_namespace,
                  y_capsule_id,
                  y_capsule_result.value(),
                  half_tick_first_result.value(),
                  WorldFirstPersonControllerCommand{
                      PhysicsVector3{
                          1.0,
                          0.0,
                          0.0
                      }
                  },
                  y_traversal_result.value(),
                  controller_settings,
                  50ms)
            : Result<WorldPosition>{
                  std::unexpected{
                      half_tick_first_result.error()
                  }
              };

    check(
        state,
        half_tick_second_result.has_value(),
        "Second partitioned fixed step resolves");

    const auto partitioned_displacement_result =
        half_tick_second_result.has_value()
            ? y_start_result.
                  value().
                  displacement_to(
                      half_tick_second_result.value())
            : Result<WorldDisplacement>{
                  std::unexpected{
                      half_tick_second_result.error()
                  }
              };

    check(
        state,
        partitioned_displacement_result.
                has_value() &&
            nearly_equal(
                partitioned_displacement_result.
                    value().
                    x,
                0.6),
        "Two 50 ms controller ticks match one 100 ms flat-ground tick");

    if (!zero_speed_settings_result.has_value())
    {
        return finish(state);
    }

    const auto disabled_motion_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            zero_speed_settings_result.value(),
            100ms);

    check(
        state,
        disabled_motion_result.has_value() &&
            disabled_motion_result.value() ==
                y_start_result.value(),
        "Zero maximum speed disables movement without bypassing grounding");

    const auto constrained_budget_settings_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.1,
            1U,
            4U);

    check(
        state,
        constrained_budget_settings_result.
            has_value(),
        "Constrained controller budget settings are created");

    if (!constrained_budget_settings_result.has_value())
    {
        return finish(state);
    }

    const auto constrained_budget_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            constrained_budget_settings_result.
                value(),
            100ms);

    check(
        state,
        !constrained_budget_result.has_value() &&
            constrained_budget_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Controller propagates deterministic substep-budget exhaustion");

    const auto x_up_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            x_capsule_id,
            x_capsule_result.value(),
            x_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    10.0,
                    1.0,
                    0.0
                }
            },
            x_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        x_up_result.has_value(),
        "Positive-X-up controller movement resolves");

    if (!x_up_result.has_value())
    {
        return finish(state);
    }

    const auto x_up_displacement_result =
        x_start_result.
            value().
            displacement_to(
                x_up_result.value());

    check(
        state,
        x_up_displacement_result.has_value() &&
            nearly_equal(
                x_up_displacement_result.
                    value().
                    x,
                0.0) &&
            nearly_equal(
                x_up_displacement_result.
                    value().
                    y,
                0.6) &&
            nearly_equal(
                x_up_displacement_result.
                    value().
                    z,
                0.0),
        "Controller planarity follows arbitrary traversal up direction");

    const auto y_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            full_speed_result.value(),
            y_traversal_result.value());

    check(
        state,
        y_ground_result.has_value() &&
            y_ground_result.
                value().
                has_value() &&
            y_ground_result.
                value()->
                pair().
                contains(y_floor_id),
        "Controller movement remains on Positive-Y support");

    const auto x_ground_result =
        query_world_capsule_ground_contact(
            registry,
            world_namespace,
            x_capsule_id,
            x_capsule_result.value(),
            x_up_result.value(),
            x_traversal_result.value());

    check(
        state,
        x_ground_result.has_value() &&
            x_ground_result.
                value().
                has_value() &&
            x_ground_result.
                value()->
                pair().
                contains(x_floor_id),
        "Controller movement remains on arbitrary-up support");

    const auto unsupported_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                100.0,
                0.0,
                0.0
            });

    check(
        state,
        unsupported_start_result.has_value(),
        "Unsupported controller start position is created");

    if (!unsupported_start_result.has_value())
    {
        return finish(state);
    }

    const auto unsupported_controller_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            unsupported_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        !unsupported_controller_result.has_value() &&
            unsupported_controller_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Controller propagates unsupported-ground invalid state");

    const Nanoseconds default_runtime_step{
        16666667
    };

    const auto default_runtime_tick_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            default_runtime_step);

    check(
        state,
        default_runtime_tick_result.has_value(),
        "Default-runtime controller tick resolves");

    if (!default_runtime_tick_result.has_value())
    {
        return finish(state);
    }

    const auto default_runtime_displacement_result =
        y_start_result.
            value().
            displacement_to(
                default_runtime_tick_result.value());

    check(
        state,
        default_runtime_displacement_result.
                has_value() &&
            nearly_equal(
                default_runtime_displacement_result.
                    value().
                    x,
                0.100000002) &&
            nearly_equal(
                default_runtime_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                default_runtime_displacement_result.
                    value().
                    z,
                0.0),
        "Default 16666667 ns tick integrates exact controller distance");

    const auto one_substep_settings_result =
        WorldFirstPersonControllerSettings::create(
            6.0,
            0.1,
            1U,
            4U);

    check(
        state,
        one_substep_settings_result.has_value(),
        "One-substep controller settings are created");

    if (!one_substep_settings_result.has_value())
    {
        return finish(state);
    }

    const auto default_tick_budget_result =
        step_world_first_person_controller(
            registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            y_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            one_substep_settings_result.value(),
            default_runtime_step);

    check(
        state,
        !default_tick_budget_result.has_value() &&
            default_tick_budget_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Nanosecond-rounded default tick preserves deterministic substep boundary");

    const ColliderId boundary_floor_id{
        EntityId{
            world_namespace,
            300ULL
        },
        1U
    };

    const auto boundary_floor_shape_result =
        BoxShape::create(
            PhysicsVector3{
                20.0,
                0.5,
                10.0
            });

    check(
        state,
        boundary_floor_shape_result.has_value(),
        "Cross-cell controller floor shape is created");

    if (!boundary_floor_shape_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_floor_geometry_result =
        ColliderGeometry::create(
            boundary_floor_id,
            boundary_floor_shape_result.value(),
            PhysicsVector3{
                500.0,
                -2.5,
                0.0
            });

    check(
        state,
        boundary_floor_geometry_result.has_value(),
        "Cross-cell controller floor geometry is created");

    if (!boundary_floor_geometry_result.has_value())
    {
        return finish(state);
    }

    const WorldCellKey boundary_cell_key{
        world_namespace,
        WorldCell{}
    };

    const std::array<
        ColliderGeometry,
        1U>
        boundary_colliders{
            boundary_floor_geometry_result.value()
        };

    const auto boundary_set_result =
        WorldCellColliderSet::create(
            boundary_cell_key,
            std::span<
                const ColliderGeometry>{
                    boundary_colliders
                });

    check(
        state,
        boundary_set_result.has_value(),
        "Cross-cell controller collider set is created");

    if (!boundary_set_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_payload_result =
        serialize_world_cell_collider_payload(
            boundary_set_result.value());

    check(
        state,
        boundary_payload_result.has_value(),
        "Cross-cell controller payload serializes");

    if (!boundary_payload_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_snapshot_result =
        WorldCellSnapshot::create(
            boundary_cell_key,
            2ULL,
            std::span<const std::byte>{
                boundary_payload_result.value()
            });

    check(
        state,
        boundary_snapshot_result.has_value(),
        "Cross-cell controller snapshot is created");

    if (!boundary_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto boundary_residency_result =
        WorldCellResidency::create(
            boundary_cell_key);

    check(
        state,
        boundary_residency_result.has_value(),
        "Cross-cell controller residency is created");

    if (!boundary_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency boundary_residency =
        std::move(
            boundary_residency_result.value());

    check(
        state,
        make_resident(
            boundary_residency,
            boundary_snapshot_result.value(),
            202ULL),
        "Cross-cell controller cell becomes resident");

    WorldCellColliderRegistry boundary_registry{};

    check(
        state,
        boundary_registry.synchronize(
            boundary_residency).
            has_value(),
        "Cross-cell controller colliders activate");

    check(
        state,
        boundary_registry.is_valid() &&
            boundary_registry.
                active_cell_count() == 1U &&
            boundary_registry.
                collider_count() == 1U,
        "Cross-cell controller registry is valid");

    const auto boundary_start_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                511.75,
                0.0,
                0.0
            });

    check(
        state,
        boundary_start_result.has_value(),
        "Cross-cell controller start is created");

    if (!boundary_start_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_controller_result =
        step_world_first_person_controller(
            boundary_registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            boundary_start_result.value(),
            WorldFirstPersonControllerCommand{
                PhysicsVector3{
                    1.0,
                    0.0,
                    0.0
                }
            },
            y_traversal_result.value(),
            controller_settings,
            100ms);

    check(
        state,
        boundary_controller_result.has_value(),
        "Cross-cell controller movement resolves");

    if (!boundary_controller_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        boundary_controller_result.
                value().
                cell() ==
            WorldCell{
                1,
                0,
                0
            } &&
            nearly_equal(
                boundary_controller_result.
                    value().
                    local().
                    x,
                -511.65) &&
            nearly_equal(
                boundary_controller_result.
                    value().
                    local().
                    y,
                0.0) &&
            nearly_equal(
                boundary_controller_result.
                    value().
                    local().
                    z,
                0.0),
        "Cross-cell controller result normalizes world position");

    const auto boundary_displacement_result =
        boundary_start_result.
            value().
            displacement_to(
                boundary_controller_result.value());

    check(
        state,
        boundary_displacement_result.has_value() &&
            nearly_equal(
                boundary_displacement_result.
                    value().
                    x,
                0.6) &&
            nearly_equal(
                boundary_displacement_result.
                    value().
                    y,
                0.0) &&
            nearly_equal(
                boundary_displacement_result.
                    value().
                    z,
                0.0),
        "Cross-cell normalization preserves controller displacement");

    const auto boundary_ground_result =
        query_world_capsule_ground_contact(
            boundary_registry,
            world_namespace,
            y_capsule_id,
            y_capsule_result.value(),
            boundary_controller_result.value(),
            y_traversal_result.value());

    check(
        state,
        boundary_ground_result.has_value() &&
            boundary_ground_result.
                value().
                has_value() &&
            boundary_ground_result.
                value()->
                pair().
                contains(boundary_floor_id),
        "Cross-cell controller movement remains grounded");

    return finish(state);
}
