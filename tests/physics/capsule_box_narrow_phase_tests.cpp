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
            << "\nCapsule-box narrow-phase test summary: "
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
                generate_capsule_box_contact(
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

    const auto capsule_result =
        CapsuleShape::create(
            1.0,
            2.0,
            PhysicsUnitVector3::
                positive_y());

    const auto zero_segment_capsule_result =
        CapsuleShape::create(
            1.0,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    const auto short_x_capsule_result =
        CapsuleShape::create(
            1.0,
            0.5,
            PhysicsUnitVector3::
                positive_x());

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                2.0,
                3.0,
                4.0
            });

    const auto cube_result =
        BoxShape::create(
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            });

    const auto sphere_result =
        SphereShape::create(
            1.0);

    check(
        state,
        capsule_result.has_value(),
        "Capsule fixture is valid");

    check(
        state,
        zero_segment_capsule_result.
            has_value(),
        "Zero-segment capsule fixture is valid");

    check(
        state,
        short_x_capsule_result.has_value(),
        "Short X-axis capsule fixture is valid");

    check(
        state,
        box_result.has_value(),
        "Box fixture is valid");

    check(
        state,
        cube_result.has_value(),
        "Cube fixture is valid");

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

    if (!capsule_result.has_value() ||
        !zero_segment_capsule_result.
            has_value() ||
        !short_x_capsule_result.has_value() ||
        !box_result.has_value() ||
        !cube_result.has_value() ||
        !sphere_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape capsule_shape{
        capsule_result.value()
    };

    const CollisionShape
        zero_segment_capsule_shape{
            zero_segment_capsule_result.value()
        };

    const CollisionShape short_x_capsule_shape{
        short_x_capsule_result.value()
    };

    const CollisionShape box_shape{
        box_result.value()
    };

    const CollisionShape cube_shape{
        cube_result.value()
    };

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const auto capsule_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            physics_zero_vector);

    const auto capsule_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            capsule_shape,
            physics_zero_vector);

    const auto box_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            box_shape,
            physics_zero_vector);

    const auto box_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            box_shape,
            physics_zero_vector);

    const auto sphere_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            physics_zero_vector);

    check(
        state,
        capsule_a_origin_result.has_value(),
        "Capsule A origin fixture is valid");

    check(
        state,
        capsule_b_origin_result.has_value(),
        "Capsule B origin fixture is valid");

    check(
        state,
        box_a_origin_result.has_value(),
        "Box A origin fixture is valid");

    check(
        state,
        box_b_origin_result.has_value(),
        "Box B origin fixture is valid");

    check(
        state,
        sphere_b_origin_result.has_value(),
        "Sphere B origin fixture is valid");

    if (!capsule_a_origin_result.has_value() ||
        !capsule_b_origin_result.has_value() ||
        !box_a_origin_result.has_value() ||
        !box_b_origin_result.has_value() ||
        !sphere_b_origin_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_capsule_box_contact(
            capsule_a_origin_result.value(),
            box_a_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-box query rejects identical collider identity");

    check_failure(
        state,
        generate_capsule_box_contact(
            capsule_a_origin_result.value(),
            capsule_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-box query rejects two capsules");

    check_failure(
        state,
        generate_capsule_box_contact(
            box_a_origin_result.value(),
            box_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-box query rejects two boxes");

    check_failure(
        state,
        generate_capsule_box_contact(
            capsule_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Capsule-box query rejects a sphere");

    const auto separated_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                3.25,
                0.0,
                0.0
            });

    check(
        state,
        separated_capsule_result.has_value(),
        "Separated capsule fixture is valid");

    if (!separated_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto separated_result =
        generate_capsule_box_contact(
            separated_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        separated_result.has_value(),
        "Separated capsule-box query succeeds");

    check(
        state,
        separated_result.has_value() &&
            !separated_result.
                value().
                has_value(),
        "Separated capsule and box produce no contact");

    const auto reversed_separated_result =
        generate_capsule_box_contact(
            box_b_origin_result.value(),
            separated_capsule_result.value());

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

    const auto touching_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_capsule_result.has_value(),
        "Face-touching capsule fixture is valid");

    if (!touching_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_capsule_box_contact(
            touching_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "Face-touching capsule and box produce a contact");

    if (!touching_result.has_value() ||
        !touching_result.value().has_value())
    {
        return finish(state);
    }

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

    const BroadPhasePair pair_ab =
        pair_ab_result.value();

    const auto negative_x_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            });

    check(
        state,
        negative_x_result.has_value(),
        "Negative-X normal fixture is valid");

    if (!negative_x_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact touching_contact =
        touching_result.value().value();

    check(
        state,
        touching_contact.pair() ==
            pair_ab,
        "Touching contact preserves canonical pair");

    check(
        state,
        touching_contact.normal() ==
            negative_x_result.value(),
        "Touching normal points from capsule toward box");

    check(
        state,
        touching_contact.point() ==
            PhysicsVector3{
                2.0,
                -2.0,
                0.0
            },
        "Face-touching contact selects deterministic segment endpoint");

    check(
        state,
        touching_contact.
            penetration_depth() ==
            0.0,
        "Face-touching contact has zero depth");

    check(
        state,
        touching_contact.is_touching(),
        "Face boundary contact reports touching");

    const auto reversed_touching_result =
        generate_capsule_box_contact(
            box_b_origin_result.value(),
            touching_capsule_result.value());

    check(
        state,
        reversed_touching_result.has_value() &&
            reversed_touching_result.
                value().
                has_value(),
        "Reversed face-touching query produces a contact");

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
        "Face-touching result ignores argument order");

    const auto penetrating_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                2.5,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_capsule_result.has_value(),
        "Face-penetrating capsule fixture is valid");

    if (!penetrating_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_capsule_box_contact(
            penetrating_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        penetrating_result.has_value() &&
            penetrating_result.
                value().
                has_value(),
        "Face penetration produces a contact");

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
            negative_x_result.value(),
        "Face penetration preserves canonical normal");

    check(
        state,
        penetrating_contact.point() ==
            PhysicsVector3{
                1.75,
                -2.0,
                0.0
            },
        "Face penetration averages opposing surfaces");

    check(
        state,
        penetrating_contact.
            penetration_depth() ==
            0.5,
        "Face penetration calculates depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Face penetration reports overlap");

    const auto reversed_penetrating_result =
        generate_capsule_box_contact(
            box_b_origin_result.value(),
            penetrating_capsule_result.value());

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
        "Face penetration ignores argument order");

    const auto left_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                -2.5,
                0.0,
                0.0
            });

    check(
        state,
        left_capsule_result.has_value(),
        "Negative-X penetration fixture is valid");

    if (!left_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto left_result =
        generate_capsule_box_contact(
            left_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        left_result.has_value() &&
            left_result.
                value().
                has_value(),
        "Negative-X penetration produces a contact");

    if (!left_result.has_value() ||
        !left_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact left_contact =
        left_result.value().value();

    check(
        state,
        left_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Negative-X capsule points toward box along positive X");

    check(
        state,
        left_contact.point() ==
            PhysicsVector3{
                -1.75,
                -2.0,
                0.0
            },
        "Negative-X penetration preserves contact point");

    check(
        state,
        left_contact.
            penetration_depth() ==
            0.5,
        "Negative-X penetration preserves depth");

    const auto top_touching_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                0.0,
                6.0,
                0.0
            });

    check(
        state,
        top_touching_capsule_result.has_value(),
        "Top-touching capsule fixture is valid");

    if (!top_touching_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto top_touching_result =
        generate_capsule_box_contact(
            top_touching_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        top_touching_result.has_value() &&
            top_touching_result.
                value().
                has_value(),
        "Capsule end-cap touching box top produces a contact");

    if (!top_touching_result.has_value() ||
        !top_touching_result.value().has_value())
    {
        return finish(state);
    }

    const auto negative_y_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                0.0,
                -1.0,
                0.0
            });

    check(
        state,
        negative_y_result.has_value(),
        "Negative-Y normal fixture is valid");

    if (!negative_y_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact top_touching_contact =
        top_touching_result.value().value();

    check(
        state,
        top_touching_contact.normal() ==
            negative_y_result.value(),
        "Top-touching normal points from capsule toward box");

    check(
        state,
        top_touching_contact.point() ==
            PhysicsVector3{
                0.0,
                3.0,
                0.0
            },
        "Top-touching contact preserves shared surface point");

    check(
        state,
        top_touching_contact.
            penetration_depth() ==
            0.0,
        "Top-touching contact has zero depth");

    check(
        state,
        top_touching_contact.is_touching(),
        "Top boundary contact reports touching");

    const auto top_penetrating_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                0.0,
                5.5,
                0.0
            });

    check(
        state,
        top_penetrating_capsule_result.
            has_value(),
        "Top-penetrating capsule fixture is valid");

    if (!top_penetrating_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const auto top_penetrating_result =
        generate_capsule_box_contact(
            top_penetrating_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        top_penetrating_result.has_value() &&
            top_penetrating_result.
                value().
                has_value(),
        "Capsule end-cap penetration produces a contact");

    if (!top_penetrating_result.has_value() ||
        !top_penetrating_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact top_penetrating_contact =
        top_penetrating_result.
            value().
            value();

    check(
        state,
        top_penetrating_contact.normal() ==
            negative_y_result.value(),
        "Top penetration preserves canonical normal");

    check(
        state,
        top_penetrating_contact.point() ==
            PhysicsVector3{
                0.0,
                2.75,
                0.0
            },
        "Top penetration averages opposing surfaces");

    check(
        state,
        top_penetrating_contact.
            penetration_depth() ==
            0.5,
        "Top penetration calculates depth");

    const auto corner_touching_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                2.6,
                0.0,
                4.8
            });

    check(
        state,
        corner_touching_capsule_result.
            has_value(),
        "Corner-touching capsule fixture is valid");

    if (!corner_touching_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const auto corner_touching_result =
        generate_capsule_box_contact(
            corner_touching_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        corner_touching_result.has_value() &&
            corner_touching_result.
                value().
                has_value(),
        "Capsule side touching box corner produces a contact");

    if (!corner_touching_result.has_value() ||
        !corner_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact corner_touching_contact =
        corner_touching_result.
            value().
            value();

    check(
        state,
        nearly_equal(
            corner_touching_contact.
                normal().
                vector(),
            PhysicsVector3{
                -0.6,
                0.0,
                -0.8
            }),
        "Corner contact normal points from capsule toward box");

    check(
        state,
        nearly_equal(
            corner_touching_contact.point(),
            PhysicsVector3{
                2.0,
                -2.0,
                4.0
            }),
        "Corner contact preserves shared box corner");

    check(
        state,
        nearly_equal(
            corner_touching_contact.
                penetration_depth(),
            0.0),
        "Corner boundary contact has zero depth");

    check(
        state,
        corner_touching_contact.is_touching(),
        "Corner boundary contact reports touching");

    const auto endpoint_touching_capsule_result =
        ColliderGeometry::create(
            collider_a,
            short_x_capsule_shape,
            PhysicsVector3{
                3.1,
                3.8,
                0.0
            });

    check(
        state,
        endpoint_touching_capsule_result.
            has_value(),
        "Endpoint corner fixture is valid");

    if (!endpoint_touching_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const auto endpoint_touching_result =
        generate_capsule_box_contact(
            endpoint_touching_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        endpoint_touching_result.has_value() &&
            endpoint_touching_result.
                value().
                has_value(),
        "Capsule endpoint touching a box edge produces a contact");

    if (!endpoint_touching_result.has_value() ||
        !endpoint_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact endpoint_touching_contact =
        endpoint_touching_result.
            value().
            value();

    check(
        state,
        nearly_equal(
            endpoint_touching_contact.
                normal().
                vector(),
            PhysicsVector3{
                -0.6,
                -0.8,
                0.0
            }),
        "Endpoint contact uses segment-to-box closest-point normal");

    check(
        state,
        nearly_equal(
            endpoint_touching_contact.point(),
            PhysicsVector3{
                2.0,
                3.0,
                0.0
            }),
        "Endpoint contact preserves shared box edge point");

    check(
        state,
        nearly_equal(
            endpoint_touching_contact.
                penetration_depth(),
            0.0),
        "Endpoint boundary contact has zero depth");

    const auto cube_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            physics_zero_vector);

    check(
        state,
        cube_b_origin_result.has_value(),
        "Cube B origin fixture is valid");

    if (!cube_b_origin_result.has_value())
    {
        return finish(state);
    }

    const auto centered_inside_result =
        generate_capsule_box_contact(
            capsule_a_origin_result.value(),
            cube_b_origin_result.value());

    check(
        state,
        centered_inside_result.has_value() &&
            centered_inside_result.
                value().
                has_value(),
        "Capsule centered inside cube produces a contact");

    if (!centered_inside_result.has_value() ||
        !centered_inside_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact centered_inside_contact =
        centered_inside_result.
            value().
            value();

    check(
        state,
        centered_inside_contact.normal() ==
            negative_x_result.value(),
        "Equal-distance inside tie selects positive-X exit");

    check(
        state,
        centered_inside_contact.point() ==
            PhysicsVector3{
                1.5,
                -2.0,
                0.0
            },
        "Inside contact averages facing capsule and box surfaces");

    check(
        state,
        centered_inside_contact.
            penetration_depth() ==
            3.0,
        "Inside contact includes capsule radius and exit distance");

    check(
        state,
        !centered_inside_contact.is_touching(),
        "Inside contact reports penetration");

    const auto near_y_capsule_result =
        ColliderGeometry::create(
            collider_a,
            short_x_capsule_shape,
            PhysicsVector3{
                0.0,
                2.5,
                0.0
            });

    check(
        state,
        near_y_capsule_result.has_value(),
        "Near-positive-Y inside capsule fixture is valid");

    if (!near_y_capsule_result.has_value())
    {
        return finish(state);
    }

    const auto near_y_result =
        generate_capsule_box_contact(
            near_y_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        near_y_result.has_value() &&
            near_y_result.
                value().
                has_value(),
        "Inside capsule near positive-Y face produces a contact");

    if (!near_y_result.has_value() ||
        !near_y_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact near_y_contact =
        near_y_result.value().value();

    check(
        state,
        near_y_contact.normal() ==
            negative_y_result.value(),
        "Inside contact selects nearest positive-Y exit");

    check(
        state,
        near_y_contact.point() ==
            PhysicsVector3{
                -0.5,
                3.25,
                0.0
            },
        "Inside positive-Y contact averages facing surfaces");

    check(
        state,
        near_y_contact.
            penetration_depth() ==
            1.5,
        "Inside positive-Y contact preserves exit depth");

    const auto boundary_center_capsule_result =
        ColliderGeometry::create(
            collider_a,
            capsule_shape,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        boundary_center_capsule_result.
            has_value(),
        "Capsule center-line boundary fixture is valid");

    if (!boundary_center_capsule_result.
            has_value())
    {
        return finish(state);
    }

    const auto boundary_center_result =
        generate_capsule_box_contact(
            boundary_center_capsule_result.value(),
            box_b_origin_result.value());

    check(
        state,
        boundary_center_result.has_value() &&
            boundary_center_result.
                value().
                has_value(),
        "Capsule center line on box face produces a contact");

    if (!boundary_center_result.has_value() ||
        !boundary_center_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact boundary_center_contact =
        boundary_center_result.
            value().
            value();

    check(
        state,
        boundary_center_contact.normal() ==
            negative_x_result.value(),
        "Boundary-center contact preserves positive-X exit");

    check(
        state,
        boundary_center_contact.point() ==
            PhysicsVector3{
                2.5,
                -2.0,
                0.0
            },
        "Boundary-center contact averages facing surfaces");

    check(
        state,
        boundary_center_contact.
            penetration_depth() ==
            1.0,
        "Boundary-center overlap equals capsule radius");

    const auto zero_capsule_a_result =
        ColliderGeometry::create(
            collider_a,
            zero_segment_capsule_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        zero_capsule_a_result.has_value(),
        "Zero-segment capsule geometry is valid");

    if (!zero_capsule_a_result.has_value())
    {
        return finish(state);
    }

    const auto zero_capsule_contact_result =
        generate_capsule_box_contact(
            zero_capsule_a_result.value(),
            box_b_origin_result.value());

    check(
        state,
        zero_capsule_contact_result.has_value() &&
            zero_capsule_contact_result.
                value().
                has_value(),
        "Zero-segment capsule touching box produces a contact");

    if (!zero_capsule_contact_result.has_value() ||
        !zero_capsule_contact_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact zero_capsule_contact =
        zero_capsule_contact_result.
            value().
            value();

    check(
        state,
        zero_capsule_contact.normal() ==
            negative_x_result.value(),
        "Zero-segment capsule preserves sphere-like normal");

    check(
        state,
        zero_capsule_contact.point() ==
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
        "Zero-segment capsule preserves sphere-like contact point");

    check(
        state,
        zero_capsule_contact.
            penetration_depth() ==
            0.0,
        "Zero-segment touching contact has zero depth");

    const auto capsule_b_touching_result =
        ColliderGeometry::create(
            collider_b,
            capsule_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        capsule_b_touching_result.has_value(),
        "Capsule B touching fixture is valid");

    if (!capsule_b_touching_result.has_value())
    {
        return finish(state);
    }

    const auto box_first_result =
        generate_capsule_box_contact(
            capsule_b_touching_result.value(),
            box_a_origin_result.value());

    check(
        state,
        box_first_result.has_value() &&
            box_first_result.
                value().
                has_value(),
        "Canonical box-first pair produces a contact");

    if (!box_first_result.has_value() ||
        !box_first_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact box_first_contact =
        box_first_result.value().value();

    check(
        state,
        box_first_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Canonical box-first normal points from box toward capsule");

    check(
        state,
        box_first_contact.point() ==
            PhysicsVector3{
                2.0,
                -2.0,
                0.0
            },
        "Canonical box-first contact preserves point");

    const auto reversed_box_first_result =
        generate_capsule_box_contact(
            box_a_origin_result.value(),
            capsule_b_touching_result.value());

    check(
        state,
        reversed_box_first_result.has_value() &&
            reversed_box_first_result.
                value().
                has_value() &&
            reversed_box_first_result.
                value().
                value() ==
                box_first_contact,
        "Canonical box-first result ignores argument order");

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

    const auto huge_box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                large_dimension,
                large_dimension,
                large_dimension
            });

    check(
        state,
        huge_capsule_shape_result.has_value(),
        "Huge capsule shape fixture is valid");

    check(
        state,
        huge_box_shape_result.has_value(),
        "Huge box shape fixture is valid");

    if (!huge_capsule_shape_result.has_value() ||
        !huge_box_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape huge_capsule_shape{
        huge_capsule_shape_result.value()
    };

    const CollisionShape huge_box_shape{
        huge_box_shape_result.value()
    };

    const PhysicsScalar large_center =
        maximum *
        0.75;

    const auto huge_capsule_geometry_result =
        ColliderGeometry::create(
            collider_a,
            huge_capsule_shape,
            PhysicsVector3{
                -large_center,
                0.0,
                0.0
            });

    const auto huge_box_geometry_result =
        ColliderGeometry::create(
            collider_b,
            huge_box_shape,
            PhysicsVector3{
                large_center,
                0.0,
                0.0
            });

    check(
        state,
        huge_capsule_geometry_result.has_value(),
        "Huge capsule geometry fixture is valid");

    check(
        state,
        huge_box_geometry_result.has_value(),
        "Huge box geometry fixture is valid");

    if (!huge_capsule_geometry_result.has_value() ||
        !huge_box_geometry_result.has_value())
    {
        return finish(state);
    }

    const auto huge_separation_result =
        generate_capsule_box_contact(
            huge_capsule_geometry_result.value(),
            huge_box_geometry_result.value());

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
        "Huge-coordinate capsule and box remain separated");

    const auto maximum_capsule_shape_result =
        CapsuleShape::create(
            maximum,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    const auto maximum_box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            });

    check(
        state,
        maximum_capsule_shape_result.has_value(),
        "Maximum-radius capsule fixture is valid");

    check(
        state,
        maximum_box_shape_result.has_value(),
        "Maximum-size box fixture is valid");

    if (!maximum_capsule_shape_result.has_value() ||
        !maximum_box_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_capsule_shape{
        maximum_capsule_shape_result.value()
    };

    const CollisionShape maximum_box_shape{
        maximum_box_shape_result.value()
    };

    const auto maximum_capsule_geometry_result =
        ColliderGeometry::create(
            collider_a,
            maximum_capsule_shape,
            physics_zero_vector);

    const auto maximum_box_geometry_result =
        ColliderGeometry::create(
            collider_b,
            maximum_box_shape,
            physics_zero_vector);

    check(
        state,
        maximum_capsule_geometry_result.
            has_value(),
        "Maximum capsule geometry fixture is valid");

    check(
        state,
        maximum_box_geometry_result.
            has_value(),
        "Maximum box geometry fixture is valid");

    if (!maximum_capsule_geometry_result.
            has_value() ||
        !maximum_box_geometry_result.
            has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_capsule_box_contact(
            maximum_capsule_geometry_result.
                value(),
            maximum_box_geometry_result.
                value()),
        ErrorCode::invalid_argument,
        "Capsule-box query rejects overflowing penetration depth");

    const auto capsule_c_touching_result =
        ColliderGeometry::create(
            collider_c,
            capsule_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        capsule_c_touching_result.has_value(),
        "Alternate-owner capsule fixture is valid");

    if (!capsule_c_touching_result.has_value())
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

    const auto alternate_contact_result =
        generate_capsule_box_contact(
            capsule_c_touching_result.value(),
            box_b_origin_result.value());

    check(
        state,
        alternate_contact_result.has_value() &&
            alternate_contact_result.
                value().
                has_value(),
        "Alternate persistent pair produces a contact");

    check(
        state,
        pair_bc_result.has_value() &&
            alternate_contact_result.
                has_value() &&
            alternate_contact_result.
                value().
                has_value() &&
            alternate_contact_result.
                value().
                value().
                pair() ==
                pair_bc_result.value(),
        "Alternate contact preserves persistent pair");

    check(
        state,
        alternate_contact_result.has_value() &&
            alternate_contact_result.
                value().
                has_value() &&
            alternate_contact_result.
                value().
                value().
                normal() ==
                PhysicsUnitVector3::
                    positive_x(),
        "Alternate contact normal follows persistent identity order");

    return finish(state);
}