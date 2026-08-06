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
            << "\nSphere-capsule narrow-phase test summary: "
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
                generate_sphere_capsule_contact(
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

    const auto box_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

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
        box_result.has_value(),
        "Box fixture is valid");

    if (!sphere_result.has_value() ||
        !capsule_result.has_value() ||
        !zero_segment_capsule_result.
            has_value() ||
        !box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const CollisionShape capsule_shape{
        capsule_result.value()
    };

    const CollisionShape
        zero_segment_capsule_shape{
            zero_segment_capsule_result.value()
        };

    const CollisionShape box_shape{
        box_result.value()
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
        capsule_a_origin_result.has_value(),
        "Capsule A origin fixture is valid");

    check(
        state,
        capsule_b_origin_result.has_value(),
        "Capsule B origin fixture is valid");

    check(
        state,
        box_b_origin_result.has_value(),
        "Box B origin fixture is valid");

    if (!sphere_a_origin_result.has_value() ||
        !sphere_b_origin_result.has_value() ||
        !capsule_a_origin_result.has_value() ||
        !capsule_b_origin_result.has_value() ||
        !box_b_origin_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_capsule_contact(
            sphere_a_origin_result.value(),
            capsule_a_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-capsule query rejects identical collider identity");

    check_failure(
        state,
        generate_sphere_capsule_contact(
            sphere_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-capsule query rejects two spheres");

    check_failure(
        state,
        generate_sphere_capsule_contact(
            capsule_a_origin_result.value(),
            capsule_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-capsule query rejects two capsules");

    check_failure(
        state,
        generate_sphere_capsule_contact(
            sphere_a_origin_result.value(),
            box_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-capsule query rejects a box");

    const auto separated_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                2.25,
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

    const auto separated_result =
        generate_sphere_capsule_contact(
            separated_sphere_result.value(),
            capsule_b_origin_result.value());

    check(
        state,
        separated_result.has_value(),
        "Separated sphere-capsule query succeeds");

    check(
        state,
        separated_result.has_value() &&
            !separated_result.
                value().
                has_value(),
        "Separated sphere and capsule produce no contact");

    const auto reversed_separated_result =
        generate_sphere_capsule_contact(
            capsule_b_origin_result.value(),
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
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_sphere_result.has_value(),
        "Side-touching sphere fixture is valid");

    if (!touching_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_sphere_capsule_contact(
            touching_sphere_result.value(),
            capsule_b_origin_result.value());

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "Side-touching sphere and capsule produce a contact");

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
            pair_ab_result.value(),
        "Touching contact preserves canonical pair");

    check(
        state,
        touching_contact.normal() ==
            negative_x_result.value(),
        "Touching normal points from sphere to capsule");

    check(
        state,
        touching_contact.point() ==
            PhysicsVector3{
                1.0,
                0.0,
                0.0
            },
        "Side-touching contact preserves shared surface point");

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
        generate_sphere_capsule_contact(
            capsule_b_origin_result.value(),
            touching_sphere_result.value());

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

    const auto penetrating_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_sphere_result.has_value(),
        "Side-penetrating sphere fixture is valid");

    if (!penetrating_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_sphere_capsule_contact(
            penetrating_sphere_result.value(),
            capsule_b_origin_result.value());

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
            negative_x_result.value(),
        "Side penetration preserves normal");

    check(
        state,
        penetrating_contact.point() ==
            PhysicsVector3{
                0.75,
                0.0,
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
        generate_sphere_capsule_contact(
            capsule_b_origin_result.value(),
            penetrating_sphere_result.value());

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

    const auto end_touching_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                0.0,
                4.0,
                0.0
            });

    check(
        state,
        end_touching_sphere_result.has_value(),
        "End-cap touching sphere fixture is valid");

    if (!end_touching_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto end_touching_result =
        generate_sphere_capsule_contact(
            end_touching_sphere_result.value(),
            capsule_b_origin_result.value());

    check(
        state,
        end_touching_result.has_value() &&
            end_touching_result.
                value().
                has_value(),
        "End-cap touching produces a contact");

    if (!end_touching_result.has_value() ||
        !end_touching_result.value().has_value())
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

    const CollisionContact end_touching_contact =
        end_touching_result.value().value();

    check(
        state,
        end_touching_contact.normal() ==
            negative_y_result.value(),
        "End-cap touching normal points toward capsule");

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

    const auto end_penetrating_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                0.0,
                3.5,
                0.0
            });

    check(
        state,
        end_penetrating_sphere_result.has_value(),
        "End-cap penetrating sphere fixture is valid");

    if (!end_penetrating_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto end_penetrating_result =
        generate_sphere_capsule_contact(
            end_penetrating_sphere_result.value(),
            capsule_b_origin_result.value());

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
        end_penetrating_result.
            value().
            value();

    check(
        state,
        end_penetrating_contact.normal() ==
            negative_y_result.value(),
        "End-cap penetration preserves normal");

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

    const auto center_line_sphere_result =
        ColliderGeometry::create(
            collider_a,
            sphere_shape,
            PhysicsVector3{
                0.0,
                1.0,
                0.0
            });

    check(
        state,
        center_line_sphere_result.has_value(),
        "Center-line sphere fixture is valid");

    if (!center_line_sphere_result.has_value())
    {
        return finish(state);
    }

    const auto center_line_result =
        generate_sphere_capsule_contact(
            center_line_sphere_result.value(),
            capsule_b_origin_result.value());

    check(
        state,
        center_line_result.has_value() &&
            center_line_result.
                value().
                has_value(),
        "Sphere on capsule center line produces a contact");

    if (!center_line_result.has_value() ||
        !center_line_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact center_line_contact =
        center_line_result.value().value();

    check(
        state,
        center_line_contact.normal() ==
            negative_x_result.value(),
        "Center-line overlap uses deterministic perpendicular normal");

    check(
        state,
        center_line_contact.point() ==
            PhysicsVector3{
                0.0,
                1.0,
                0.0
            },
        "Center-line contact preserves projected point");

    check(
        state,
        center_line_contact.
            penetration_depth() ==
            2.0,
        "Center-line overlap uses combined radii");

    const auto zero_capsule_b_result =
        ColliderGeometry::create(
            collider_b,
            zero_segment_capsule_shape,
            physics_zero_vector);

    check(
        state,
        zero_capsule_b_result.has_value(),
        "Zero-segment capsule geometry is valid");

    if (!zero_capsule_b_result.has_value())
    {
        return finish(state);
    }

    const auto coincident_zero_result =
        generate_sphere_capsule_contact(
            sphere_a_origin_result.value(),
            zero_capsule_b_result.value());

    check(
        state,
        coincident_zero_result.has_value() &&
            coincident_zero_result.
                value().
                has_value(),
        "Coincident zero-segment capsule produces a contact");

    if (!coincident_zero_result.has_value() ||
        !coincident_zero_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact coincident_zero_contact =
        coincident_zero_result.
            value().
            value();

    check(
        state,
        coincident_zero_contact.normal() ==
            negative_x_result.value(),
        "Zero-segment fallback selects deterministic X");

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
        "Zero-segment capsule behaves like a sphere");

    const auto sphere_b_right_result =
        ColliderGeometry::create(
            collider_b,
            sphere_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        sphere_b_right_result.has_value(),
        "Canonical second sphere fixture is valid");

    if (!sphere_b_right_result.has_value())
    {
        return finish(state);
    }

    const auto canonical_orientation_result =
        generate_sphere_capsule_contact(
            sphere_b_right_result.value(),
            capsule_a_origin_result.value());

    check(
        state,
        canonical_orientation_result.has_value() &&
            canonical_orientation_result.
                value().
                has_value(),
        "Canonical capsule-first query produces a contact");

    if (!canonical_orientation_result.has_value() ||
        !canonical_orientation_result.
            value().
            has_value())
    {
        return finish(state);
    }

    const CollisionContact canonical_contact =
        canonical_orientation_result.
            value().
            value();

    check(
        state,
        canonical_contact.normal() ==
            PhysicsUnitVector3::
                positive_x(),
        "Canonical normal follows persistent identity order");

    check(
        state,
        canonical_contact.point() ==
            PhysicsVector3{
                0.75,
                0.0,
                0.0
            },
        "Canonical orientation preserves contact point");

    check(
        state,
        canonical_contact.
            penetration_depth() ==
            0.5,
        "Canonical orientation preserves depth");

    const auto reversed_canonical_result =
        generate_sphere_capsule_contact(
            capsule_a_origin_result.value(),
            sphere_b_right_result.value());

    check(
        state,
        reversed_canonical_result.has_value() &&
            reversed_canonical_result.
                value().
                has_value() &&
            reversed_canonical_result.
                value().
                value() ==
                canonical_contact,
        "Canonical result ignores argument order");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const PhysicsScalar large_dimension =
        maximum /
        8.0;

    const auto huge_sphere_shape_result =
        SphereShape::create(
            large_dimension);

    const auto huge_capsule_shape_result =
        CapsuleShape::create(
            large_dimension,
            large_dimension,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        huge_sphere_shape_result.has_value(),
        "Huge sphere shape fixture is valid");

    check(
        state,
        huge_capsule_shape_result.has_value(),
        "Huge capsule shape fixture is valid");

    if (!huge_sphere_shape_result.has_value() ||
        !huge_capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape huge_sphere_shape{
        huge_sphere_shape_result.value()
    };

    const CollisionShape huge_capsule_shape{
        huge_capsule_shape_result.value()
    };

    const PhysicsScalar large_center =
        maximum *
        0.75;

    const auto huge_sphere_geometry_result =
        ColliderGeometry::create(
            collider_a,
            huge_sphere_shape,
            PhysicsVector3{
                -large_center,
                0.0,
                0.0
            });

    const auto huge_capsule_geometry_result =
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
        huge_sphere_geometry_result.has_value(),
        "Huge sphere geometry fixture is valid");

    check(
        state,
        huge_capsule_geometry_result.has_value(),
        "Huge capsule geometry fixture is valid");

    if (!huge_sphere_geometry_result.has_value() ||
        !huge_capsule_geometry_result.has_value())
    {
        return finish(state);
    }

    const auto huge_separation_result =
        generate_sphere_capsule_contact(
            huge_sphere_geometry_result.value(),
            huge_capsule_geometry_result.value());

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
        "Huge-coordinate shapes remain separated");

    const PhysicsScalar overflow_radius =
        maximum *
        0.75;

    const auto overflow_sphere_shape_result =
        SphereShape::create(
            overflow_radius);

    const auto overflow_capsule_shape_result =
        CapsuleShape::create(
            overflow_radius,
            0.0,
            PhysicsUnitVector3::
                positive_y());

    check(
        state,
        overflow_sphere_shape_result.has_value(),
        "Overflow sphere shape fixture is valid");

    check(
        state,
        overflow_capsule_shape_result.has_value(),
        "Overflow capsule shape fixture is valid");

    if (!overflow_sphere_shape_result.has_value() ||
        !overflow_capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape overflow_sphere_shape{
        overflow_sphere_shape_result.value()
    };

    const CollisionShape overflow_capsule_shape{
        overflow_capsule_shape_result.value()
    };

    const auto overflow_sphere_geometry_result =
        ColliderGeometry::create(
            collider_a,
            overflow_sphere_shape,
            physics_zero_vector);

    const auto overflow_capsule_geometry_result =
        ColliderGeometry::create(
            collider_b,
            overflow_capsule_shape,
            physics_zero_vector);

    check(
        state,
        overflow_sphere_geometry_result.has_value(),
        "Overflow sphere geometry fixture is valid");

    check(
        state,
        overflow_capsule_geometry_result.has_value(),
        "Overflow capsule geometry fixture is valid");

    if (!overflow_sphere_geometry_result.has_value() ||
        !overflow_capsule_geometry_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_sphere_capsule_contact(
            overflow_sphere_geometry_result.value(),
            overflow_capsule_geometry_result.value()),
        ErrorCode::invalid_argument,
        "Sphere-capsule query rejects overflowing penetration depth");

    const auto capsule_c_origin_result =
        ColliderGeometry::create(
            collider_c,
            capsule_shape,
            physics_zero_vector);

    check(
        state,
        capsule_c_origin_result.has_value(),
        "Alternate-owner capsule fixture is valid");

    if (!capsule_c_origin_result.has_value())
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

    const auto alternate_contact_result =
        generate_sphere_capsule_contact(
            touching_sphere_result.value(),
            capsule_c_origin_result.value());

    check(
        state,
        alternate_contact_result.has_value() &&
            alternate_contact_result.
                value().
                has_value(),
        "Alternate persistent pair produces a contact");

    check(
        state,
        pair_ac_result.has_value() &&
            alternate_contact_result.
                has_value() &&
            alternate_contact_result.
                value().
                has_value() &&
            alternate_contact_result.
                value().
                value().
                pair() ==
                pair_ac_result.value(),
        "Alternate contact preserves persistent pair");

    return finish(state);
}