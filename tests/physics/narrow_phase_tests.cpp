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
            << "\nNarrow-phase test summary: "
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
                generate_sphere_sphere_contact(
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

    const auto radius_two_result =
        SphereShape::create(
            2.0);

    const auto radius_three_result =
        SphereShape::create(
            3.0);

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    check(
        state,
        radius_two_result.has_value(),
        "Radius-two sphere fixture is valid");

    check(
        state,
        radius_three_result.has_value(),
        "Radius-three sphere fixture is valid");

    check(
        state,
        box_result.has_value(),
        "Box fixture is valid");

    if (!radius_two_result.has_value() ||
        !radius_three_result.has_value() ||
        !box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape radius_two{
        radius_two_result.value()
    };

    const CollisionShape radius_three{
        radius_three_result.value()
    };

    const CollisionShape box_shape{
        box_result.value()
    };

    const auto origin_a_result =
        ColliderGeometry::create(
            collider_a,
            radius_two,
            physics_zero_vector);

    const auto origin_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            physics_zero_vector);

    const auto box_a_result =
        ColliderGeometry::create(
            collider_a,
            box_shape,
            physics_zero_vector);

    const auto box_b_result =
        ColliderGeometry::create(
            collider_b,
            box_shape,
            physics_zero_vector);

    check(
        state,
        origin_a_result.has_value(),
        "Origin A sphere geometry fixture is valid");

    check(
        state,
        origin_b_result.has_value(),
        "Origin B sphere geometry fixture is valid");

    check(
        state,
        box_a_result.has_value(),
        "A box geometry fixture is valid");

    check(
        state,
        box_b_result.has_value(),
        "B box geometry fixture is valid");

    if (!origin_a_result.has_value() ||
        !origin_b_result.has_value() ||
        !box_a_result.has_value() ||
        !box_b_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            origin_a_result.value()),
        ErrorCode::invalid_argument,
        "Sphere contact rejects identical collider identity");

    check_failure(
        state,
        generate_sphere_sphere_contact(
            box_a_result.value(),
            origin_b_result.value()),
        ErrorCode::invalid_argument,
        "Sphere contact rejects non-sphere first geometry");

    check_failure(
        state,
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            box_b_result.value()),
        ErrorCode::invalid_argument,
        "Sphere contact rejects non-sphere second geometry");

    check_failure(
        state,
        generate_sphere_sphere_contact(
            box_a_result.value(),
            box_b_result.value()),
        ErrorCode::invalid_argument,
        "Sphere contact rejects two non-sphere geometries");

    const auto separated_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                5.25,
                0.0,
                0.0
            });

    check(
        state,
        separated_b_result.has_value(),
        "Separated sphere geometry fixture is valid");

    if (!separated_b_result.has_value())
    {
        return finish(state);
    }

    const auto separated_contact_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            separated_b_result.value());

    check(
        state,
        separated_contact_result.has_value(),
        "Separated sphere query succeeds");

    check(
        state,
        separated_contact_result.has_value() &&
            !separated_contact_result.
                value().
                has_value(),
        "Separated spheres produce no contact");

    const auto reversed_separated_result =
        generate_sphere_sphere_contact(
            separated_b_result.value(),
            origin_a_result.value());

    check(
        state,
        reversed_separated_result.has_value(),
        "Reversed separated sphere query succeeds");

    check(
        state,
        reversed_separated_result.has_value() &&
            !reversed_separated_result.
                value().
                has_value(),
        "Reversed separated spheres produce no contact");

    const auto touching_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                5.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_b_result.has_value(),
        "Touching sphere geometry fixture is valid");

    if (!touching_b_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            touching_b_result.value());

    check(
        state,
        touching_result.has_value(),
        "Touching sphere query succeeds");

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "Touching spheres produce a contact");

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

    check(
        state,
        touching_contact.pair() ==
            pair_ab,
        "Touching contact preserves canonical pair");

    check(
        state,
        touching_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Touching contact normal points from canonical first to second");

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
        "Touching sphere contact reports touching");

    const auto reversed_touching_result =
        generate_sphere_sphere_contact(
            touching_b_result.value(),
            origin_a_result.value());

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

    const auto penetrating_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                4.0,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_b_result.has_value(),
        "Penetrating sphere geometry fixture is valid");

    if (!penetrating_b_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            penetrating_b_result.value());

    check(
        state,
        penetrating_result.has_value() &&
            penetrating_result.
                value().
                has_value(),
        "Penetrating spheres produce a contact");

    if (!penetrating_result.has_value() ||
        !penetrating_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact penetrating_contact =
        penetrating_result.value().value();

    check(
        state,
        penetrating_contact.pair() ==
            pair_ab,
        "Penetrating contact preserves canonical pair");

    check(
        state,
        penetrating_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Penetrating contact preserves positive-X normal");

    check(
        state,
        penetrating_contact.point() ==
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            },
        "Penetrating contact averages opposing surface points");

    check(
        state,
        penetrating_contact.penetration_depth() ==
            1.0,
        "Penetrating contact calculates overlap depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Penetrating sphere contact reports penetration");

    const auto reversed_penetrating_result =
        generate_sphere_sphere_contact(
            penetrating_b_result.value(),
            origin_a_result.value());

    check(
        state,
        reversed_penetrating_result.has_value() &&
            reversed_penetrating_result.
                value().
                has_value(),
        "Reversed penetrating query produces a contact");

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
        "Reversed penetrating arguments produce identical contact");

    const auto left_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                -4.0,
                0.0,
                0.0
            });

    check(
        state,
        left_b_result.has_value(),
        "Negative-X sphere geometry fixture is valid");

    if (!left_b_result.has_value())
    {
        return finish(state);
    }

    const auto left_contact_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            left_b_result.value());

    check(
        state,
        left_contact_result.has_value() &&
            left_contact_result.
                value().
                has_value(),
        "Negative-X overlap produces a contact");

    if (!left_contact_result.has_value() ||
        !left_contact_result.value().has_value())
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

    check(
        state,
        negative_x_result.has_value(),
        "Negative-X unit normal fixture is valid");

    if (!negative_x_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact left_contact =
        left_contact_result.value().value();

    check(
        state,
        left_contact.normal() ==
            negative_x_result.value(),
        "Contact normal follows canonical first collider toward second");

    check(
        state,
        left_contact.point() ==
            PhysicsVector3{
                -1.5,
                0.0,
                0.0
            },
        "Negative-X contact point preserves orientation");

    check(
        state,
        left_contact.penetration_depth() ==
            1.0,
        "Negative-X contact preserves penetration depth");

    const auto reversed_left_result =
        generate_sphere_sphere_contact(
            left_b_result.value(),
            origin_a_result.value());

    check(
        state,
        reversed_left_result.has_value() &&
            reversed_left_result.
                value().
                has_value() &&
            reversed_left_result.
                value().
                value() ==
                left_contact,
        "Negative-X contact remains identical across argument order");

    const auto diagonal_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                3.0,
                4.0,
                0.0
            });

    check(
        state,
        diagonal_b_result.has_value(),
        "Diagonal sphere geometry fixture is valid");

    if (!diagonal_b_result.has_value())
    {
        return finish(state);
    }

    const auto diagonal_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            diagonal_b_result.value());

    check(
        state,
        diagonal_result.has_value() &&
            diagonal_result.
                value().
                has_value(),
        "Diagonal touching spheres produce a contact");

    if (!diagonal_result.has_value() ||
        !diagonal_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact diagonal_contact =
        diagonal_result.value().value();

    check(
        state,
        nearly_equal(
            diagonal_contact.
                normal().
                vector(),
            PhysicsVector3{
                0.6,
                0.8,
                0.0
            }),
        "Diagonal contact normal preserves center direction");

    check(
        state,
        nearly_equal(
            diagonal_contact.point(),
            PhysicsVector3{
                1.2,
                1.6,
                0.0
            }),
        "Diagonal touching contact preserves shared surface point");

    check(
        state,
        nearly_equal(
            diagonal_contact.
                penetration_depth(),
            0.0),
        "Diagonal touching contact has zero depth");

    check(
        state,
        diagonal_contact.is_touching(),
        "Diagonal boundary contact reports touching");

    const auto coincident_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            origin_b_result.value());

    check(
        state,
        coincident_result.has_value() &&
            coincident_result.
                value().
                has_value(),
        "Coincident spheres produce a deterministic contact");

    if (!coincident_result.has_value() ||
        !coincident_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact coincident_contact =
        coincident_result.value().value();

    check(
        state,
        coincident_contact.pair() ==
            pair_ab,
        "Coincident contact preserves canonical pair");

    check(
        state,
        coincident_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Coincident centers use positive-X fallback normal");

    check(
        state,
        coincident_contact.point() ==
            PhysicsVector3{
                -0.5,
                0.0,
                0.0
            },
        "Coincident contact point follows canonical sphere radii");

    check(
        state,
        coincident_contact.
            penetration_depth() ==
            5.0,
        "Coincident contact depth equals combined radii");

    check(
        state,
        !coincident_contact.is_touching(),
        "Coincident contact reports penetration");

    const auto reversed_coincident_result =
        generate_sphere_sphere_contact(
            origin_b_result.value(),
            origin_a_result.value());

    check(
        state,
        reversed_coincident_result.has_value() &&
            reversed_coincident_result.
                value().
                has_value() &&
            reversed_coincident_result.
                value().
                value() ==
                coincident_contact,
        "Coincident contact remains identical across argument order");

    const auto contained_b_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            PhysicsVector3{
                1.0,
                0.0,
                0.0
            });

    check(
        state,
        contained_b_result.has_value(),
        "Contained sphere geometry fixture is valid");

    if (!contained_b_result.has_value())
    {
        return finish(state);
    }

    const auto contained_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            contained_b_result.value());

    check(
        state,
        contained_result.has_value() &&
            contained_result.
                value().
                has_value(),
        "Contained spheres produce a contact");

    if (!contained_result.has_value() ||
        !contained_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact contained_contact =
        contained_result.value().value();

    check(
        state,
        contained_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Contained contact preserves center direction");

    check(
        state,
        contained_contact.point() ==
            physics_zero_vector,
        "Contained contact averages opposing surface points");

    check(
        state,
        contained_contact.
            penetration_depth() ==
            4.0,
        "Contained contact calculates combined-radius overlap");

    const auto canonical_first_right_result =
        ColliderGeometry::create(
            collider_a,
            radius_two,
            PhysicsVector3{
                4.0,
                0.0,
                0.0
            });

    const auto canonical_second_left_result =
        ColliderGeometry::create(
            collider_b,
            radius_three,
            physics_zero_vector);

    check(
        state,
        canonical_first_right_result.
            has_value(),
        "Canonical-first-right fixture is valid");

    check(
        state,
        canonical_second_left_result.
            has_value(),
        "Canonical-second-left fixture is valid");

    if (!canonical_first_right_result.
            has_value() ||
        !canonical_second_left_result.
            has_value())
    {
        return finish(state);
    }

    const auto canonical_orientation_result =
        generate_sphere_sphere_contact(
            canonical_second_left_result.value(),
            canonical_first_right_result.value());

    check(
        state,
        canonical_orientation_result.
            has_value() &&
            canonical_orientation_result.
                value().
                has_value(),
        "Canonical orientation query produces a contact");

    if (!canonical_orientation_result.
            has_value() ||
        !canonical_orientation_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact
        canonical_orientation_contact =
            canonical_orientation_result.
                value().
                value();

    check(
        state,
        canonical_orientation_contact.
            normal() ==
            negative_x_result.value(),
        "Normal orientation follows identity order rather than argument order");

    check(
        state,
        canonical_orientation_contact.
            point() ==
            PhysicsVector3{
                2.5,
                0.0,
                0.0
            },
        "Canonical orientation preserves averaged surface point");

    check(
        state,
        canonical_orientation_contact.
            penetration_depth() ==
            1.0,
        "Canonical orientation preserves penetration depth");

    const PhysicsScalar large_scale =
        std::numeric_limits<
            PhysicsScalar>::max();

    const auto large_radius_result =
        SphereShape::create(
            large_scale /
            8.0);

    check(
        state,
        large_radius_result.has_value(),
        "Large-radius sphere fixture is valid");

    if (!large_radius_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape large_radius_sphere{
        large_radius_result.value()
    };

    const PhysicsScalar large_center =
        large_scale *
        0.75;

    const auto large_a_result =
        ColliderGeometry::create(
            collider_a,
            large_radius_sphere,
            PhysicsVector3{
                large_center,
                0.0,
                0.0
            });

    const auto large_b_result =
        ColliderGeometry::create(
            collider_b,
            large_radius_sphere,
            PhysicsVector3{
                -large_center,
                0.0,
                0.0
            });

    check(
        state,
        large_a_result.has_value(),
        "Large positive-center sphere fixture is valid");

    check(
        state,
        large_b_result.has_value(),
        "Large negative-center sphere fixture is valid");

    if (!large_a_result.has_value() ||
        !large_b_result.has_value())
    {
        return finish(state);
    }

    const auto large_separation_result =
        generate_sphere_sphere_contact(
            large_a_result.value(),
            large_b_result.value());

    check(
        state,
        large_separation_result.has_value(),
        "Large opposite-center query avoids intermediate overflow");

    check(
        state,
        large_separation_result.has_value() &&
            !large_separation_result.
                value().
                has_value(),
        "Large opposite-center spheres remain separated");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const auto maximum_sphere_result =
        SphereShape::create(
            maximum);

    check(
        state,
        maximum_sphere_result.has_value(),
        "Maximum-radius sphere fixture is valid");

    if (!maximum_sphere_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_sphere{
        maximum_sphere_result.value()
    };

    const auto maximum_a_result =
        ColliderGeometry::create(
            collider_a,
            maximum_sphere,
            physics_zero_vector);

    const auto maximum_b_result =
        ColliderGeometry::create(
            collider_b,
            maximum_sphere,
            physics_zero_vector);

    check(
        state,
        maximum_a_result.has_value(),
        "Maximum-radius A geometry fixture is valid");

    check(
        state,
        maximum_b_result.has_value(),
        "Maximum-radius B geometry fixture is valid");

    if (!maximum_a_result.has_value() ||
        !maximum_b_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_sphere_contact(
            maximum_a_result.value(),
            maximum_b_result.value()),
        ErrorCode::invalid_argument,
        "Sphere contact rejects overflowing penetration depth");

    const auto c_geometry_result =
        ColliderGeometry::create(
            collider_c,
            radius_two,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            });

    check(
        state,
        c_geometry_result.has_value(),
        "Alternate-owner sphere geometry fixture is valid");

    if (!c_geometry_result.has_value())
    {
        return finish(state);
    }

    const auto pair_ac_result =
        BroadPhasePair::create(
            collider_a,
            collider_c);

    check(
        state,
        pair_ac_result.has_value(),
        "A-C canonical pair fixture is valid");

    const auto alternate_pair_contact_result =
        generate_sphere_sphere_contact(
            origin_a_result.value(),
            c_geometry_result.value());

    check(
        state,
        alternate_pair_contact_result.
            has_value() &&
            alternate_pair_contact_result.
                value().
                has_value(),
        "Alternate persistent pair produces a contact");

    check(
        state,
        pair_ac_result.has_value() &&
            alternate_pair_contact_result.
                has_value() &&
            alternate_pair_contact_result.
                value().
                has_value() &&
            alternate_pair_contact_result.
                value().
                value().
                pair() ==
                pair_ac_result.value(),
        "Alternate contact preserves its canonical persistent pair");

    return finish(state);
}