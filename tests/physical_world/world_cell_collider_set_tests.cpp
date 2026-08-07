#include "oros/physical_world/world_cell_collider_set.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell collider set test "
            << "summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    static_assert(
        std::is_copy_constructible_v<
            WorldCellColliderSet>);

    static_assert(
        std::is_copy_assignable_v<
            WorldCellColliderSet>);

    static_assert(
        std::is_move_constructible_v<
            WorldCellColliderSet>);

    static_assert(
        std::is_move_assignable_v<
            WorldCellColliderSet>);

    TestState state{};

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000008ULL
        };

    constexpr std::uint64_t
        other_world_namespace{
            0x4F524F5300000009ULL
        };

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{
            -15,
            25,
            -35
        }
    };

    const WorldCellKey other_cell_key{
        world_namespace,
        WorldCell{
            10,
            20,
            30
        }
    };

    check_failure(
        state,
        WorldCellColliderSet::create(
            invalid_world_cell_key,
            std::span<
                const ColliderGeometry>{}),
        ErrorCode::invalid_argument,
        "Creation rejects an invalid cell key");

    const auto empty_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{});

    check(
        state,
        empty_set_result.has_value(),
        "Valid cell can own an empty collider set");

    if (!empty_set_result.has_value())
    {
        return finish(state);
    }

    const WorldCellColliderSet&
        empty_set =
            empty_set_result.value();

    check(
        state,
        empty_set.is_valid(),
        "Empty collider set is valid");

    check(
        state,
        empty_set.cell_key() ==
            cell_key,
        "Empty collider set preserves its cell key");

    check(
        state,
        empty_set.empty(),
        "Empty collider set reports empty");

    check(
        state,
        empty_set.size() == 0U,
        "Empty collider set reports zero size");

    check(
        state,
        empty_set.colliders().empty(),
        "Empty collider set exposes an empty span");

    const ColliderId sphere_id{
        EntityId{
            world_namespace,
            30ULL
        },
        1U
    };

    const ColliderId box_id{
        EntityId{
            world_namespace,
            10ULL
        },
        2U
    };

    const ColliderId capsule_id{
        EntityId{
            world_namespace,
            20ULL
        },
        3U
    };

    const ColliderId missing_id{
        EntityId{
            world_namespace,
            40ULL
        },
        1U
    };

    const ColliderId other_namespace_id{
        EntityId{
            other_world_namespace,
            1ULL
        },
        1U
    };

    const auto sphere_shape_result =
        SphereShape::create(
            2.5);

    const auto box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                4.0,
                5.0,
                6.0
            });

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.5,
            3.0,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        sphere_shape_result.has_value(),
        "Sphere fixture is created");

    check(
        state,
        box_shape_result.has_value(),
        "Box fixture is created");

    check(
        state,
        capsule_shape_result.has_value(),
        "Capsule fixture is created");

    if (!sphere_shape_result.has_value() ||
        !box_shape_result.has_value() ||
        !capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const auto sphere_geometry_result =
        ColliderGeometry::create(
            sphere_id,
            sphere_shape_result.value(),
            PhysicsVector3{
                511.0,
                -512.0,
                0.0
            });

    const auto box_geometry_result =
        ColliderGeometry::create(
            box_id,
            box_shape_result.value(),
            PhysicsVector3{
                -100.0,
                200.0,
                -300.0
            });

    const auto capsule_geometry_result =
        ColliderGeometry::create(
            capsule_id,
            capsule_shape_result.value(),
            PhysicsVector3{
                0.25,
                -0.5,
                0.75
            });

    check(
        state,
        sphere_geometry_result.has_value(),
        "Sphere geometry fixture is created");

    check(
        state,
        box_geometry_result.has_value(),
        "Box geometry fixture is created");

    check(
        state,
        capsule_geometry_result.has_value(),
        "Capsule geometry fixture is created");

    if (!sphere_geometry_result.has_value() ||
        !box_geometry_result.has_value() ||
        !capsule_geometry_result.has_value())
    {
        return finish(state);
    }

    const ColliderGeometry sphere_geometry =
        sphere_geometry_result.value();

    const ColliderGeometry box_geometry =
        box_geometry_result.value();

    const ColliderGeometry capsule_geometry =
        capsule_geometry_result.value();

    const std::array<
        ColliderGeometry,
        3U>
        unsorted_colliders{
            sphere_geometry,
            capsule_geometry,
            box_geometry
        };

    const auto set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    unsorted_colliders
                });

    check(
        state,
        set_result.has_value(),
        "Valid collider set is created");

    if (!set_result.has_value())
    {
        return finish(state);
    }

    const WorldCellColliderSet&
        collider_set =
            set_result.value();

    check(
        state,
        collider_set.is_valid(),
        "Created collider set satisfies its invariants");

    check(
        state,
        collider_set.cell_key() ==
            cell_key,
        "Collider set preserves its cell key");

    check(
        state,
        !collider_set.empty(),
        "Populated collider set reports nonempty");

    check(
        state,
        collider_set.size() == 3U,
        "Collider set reports its collider count");

    check(
        state,
        collider_set.colliders().size() ==
            3U,
        "Collider span exposes every collider");

    check(
        state,
        collider_set.colliders()[0U].
            collider() ==
            box_id,
        "Collider set sorts the first identity canonically");

    check(
        state,
        collider_set.colliders()[1U].
            collider() ==
            capsule_id,
        "Collider set sorts the second identity canonically");

    check(
        state,
        collider_set.colliders()[2U].
            collider() ==
            sphere_id,
        "Collider set sorts the third identity canonically");

    check(
        state,
        collider_set.colliders()[0U] ==
            box_geometry,
        "Canonical ordering preserves box geometry");

    check(
        state,
        collider_set.colliders()[1U] ==
            capsule_geometry,
        "Canonical ordering preserves capsule geometry");

    check(
        state,
        collider_set.colliders()[2U] ==
            sphere_geometry,
        "Canonical ordering preserves sphere geometry");

    check(
        state,
        collider_set.contains(
            sphere_id),
        "Collider set contains the sphere identity");

    check(
        state,
        collider_set.contains(
            box_id),
        "Collider set contains the box identity");

    check(
        state,
        collider_set.contains(
            capsule_id),
        "Collider set contains the capsule identity");

    check(
        state,
        !collider_set.contains(
            missing_id),
        "Collider set does not contain a missing identity");

    check(
        state,
        !collider_set.contains(
            invalid_collider_id),
        "Collider lookup rejects an invalid identity");

    check(
        state,
        !collider_set.contains(
            other_namespace_id),
        "Collider lookup rejects a different namespace");

    const ColliderGeometry*
        found_capsule =
            collider_set.find(
                capsule_id);

    check(
        state,
        found_capsule != nullptr,
        "Find returns the requested collider");

    check(
        state,
        found_capsule != nullptr &&
            *found_capsule ==
                capsule_geometry,
        "Find preserves the complete collider geometry");

    check(
        state,
        collider_set.find(
            missing_id) == nullptr,
        "Find returns null for a missing collider");

    check(
        state,
        collider_set.find(
            invalid_collider_id) ==
            nullptr,
        "Find returns null for an invalid collider");

    const std::array<
        ColliderGeometry,
        2U>
        duplicate_colliders{
            sphere_geometry,
            sphere_geometry
        };

    check_failure(
        state,
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    duplicate_colliders
                }),
        ErrorCode::invalid_argument,
        "Creation rejects duplicate persistent identities");

    const auto other_namespace_geometry_result =
        ColliderGeometry::create(
            other_namespace_id,
            sphere_shape_result.value(),
            PhysicsVector3{});

    check(
        state,
        other_namespace_geometry_result.
            has_value(),
        "Different-namespace geometry fixture is created");

    if (!other_namespace_geometry_result.
        has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        other_namespace_colliders{
            other_namespace_geometry_result.
                value()
        };

    check_failure(
        state,
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    other_namespace_colliders
                }),
        ErrorCode::invalid_argument,
        "Creation rejects a collider from another namespace");

    const auto positive_boundary_geometry_result =
        ColliderGeometry::create(
            ColliderId{
                EntityId{
                    world_namespace,
                    50ULL
                },
                1U
            },
            sphere_shape_result.value(),
            PhysicsVector3{
                512.0,
                0.0,
                0.0
            });

    check(
        state,
        positive_boundary_geometry_result.
            has_value(),
        "Positive-boundary geometry fixture is created");

    if (!positive_boundary_geometry_result.
        has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        positive_boundary_colliders{
            positive_boundary_geometry_result.
                value()
        };

    check_failure(
        state,
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    positive_boundary_colliders
                }),
        ErrorCode::invalid_argument,
        "Creation rejects positive half-extent center");

    const auto below_negative_boundary_result =
        ColliderGeometry::create(
            ColliderId{
                EntityId{
                    world_namespace,
                    51ULL
                },
                1U
            },
            sphere_shape_result.value(),
            PhysicsVector3{
                -512.0001,
                0.0,
                0.0
            });

    check(
        state,
        below_negative_boundary_result.
            has_value(),
        "Below-boundary geometry fixture is created");

    if (!below_negative_boundary_result.
        has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        below_negative_boundary_colliders{
            below_negative_boundary_result.
                value()
        };

    check_failure(
        state,
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    below_negative_boundary_colliders
                }),
        ErrorCode::invalid_argument,
        "Creation rejects center below negative half extent");

    const auto negative_boundary_geometry_result =
        ColliderGeometry::create(
            ColliderId{
                EntityId{
                    world_namespace,
                    52ULL
                },
                1U
            },
            sphere_shape_result.value(),
            PhysicsVector3{
                -512.0,
                -512.0,
                -512.0
            });

    check(
        state,
        negative_boundary_geometry_result.
            has_value(),
        "Negative-boundary geometry fixture is created");

    if (!negative_boundary_geometry_result.
        has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        negative_boundary_colliders{
            negative_boundary_geometry_result.
                value()
        };

    const auto negative_boundary_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    negative_boundary_colliders
                });

    check(
        state,
        negative_boundary_set_result.
            has_value(),
        "Negative half extent is a valid local center");

    check(
        state,
        negative_boundary_set_result.
            has_value() &&
            negative_boundary_set_result.
                value().
                is_valid(),
        "Negative-boundary collider set is valid");

    const auto other_cell_set_result =
        WorldCellColliderSet::create(
            other_cell_key,
            std::span<
                const ColliderGeometry>{
                    unsorted_colliders
                });

    check(
        state,
        other_cell_set_result.has_value(),
        "Same namespace can create another cell set");

    check(
        state,
        other_cell_set_result.has_value() &&
            other_cell_set_result.value().
                cell_key() ==
                other_cell_key,
        "Another cell set preserves its distinct cell identity");

    const WorldCellColliderSet copied_set{
        collider_set
    };

    check(
        state,
        copied_set.is_valid(),
        "Copy-constructed collider set remains valid");

    check(
        state,
        copied_set ==
            collider_set,
        "Copy construction preserves complete set state");

    auto assignment_target_result =
        WorldCellColliderSet::create(
            other_cell_key,
            std::span<
                const ColliderGeometry>{});

    check(
        state,
        assignment_target_result.
            has_value(),
        "Copy-assignment target is created");

    if (!assignment_target_result.
        has_value())
    {
        return finish(state);
    }

    WorldCellColliderSet assigned_set =
        std::move(
            assignment_target_result.
                value());

    assigned_set =
        collider_set;

    check(
        state,
        assigned_set.is_valid(),
        "Copy-assigned collider set remains valid");

    check(
        state,
        assigned_set ==
            collider_set,
        "Copy assignment preserves complete set state");

    WorldCellColliderSet moved_set{
        std::move(
            assigned_set)
    };

    check(
        state,
        moved_set.is_valid(),
        "Move-constructed collider set remains valid");

    check(
        state,
        moved_set ==
            collider_set,
        "Move construction preserves complete set state");

    return finish(state);
}