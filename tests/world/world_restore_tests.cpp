#include "oros/world/world.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
#include <utility>

namespace
{
    struct Checks { int total{}; int failed{}; };
    void check(Checks& c, bool valid, std::string_view message)
    {
        ++c.total;
        if (!valid)
        {
            ++c.failed;
            std::cerr << "[fail] " << message << '\n';
        }
    }
}

int main()
{
    using namespace oros::world;
    using namespace oros::foundation;
    Checks c{};
    constexpr std::uint64_t ns{0x4F524F53ULL};
    constexpr std::uint64_t max{
        (std::numeric_limits<std::uint64_t>::max)()
    };
    const std::array<EntityId, 3U> saved{{
        {ns, 7ULL}, {ns, 1ULL}, {ns, 3ULL}
    }};
    auto r = World::restore(ns, 12ULL, saved);
    check(c, r.has_value(), "Unsorted IDs restore with historical watermark");
    if (!r.has_value()) return 1;
    World w{std::move(r.value())};
    check(c, w.last_issued_sequence() == 12ULL,
        "Restored World retains global issued watermark");
    check(c, w.entity_count() == 3U &&
        w.contains(saved[0]) && w.contains(saved[1]) &&
        w.contains(saved[2]), "Exact World identity restored");
    check(c, !w.contains({ns, 12ULL}) &&
        !w.contains({ns, 2ULL}) && w.position_count() == 0U,
        "Dead/nonresident identities are not made live");

    const auto pos = WorldPosition::origin();
    check(c, pos.has_value(), "Position fixture created");
    if (pos.has_value())
    {
        const auto a = w.add_position(saved[0], pos.value());
        const auto* p = w.find_position(saved[0]);
        check(c, a.has_value() && p != nullptr &&
            *p == pos.value(), "Restored ID accepts its World position");
    }
    const auto created = w.create_entity();
    check(c, created.has_value() &&
        created.value() == EntityId{ns, 13ULL},
        "New IDs skip missing previously issued identities");
    check(c, w.last_issued_sequence() == 13ULL,
        "World watermark increments after restore and issuance");
    const auto deleted = w.destroy_entity(saved[2]);
    const auto later = w.create_entity();
    check(c, deleted.has_value() && !w.contains(saved[2]) &&
        later.has_value() && later.value() == EntityId{ns, 14ULL},
        "Destroy after restoration does not recycle IDs");

    check(c, w.last_issued_sequence() == 14ULL,
        "Deleting another identity never lowers issued watermark");

    // Simulated save/restart records the high-water mark from World
    // itself: max(live IDs) is NOT correct after retiring the last ID.
    auto fresh = World::create(ns + 2ULL);
    check(c, fresh.has_value(), "Fresh World created for round-trip");
    if (fresh.has_value())
    {
        World live{std::move(fresh.value())};
        check(c, live.last_issued_sequence() == 0ULL,
            "Fresh World watermark is zero");
        const auto one = live.create_entity();
        const auto two = live.create_entity();
        const auto three = live.create_entity();
        check(c, one.has_value() && two.has_value() &&
            three.has_value() && live.last_issued_sequence() == 3ULL,
            "Issuing identities records global sequence three");
        if (one.has_value() && two.has_value() && three.has_value())
        {
            const auto retired = live.destroy_entity(three.value());
            check(c, retired.has_value() &&
                live.last_issued_sequence() == 3ULL &&
                live.entity_count() == 2U,
                "Retiring newest identity does not rewind watermark");
            const std::array<EntityId, 2U> survivors{{
                one.value(), two.value()
            }};
            auto reloaded = World::restore(
                live.world_namespace(),
                live.last_issued_sequence(),
                survivors);
            check(c, reloaded.has_value(),
                "Authoritative issued watermark restores successfully");
            if (reloaded.has_value())
            {
                World after_restart{std::move(reloaded.value())};
                const auto next = after_restart.create_entity();
                check(c, next.has_value() &&
                    next.value() == EntityId{ns + 2ULL, 4ULL} &&
                    !after_restart.contains(three.value()) &&
                    after_restart.last_issued_sequence() == 4ULL,
                    "Restart never reissues highest retired identity");
            }
        }
    }

    const std::array<EntityId, 2U> duplicate{{
        {ns, 5ULL}, {ns, 5ULL}
    }};
    const auto dup = World::restore(ns, 9ULL, duplicate);
    check(c, !dup.has_value() &&
        dup.error().code == ErrorCode::invalid_argument,
        "Duplicate input fails atomically");
    const std::array<EntityId, 1U> invalid{{
        {ns, 0ULL}
    }};
    const auto bad = World::restore(ns, 12ULL, invalid);
    check(c, !bad.has_value() &&
        bad.error().code == ErrorCode::invalid_argument,
        "Invalid identity rejected");
    const std::array<EntityId, 1U> foreign{{
        {ns + 1ULL, 1ULL}
    }};
    const auto other = World::restore(ns, 12ULL, foreign);
    check(c, !other.has_value() &&
        other.error().code == ErrorCode::invalid_argument,
        "Foreign namespace rejected");
    const std::array<EntityId, 1U> future{{
        {ns, 13ULL}
    }};
    const auto ahead = World::restore(ns, 12ULL, future);
    check(c, !ahead.has_value() &&
        ahead.error().code == ErrorCode::invalid_argument,
        "IDs newer than watermark rejected");
    const auto zero = World::restore(
        0ULL, 0ULL, std::span<const EntityId>{});
    check(c, !zero.has_value() &&
        zero.error().code == ErrorCode::invalid_argument,
        "Zero namespace rejected");
    const auto missing_watermark = World::restore(ns, 0ULL, saved);
    check(c, !missing_watermark.has_value(),
        "Nonempty snapshot rejects zero watermark");

    auto empty = World::restore(
        ns, 0ULL, std::span<const EntityId>{});
    check(c, empty.has_value(), "Empty never-issued namespace restores");
    if (empty.has_value())
    {
        World e{std::move(empty.value())};
        const auto first = e.create_entity();
        check(c, first.has_value() &&
            first.value() == EntityId{ns, 1ULL},
            "First ID after empty restoration is one");
    }

    const std::array<EntityId, 1U> last{{{ns, max}}};
    auto exhausted = World::restore(ns, max, last);
    check(c, exhausted.has_value(), "Max-ID snapshot reconstructs");
    if (exhausted.has_value())
    {
        World e{std::move(exhausted.value())};
        const auto next = e.create_entity();
        check(c, !next.has_value() &&
            next.error().code == ErrorCode::invalid_state &&
            e.contains(last[0]), "Exhausted state rejects further creation");
    }
    const std::array<EntityId, 1U> before_last{{{ns, max - 1ULL}}};
    auto boundary = World::restore(ns, max - 1ULL, before_last);
    check(c, boundary.has_value(), "Near-maximum watermark restores");
    if (boundary.has_value())
    {
        World e{std::move(boundary.value())};
        const auto final_id = e.create_entity();
        const auto overflow = e.create_entity();
        check(c, final_id.has_value() &&
            final_id.value() == EntityId{ns, max} &&
            !overflow.has_value() &&
            overflow.error().code == ErrorCode::invalid_state,
            "Final available ID is generated only once");
    }
    if (exhausted.has_value())
    {
        // Move above has consumed the returned World; inspect generator
        // exhaustion via an independent reconstruction instead.
        auto again = World::restore(ns, max, last);
        check(c, again.has_value() &&
            again->last_issued_sequence() == max,
            "Maximum issued watermark never wraps to zero");
    }

    std::cout << "World restore " << c.total - c.failed
              << '/' << c.total << " checks passed\n";
    return c.failed == 0 ? 0 : 1;
}
