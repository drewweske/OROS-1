#include "oros/physics/broad_phase_pair.hpp"

#include "oros/foundation/error.hpp"

#include <cstdint>
#include <iostream>
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
            << "\nBroad-phase pair test summary: "
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
            BroadPhasePair>);

    static_assert(
        std::is_copy_constructible_v<
            BroadPhasePair>);

    static_assert(
        std::is_move_constructible_v<
            BroadPhasePair>);

    static_assert(
        std::is_copy_assignable_v<
            BroadPhasePair>);

    static_assert(
        std::is_move_assignable_v<
            BroadPhasePair>);

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

    constexpr ColliderId collider_d{
        EntityId{
            2U,
            1U
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
        collider_d.is_valid());

    check_failure(
        state,
        BroadPhasePair::create(
            invalid_collider_id,
            collider_a),
        ErrorCode::invalid_argument,
        "Pair rejects invalid first collider");

    check_failure(
        state,
        BroadPhasePair::create(
            collider_a,
            invalid_collider_id),
        ErrorCode::invalid_argument,
        "Pair rejects invalid second collider");

    check_failure(
        state,
        BroadPhasePair::create(
            invalid_collider_id,
            invalid_collider_id),
        ErrorCode::invalid_argument,
        "Pair rejects two invalid colliders");

    const ColliderId invalid_owner{
        EntityId{},
        1U
    };

    check_failure(
        state,
        BroadPhasePair::create(
            invalid_owner,
            collider_a),
        ErrorCode::invalid_argument,
        "Pair rejects collider with invalid owner");

    const ColliderId invalid_shape_slot{
        EntityId{
            1U,
            1U
        },
        0U
    };

    check_failure(
        state,
        BroadPhasePair::create(
            collider_a,
            invalid_shape_slot),
        ErrorCode::invalid_argument,
        "Pair rejects collider with shape slot zero");

    check_failure(
        state,
        BroadPhasePair::create(
            collider_a,
            collider_a),
        ErrorCode::invalid_argument,
        "Pair rejects identical collider identities");

    const auto forward_pair_result =
        BroadPhasePair::create(
            collider_a,
            collider_b);

    check(
        state,
        forward_pair_result.has_value(),
        "Pair accepts two distinct valid colliders");

    if (!forward_pair_result.has_value())
    {
        return finish(state);
    }

    const BroadPhasePair forward_pair =
        forward_pair_result.value();

    check(
        state,
        forward_pair.first_collider() ==
            collider_a,
        "Forward pair preserves lower collider first");

    check(
        state,
        forward_pair.second_collider() ==
            collider_b,
        "Forward pair preserves higher collider second");

    check(
        state,
        forward_pair.first_collider() <
            forward_pair.second_collider(),
        "Pair stores colliders in ascending identity order");

    check(
        state,
        forward_pair.contains(
            collider_a),
        "Pair contains first collider");

    check(
        state,
        forward_pair.contains(
            collider_b),
        "Pair contains second collider");

    check(
        state,
        !forward_pair.contains(
            collider_c),
        "Pair rejects unrelated collider membership");

    check(
        state,
        !forward_pair.contains(
            invalid_collider_id),
        "Pair does not contain invalid collider identity");

    const auto reverse_pair_result =
        BroadPhasePair::create(
            collider_b,
            collider_a);

    check(
        state,
        reverse_pair_result.has_value(),
        "Pair accepts reversed collider input");

    if (!reverse_pair_result.has_value())
    {
        return finish(state);
    }

    const BroadPhasePair reverse_pair =
        reverse_pair_result.value();

    check(
        state,
        reverse_pair.first_collider() ==
            collider_a,
        "Reversed input canonicalizes lower collider first");

    check(
        state,
        reverse_pair.second_collider() ==
            collider_b,
        "Reversed input canonicalizes higher collider second");

    check(
        state,
        reverse_pair ==
            forward_pair,
        "Forward and reversed inputs produce equal pair values");

    check(
        state,
        !(forward_pair <
            reverse_pair),
        "Equivalent pair is not ordered before itself");

    check(
        state,
        !(reverse_pair <
            forward_pair),
        "Equivalent canonical pair ordering is symmetric");

    const auto pair_ac_result =
        BroadPhasePair::create(
            collider_a,
            collider_c);

    const auto pair_bc_result =
        BroadPhasePair::create(
            collider_b,
            collider_c);

    const auto pair_ad_result =
        BroadPhasePair::create(
            collider_a,
            collider_d);

    check(
        state,
        pair_ac_result.has_value(),
        "A-C pair fixture is valid");

    check(
        state,
        pair_bc_result.has_value(),
        "B-C pair fixture is valid");

    check(
        state,
        pair_ad_result.has_value(),
        "A-D pair fixture is valid");

    if (!pair_ac_result.has_value() ||
        !pair_bc_result.has_value() ||
        !pair_ad_result.has_value())
    {
        return finish(state);
    }

    const BroadPhasePair pair_ac =
        pair_ac_result.value();

    const BroadPhasePair pair_bc =
        pair_bc_result.value();

    const BroadPhasePair pair_ad =
        pair_ad_result.value();

    check(
        state,
        forward_pair <
            pair_ac,
        "Pair ordering compares second collider after equal first");

    check(
        state,
        pair_ac <
            pair_bc,
        "Pair ordering compares first collider before second");

    check(
        state,
        pair_ac <
            pair_ad,
        "Pair ordering follows persistent collider identity order");

    check(
        state,
        !(pair_bc <
            pair_ac),
        "Pair ordering rejects reverse lexicographic relation");

    check(
        state,
        pair_ac !=
            pair_bc,
        "Different collider combinations produce different pairs");

    constexpr ColliderId maximum_collider{
        EntityId{
            UINT64_MAX,
            UINT64_MAX
        },
        UINT32_MAX
    };

    static_assert(
        maximum_collider.is_valid());

    const auto maximum_pair_result =
        BroadPhasePair::create(
            collider_a,
            maximum_collider);

    check(
        state,
        maximum_pair_result.has_value(),
        "Pair accepts maximum persistent collider identity");

    check(
        state,
        maximum_pair_result.has_value() &&
            maximum_pair_result.
                value().
                first_collider() ==
                collider_a,
        "Maximum identity pair preserves lower collider");

    check(
        state,
        maximum_pair_result.has_value() &&
            maximum_pair_result.
                value().
                second_collider() ==
                maximum_collider,
        "Maximum identity pair preserves maximum collider");

    BroadPhasePair copied_pair =
        forward_pair;

    check(
        state,
        copied_pair ==
            forward_pair,
        "Pair copy preserves canonical identities");

    BroadPhasePair moved_pair =
        std::move(
            copied_pair);

    check(
        state,
        moved_pair ==
            forward_pair,
        "Pair move preserves canonical identities");

    BroadPhasePair assigned_pair =
        pair_ac;

    assigned_pair =
        forward_pair;

    check(
        state,
        assigned_pair ==
            forward_pair,
        "Pair copy assignment preserves canonical identities");

    BroadPhasePair move_assigned_pair =
        pair_ac;

    move_assigned_pair =
        std::move(
            assigned_pair);

    check(
        state,
        move_assigned_pair ==
            forward_pair,
        "Pair move assignment preserves canonical identities");

    return finish(state);
}