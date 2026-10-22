#include "oros/ai/navigation_node_id.hpp"
#include "oros/ai/navigation_portal_id.hpp"

#include <compare>
#include <cstdint>
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
}

int main()
{
    using namespace oros::ai;
    using namespace oros::world;

    static_assert(
        !std::is_same_v<
            NavigationPortalId,
            NavigationNodeId>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationPortalId::cell),
            WorldCell>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationPortalId::local_id),
            std::uint64_t>);

    static_assert(
        std::is_trivially_copyable_v<
            NavigationPortalId>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalId&>().
                is_valid()));

    static_assert(
        noexcept(
            static_cast<bool>(
                std::declval<
                    const NavigationPortalId&>())));

    static_assert(
        !std::is_convertible_v<
            NavigationPortalId,
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationPortalId&>() <=>
            std::declval<
                const NavigationPortalId&>()));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationPortalId&>() <=>
                std::declval<
                    const NavigationPortalId&>()),
            std::strong_ordering>);

    static_assert(
        !invalid_navigation_portal_id.
            is_valid());

    TestState state{};

    constexpr NavigationPortalId
        default_id{};

    check(
        state,
        !default_id.is_valid(),
        "Default navigation portal identity is invalid");

    constexpr NavigationPortalId
        arbitrary_zero_id{
            WorldCell{
                19,
                -27,
                43
            },
            0ULL
        };

    check(
        state,
        !arbitrary_zero_id.is_valid(),
        "Zero local portal id is invalid in an arbitrary world cell");

    constexpr NavigationPortalId
        origin_id{
            WorldCell{
                0,
                0,
                0
            },
            1ULL
        };

    check(
        state,
        origin_id.is_valid(),
        "Nonzero local portal id is valid in the origin world cell");

    constexpr NavigationPortalId
        negative_cell_id{
            WorldCell{
                -17,
                -9,
                -31
            },
            42ULL
        };

    check(
        state,
        negative_cell_id.is_valid(),
        "Nonzero local portal id is valid in a negative world cell");

    constexpr NavigationPortalId
        minimum_cell_id{
            WorldCell{
                std::numeric_limits<
                    std::int64_t>::min(),
                0,
                0
            },
            7ULL
        };

    check(
        state,
        minimum_cell_id.is_valid(),
        "Nonzero local portal id is valid at INT64_MIN cell coordinate");

    constexpr NavigationPortalId
        maximum_cell_id{
            WorldCell{
                std::numeric_limits<
                    std::int64_t>::max(),
                0,
                0
            },
            7ULL
        };

    check(
        state,
        maximum_cell_id.is_valid(),
        "Nonzero local portal id is valid at INT64_MAX cell coordinate");

    check(
        state,
        static_cast<bool>(
            origin_id) ==
            origin_id.is_valid() &&
            static_cast<bool>(
                arbitrary_zero_id) ==
            arbitrary_zero_id.is_valid(),
        "Explicit bool conversion mirrors portal is_valid");

    constexpr NavigationPortalId
        equal_origin_id{
            WorldCell{
                0,
                0,
                0
            },
            1ULL
        };

    check(
        state,
        equal_origin_id ==
            origin_id,
        "Navigation portal equality includes identical cell and local id");

    constexpr NavigationPortalId
        changed_x_id{
            WorldCell{
                1,
                0,
                0
            },
            1ULL
        };

    check(
        state,
        changed_x_id !=
            origin_id,
        "Changing world-cell x changes navigation portal identity");

    constexpr NavigationPortalId
        changed_y_id{
            WorldCell{
                0,
                1,
                0
            },
            1ULL
        };

    check(
        state,
        changed_y_id !=
            origin_id,
        "Changing world-cell y changes navigation portal identity");

    constexpr NavigationPortalId
        changed_z_id{
            WorldCell{
                0,
                0,
                1
            },
            1ULL
        };

    check(
        state,
        changed_z_id !=
            origin_id,
        "Changing world-cell z changes navigation portal identity");

    constexpr NavigationPortalId
        changed_local_id{
            WorldCell{
                0,
                0,
                0
            },
            2ULL
        };

    check(
        state,
        changed_local_id !=
            origin_id,
        "Changing local id changes navigation portal identity");

    constexpr NavigationPortalId
        earlier_cell_large_local{
            WorldCell{
                -1,
                99,
                99
            },
            std::numeric_limits<
                std::uint64_t>::max()
        };

    constexpr NavigationPortalId
        later_cell_small_local{
            WorldCell{
                0,
                -99,
                -99
            },
            1ULL
        };

    check(
        state,
        earlier_cell_large_local <
            later_cell_small_local,
        "Canonical portal comparison orders world cell before local id");

    constexpr NavigationPortalId
        earlier_y_large_local{
            WorldCell{
                5,
                -1,
                99
            },
            std::numeric_limits<
                std::uint64_t>::max()
        };

    constexpr NavigationPortalId
        later_y_small_local{
            WorldCell{
                5,
                0,
                -99
            },
            1ULL
        };

    check(
        state,
        earlier_y_large_local <
            later_y_small_local,
        "Canonical portal comparison preserves WorldCell y precedence");

    constexpr NavigationPortalId
        earlier_z_large_local{
            WorldCell{
                5,
                7,
                -1
            },
            std::numeric_limits<
                std::uint64_t>::max()
        };

    constexpr NavigationPortalId
        later_z_small_local{
            WorldCell{
                5,
                7,
                0
            },
            1ULL
        };

    check(
        state,
        earlier_z_large_local <
            later_z_small_local,
        "Canonical portal comparison preserves WorldCell z precedence");

    constexpr NavigationPortalId
        same_cell_lower_local{
            WorldCell{
                5,
                7,
                9
            },
            12ULL
        };

    constexpr NavigationPortalId
        same_cell_higher_local{
            WorldCell{
                5,
                7,
                9
            },
            13ULL
        };

    check(
        state,
        same_cell_lower_local <
            same_cell_higher_local,
        "Canonical portal comparison orders local id after the complete WorldCell");

    check(
        state,
        !invalid_navigation_portal_id.
            is_valid() &&
            !static_cast<bool>(
                invalid_navigation_portal_id) &&
            invalid_navigation_portal_id.
                local_id == 0ULL,
        "invalid_navigation_portal_id is the invalid identity");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
