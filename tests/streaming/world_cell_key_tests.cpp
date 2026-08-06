#include "oros/streaming/world_cell_key.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
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
            << "\nWorld cell key test summary: "
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
        !invalid_world_cell_key.is_valid(),
        "Invalid world cell key reports invalid");

    check(
        state,
        !static_cast<bool>(
            invalid_world_cell_key),
        "Invalid world cell key converts to false");

    const WorldCellKey origin_key{
        1ULL,
        WorldCell{
            0,
            0,
            0
        }
    };

    check(
        state,
        origin_key.is_valid(),
        "Non-zero namespace produces a valid key");

    check(
        state,
        static_cast<bool>(
            origin_key),
        "Valid world cell key converts to true");

    check(
        state,
        origin_key.world_namespace == 1ULL,
        "World cell key preserves its namespace");

    check(
        state,
        origin_key.cell ==
            WorldCell{
                0,
                0,
                0
            },
        "World cell key preserves its coordinates");

    const WorldCellKey origin_copy{
        1ULL,
        WorldCell{
            0,
            0,
            0
        }
    };

    check(
        state,
        origin_key == origin_copy,
        "Equal keys compare equal");

    const WorldCellKey different_namespace{
        2ULL,
        WorldCell{
            0,
            0,
            0
        }
    };

    const WorldCellKey different_x{
        1ULL,
        WorldCell{
            1,
            0,
            0
        }
    };

    const WorldCellKey different_y{
        1ULL,
        WorldCell{
            0,
            1,
            0
        }
    };

    const WorldCellKey different_z{
        1ULL,
        WorldCell{
            0,
            0,
            1
        }
    };

    check(
        state,
        origin_key !=
            different_namespace,
        "Namespace participates in equality");

    check(
        state,
        origin_key !=
            different_x,
        "X coordinate participates in equality");

    check(
        state,
        origin_key !=
            different_y,
        "Y coordinate participates in equality");

    check(
        state,
        origin_key !=
            different_z,
        "Z coordinate participates in equality");

    check(
        state,
        origin_key <
            different_namespace,
        "Keys provide deterministic namespace ordering");

    check(
        state,
        origin_key <
            different_x,
        "Keys provide deterministic coordinate ordering");

    const std::string
        origin_text =
            to_string(
                origin_key);

    check(
        state,
        origin_text ==
            "0000000000000001:0:0:0",
        "Origin key has canonical serialized text");

    const WorldCellKey mixed_key{
        0x4F524F5300000007ULL,
        WorldCell{
            -42,
            17,
            -9001
        }
    };

    const std::string
        mixed_text =
            to_string(
                mixed_key);

    check(
        state,
        mixed_text ==
            "4f524f5300000007:-42:17:-9001",
        "Mixed-sign key serializes canonically");

    const auto parsed_origin =
        parse_world_cell_key(
            origin_text);

    check(
        state,
        parsed_origin.has_value(),
        "Canonical origin text parses");

    check(
        state,
        parsed_origin.has_value() &&
            parsed_origin.value() ==
                origin_key,
        "Origin parse restores the exact key");

    const auto parsed_mixed =
        parse_world_cell_key(
            mixed_text);

    check(
        state,
        parsed_mixed.has_value(),
        "Canonical mixed-sign text parses");

    check(
        state,
        parsed_mixed.has_value() &&
            parsed_mixed.value() ==
                mixed_key,
        "Mixed-sign parse restores the exact key");

    const std::int64_t minimum_coordinate =
        (std::numeric_limits<
            std::int64_t>::min)();

    const std::int64_t maximum_coordinate =
        (std::numeric_limits<
            std::int64_t>::max)();

    const WorldCellKey boundary_key{
        (std::numeric_limits<
            std::uint64_t>::max)(),
        WorldCell{
            minimum_coordinate,
            maximum_coordinate,
            minimum_coordinate
        }
    };

    const std::string
        boundary_text =
            to_string(
                boundary_key);

    check(
        state,
        boundary_text ==
            "ffffffffffffffff:"
            "-9223372036854775808:"
            "9223372036854775807:"
            "-9223372036854775808",
        "Boundary coordinates serialize exactly");

    const auto parsed_boundary =
        parse_world_cell_key(
            boundary_text);

    check(
        state,
        parsed_boundary.has_value(),
        "Boundary key text parses");

    check(
        state,
        parsed_boundary.has_value() &&
            parsed_boundary.value() ==
                boundary_key,
        "Boundary parse restores every bit");

    const auto zero_namespace =
        parse_world_cell_key(
            "0000000000000000:0:0:0");

    check(
        state,
        !zero_namespace.has_value(),
        "Zero namespace is rejected");

    const auto short_namespace =
        parse_world_cell_key(
            "1:0:0:0");

    check(
        state,
        !short_namespace.has_value(),
        "Short namespace text is rejected");

    const auto uppercase_namespace =
        parse_world_cell_key(
            "4F524F5300000007:-42:17:-9001");

    check(
        state,
        !uppercase_namespace.has_value(),
        "Uppercase namespace text is non-canonical");

    const auto missing_coordinate =
        parse_world_cell_key(
            "0000000000000001:0:0");

    check(
        state,
        !missing_coordinate.has_value(),
        "Missing coordinate is rejected");

    const auto extra_coordinate =
        parse_world_cell_key(
            "0000000000000001:0:0:0:0");

    check(
        state,
        !extra_coordinate.has_value(),
        "Extra coordinate is rejected");

    const auto empty_x =
        parse_world_cell_key(
            "0000000000000001::0:0");

    check(
        state,
        !empty_x.has_value(),
        "Empty X coordinate is rejected");

    const auto empty_y =
        parse_world_cell_key(
            "0000000000000001:0::0");

    check(
        state,
        !empty_y.has_value(),
        "Empty Y coordinate is rejected");

    const auto empty_z =
        parse_world_cell_key(
            "0000000000000001:0:0:");

    check(
        state,
        !empty_z.has_value(),
        "Empty Z coordinate is rejected");

    const auto leading_zero =
        parse_world_cell_key(
            "0000000000000001:01:0:0");

    check(
        state,
        !leading_zero.has_value(),
        "Leading-zero coordinate is non-canonical");

    const auto negative_zero =
        parse_world_cell_key(
            "0000000000000001:-0:0:0");

    check(
        state,
        !negative_zero.has_value(),
        "Negative zero coordinate is non-canonical");

    const auto explicit_positive =
        parse_world_cell_key(
            "0000000000000001:+1:0:0");

    check(
        state,
        !explicit_positive.has_value(),
        "Explicit positive sign is rejected");

    const auto x_overflow =
        parse_world_cell_key(
            "0000000000000001:"
            "9223372036854775808:0:0");

    check(
        state,
        !x_overflow.has_value(),
        "Positive coordinate overflow is rejected");

    const auto y_underflow =
        parse_world_cell_key(
            "0000000000000001:0:"
            "-9223372036854775809:0");

    check(
        state,
        !y_underflow.has_value(),
        "Negative coordinate underflow is rejected");

    const auto invalid_hex =
        parse_world_cell_key(
            "000000000000000g:0:0:0");

    check(
        state,
        !invalid_hex.has_value(),
        "Invalid namespace hexadecimal digit is rejected");

    const auto whitespace =
        parse_world_cell_key(
            "0000000000000001: 0:0:0");

    check(
        state,
        !whitespace.has_value(),
        "Coordinate whitespace is rejected");

    const WorldCellKeyHash hasher{};

    const std::size_t origin_hash =
        hasher(
            origin_key);

    check(
        state,
        origin_hash ==
            hasher(
                origin_copy),
        "Equal keys produce equal hashes");

    check(
        state,
        origin_hash ==
            hasher(
                origin_key),
        "World cell key hashing is repeatable");

    std::unordered_set<
        WorldCellKey,
        WorldCellKeyHash>
        keys{};

    keys.insert(
        origin_key);

    keys.insert(
        origin_copy);

    keys.insert(
        different_namespace);

    keys.insert(
        different_x);

    keys.insert(
        different_y);

    keys.insert(
        different_z);

    keys.insert(
        mixed_key);

    check(
        state,
        keys.size() == 6U,
        "Hash set deduplicates equal cell keys");

    check(
        state,
        keys.contains(
            origin_copy),
        "Hash set resolves an equal copied key");

    check(
        state,
        keys.contains(
            mixed_key),
        "Hash set resolves a mixed-sign key");

    check(
        state,
        !keys.contains(
            WorldCellKey{
                99ULL,
                WorldCell{
                    99,
                    99,
                    99
                }
            }),
        "Hash set rejects an absent key");

    return finish(state);
}