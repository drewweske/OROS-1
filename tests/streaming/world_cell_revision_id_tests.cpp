#include "oros/streaming/world_cell_revision_id.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <unordered_set>

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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell revision identity test "
            << "summary: "
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
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        !invalid_world_cell_revision_id.is_valid(),
        "Invalid revision identity reports invalid");

    check(
        state,
        !static_cast<bool>(
            invalid_world_cell_revision_id),
        "Invalid revision identity converts to false");

    const WorldCellKey cell_key{
        0x4F524F5300000007ULL,
        WorldCell{
            -42,
            17,
            9001
        }
    };

    const WorldCellRevisionId invalid_key_id{
        invalid_world_cell_key,
        1ULL
    };

    check(
        state,
        !invalid_key_id.is_valid(),
        "Invalid cell key produces invalid revision identity");

    const WorldCellRevisionId zero_revision_id{
        cell_key,
        0ULL
    };

    check(
        state,
        !zero_revision_id.is_valid(),
        "Zero revision produces invalid revision identity");

    const WorldCellRevisionId revision_id{
        cell_key,
        7ULL
    };

    check(
        state,
        revision_id.is_valid(),
        "Valid revision identity reports valid");

    check(
        state,
        static_cast<bool>(
            revision_id),
        "Valid revision identity converts to true");

    check(
        state,
        revision_id.cell_key == cell_key,
        "Revision identity preserves its cell key");

    check(
        state,
        revision_id.revision == 7ULL,
        "Revision identity preserves its revision");

    const WorldCellRevisionId equal_revision_id{
        cell_key,
        7ULL
    };

    check(
        state,
        equal_revision_id == revision_id,
        "Equal revision identities compare equal");

    const WorldCellRevisionId different_revision_id{
        cell_key,
        8ULL
    };

    check(
        state,
        different_revision_id != revision_id,
        "Revision participates in equality");

    const WorldCellKey different_cell_key{
        0x4F524F5300000007ULL,
        WorldCell{
            -41,
            17,
            9001
        }
    };

    const WorldCellRevisionId
        different_cell_revision_id{
            different_cell_key,
            7ULL
        };

    check(
        state,
        different_cell_revision_id != revision_id,
        "Cell key participates in equality");

    const WorldCellRevisionId lower_revision_id{
        cell_key,
        6ULL
    };

    check(
        state,
        lower_revision_id < revision_id,
        "Revision identities provide revision ordering");

    const WorldCellKey lower_namespace_key{
        0x4F524F5300000006ULL,
        WorldCell{
            -42,
            17,
            9001
        }
    };

    const WorldCellRevisionId lower_cell_id{
        lower_namespace_key,
        999ULL
    };

    check(
        state,
        lower_cell_id < revision_id,
        "Cell identity precedes revision in ordering");

    const std::string canonical_text =
        to_string(
            revision_id);

    check(
        state,
        canonical_text ==
            "4f524f5300000007:-42:17:9001@7",
        "Revision identity has canonical serialized text");

    const auto parsed_revision_id =
        parse_world_cell_revision_id(
            canonical_text);

    check(
        state,
        parsed_revision_id.has_value(),
        "Canonical revision identity text parses");

    check(
        state,
        parsed_revision_id.has_value() &&
            parsed_revision_id.value() ==
                revision_id,
        "Canonical parse restores the exact identity");

    const WorldCellKey boundary_cell_key{
        0xFFFFFFFFFFFFFFFFULL,
        WorldCell{
            (std::numeric_limits<
                std::int64_t>::min)(),
            0,
            (std::numeric_limits<
                std::int64_t>::max)()
        }
    };

    const WorldCellRevisionId boundary_id{
        boundary_cell_key,
        (std::numeric_limits<
            std::uint64_t>::max)()
    };

    const std::string boundary_text =
        to_string(
            boundary_id);

    check(
        state,
        boundary_text ==
            "ffffffffffffffff:"
            "-9223372036854775808:"
            "0:"
            "9223372036854775807@"
            "18446744073709551615",
        "Boundary revision identity serializes exactly");

    const auto parsed_boundary_id =
        parse_world_cell_revision_id(
            boundary_text);

    check(
        state,
        parsed_boundary_id.has_value(),
        "Boundary revision identity text parses");

    check(
        state,
        parsed_boundary_id.has_value() &&
            parsed_boundary_id.value() ==
                boundary_id,
        "Boundary parse restores every identity bit");

    const auto missing_separator =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001");

    check(
        state,
        !missing_separator.has_value(),
        "Missing revision separator is rejected");

    const auto empty_cell_key =
        parse_world_cell_revision_id(
            "@7");

    check(
        state,
        !empty_cell_key.has_value(),
        "Empty cell key is rejected");

    const auto empty_revision =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@");

    check(
        state,
        !empty_revision.has_value(),
        "Empty revision is rejected");

    const auto extra_separator =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@7@8");

    check(
        state,
        !extra_separator.has_value(),
        "Extra revision separator is rejected");

    const auto invalid_cell_key =
        parse_world_cell_revision_id(
            "0000000000000000:-42:17:9001@7");

    check(
        state,
        !invalid_cell_key.has_value(),
        "Invalid persistent cell key is rejected");

    const auto short_namespace =
        parse_world_cell_revision_id(
            "4f524f53:-42:17:9001@7");

    check(
        state,
        !short_namespace.has_value(),
        "Short cell namespace is rejected");

    const auto uppercase_namespace =
        parse_world_cell_revision_id(
            "4F524F5300000007:-42:17:9001@7");

    check(
        state,
        !uppercase_namespace.has_value(),
        "Uppercase namespace text is non-canonical");

    const auto zero_revision =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@0");

    check(
        state,
        !zero_revision.has_value(),
        "Zero serialized revision is rejected");

    const auto leading_zero_revision =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@07");

    check(
        state,
        !leading_zero_revision.has_value(),
        "Leading-zero revision is non-canonical");

    const auto explicit_positive_revision =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@+7");

    check(
        state,
        !explicit_positive_revision.has_value(),
        "Explicit positive revision sign is rejected");

    const auto negative_revision =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@-7");

    check(
        state,
        !negative_revision.has_value(),
        "Negative revision is rejected");

    const auto revision_overflow =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@"
            "18446744073709551616");

    check(
        state,
        !revision_overflow.has_value(),
        "Revision overflow is rejected");

    const auto leading_revision_whitespace =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@ 7");

    check(
        state,
        !leading_revision_whitespace.has_value(),
        "Leading revision whitespace is rejected");

    const auto trailing_revision_whitespace =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42:17:9001@7 ");

    check(
        state,
        !trailing_revision_whitespace.has_value(),
        "Trailing revision whitespace is rejected");

    const auto coordinate_whitespace =
        parse_world_cell_revision_id(
            "4f524f5300000007:-42: 17:9001@7");

    check(
        state,
        !coordinate_whitespace.has_value(),
        "Cell coordinate whitespace is rejected");

    const std::size_t revision_hash =
        WorldCellRevisionIdHash{}(
            revision_id);

    const std::size_t equal_revision_hash =
        WorldCellRevisionIdHash{}(
            equal_revision_id);

    check(
        state,
        revision_hash ==
            equal_revision_hash,
        "Equal revision identities produce equal hashes");

    check(
        state,
        revision_hash ==
            WorldCellRevisionIdHash{}(
                revision_id),
        "Revision identity hashing is repeatable");

    std::unordered_set<
        WorldCellRevisionId,
        WorldCellRevisionIdHash>
        revision_ids{};

    revision_ids.insert(
        revision_id);

    revision_ids.insert(
        equal_revision_id);

    revision_ids.insert(
        different_revision_id);

    revision_ids.insert(
        different_cell_revision_id);

    check(
        state,
        revision_ids.size() == 3U,
        "Hash set deduplicates equal revision identities");

    check(
        state,
        revision_ids.contains(
            equal_revision_id),
        "Hash set resolves an equal copied identity");

    check(
        state,
        revision_ids.contains(
            different_revision_id),
        "Hash set distinguishes a different revision");

    check(
        state,
        revision_ids.contains(
            different_cell_revision_id),
        "Hash set distinguishes a different cell key");

    const WorldCellRevisionId absent_id{
        cell_key,
        999ULL
    };

    check(
        state,
        !revision_ids.contains(
            absent_id),
        "Hash set rejects an absent revision identity");

    return finish(state);
}