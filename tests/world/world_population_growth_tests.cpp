#include "oros/world/world.hpp"

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <string_view>
#include <utility>

namespace
{
    struct CheckState final
    {
        std::size_t total{};
        std::size_t failed{};
    };

    void check(
        CheckState& state,
        const bool passed,
        const std::string_view description)
    {
        ++state.total;
        if (!passed)
        {
            ++state.failed;
            std::cerr << "[fail] " << description << '\n';
        }
    }
}

int main()
{
    using namespace oros::world;
    using namespace oros::foundation;

    CheckState state{};
    // Deterministic scenario sizes; durations are *diagnostics*, never
    // test pass criteria or claimed relative speedup against another SHA.
    constexpr std::array<std::uint64_t, 4U> populations{
        2ULL, 100ULL, 1'000ULL, 10'000ULL
    };
    constexpr std::uint64_t base_namespace = 0x4F524F5301000000ULL;

    for (std::size_t scenario = 0U;
         scenario < populations.size(); ++scenario)
    {
        const std::uint64_t count = populations[scenario];
        const std::uint64_t world_namespace =
            base_namespace + static_cast<std::uint64_t>(scenario);
        auto result = World::create(world_namespace);
        check(state, result.has_value(),
            "World creates with a unique benchmark namespace");
        if (!result.has_value())
        {
            return 1;
        }

        World world{std::move(result.value())};

        const auto started = std::chrono::steady_clock::now();
        bool sequential = true;
        for (std::uint64_t sequence = 1ULL;
             sequence <= count; ++sequence)
        {
            const auto entity = world.create_entity();
            if (!entity.has_value() ||
                entity.value() != EntityId{world_namespace, sequence})
            {
                sequential = false;
                break;
            }
        }
        const auto elapsed =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - started).count();

        check(state,
            sequential && world.entity_count() ==
                static_cast<std::size_t>(count),
            "All entities retain consecutive stable World identities");
        if (!sequential)
        {
            return 1;
        }

        // Attach positions after measuring entity creation. Check that
        // the component store and existence remain independently owned.
        const auto origin = WorldPosition::origin();
        check(state, origin.has_value(), "World origin is valid");
        if (!origin.has_value())
        {
            return 1;
        }
        bool attachment_ok = true;
        std::size_t attachment_count{};
        for (std::uint64_t sequence = 1ULL;
             sequence <= count; sequence += 8ULL)
        {
            const auto attached = world.add_position(
                EntityId{world_namespace, sequence},
                origin.value());
            if (!attached.has_value())
            {
                attachment_ok = false;
                break;
            }
            ++attachment_count;
        }
        check(state,
            attachment_ok &&
                world.position_count() == attachment_count,
            "Sparse entity positions remain World-owned");

        std::size_t retired_count{};
        bool retired_ok = true;
        for (std::uint64_t sequence = 1ULL;
             sequence <= count; sequence += 3ULL)
        {
            const auto removed = world.destroy_entity(
                EntityId{world_namespace, sequence});
            if (!removed.has_value() ||
                world.contains(EntityId{world_namespace, sequence}))
            {
                retired_ok = false;
                break;
            }
            ++retired_count;
        }
        check(state,
            retired_ok &&
                world.entity_count() ==
                    static_cast<std::size_t>(count) - retired_count,
            "Sparse removals preserve independent surviving identities");

        const auto fresh = world.create_entity();
        check(state,
            fresh.has_value() &&
                fresh.value() ==
                    EntityId{world_namespace, count + 1ULL} &&
                world.contains(fresh.value()),
            "Entity IDs remain monotonic after sparse destruction");

        std::cout << "[diagnostic] World entity creation: "
                  << count << " entities, "
                  << elapsed << " microseconds, "
                  << retired_count << " retired\n";
    }

    std::cout << "World population growth: "
              << state.total - state.failed << '/'
              << state.total << " passed\n";
    return state.failed == 0U ? 0 : 1;
}
