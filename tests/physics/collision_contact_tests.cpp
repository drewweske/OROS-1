#include "oros/physics/collision_contact.hpp"

#include "oros/foundation/error.hpp"

#include <iostream>
#include <limits>
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
            << "\nCollision contact test summary: "
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
        !std::is_default_constructible_v<
            CollisionContact>);

    static_assert(
        std::is_copy_constructible_v<
            CollisionContact>);

    static_assert(
        std::is_move_constructible_v<
            CollisionContact>);

    static_assert(
        std::is_copy_assignable_v<
            CollisionContact>);

    static_assert(
        std::is_move_assignable_v<
            CollisionContact>);

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
            1U
        },
        3U
    };

    static_assert(
        collider_a.is_valid());

    static_assert(
        collider_b.is_valid());

    static_assert(
        collider_c.is_valid());

    const auto pair_ab_result =
        BroadPhasePair::create(
            collider_a,
            collider_b);

    check(
        state,
        pair_ab_result.has_value(),
        "A-B contact pair fixture is valid");

    if (!pair_ab_result.has_value())
    {
        return finish(state);
    }

    const BroadPhasePair pair_ab =
        pair_ab_result.value();

    const PhysicsUnitVector3 positive_y =
        PhysicsUnitVector3::positive_y();

    check(
        state,
        positive_y.is_valid(),
        "Contact normal fixture is valid");

    const PhysicsVector3 contact_point{
        10.0,
        20.0,
        30.0
    };

    check_failure(
        state,
        CollisionContact::create(
            pair_ab,
            PhysicsVector3{
                std::numeric_limits<
                    PhysicsScalar>::infinity(),
                0.0,
                0.0
            },
            positive_y,
            0.0),
        ErrorCode::invalid_argument,
        "Contact rejects infinite point");

    check_failure(
        state,
        CollisionContact::create(
            pair_ab,
            PhysicsVector3{
                0.0,
                std::numeric_limits<
                    PhysicsScalar>::quiet_NaN(),
                0.0
            },
            positive_y,
            0.0),
        ErrorCode::invalid_argument,
        "Contact rejects NaN point");

    check_failure(
        state,
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            -0.01),
        ErrorCode::invalid_argument,
        "Contact rejects negative penetration depth");

    check_failure(
        state,
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            std::numeric_limits<
                PhysicsScalar>::infinity()),
        ErrorCode::invalid_argument,
        "Contact rejects infinite penetration depth");

    check_failure(
        state,
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN()),
        ErrorCode::invalid_argument,
        "Contact rejects NaN penetration depth");

    const auto touching_contact_result =
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            0.0);

    check(
        state,
        touching_contact_result.has_value(),
        "Contact accepts zero penetration depth");

    if (!touching_contact_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact touching_contact =
        touching_contact_result.value();

    check(
        state,
        touching_contact.pair() ==
            pair_ab,
        "Contact preserves canonical collider pair");

    check(
        state,
        touching_contact.point() ==
            contact_point,
        "Contact preserves world-space point");

    check(
        state,
        touching_contact.normal() ==
            positive_y,
        "Contact preserves unit normal");

    check(
        state,
        touching_contact.normal().vector() ==
            physics_positive_y,
        "Contact normal preserves direction");

    check(
        state,
        touching_contact.penetration_depth() ==
            0.0,
        "Touching contact preserves zero depth");

    check(
        state,
        touching_contact.is_touching(),
        "Zero-depth contact reports touching");

    const auto equivalent_contact_result =
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            0.0);

    check(
        state,
        equivalent_contact_result.has_value(),
        "Equivalent contact fixture is valid");

    check(
        state,
        equivalent_contact_result.has_value() &&
            equivalent_contact_result.value() ==
                touching_contact,
        "Equivalent contact values compare equal");

    const auto penetrating_contact_result =
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_y,
            0.75);

    check(
        state,
        penetrating_contact_result.has_value(),
        "Contact accepts positive penetration depth");

    if (!penetrating_contact_result.has_value())
    {
        return finish(state);
    }

    const CollisionContact penetrating_contact =
        penetrating_contact_result.value();

    check(
        state,
        penetrating_contact.pair() ==
            pair_ab,
        "Penetrating contact preserves collider pair");

    check(
        state,
        penetrating_contact.point() ==
            contact_point,
        "Penetrating contact preserves point");

    check(
        state,
        penetrating_contact.normal() ==
            positive_y,
        "Penetrating contact preserves normal");

    check(
        state,
        penetrating_contact.penetration_depth() ==
            0.75,
        "Penetrating contact preserves depth");

    check(
        state,
        !penetrating_contact.is_touching(),
        "Positive-depth contact reports penetration");

    check(
        state,
        penetrating_contact !=
            touching_contact,
        "Different penetration depths produce different contacts");

    const PhysicsVector3 alternate_point{
        -1.0,
        -2.0,
        -3.0
    };

    const auto alternate_point_contact_result =
        CollisionContact::create(
            pair_ab,
            alternate_point,
            positive_y,
            0.75);

    check(
        state,
        alternate_point_contact_result.has_value(),
        "Alternate contact point fixture is valid");

    check(
        state,
        alternate_point_contact_result.has_value() &&
            alternate_point_contact_result.value() !=
                penetrating_contact,
        "Different points produce different contacts");

    const PhysicsUnitVector3 positive_x =
        PhysicsUnitVector3::positive_x();

    const auto alternate_normal_contact_result =
        CollisionContact::create(
            pair_ab,
            contact_point,
            positive_x,
            0.75);

    check(
        state,
        alternate_normal_contact_result.has_value(),
        "Alternate contact normal fixture is valid");

    check(
        state,
        alternate_normal_contact_result.has_value() &&
            alternate_normal_contact_result.value() !=
                penetrating_contact,
        "Different normals produce different contacts");

    const auto pair_ac_result =
        BroadPhasePair::create(
            collider_a,
            collider_c);

    check(
        state,
        pair_ac_result.has_value(),
        "A-C contact pair fixture is valid");

    if (!pair_ac_result.has_value())
    {
        return finish(state);
    }

    const auto alternate_pair_contact_result =
        CollisionContact::create(
            pair_ac_result.value(),
            contact_point,
            positive_y,
            0.75);

    check(
        state,
        alternate_pair_contact_result.has_value(),
        "Alternate collider-pair contact fixture is valid");

    check(
        state,
        alternate_pair_contact_result.has_value() &&
            alternate_pair_contact_result.value() !=
                penetrating_contact,
        "Different collider pairs produce different contacts");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const auto maximum_contact_result =
        CollisionContact::create(
            pair_ab,
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            },
            positive_y,
            maximum);

    check(
        state,
        maximum_contact_result.has_value(),
        "Contact accepts maximum finite geometry values");

    check(
        state,
        maximum_contact_result.has_value() &&
            maximum_contact_result.
                value().
                point() ==
                PhysicsVector3{
                    maximum,
                    maximum,
                    maximum
                },
        "Contact preserves maximum finite point");

    check(
        state,
        maximum_contact_result.has_value() &&
            maximum_contact_result.
                value().
                penetration_depth() ==
                maximum,
        "Contact preserves maximum finite depth");

    CollisionContact copied_contact =
        penetrating_contact;

    check(
        state,
        copied_contact ==
            penetrating_contact,
        "Contact copy preserves all values");

    CollisionContact moved_contact =
        std::move(
            copied_contact);

    check(
        state,
        moved_contact ==
            penetrating_contact,
        "Contact move preserves all values");

    CollisionContact assigned_contact =
        touching_contact;

    assigned_contact =
        penetrating_contact;

    check(
        state,
        assigned_contact ==
            penetrating_contact,
        "Contact copy assignment preserves all values");

    CollisionContact move_assigned_contact =
        touching_contact;

    move_assigned_contact =
        std::move(
            assigned_contact);

    check(
        state,
        move_assigned_contact ==
            penetrating_contact,
        "Contact move assignment preserves all values");

    return finish(state);
}