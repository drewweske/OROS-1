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
            << "\nSphere-box narrow-phase test summary: "
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
                generate_sphere_box_contact(
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

    const auto sphere_result =
        SphereShape::create(
            1.0);

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

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

    check(
        state,
        box_result.has_value(),
        "Box fixture is valid");

    check(
        state,
        cube_result.has_value(),
        "Cube fixture is valid");

    if (!sphere_result.has_value() ||
        !box_result.has_value() ||
        !cube_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const CollisionShape box_shape{
        box_result.value()
    };

    const CollisionShape cube_shape{
        cube_result.value()
    };

    const auto sphere_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            physics_zero_vector);

    const auto sphere_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
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

    check(
        state,
        sphere_a_origin_result.has_value(),
        "Sphere A origin fixture is valid");

    check(
        state,
        sphere_b_origin_result.has_value(),
        "Sphere B origin fixture is valid");

    check(
        state,
        box_a_origin_result.has_value(),
        "Box A origin fixture is valid");

    check(
        state,
        box_b_origin_result.has_value(),
        "Box B origin fixture is valid");

    if (!sphere_a_origin_result.has_value() ||
        !sphere_b_origin_result.has_value() ||
        !box_a_origin_result.has_value() ||
        !box_b_origin_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_box_contact(
            sphere_a_origin_result.value(),
            sphere_a_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-box query rejects identical collider identity");

    check_failure(
        state,
        generate_sphere_box_contact(
            sphere_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-box query rejects two spheres");

    check_failure(
        state,
        generate_sphere_box_contact(
            box_a_origin_result.value(),
            box_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-box query rejects two boxes");

    const auto separated_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                3.25,
                0.0,
                0.0
            });

    check(
        state,
        separated_sphere_result.has_value(),
        "Separated sphere fixture is valid");

    if (!separated_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto separated_contact_result =
        generate_sphere_box_contact(
            separated_sphere_result.value(),
            box_b_origin_result.value());

    check(
        state,
        separated_contact_result.has_value(),
        "Separated sphere-box query succeeds");

    check(
        state,
        separated_contact_result.has_value() &&
            !separated_contact_result.
                value().
                has_value(),
        "Separated sphere and box produce no contact");

    const auto reversed_separated_result =
        generate_sphere_box_contact(
            box_b_origin_result.value(),
            separated_sphere_result.value());

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

    const auto touching_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_sphere_result.has_value(),
        "Touching sphere fixture is valid");

    if (!touching_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_sphere_box_contact(
            touching_sphere_result.value(),
            box_b_origin_result.value());

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "Face-touching sphere and box produce a contact");

    if (!touching_result.has_value() ||
        !touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact touching_contact =
        touching_result.value().value();

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

    check(
        state,
        touching_contact.pair() ==
            pair_ab,
        "Touching contact preserves canonical pair");

    check(
        state,
        touching_contact.normal() ==
            negative_x_result.value(),
        "Touching normal points from canonical sphere toward box");

    check(
        state,
        touching_contact.point() ==
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
        "Touching contact preserves shared surface point");

    check(
        state,
        touching_contact.penetration_depth() ==
            0.0,
        "Touching contact has zero penetration depth");

    check(
        state,
        touching_contact.is_touching(),
        "Face boundary contact reports touching");

    const auto reversed_touching_result =
        generate_sphere_box_contact(
            box_b_origin_result.value(),
            touching_sphere_result.value());

    check(
        state,
        reversed_touching_result.has_value() &&
            reversed_touching_result.
                value().
                has_value(),
        "Reversed touching query produces a contact");

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
        "Reversed touching arguments produce identical contact");

    const auto penetrating_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                2.5,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_sphere_result.has_value(),
        "Face-penetrating sphere fixture is valid");

    if (!penetrating_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_sphere_box_contact(
            penetrating_sphere_result.value(),
            box_b_origin_result.value());

    check(
        state,
        penetrating_result.has_value() &&
            penetrating_result.
                value().
                has_value(),
        "Face-penetrating sphere and box produce a contact");

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
                0.0,
                0.0
            },
        "Face penetration averages opposing surface points");

    check(
        state,
        penetrating_contact.penetration_depth() ==
            0.5,
        "Face penetration calculates overlap depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Face penetration reports overlap");

    const auto left_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                -2.5,
                0.0,
                0.0
            });

    check(
        state,
        left_sphere_result.has_value(),
        "Negative-X penetration fixture is valid");

    if (!left_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto left_result =
        generate_sphere_box_contact(
            left_sphere_result.value(),
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
            PhysicsUnitVector3::positive_x(),
        "Negative-X sphere points toward box along positive X");

    check(
        state,
        left_contact.point() ==
            PhysicsVector3{
                -1.75,
                0.0,
                0.0
            },
        "Negative-X penetration preserves contact point");

    check(
        state,
        left_contact.penetration_depth() ==
            0.5,
        "Negative-X penetration preserves depth");

    const auto corner_touching_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                2.6,
                3.8,
                4.0
            });

    check(
        state,
        corner_touching_sphere_result.has_value(),
        "Corner-touching sphere fixture is valid");

    if (!corner_touching_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto corner_touching_result =
        generate_sphere_box_contact(
            corner_touching_sphere_result.value(),
            box_b_origin_result.value());

    check(
        state,
        corner_touching_result.has_value() &&
            corner_touching_result.
                value().
                has_value(),
        "Corner-touching sphere produces a contact");

    if (!corner_touching_result.has_value() ||
        !corner_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact corner_contact =
        corner_touching_result.
            value().
            value();

    check(
        state,
        nearly_equal(
            corner_contact.normal().vector(),
            PhysicsVector3{
                -0.6,
                -0.8,
                0.0
            }),
        "Corner contact normal points from sphere toward box");

    check(
        state,
        nearly_equal(
            corner_contact.point(),
            PhysicsVector3{
                2.0,
                3.0,
                4.0
            }),
        "Corner contact preserves shared corner point");

    check(
        state,
        nearly_equal(
            corner_contact.penetration_depth(),
            0.0),
        "Corner boundary contact has zero depth");

    check(
        state,
        corner_contact.is_touching(),
        "Corner boundary contact reports touching");

    const auto inside_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            physics_zero_vector);

    check(
        state,
        inside_sphere_result.has_value(),
        "Inside-box sphere fixture is valid");

    if (!inside_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto inside_result =
        generate_sphere_box_contact(
            inside_sphere_result.value(),
            box_b_origin_result.value());

    check(
        state,
        inside_result.has_value() &&
            inside_result.
                value().
                has_value(),
        "Sphere centered inside box produces a contact");

    if (!inside_result.has_value() ||
        !inside_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact inside_contact =
        inside_result.value().value();

    check(
        state,
        inside_contact.normal() ==
            negative_x_result.value(),
        "Inside sphere uses nearest positive-X face");

    check(
        state,
        inside_contact.point() ==
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            },
        "Inside contact averages sphere and box surfaces");

    check(
        state,
        inside_contact.penetration_depth() ==
            3.0,
        "Inside contact includes radius and distance to exit");

    check(
        state,
        !inside_contact.is_touching(),
        "Inside contact reports penetration");

    const auto near_y_face_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                0.0,
                2.5,
                0.0
            });

    check(
        state,
        near_y_face_result.has_value(),
        "Near-positive-Y inside fixture is valid");

    if (!near_y_face_result.has_value())
    {
        return finish(state);
    }

    const auto near_y_result =
        generate_sphere_box_contact(
            near_y_face_result.value(),
            box_b_origin_result.value());

    check(
        state,
        near_y_result.has_value() &&
            near_y_result.
                value().
                has_value(),
        "Inside sphere near positive-Y face produces a contact");

    if (!near_y_result.has_value() ||
        !near_y_result.value().has_value())
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

    const CollisionContact near_y_contact =
        near_y_result.value().value();

    check(
        state,
        near_y_contact.normal() ==
            negative_y_result.value(),
        "Inside contact selects the nearest positive-Y face");

    check(
        state,
        near_y_contact.point() ==
            PhysicsVector3{
                0.0,
                3.25,
                0.0
            },
        "Inside positive-Y contact preserves surface average");

    check(
        state,
        near_y_contact.penetration_depth() ==
            1.5,
        "Inside positive-Y contact preserves exit depth");

    const auto boundary_center_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        boundary_center_result.has_value(),
        "Box-boundary center fixture is valid");

    if (!boundary_center_result.has_value())
    {
        return finish(state);
    }

    const auto boundary_center_contact_result =
        generate_sphere_box_contact(
            boundary_center_result.value(),
            box_b_origin_result.value());

    check(
        state,
        boundary_center_contact_result.
            has_value() &&
            boundary_center_contact_result.
                value().
                has_value(),
        "Sphere center on box boundary produces a contact");

    if (!boundary_center_contact_result.
            has_value() ||
        !boundary_center_contact_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact
        boundary_center_contact =
            boundary_center_contact_result.
                value().
                value();

    check(
        state,
        boundary_center_contact.normal() ==
            negative_x_result.value(),
        "Boundary-center contact preserves outward face direction");

    check(
        state,
        boundary_center_contact.point() ==
            PhysicsVector3{
                2.5,
                0.0,
                0.0
            },
        "Boundary-center contact preserves surface average");

    check(
        state,
        boundary_center_contact.
            penetration_depth() ==
            1.0,
        "Boundary-center overlap equals sphere radius");

    const auto cube_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            physics_zero_vector);

    check(
        state,
        cube_b_result.has_value(),
        "Cube B geometry fixture is valid");

    if (!cube_b_result.has_value())
    {
        return finish(state);
    }

    const auto centered_cube_result =
        generate_sphere_box_contact(
            inside_sphere_result.value(),
            cube_b_result.value());

    check(
        state,
        centered_cube_result.has_value() &&
            centered_cube_result.
                value().
                has_value(),
        "Sphere centered in cube produces a contact");

    if (!centered_cube_result.has_value() ||
        !centered_cube_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact centered_cube_contact =
        centered_cube_result.value().value();

    check(
        state,
        centered_cube_contact.normal() ==
            negative_x_result.value(),
        "Equal-distance face tie deterministically selects positive X");

    check(
        state,
        centered_cube_contact.point() ==
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            },
        "Tie-selected cube face preserves contact point");

    check(
        state,
        centered_cube_contact.
            penetration_depth() ==
            3.0,
        "Tie-selected cube face preserves penetration depth");

    const auto sphere_b_touching_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        sphere_b_touching_result.has_value(),
        "Sphere B touching fixture is valid");

    if (!sphere_b_touching_result.has_value())
    {
        return finish(state);
    }

    const auto box_first_contact_result =
        generate_sphere_box_contact(
            sphere_b_touching_result.value(),
            box_a_origin_result.value());

    check(
        state,
        box_first_contact_result.has_value() &&
            box_first_contact_result.
                value().
                has_value(),
        "Canonical box-first pair produces a contact");

    if (!box_first_contact_result.has_value() ||
        !box_first_contact_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact box_first_contact =
        box_first_contact_result.
            value().
            value();

    check(
        state,
        box_first_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Canonical box-first normal points from box toward sphere");

    check(
        state,
        box_first_contact.point() ==
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
        "Canonical box-first contact preserves point");

    const auto reversed_box_first_result =
        generate_sphere_box_contact(
            box_a_origin_result.value(),
            sphere_b_touching_result.value());

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

    const auto maximum_sphere_result =
        SphereShape::create(
            maximum);

    const auto maximum_box_result =
        BoxShape::create(
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            });

    check(
        state,
        maximum_sphere_result.has_value(),
        "Maximum-radius sphere fixture is valid");

    check(
        state,
        maximum_box_result.has_value(),
        "Maximum-size box fixture is valid");

    if (!maximum_sphere_result.has_value() ||
        !maximum_box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_sphere_shape{
        maximum_sphere_result.value()
    };

    const CollisionShape maximum_box_shape{
        maximum_box_result.value()
    };

    const auto maximum_sphere_geometry_result =
        ColliderGeometry::create(
            collider_a,
            maximum_sphere_shape,
            physics_zero_vector);

    const auto maximum_box_geometry_result =
        ColliderGeometry::create(
            collider_b,
            maximum_box_shape,
            physics_zero_vector);

    check(
        state,
        maximum_sphere_geometry_result.
            has_value(),
        "Maximum sphere geometry fixture is valid");

    check(
        state,
        maximum_box_geometry_result.
            has_value(),
        "Maximum box geometry fixture is valid");

    if (!maximum_sphere_geometry_result.
            has_value() ||
        !maximum_box_geometry_result.
            has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_box_contact(
            maximum_sphere_geometry_result.
                value(),
            maximum_box_geometry_result.
                value()),
        ErrorCode::invalid_argument,
        "Sphere-box query rejects overflowing penetration depth");

    const auto alternate_sphere_result =
        ColliderGeometry::create(
            collider_c,
            sphere_shape,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        alternate_sphere_result.has_value(),
        "Alternate-owner sphere fixture is valid");

    if (!alternate_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto alternate_pair_result =
        BroadPhasePair::create(
            collider_b,
            collider_c);

    check(
        state,
        alternate_pair_result.has_value(),
        "B-C canonical pair fixture is valid");

    const auto alternate_contact_result =
        generate_sphere_box_contact(
            alternate_sphere_result.value(),
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
        alternate_pair_result.has_value() &&
            alternate_contact_result.
                has_value() &&
            alternate_contact_result.
                value().
                has_value() &&
            alternate_contact_result.
                value().
                value().
                pair() ==
                alternate_pair_result.value(),
        "Alternate contact preserves its persistent pair");

    return finish(state);
}