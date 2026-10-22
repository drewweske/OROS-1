#include "oros/world/world_time.hpp"

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
    using namespace oros::world;

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const WorldTime&>() <=>
                std::declval<
                    const WorldTime&>()),
            std::strong_ordering>);

    static_assert(
        std::is_nothrow_default_constructible_v<
            WorldTime>);

    static_assert(
        std::is_nothrow_copy_constructible_v<
            WorldTime>);

    static_assert(
        std::is_nothrow_copy_assignable_v<
            WorldTime>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            WorldTime>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            WorldTime>);

    static_assert(
        std::is_trivially_copyable_v<
            WorldTime>);

    TestState state{};

    const WorldTime default_time{};

    const WorldTime epoch =
        WorldTime::epoch();

    const WorldTime first_microsecond =
        WorldTime::
            from_microseconds_since_epoch(
                1ULL);

    const WorldTime one_second =
        WorldTime::
            from_microseconds_since_epoch(
                1'000'000ULL);

    const WorldTime one_minute =
        WorldTime::
            from_microseconds_since_epoch(
                60'000'000ULL);

    const WorldTime maximum =
        WorldTime::
            from_microseconds_since_epoch(
                (
                    std::numeric_limits<
                        std::uint64_t>::max
                )());

    check(
        state,
        default_time == epoch,
        "Default WorldTime is the world epoch");

    check(
        state,
        epoch.is_epoch(),
        "World epoch reports epoch state");

    check(
        state,
        epoch.microseconds_since_epoch() ==
            0ULL,
        "World epoch has zero elapsed world microseconds");

    check(
        state,
        !first_microsecond.is_epoch(),
        "Nonzero WorldTime is not the world epoch");

    check(
        state,
        first_microsecond.
                microseconds_since_epoch() ==
            1ULL,
        "WorldTime preserves one microsecond exactly");

    check(
        state,
        one_second.
                microseconds_since_epoch() ==
            1'000'000ULL,
        "WorldTime preserves one second as exact microseconds");

    check(
        state,
        one_minute.
                microseconds_since_epoch() ==
            60'000'000ULL,
        "WorldTime preserves one minute as exact microseconds");

    check(
        state,
        epoch <
            first_microsecond &&
            first_microsecond <
                one_second &&
            one_second <
                one_minute,
        "WorldTime orders absolute coordinates chronologically");

    check(
        state,
        one_minute >
            one_second,
        "WorldTime supports deterministic reverse ordering");

    check(
        state,
        WorldTime::
                from_microseconds_since_epoch(
                    1'000'000ULL) ==
            one_second,
        "Equal world-time coordinates compare equal");

    check(
        state,
        maximum.
                microseconds_since_epoch() ==
            (
                std::numeric_limits<
                    std::uint64_t>::max
            )(),
        "WorldTime preserves the complete uint64 coordinate range");

    check(
        state,
        maximum >
            one_minute,
        "Maximum WorldTime remains ordered after ordinary times");

    const WorldTime copied =
        one_minute;

    check(
        state,
        copied ==
            one_minute,
        "WorldTime copy preserves exact chronology");

    WorldTime assigned{};
    assigned =
        one_second;

    check(
        state,
        assigned ==
            one_second,
        "WorldTime assignment preserves exact chronology");

    check(
        state,
        epoch ==
            WorldTime::epoch(),
        "World epoch construction is deterministic");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}