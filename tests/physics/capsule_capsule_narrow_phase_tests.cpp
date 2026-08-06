#include "oros/physics/narrow_phase.hpp"

#include "oros/foundation/error.hpp"

#include <iostream>
#include <limits>
#include <optional>
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
            << "\nCapsule-capsule narrow-phase test summary: "
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
    using namespace oros::physics;
    using oros::foundation::ErrorCode;
    using oros::world::EntityId;

    static_assert(
        std::is_same_v<
            decltype(
                generate_capsule_capsule_contact(
                    std::declval<
                        const ColliderGeometry&>(),
                    std::declval<
                        const ColliderGeometry&>())),
            oros::foundation::Result<
                std::optional<
                    CollisionContact>>>);

    TestState state{};

    constexpr ColliderId collider_a{
        EntityId{
            1U,
            1U
        },
        1U
    };

    constexpr ColliderId collider_b{
        EntityId{
            1U,
            1U
        },
        2U
    };

    constexpr ColliderId collider_c{
        EntityId{
            1U,
            2U
        },
        1U
    };

    static_assert(
        collider_a.is_valid());

    static_assert(
        collider_b.is_valid());

    static_assert(
        collider_c.is_valid());

    static_assert(
        collider_a <
            collider_b);

    static_assert(
        collider_b <
            collider_c);

    const auto vertical_capsule_result =
        CapsuleShape::create(
            1.0,
            2.0,
            PhysicsUnitVector3::
                positive_y());

    const auto horizontal_capsule_result =
        CapsuleShape::create(
            1.0,
            2.0,
            PhysicsUnitVector3::
                positive_x());

    const auto zero_segment_capsule_result =
        CapsuleShape::create(
            1.0,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    const auto sphere_result =
        SphereShape::create(
            1.0);

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    check(
        state,
        vertical_capsule_result.has_value(),
        "Vertical capsule fixture is valid");

    check(
        state,
        horizontal_capsule_result.has_value(),
        "Horizontal capsule fixture is valid");

    check(
        state,
        zero_segment_capsule_result.
            has_value(),
        "Zero-segment capsule fixture is valid");

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

    check(
        state,
        box_result.has_value(),
        "Box fixture is valid");

    if (!vertical_capsule_result.has_value() ||
        !horizontal_capsule_result.has_value() ||
        !zero_segment_capsule_result.
            has_value() ||
        !sphere_result.has_value() ||
        !box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape vertical_capsule_shape{
        vertical_capsule_result.value()
    };

    const CollisionShape horizontal_capsule_shape{
        horizontal_capsule_result.value()
    };

    const CollisionShape zero_segment_capsule_shape{
        zero_segment_capsule_result.value()
    };

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const CollisionShape box_shape{
        box_result.value()
    };

    const auto vertical_capsule_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            vertical_capsule_shape,
            physics_zero_vector);

    const auto vertical_capsule_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            physics_zero_vector);

    const auto horizontal_capsule_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            horizontal_capsule_shape,
            physics_zero_vector);

    const auto zero_capsule_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            zero_segment_capsule_shape,
            physics_zero_vector);

    const auto zero_capsule_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            zero_segment_capsule_shape,
            physics_zero_vector);

    const auto sphere_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            physics_zero_vector);

    const auto box_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            box_shape,
            physics_zero_vector);

    check(
        state,
        vertical_capsule_a_origin_result.
            has_value(),
        "Vertical capsule A origin fixture is valid");

    check(
        state,
        vertical_capsule_b_origin_result.
            has_value(),
        "Vertical capsule B origin fixture is valid");

    check(
        state,
        horizontal_capsule_b_origin_result.
            has_value(),
        "Horizontal capsule B origin fixture is valid");

    check(
        state,
        zero_capsule_a_origin_result.
            has_value(),
        "Zero-segment capsule A origin fixture is valid");

    check(
        state,
        zero_capsule_b_origin_result.
            has_value(),
        "Zero-segment capsule B origin fixture is valid");

    check(
        state,
        sphere_b_origin_result.has_value(),
        "Sphere B origin fixture is valid");

    check(
        state,
        box_b_origin_result.has_value(),
        "Box B origin fixture is valid");

    if (!vertical_capsule_a_origin_result.
            has_value() ||
        !vertical_capsule_b_origin_result.
            has_value() ||
        !horizontal_capsule_b_origin_result.
            has_value() ||
        !zero_capsule_a_origin_result.
            has_value() ||
        !zero_capsule_b_origin_result.
            has_value() ||
        !sphere_b_origin_result.has_value() ||
        !box_b_origin_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            zero_capsule_a_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-capsule query rejects identical collider identity");

    check_failure(
        state,
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-capsule query rejects a sphere");

    check_failure(
        state,
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            box_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-capsule query rejects a box");

    const auto pair_ab_result =
        BroadPhasePair::create(
            collider_a,
            collider_b);

    check(
        state,
        pair_ab_result.has_value(),
        "A-B canonical pair fixture is valid");

    if (!pair_ab_result.has_value())
    {
        return finish(state);
    }

    const auto negative_x_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            });

    const auto negative_z_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                0.0,
                0.0,
                -1.0
            });

    check(
        state,
        negative_x_result.has_value(),
        "Negative-X normal fixture is valid");

    check(
        state,
        negative_z_result.has_value(),
        "Negative-Z normal fixture is valid");

    if (!negative_x_result.has_value() ||
        !negative_z_result.has_value())
    {
        return finish(state);
    }

    const auto separated_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                2.25,
                0.0,
                0.0
            });

    check(
        state,
        separated_capsule_b_result.has_value(),
        "Separated capsule fixture is valid");

    if (!separated_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto separated_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            separated_capsule_b_result.value());

    check(
        state,
        separated_result.has_value(),
        "Separated capsule-capsule query succeeds");

    check(
        state,
        separated_result.has_value() &&
            !separated_result.
                value().
                has_value(),
        "Separated capsules produce no contact");

    const auto reversed_separated_result =
        generate_capsule_capsule_contact(
            separated_capsule_b_result.value(),
            vertical_capsule_a_origin_result.value());

    check(
        state,
        reversed_separated_result.has_value(),
        "Reversed separated query succeeds");

    check(
        state,
        reversed_separated_result.has_value() &&
            !reversed_separated_result.
                value().
                has_value(),
        "Reversed separated query produces no contact");

    const auto touching_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_capsule_b_result.has_value(),
        "Side-touching capsule fixture is valid");

    if (!touching_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            touching_capsule_b_result.value());

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "Side-touching capsules produce a contact");

    if (!touching_result.has_value() ||
        !touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact touching_contact =
        touching_result.value().value();

    check(
        state,
        touching_contact.pair() ==
            pair_ab_result.value(),
        "Side-touching contact preserves canonical pair");

    check(
        state,
        touching_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Side-touching normal points from A toward B");

    check(
        state,
        touching_contact.point() ==
            PhysicsVector3{
                1.0,
                -2.0,
                0.0
            },
        "Parallel touching contact selects deterministic lower endpoints");

    check(
        state,
        touching_contact.
            penetration_depth() ==
            0.0,
        "Side-touching contact has zero depth");

    check(
        state,
        touching_contact.is_touching(),
        "Side-touching contact reports touching");

    const auto reversed_touching_result =
        generate_capsule_capsule_contact(
            touching_capsule_b_result.value(),
            vertical_capsule_a_origin_result.value());

    check(
        state,
        reversed_touching_result.has_value() &&
            reversed_touching_result.
                value().
                has_value(),
        "Reversed side-touching query produces a contact");

    check(
        state,
        reversed_touching_result.has_value() &&
            reversed_touching_result.
                value().
                has_value() &&
            reversed_touching_result.
                value().
                value() ==
                touching_contact,
        "Side-touching result ignores argument order");

    const auto penetrating_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_capsule_b_result.has_value(),
        "Side-penetrating capsule fixture is valid");

    if (!penetrating_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            penetrating_capsule_b_result.value());

    check(
        state,
        penetrating_result.has_value() &&
            penetrating_result.
                value().
                has_value(),
        "Side penetration produces a contact");

    if (!penetrating_result.has_value() ||
        !penetrating_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact penetrating_contact =
        penetrating_result.value().value();

    check(
        state,
        penetrating_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Side penetration preserves positive-X normal");

    check(
        state,
        penetrating_contact.point() ==
            PhysicsVector3{
                0.75,
                -2.0,
                0.0
            },
        "Side penetration averages opposing surfaces");

    check(
        state,
        penetrating_contact.
            penetration_depth() ==
            0.5,
        "Side penetration calculates depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Side penetration reports overlap");

    const auto reversed_penetrating_result =
        generate_capsule_capsule_contact(
            penetrating_capsule_b_result.value(),
            vertical_capsule_a_origin_result.value());

    check(
        state,
        reversed_penetrating_result.has_value() &&
            reversed_penetrating_result.
                value().
                has_value() &&
            reversed_penetrating_result.
                value().
                value() ==
                penetrating_contact,
        "Side penetration ignores argument order");

    const auto negative_side_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                -1.5,
                0.0,
                0.0
            });

    check(
        state,
        negative_side_capsule_b_result.has_value(),
        "Negative-side capsule fixture is valid");

    if (!negative_side_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto negative_side_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            negative_side_capsule_b_result.value());

    check(
        state,
        negative_side_result.has_value() &&
            negative_side_result.
                value().
                has_value(),
        "Negative-side penetration produces a contact");

    if (!negative_side_result.has_value() ||
        !negative_side_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact negative_side_contact =
        negative_side_result.value().value();

    check(
        state,
        negative_side_contact.normal() ==
            negative_x_result.value(),
        "Negative-side normal points from A toward B");

    check(
        state,
        negative_side_contact.point() ==
            PhysicsVector3{
                -0.75,
                -2.0,
                0.0
            },
        "Negative-side contact averages opposing surfaces");

    check(
        state,
        negative_side_contact.
            penetration_depth() ==
            0.5,
        "Negative-side penetration preserves depth");

    const auto end_touching_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                0.0,
                6.0,
                0.0
            });

    check(
        state,
        end_touching_capsule_b_result.has_value(),
        "End-cap touching capsule fixture is valid");

    if (!end_touching_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto end_touching_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            end_touching_capsule_b_result.value());

    check(
        state,
        end_touching_result.has_value() &&
            end_touching_result.
                value().
                has_value(),
        "End-cap touching capsules produce a contact");

    if (!end_touching_result.has_value() ||
        !end_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact end_touching_contact =
        end_touching_result.value().value();

    check(
        state,
        end_touching_contact.normal() ==
            PhysicsUnitVector3::
                positive_y(),
        "End-cap touching normal points upward");

    check(
        state,
        end_touching_contact.point() ==
            PhysicsVector3{
                0.0,
                3.0,
                0.0
            },
        "End-cap touching preserves shared surface point");

    check(
        state,
        end_touching_contact.
            penetration_depth() ==
            0.0,
        "End-cap touching has zero depth");

    check(
        state,
        end_touching_contact.is_touching(),
        "End-cap boundary reports touching");

    const auto end_penetrating_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            vertical_capsule_shape,
            PhysicsVector3{
                0.0,
                5.5,
                0.0
            });

    check(
        state,
        end_penetrating_capsule_b_result.
            has_value(),
        "End-cap penetrating capsule fixture is valid");

    if (!end_penetrating_capsule_b_result.
            has_value())
    {
        return finish(state);
    }

    const auto end_penetrating_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            end_penetrating_capsule_b_result.value());

    check(
        state,
        end_penetrating_result.has_value() &&
            end_penetrating_result.
                value().
                has_value(),
        "End-cap penetration produces a contact");

    if (!end_penetrating_result.has_value() ||
        !end_penetrating_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact end_penetrating_contact =
        end_penetrating_result.value().value();

    check(
        state,
        end_penetrating_contact.normal() ==
            PhysicsUnitVector3::
                positive_y(),
        "End-cap penetration preserves upward normal");

    check(
        state,
        end_penetrating_contact.point() ==
            PhysicsVector3{
                0.0,
                2.75,
                0.0
            },
        "End-cap penetration averages opposing surfaces");

    check(
        state,
        end_penetrating_contact.
            penetration_depth() ==
            0.5,
        "End-cap penetration calculates depth");

    const auto endpoint_side_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            horizontal_capsule_shape,
            PhysicsVector3{
                0.0,
                4.0,
                0.0
            });

    check(
        state,
        endpoint_side_capsule_b_result.
            has_value(),
        "Endpoint-to-side capsule fixture is valid");

    if (!endpoint_side_capsule_b_result.
            has_value())
    {
        return finish(state);
    }

    const auto endpoint_side_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            endpoint_side_capsule_b_result.value());

    check(
        state,
        endpoint_side_result.has_value() &&
            endpoint_side_result.
                value().
                has_value(),
        "Capsule endpoint touching another segment produces a contact");

    if (!endpoint_side_result.has_value() ||
        !endpoint_side_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact endpoint_side_contact =
        endpoint_side_result.value().value();

    check(
        state,
        endpoint_side_contact.normal() ==
            PhysicsUnitVector3::
                positive_y(),
        "Endpoint-to-side normal follows closest points");

    check(
        state,
        endpoint_side_contact.point() ==
            PhysicsVector3{
                0.0,
                3.0,
                0.0
            },
        "Endpoint-to-side contact preserves shared surface point");

    check(
        state,
        endpoint_side_contact.
            penetration_depth() ==
            0.0,
        "Endpoint-to-side boundary has zero depth");

    const auto crossed_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            horizontal_capsule_b_origin_result.value());

    check(
        state,
        crossed_result.has_value() &&
            crossed_result.
                value().
                has_value(),
        "Crossed capsule center lines produce a contact");

    if (!crossed_result.has_value() ||
        !crossed_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact crossed_contact =
        crossed_result.value().value();

    check(
        state,
        crossed_contact.normal() ==
            negative_z_result.value(),
        "Crossed capsules use deterministic axis-cross normal");

    check(
        state,
        crossed_contact.point() ==
            physics_zero_vector,
        "Crossed capsule contact preserves intersection point");

    check(
        state,
        crossed_contact.
            penetration_depth() ==
            2.0,
        "Crossed capsule overlap uses combined radii");

    const auto reversed_crossed_result =
        generate_capsule_capsule_contact(
            horizontal_capsule_b_origin_result.value(),
            vertical_capsule_a_origin_result.value());

    check(
        state,
        reversed_crossed_result.has_value() &&
            reversed_crossed_result.
                value().
                has_value() &&
            reversed_crossed_result.
                value().
                value() ==
                crossed_contact,
        "Crossed capsule result ignores argument order");

    const auto coincident_parallel_result =
        generate_capsule_capsule_contact(
            vertical_capsule_a_origin_result.value(),
            vertical_capsule_b_origin_result.value());

    check(
        state,
        coincident_parallel_result.has_value() &&
            coincident_parallel_result.
                value().
                has_value(),
        "Coincident parallel capsules produce a contact");

    if (!coincident_parallel_result.has_value() ||
        !coincident_parallel_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact coincident_parallel_contact =
        coincident_parallel_result.
            value().
            value();

    check(
        state,
        coincident_parallel_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Coincident parallel capsules use deterministic perpendicular normal");

    check(
        state,
        coincident_parallel_contact.point() ==
            PhysicsVector3{
                0.0,
                -2.0,
                0.0
            },
        "Coincident parallel contact selects deterministic lower endpoints");

    check(
        state,
        coincident_parallel_contact.
            penetration_depth() ==
            2.0,
        "Coincident parallel overlap uses combined radii");

    const auto coincident_zero_result =
        generate_capsule_capsule_contact(
            zero_capsule_a_origin_result.value(),
            zero_capsule_b_origin_result.value());

    check(
        state,
        coincident_zero_result.has_value() &&
            coincident_zero_result.
                value().
                has_value(),
        "Coincident zero-segment capsules produce a contact");

    if (!coincident_zero_result.has_value() ||
        !coincident_zero_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact coincident_zero_contact =
        coincident_zero_result.value().value();

    check(
        state,
        coincident_zero_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Coincident zero-segment capsules select positive X");

    check(
        state,
        coincident_zero_contact.point() ==
            physics_zero_vector,
        "Coincident zero-segment contact preserves center");

    check(
        state,
        coincident_zero_contact.
            penetration_depth() ==
            2.0,
        "Coincident zero-segment capsules behave like spheres");

    const auto touching_zero_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            zero_segment_capsule_shape,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_zero_capsule_b_result.
            has_value(),
        "Touching zero-segment capsule fixture is valid");

    if (!touching_zero_capsule_b_result.
            has_value())
    {
        return finish(state);
    }

    const auto touching_zero_result =
        generate_capsule_capsule_contact(
            zero_capsule_a_origin_result.value(),
            touching_zero_capsule_b_result.value());

    check(
        state,
        touching_zero_result.has_value() &&
            touching_zero_result.
                value().
                has_value(),
        "Touching zero-segment capsules produce a contact");

    if (!touching_zero_result.has_value() ||
        !touching_zero_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact touching_zero_contact =
        touching_zero_result.value().value();

    check(
        state,
        touching_zero_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Touching zero-segment normal points from A toward B");

    check(
        state,
        touching_zero_contact.point() ==
            PhysicsVector3{
                1.0,
                0.0,
                0.0
            },
        "Touching zero-segment contact preserves shared surface point");

    check(
        state,
        touching_zero_contact.
            penetration_depth() ==
            0.0,
        "Touching zero-segment contact has zero depth");

    const auto capsule_c_right_result =
        ColliderGeometry::create(
            collider_c,
            vertical_capsule_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        capsule_c_right_result.has_value(),
        "Alternate-owner capsule fixture is valid");

    if (!capsule_c_right_result.has_value())
    {
        return finish(state);
    }

    const auto pair_bc_result =
        BroadPhasePair::create(
            collider_b,
            collider_c);

    check(
        state,
        pair_bc_result.has_value(),
        "B-C canonical pair fixture is valid");

    if (!pair_bc_result.has_value())
    {
        return finish(state);
    }

    const auto alternate_contact_result =
        generate_capsule_capsule_contact(
            vertical_capsule_b_origin_result.value(),
            capsule_c_right_result.value());

    check(
        state,
        alternate_contact_result.has_value() &&
            alternate_contact_result.
                value().
                has_value(),
        "Alternate persistent pair produces a contact");

    if (!alternate_contact_result.has_value() ||
        !alternate_contact_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact alternate_contact =
        alternate_contact_result.
            value().
            value();

    check(
        state,
        alternate_contact.pair() ==
            pair_bc_result.value(),
        "Alternate contact preserves persistent pair");

    check(
        state,
        alternate_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Alternate contact normal follows persistent identity order");

    check(
        state,
        alternate_contact.point() ==
            PhysicsVector3{
                0.75,
                -2.0,
                0.0
            },
        "Alternate contact preserves contact point");

    check(
        state,
        alternate_contact.
            penetration_depth() ==
            0.5,
        "Alternate contact preserves penetration depth");

    const auto reversed_alternate_result =
        generate_capsule_capsule_contact(
            capsule_c_right_result.value(),
            vertical_capsule_b_origin_result.value());

    check(
        state,
        reversed_alternate_result.has_value() &&
            reversed_alternate_result.
                value().
                has_value() &&
            reversed_alternate_result.
                value().
                value() ==
                alternate_contact,
        "Alternate result ignores argument order");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const PhysicsScalar large_dimension =
        maximum /
        8.0;

    const auto huge_capsule_shape_result =
        CapsuleShape::create(
            large_dimension,
            large_dimension,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        huge_capsule_shape_result.has_value(),
        "Huge capsule shape fixture is valid");

    if (!huge_capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape huge_capsule_shape{
        huge_capsule_shape_result.value()
    };

    const PhysicsScalar large_center =
        maximum *
        0.75;

    const auto huge_capsule_a_result =
        ColliderGeometry::create(
            collider_a,
            huge_capsule_shape,
            PhysicsVector3{
                -large_center,
                0.0,
                0.0
            });

    const auto huge_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            huge_capsule_shape,
            PhysicsVector3{
                large_center,
                0.0,
                0.0
            });

    check(
        state,
        huge_capsule_a_result.has_value(),
        "Huge capsule A geometry fixture is valid");

    check(
        state,
        huge_capsule_b_result.has_value(),
        "Huge capsule B geometry fixture is valid");

    if (!huge_capsule_a_result.has_value() ||
        !huge_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto huge_separation_result =
        generate_capsule_capsule_contact(
            huge_capsule_a_result.value(),
            huge_capsule_b_result.value());

    check(
        state,
        huge_separation_result.has_value(),
        "Huge-coordinate query avoids intermediate overflow");

    check(
        state,
        huge_separation_result.has_value() &&
            !huge_separation_result.
                value().
                has_value(),
        "Huge-coordinate capsules remain separated");

    const PhysicsScalar overflow_radius =
        maximum *
        0.75;

    const auto overflow_capsule_shape_result =
        CapsuleShape::create(
            overflow_radius,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        overflow_capsule_shape_result.has_value(),
        "Overflow capsule shape fixture is valid");

    if (!overflow_capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape overflow_capsule_shape{
        overflow_capsule_shape_result.value()
    };

    const auto overflow_capsule_a_result =
        ColliderGeometry::create(
            collider_a,
            overflow_capsule_shape,
            physics_zero_vector);

    const auto overflow_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            overflow_capsule_shape,
            physics_zero_vector);

    check(
        state,
        overflow_capsule_a_result.has_value(),
        "Overflow capsule A geometry fixture is valid");

    check(
        state,
        overflow_capsule_b_result.has_value(),
        "Overflow capsule B geometry fixture is valid");

    if (!overflow_capsule_a_result.has_value() ||
        !overflow_capsule_b_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_capsule_capsule_contact(
            overflow_capsule_a_result.value(),
            overflow_capsule_b_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-capsule query rejects overflowing penetration depth");

    return finish(state);
}