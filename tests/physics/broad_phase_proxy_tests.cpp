#include "oros/physics/broad_phase_proxy.hpp"

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
            << "\nBroad-phase proxy test summary: "
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
            BroadPhaseProxy>);

    static_assert(
        std::is_copy_constructible_v<
            BroadPhaseProxy>);

    static_assert(
        std::is_move_constructible_v<
            BroadPhaseProxy>);

    static_assert(
        std::is_copy_assignable_v<
            BroadPhaseProxy>);

    static_assert(
        std::is_move_assignable_v<
            BroadPhaseProxy>);

    TestState state{};

    const auto initial_bounds_result =
        AxisAlignedBounds::
            from_center_and_half_extents(
                PhysicsVector3{
                    10.0,
                    20.0,
                    30.0
                },
                PhysicsVector3{
                    1.0,
                    2.0,
                    3.0
                });

    check(
        state,
        initial_bounds_result.has_value(),
        "Initial bounds fixture is valid");

    if (!initial_bounds_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds initial_bounds =
        initial_bounds_result.value();

    check_failure(
        state,
        BroadPhaseProxy::create(
            invalid_collider_id,
            initial_bounds),
        ErrorCode::invalid_argument,
        "Proxy rejects default collider identity");

    const ColliderId invalid_owner{
        EntityId{},
        1U
    };

    check_failure(
        state,
        BroadPhaseProxy::create(
            invalid_owner,
            initial_bounds),
        ErrorCode::invalid_argument,
        "Proxy rejects collider with invalid owner");

    const ColliderId invalid_shape_slot{
        EntityId{
            1U,
            1U
        },
        0U
    };

    check_failure(
        state,
        BroadPhaseProxy::create(
            invalid_shape_slot,
            initial_bounds),
        ErrorCode::invalid_argument,
        "Proxy rejects collider with shape slot zero");

    constexpr ColliderId first_collider{
        EntityId{
            0x1111111111111111ULL,
            0x2222222222222222ULL
        },
        1U
    };

    constexpr ColliderId second_collider{
        EntityId{
            0x1111111111111111ULL,
            0x2222222222222222ULL
        },
        2U
    };

    static_assert(
        first_collider.is_valid());

    static_assert(
        second_collider.is_valid());

    const auto first_proxy_result =
        BroadPhaseProxy::create(
            first_collider,
            initial_bounds);

    check(
        state,
        first_proxy_result.has_value(),
        "Proxy accepts valid collider and bounds");

    if (!first_proxy_result.has_value())
    {
        return finish(state);
    }

    const BroadPhaseProxy first_proxy =
        first_proxy_result.value();

    check(
        state,
        first_proxy.collider() ==
            first_collider,
        "Proxy preserves persistent collider identity");

    check(
        state,
        first_proxy.bounds() ==
            initial_bounds,
        "Proxy preserves spatial bounds");

    check(
        state,
        first_proxy.bounds().minimum() ==
            PhysicsVector3{
                9.0,
                18.0,
                27.0
            },
        "Proxy exposes bounds minimum");

    check(
        state,
        first_proxy.bounds().maximum() ==
            PhysicsVector3{
                11.0,
                22.0,
                33.0
            },
        "Proxy exposes bounds maximum");

    const auto equivalent_proxy_result =
        BroadPhaseProxy::create(
            first_collider,
            initial_bounds);

    check(
        state,
        equivalent_proxy_result.has_value(),
        "Equivalent proxy fixture is valid");

    check(
        state,
        equivalent_proxy_result.has_value() &&
            equivalent_proxy_result.value() ==
                first_proxy,
        "Equivalent proxy values compare equal");

    const auto second_proxy_result =
        BroadPhaseProxy::create(
            second_collider,
            initial_bounds);

    check(
        state,
        second_proxy_result.has_value(),
        "Second collider proxy fixture is valid");

    check(
        state,
        second_proxy_result.has_value() &&
            second_proxy_result.value() !=
                first_proxy,
        "Different collider identities produce different proxies");

    const auto updated_bounds_result =
        AxisAlignedBounds::
            from_center_and_half_extents(
                PhysicsVector3{
                    -10.0,
                    -20.0,
                    -30.0
                },
                PhysicsVector3{
                    4.0,
                    5.0,
                    6.0
                });

    check(
        state,
        updated_bounds_result.has_value(),
        "Updated bounds fixture is valid");

    if (!updated_bounds_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds updated_bounds =
        updated_bounds_result.value();

    const auto updated_proxy_result =
        first_proxy.with_bounds(
            updated_bounds);

    check(
        state,
        updated_proxy_result.has_value(),
        "Proxy accepts immutable bounds replacement");

    if (!updated_proxy_result.has_value())
    {
        return finish(state);
    }

    const BroadPhaseProxy updated_proxy =
        updated_proxy_result.value();

    check(
        state,
        updated_proxy.collider() ==
            first_collider,
        "Bounds replacement preserves collider identity");

    check(
        state,
        updated_proxy.bounds() ==
            updated_bounds,
        "Bounds replacement preserves new bounds");

    check(
        state,
        updated_proxy !=
            first_proxy,
        "Bounds replacement produces a distinct proxy value");

    check(
        state,
        first_proxy.bounds() ==
            initial_bounds,
        "Bounds replacement does not mutate original proxy");

    check(
        state,
        updated_proxy.bounds().minimum() ==
            PhysicsVector3{
                -14.0,
                -25.0,
                -36.0
            },
        "Updated proxy exposes replacement minimum");

    check(
        state,
        updated_proxy.bounds().maximum() ==
            PhysicsVector3{
                -6.0,
                -15.0,
                -24.0
            },
        "Updated proxy exposes replacement maximum");

    const auto same_bounds_proxy_result =
        first_proxy.with_bounds(
            initial_bounds);

    check(
        state,
        same_bounds_proxy_result.has_value(),
        "Proxy accepts equivalent bounds replacement");

    check(
        state,
        same_bounds_proxy_result.has_value() &&
            same_bounds_proxy_result.value() ==
                first_proxy,
        "Equivalent bounds replacement preserves proxy value");

    const auto point_bounds_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                5.0,
                5.0,
                5.0
            },
            PhysicsVector3{
                5.0,
                5.0,
                5.0
            });

    check(
        state,
        point_bounds_result.has_value(),
        "Degenerate point bounds fixture is valid");

    if (!point_bounds_result.has_value())
    {
        return finish(state);
    }

    const auto point_proxy_result =
        BroadPhaseProxy::create(
            first_collider,
            point_bounds_result.value());

    check(
        state,
        point_proxy_result.has_value(),
        "Proxy accepts degenerate point bounds");

    check(
        state,
        point_proxy_result.has_value() &&
            point_proxy_result.
                value().
                bounds().
                half_extents() ==
                physics_zero_vector,
        "Point proxy preserves zero half extents");

    constexpr ColliderId maximum_collider{
        EntityId{
            UINT64_MAX,
            UINT64_MAX
        },
        UINT32_MAX
    };

    static_assert(
        maximum_collider.is_valid());

    const auto maximum_proxy_result =
        BroadPhaseProxy::create(
            maximum_collider,
            initial_bounds);

    check(
        state,
        maximum_proxy_result.has_value(),
        "Proxy accepts maximum persistent collider identity");

    check(
        state,
        maximum_proxy_result.has_value() &&
            maximum_proxy_result.
                value().
                collider() ==
                maximum_collider,
        "Proxy preserves maximum persistent identity values");

    BroadPhaseProxy copied_proxy =
        first_proxy;

    check(
        state,
        copied_proxy ==
            first_proxy,
        "Proxy copy preserves identity and bounds");

    BroadPhaseProxy moved_proxy =
        std::move(
            copied_proxy);

    check(
        state,
        moved_proxy ==
            first_proxy,
        "Proxy move preserves identity and bounds");

    BroadPhaseProxy assigned_proxy =
        updated_proxy;

    assigned_proxy =
        first_proxy;

    check(
        state,
        assigned_proxy ==
            first_proxy,
        "Proxy copy assignment preserves identity and bounds");

    BroadPhaseProxy move_assigned_proxy =
        updated_proxy;

    move_assigned_proxy =
        std::move(
            assigned_proxy);

    check(
        state,
        move_assigned_proxy ==
            first_proxy,
        "Proxy move assignment preserves identity and bounds");

    return finish(state);
}