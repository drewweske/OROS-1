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
            << "\nBox-box narrow-phase test summary: "
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
                generate_box_box_contact(
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

    const auto cube_result =
        BoxShape::create(
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    const auto large_box_result =
        BoxShape::create(
            PhysicsVector3{
                4.0,
                5.0,
                6.0
            });

    const auto sphere_result =
        SphereShape::create(
            1.0);

    check(
        state,
        cube_result.has_value(),
        "Cube fixture is valid");

    check(
        state,
        large_box_result.has_value(),
        "Large box fixture is valid");

    check(
        state,
        sphere_result.has_value(),
        "Sphere fixture is valid");

    if (!cube_result.has_value() ||
        !large_box_result.has_value() ||
        !sphere_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape cube_shape{
        cube_result.value()
    };

    const CollisionShape large_box_shape{
        large_box_result.value()
    };

    const CollisionShape sphere_shape{
        sphere_result.value()
    };

    const auto cube_a_origin_result =
        ColliderGeometry::create(
            collider_a,
            cube_shape,
            physics_zero_vector);

    const auto cube_b_origin_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            physics_zero_vector);

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

    check(
        state,
        cube_a_origin_result.has_value(),
        "Cube A origin fixture is valid");

    check(
        state,
        cube_b_origin_result.has_value(),
        "Cube B origin fixture is valid");

    check(
        state,
        sphere_a_origin_result.has_value(),
        "Sphere A origin fixture is valid");

    check(
        state,
        sphere_b_origin_result.has_value(),
        "Sphere B origin fixture is valid");

    if (!cube_a_origin_result.has_value() ||
        !cube_b_origin_result.has_value() ||
        !sphere_a_origin_result.has_value() ||
        !sphere_b_origin_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_box_box_contact(
            cube_a_origin_result.value(),
            cube_a_origin_result.value()),
        ErrorCode::invalid_argument,
        "Box query rejects identical collider identity");

    check_failure(
        state,
        generate_box_box_contact(
            sphere_a_origin_result.value(),
            cube_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Box query rejects non-box first geometry");

    check_failure(
        state,
        generate_box_box_contact(
            cube_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Box query rejects non-box second geometry");

    check_failure(
        state,
        generate_box_box_contact(
            sphere_a_origin_result.value(),
            sphere_b_origin_result.value()),
        ErrorCode::invalid_argument,
        "Box query rejects two non-box geometries");

    const auto separated_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                2.25,
                0.0,
                0.0
            });

    check(
        state,
        separated_b_result.has_value(),
        "Separated cube fixture is valid");

    if (!separated_b_result.has_value())
    {
        return finish(state);
    }

    const auto separated_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            separated_b_result.value());

    check(
        state,
        separated_result.has_value(),
        "Separated box query succeeds");

    check(
        state,
        separated_result.has_value() &&
            !separated_result.
                value().
                has_value(),
        "Separated boxes produce no contact");

    const auto reversed_separated_result =
        generate_box_box_contact(
            separated_b_result.value(),
            cube_a_origin_result.value());

    check(
        state,
        reversed_separated_result.has_value(),
        "Reversed separated box query succeeds");

    check(
        state,
        reversed_separated_result.has_value() &&
            !reversed_separated_result.
                value().
                has_value(),
        "Reversed separated boxes produce no contact");

    const auto touching_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            });

    check(
        state,
        touching_b_result.has_value(),
        "X-touching cube fixture is valid");

    if (!touching_b_result.has_value())
    {
        return finish(state);
    }

    const auto touching_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            touching_b_result.value());

    check(
        state,
        touching_result.has_value() &&
            touching_result.
                value().
                has_value(),
        "X-touching boxes produce a contact");

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
            PhysicsUnitVector3::positive_x(),
        "Touching contact normal points from first box to second");

    check(
        state,
        touching_contact.point() ==
            PhysicsVector3{
                1.0,
                0.0,
                0.0
            },
        "Touching contact preserves shared face center");

    check(
        state,
        touching_contact.penetration_depth() ==
            0.0,
        "Touching contact has zero penetration depth");

    check(
        state,
        touching_contact.is_touching(),
        "Box boundary contact reports touching");

    const auto reversed_touching_result =
        generate_box_box_contact(
            touching_b_result.value(),
            cube_a_origin_result.value());

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
            cube_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        penetrating_b_result.has_value(),
        "Positive-X penetrating cube fixture is valid");

    if (!penetrating_b_result.has_value())
    {
        return finish(state);
    }

    const auto penetrating_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            penetrating_b_result.value());

    check(
        state,
        penetrating_result.has_value() &&
            penetrating_result.
                value().
                has_value(),
        "Positive-X overlapping boxes produce a contact");

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
            PhysicsUnitVector3::positive_x(),
        "Positive-X overlap preserves normal");

    check(
        state,
        penetrating_contact.point() ==
            PhysicsVector3{
                0.75,
                0.0,
                0.0
            },
        "Positive-X overlap averages opposing surfaces");

    check(
        state,
        penetrating_contact.penetration_depth() ==
            0.5,
        "Positive-X overlap calculates penetration depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Positive-X overlap reports penetration");

    const auto reversed_penetrating_result =
        generate_box_box_contact(
            penetrating_b_result.value(),
            cube_a_origin_result.value());

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
        "Positive-X overlap ignores argument order");

    const auto negative_x_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                -1.5,
                0.0,
                0.0
            });

    check(
        state,
        negative_x_b_result.has_value(),
        "Negative-X penetrating cube fixture is valid");

    if (!negative_x_b_result.has_value())
    {
        return finish(state);
    }

    const auto negative_x_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            negative_x_b_result.value());

    check(
        state,
        negative_x_result.has_value() &&
            negative_x_result.
                value().
                has_value(),
        "Negative-X overlapping boxes produce a contact");

    if (!negative_x_result.has_value() ||
        !negative_x_result.value().has_value())
    {
        return finish(state);
    }

    const auto negative_x_normal_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            });

    check(
        state,
        negative_x_normal_result.has_value(),
        "Negative-X unit normal fixture is valid");

    if (!negative_x_normal_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact negative_x_contact =
        negative_x_result.value().value();

    check(
        state,
        negative_x_contact.normal() ==
            negative_x_normal_result.value(),
        "Negative-X overlap preserves normal");

    check(
        state,
        negative_x_contact.point() ==
            PhysicsVector3{
                -0.75,
                0.0,
                0.0
            },
        "Negative-X overlap preserves contact point");

    check(
        state,
        negative_x_contact.penetration_depth() ==
            0.5,
        "Negative-X overlap preserves depth");

    const auto positive_y_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                0.0,
                1.25,
                0.0
            });

    check(
        state,
        positive_y_b_result.has_value(),
        "Positive-Y penetrating cube fixture is valid");

    if (!positive_y_b_result.has_value())
    {
        return finish(state);
    }

    const auto positive_y_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            positive_y_b_result.value());

    check(
        state,
        positive_y_result.has_value() &&
            positive_y_result.
                value().
                has_value(),
        "Positive-Y overlapping boxes produce a contact");

    if (!positive_y_result.has_value() ||
        !positive_y_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact positive_y_contact =
        positive_y_result.value().value();

    check(
        state,
        positive_y_contact.normal() ==
            PhysicsUnitVector3::positive_y(),
        "Smallest overlap selects positive Y");

    check(
        state,
        positive_y_contact.point() ==
            PhysicsVector3{
                0.0,
                0.625,
                0.0
            },
        "Positive-Y overlap preserves contact point");

    check(
        state,
        positive_y_contact.penetration_depth() ==
            0.75,
        "Positive-Y overlap preserves depth");

    const auto negative_z_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                0.0,
                0.0,
                -1.75
            });

    check(
        state,
        negative_z_b_result.has_value(),
        "Negative-Z penetrating cube fixture is valid");

    if (!negative_z_b_result.has_value())
    {
        return finish(state);
    }

    const auto negative_z_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            negative_z_b_result.value());

    check(
        state,
        negative_z_result.has_value() &&
            negative_z_result.
                value().
                has_value(),
        "Negative-Z overlapping boxes produce a contact");

    if (!negative_z_result.has_value() ||
        !negative_z_result.value().has_value())
    {
        return finish(state);
    }

    const auto negative_z_normal_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                0.0,
                0.0,
                -1.0
            });

    check(
        state,
        negative_z_normal_result.has_value(),
        "Negative-Z unit normal fixture is valid");

    if (!negative_z_normal_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact negative_z_contact =
        negative_z_result.value().value();

    check(
        state,
        negative_z_contact.normal() ==
            negative_z_normal_result.value(),
        "Smallest overlap selects negative Z");

    check(
        state,
        negative_z_contact.point() ==
            PhysicsVector3{
                0.0,
                0.0,
                -0.875
            },
        "Negative-Z overlap preserves contact point");

    check(
        state,
        negative_z_contact.penetration_depth() ==
            0.25,
        "Negative-Z overlap preserves depth");

    const auto edge_touching_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                2.0,
                2.0,
                0.0
            });

    check(
        state,
        edge_touching_b_result.has_value(),
        "Edge-touching cube fixture is valid");

    if (!edge_touching_b_result.has_value())
    {
        return finish(state);
    }

    const auto edge_touching_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            edge_touching_b_result.value());

    check(
        state,
        edge_touching_result.has_value() &&
            edge_touching_result.
                value().
                has_value(),
        "Edge-touching boxes produce a contact");

    if (!edge_touching_result.has_value() ||
        !edge_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact edge_touching_contact =
        edge_touching_result.value().value();

    check(
        state,
        edge_touching_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Equal zero-overlap tie selects X deterministically");

    check(
        state,
        edge_touching_contact.point() ==
            PhysicsVector3{
                1.0,
                1.0,
                0.0
            },
        "Edge-touching contact preserves shared edge center");

    check(
        state,
        edge_touching_contact.
            penetration_depth() ==
            0.0,
        "Edge-touching contact has zero depth");

    const auto corner_touching_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            });

    check(
        state,
        corner_touching_b_result.has_value(),
        "Corner-touching cube fixture is valid");

    if (!corner_touching_b_result.has_value())
    {
        return finish(state);
    }

    const auto corner_touching_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            corner_touching_b_result.value());

    check(
        state,
        corner_touching_result.has_value() &&
            corner_touching_result.
                value().
                has_value(),
        "Corner-touching boxes produce a contact");

    if (!corner_touching_result.has_value() ||
        !corner_touching_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact corner_touching_contact =
        corner_touching_result.value().value();

    check(
        state,
        corner_touching_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Corner tie selects X deterministically");

    check(
        state,
        corner_touching_contact.point() ==
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            },
        "Corner contact preserves shared corner");

    check(
        state,
        corner_touching_contact.
            penetration_depth() ==
            0.0,
        "Corner contact has zero depth");

    const auto coincident_result =
        generate_box_box_contact(
            cube_a_origin_result.value(),
            cube_b_origin_result.value());

    check(
        state,
        coincident_result.has_value() &&
            coincident_result.
                value().
                has_value(),
        "Coincident boxes produce a deterministic contact");

    if (!coincident_result.has_value() ||
        !coincident_result.value().has_value())
    {
        return finish(state);
    }

    const CollisionContact coincident_contact =
        coincident_result.value().value();

    check(
        state,
        coincident_contact.normal() ==
            PhysicsUnitVector3::positive_x(),
        "Coincident equal boxes select positive X");

    check(
        state,
        coincident_contact.point() ==
            physics_zero_vector,
        "Coincident box contact preserves center point");

    check(
        state,
        coincident_contact.penetration_depth() ==
            2.0,
        "Coincident cube penetration equals full width");

    check(
        state,
        !coincident_contact.is_touching(),
        "Coincident boxes report penetration");

    const auto reversed_coincident_result =
        generate_box_box_contact(
            cube_b_origin_result.value(),
            cube_a_origin_result.value());

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
        "Coincident result ignores argument order");

    const auto large_a_result =
        ColliderGeometry::create(
            collider_a,
            large_box_shape,
            physics_zero_vector);

    const auto contained_b_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            PhysicsVector3{
                3.5,
                0.0,
                0.0
            });

    check(
        state,
        large_a_result.has_value(),
        "Large containing box fixture is valid");

    check(
        state,
        contained_b_result.has_value(),
        "Contained cube fixture is valid");

    if (!large_a_result.has_value() ||
        !contained_b_result.has_value())
    {
        return finish(state);
    }

    const auto contained_result =
        generate_box_box_contact(
            large_a_result.value(),
            contained_b_result.value());

    check(
        state,
        contained_result.has_value() &&
            contained_result.
                value().
                has_value(),
        "Contained box produces a contact");

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
        "Contained box selects nearest exit direction");

    check(
        state,
        contained_contact.point() ==
            PhysicsVector3{
                3.25,
                0.0,
                0.0
            },
        "Contained box preserves opposing surface midpoint");

    check(
        state,
        contained_contact.penetration_depth() ==
            1.5,
        "Contained box calculates exit distance");

    const auto canonical_first_right_result =
        ColliderGeometry::create(
            collider_a,
            cube_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    const auto canonical_second_left_result =
        ColliderGeometry::create(
            collider_b,
            cube_shape,
            physics_zero_vector);

    check(
        state,
        canonical_first_right_result.has_value(),
        "Canonical-first-right box fixture is valid");

    check(
        state,
        canonical_second_left_result.has_value(),
        "Canonical-second-left box fixture is valid");

    if (!canonical_first_right_result.has_value() ||
        !canonical_second_left_result.has_value())
    {
        return finish(state);
    }

    const auto canonical_orientation_result =
        generate_box_box_contact(
            canonical_second_left_result.value(),
            canonical_first_right_result.value());

    check(
        state,
        canonical_orientation_result.has_value() &&
            canonical_orientation_result.
                value().
                has_value(),
        "Canonical orientation query produces a contact");

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
            negative_x_normal_result.value(),
        "Box normal follows identity order rather than argument order");

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
        canonical_contact.penetration_depth() ==
            0.5,
        "Canonical orientation preserves depth");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const auto huge_box_result =
        BoxShape::create(
            PhysicsVector3{
                maximum / 8.0,
                maximum / 8.0,
                maximum / 8.0
            });

    check(
        state,
        huge_box_result.has_value(),
        "Huge box fixture is valid");

    if (!huge_box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape huge_box_shape{
        huge_box_result.value()
    };

    const PhysicsScalar large_center =
        maximum *
        0.75;

    const auto huge_positive_result =
        ColliderGeometry::create(
            collider_a,
            huge_box_shape,
            PhysicsVector3{
                large_center,
                0.0,
                0.0
            });

    const auto huge_negative_result =
        ColliderGeometry::create(
            collider_b,
            huge_box_shape,
            PhysicsVector3{
                -large_center,
                0.0,
                0.0
            });

    check(
        state,
        huge_positive_result.has_value(),
        "Huge positive-center box fixture is valid");

    check(
        state,
        huge_negative_result.has_value(),
        "Huge negative-center box fixture is valid");

    if (!huge_positive_result.has_value() ||
        !huge_negative_result.has_value())
    {
        return finish(state);
    }

    const auto huge_separation_result =
        generate_box_box_contact(
            huge_positive_result.value(),
            huge_negative_result.value());

    check(
        state,
        huge_separation_result.has_value(),
        "Huge opposite-center query avoids intermediate overflow");

    check(
        state,
        huge_separation_result.has_value() &&
            !huge_separation_result.
                value().
                has_value(),
        "Huge opposite-center boxes remain separated");

    const auto maximum_box_result =
        BoxShape::create(
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            });

    check(
        state,
        maximum_box_result.has_value(),
        "Maximum-size box fixture is valid");

    if (!maximum_box_result.has_value())
    {
        return finish(state);
    }

    const CollisionShape maximum_box_shape{
        maximum_box_result.value()
    };

    const auto maximum_a_result =
        ColliderGeometry::create(
            collider_a,
            maximum_box_shape,
            physics_zero_vector);

    const auto maximum_b_result =
        ColliderGeometry::create(
            collider_b,
            maximum_box_shape,
            physics_zero_vector);

    check(
        state,
        maximum_a_result.has_value(),
        "Maximum-size A geometry fixture is valid");

    check(
        state,
        maximum_b_result.has_value(),
        "Maximum-size B geometry fixture is valid");

    if (!maximum_a_result.has_value() ||
        !maximum_b_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        generate_box_box_contact(
            maximum_a_result.value(),
            maximum_b_result.value()),
        ErrorCode::invalid_argument,
        "Box query rejects overflowing penetration depth");

    const auto alternate_c_result =
        ColliderGeometry::create(
            collider_c,
            cube_shape,
            PhysicsVector3{
                1.5,
                0.0,
                0.0
            });

    check(
        state,
        alternate_c_result.has_value(),
        "Alternate-owner box fixture is valid");

    if (!alternate_c_result.has_value())
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
        generate_box_box_contact(
            cube_a_origin_result.value(),
            alternate_c_result.value());

    check(
        state,
        alternate_contact_result.has_value() &&
            alternate_contact_result.
                value().
                has_value(),
        "Alternate persistent box pair produces a contact");

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
        "Alternate box contact preserves persistent pair");

    return finish(state);
}