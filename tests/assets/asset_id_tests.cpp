#include "oros/assets/asset_id.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <unordered_set>
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
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    check(
        state,
        !invalid_asset_id.is_valid(),
        "Default asset ID is invalid");

    const AssetId missing_namespace{
        0ULL,
        1ULL
    };

    check(
        state,
        !missing_namespace.is_valid(),
        "Asset ID rejects a zero catalog namespace");

    const AssetId missing_sequence{
        1ULL,
        0ULL
    };

    check(
        state,
        !missing_sequence.is_valid(),
        "Asset ID rejects a zero asset sequence");

    const AssetId valid_asset{
        1ULL,
        1ULL
    };

    check(
        state,
        valid_asset.is_valid(),
        "Asset ID accepts non-zero components");

    const AssetId equal_asset{
        1ULL,
        1ULL
    };

    const AssetId later_sequence{
        1ULL,
        2ULL
    };

    const AssetId later_namespace{
        2ULL,
        1ULL
    };

    check(
        state,
        valid_asset == equal_asset,
        "Equal asset ID components compare equal");

    check(
        state,
        valid_asset != later_sequence,
        "Different asset sequences compare unequal");

    check(
        state,
        valid_asset < later_sequence,
        "Asset IDs order by sequence within a catalog");

    check(
        state,
        later_sequence < later_namespace,
        "Asset IDs order by catalog before sequence");

    const std::string simple_text =
        to_string(valid_asset);

    check(
        state,
        simple_text ==
            "0000000000000001:0000000000000001",
        "Asset ID serializes with fixed-width components");

    check(
        state,
        simple_text.size() == 33U,
        "Serialized asset ID contains 33 characters");

    const AssetId patterned_asset{
        0x0123456789ABCDEFULL,
        0xFEDCBA9876543210ULL
    };

    const std::string patterned_text =
        to_string(patterned_asset);

    check(
        state,
        patterned_text ==
            "0123456789abcdef:fedcba9876543210",
        "Asset ID serialization uses lowercase hexadecimal");

    const std::uint64_t maximum_value =
        (std::numeric_limits<
            std::uint64_t>::max)();

    const AssetId maximum_asset{
        maximum_value,
        maximum_value
    };

    check(
        state,
        to_string(maximum_asset) ==
            "ffffffffffffffff:ffffffffffffffff",
        "Asset ID serializes maximum component values");

    const Result<AssetId> simple_parse_result =
        parse_asset_id(simple_text);

    check(
        state,
        simple_parse_result.has_value(),
        "Parser accepts a serialized asset ID");

    check(
        state,
        simple_parse_result.has_value() &&
            simple_parse_result.value() ==
                valid_asset,
        "Parser restores serialized asset components");

    const Result<AssetId> patterned_parse_result =
        parse_asset_id(patterned_text);

    check(
        state,
        patterned_parse_result.has_value(),
        "Parser accepts hexadecimal alphabetic digits");

    check(
        state,
        patterned_parse_result.has_value() &&
            patterned_parse_result.value() ==
                patterned_asset,
        "Patterned asset ID round-trips exactly");

    const Result<AssetId> uppercase_parse_result =
        parse_asset_id(
            "0123456789ABCDEF:FEDCBA9876543210");

    check(
        state,
        uppercase_parse_result.has_value(),
        "Parser accepts uppercase hexadecimal digits");

    check(
        state,
        uppercase_parse_result.has_value() &&
            uppercase_parse_result.value() ==
                patterned_asset,
        "Uppercase parsing preserves component values");

    const Result<AssetId> maximum_parse_result =
        parse_asset_id(
            "ffffffffffffffff:ffffffffffffffff");

    check(
        state,
        maximum_parse_result.has_value(),
        "Parser accepts maximum component values");

    check(
        state,
        maximum_parse_result.has_value() &&
            maximum_parse_result.value() ==
                maximum_asset,
        "Maximum asset ID round-trips exactly");

    const Result<AssetId> empty_parse_result =
        parse_asset_id("");

    check(
        state,
        !empty_parse_result.has_value(),
        "Parser rejects empty text");

    check(
        state,
        !empty_parse_result.has_value() &&
            empty_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Empty text reports invalid_argument");

    const Result<AssetId> short_parse_result =
        parse_asset_id(
            "0000000000000001:000000000000001");

    check(
        state,
        !short_parse_result.has_value(),
        "Parser rejects a short sequence component");

    check(
        state,
        !short_parse_result.has_value() &&
            short_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Short asset ID reports invalid_argument");

    const Result<AssetId> long_parse_result =
        parse_asset_id(
            "0000000000000001:00000000000000010");

    check(
        state,
        !long_parse_result.has_value(),
        "Parser rejects a long sequence component");

    check(
        state,
        !long_parse_result.has_value() &&
            long_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Long asset ID reports invalid_argument");

    const Result<AssetId> separator_parse_result =
        parse_asset_id(
            "0000000000000001-0000000000000001");

    check(
        state,
        !separator_parse_result.has_value(),
        "Parser rejects an invalid separator");

    check(
        state,
        !separator_parse_result.has_value() &&
            separator_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid separator reports invalid_argument");

    const Result<AssetId> namespace_digit_result =
        parse_asset_id(
            "000000000000000g:0000000000000001");

    check(
        state,
        !namespace_digit_result.has_value(),
        "Parser rejects an invalid catalog digit");

    check(
        state,
        !namespace_digit_result.has_value() &&
            namespace_digit_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid catalog digit reports invalid_argument");

    const Result<AssetId> sequence_digit_result =
        parse_asset_id(
            "0000000000000001:000000000000000g");

    check(
        state,
        !sequence_digit_result.has_value(),
        "Parser rejects an invalid sequence digit");

    check(
        state,
        !sequence_digit_result.has_value() &&
            sequence_digit_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid sequence digit reports invalid_argument");

    const Result<AssetId> zero_namespace_result =
        parse_asset_id(
            "0000000000000000:0000000000000001");

    check(
        state,
        !zero_namespace_result.has_value(),
        "Parser rejects a zero catalog namespace");

    check(
        state,
        !zero_namespace_result.has_value() &&
            zero_namespace_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero parsed catalog reports invalid_argument");

    const Result<AssetId> zero_sequence_result =
        parse_asset_id(
            "0000000000000001:0000000000000000");

    check(
        state,
        !zero_sequence_result.has_value(),
        "Parser rejects a zero asset sequence");

    check(
        state,
        !zero_sequence_result.has_value() &&
            zero_sequence_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero parsed sequence reports invalid_argument");

    AssetIdHash asset_hash{};

    check(
        state,
        asset_hash(valid_asset) ==
            asset_hash(equal_asset),
        "Equal asset IDs produce equal hashes");

    std::unordered_set<AssetId, AssetIdHash>
        identity_set{};

    identity_set.insert(valid_asset);
    identity_set.insert(equal_asset);
    identity_set.insert(later_sequence);
    identity_set.insert(later_namespace);

    check(
        state,
        identity_set.size() == 3U,
        "Asset ID hash storage removes duplicates");

    check(
        state,
        identity_set.contains(valid_asset),
        "Asset ID hash storage finds the first identity");

    check(
        state,
        identity_set.contains(later_sequence),
        "Asset ID hash storage finds a later sequence");

    check(
        state,
        identity_set.contains(later_namespace),
        "Asset ID hash storage finds another catalog");

    check(
        state,
        !identity_set.contains(
            AssetId{
                3ULL,
                1ULL
            }),
        "Asset ID hash storage rejects an absent identity");

    const Result<AssetIdGenerator>
        zero_namespace_generator_result =
            AssetIdGenerator::create(0ULL);

    check(
        state,
        !zero_namespace_generator_result.has_value(),
        "Generator rejects a zero catalog namespace");

    check(
        state,
        !zero_namespace_generator_result.has_value() &&
            zero_namespace_generator_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Zero generator catalog reports invalid_argument");

    const Result<AssetIdGenerator>
        zero_sequence_generator_result =
            AssetIdGenerator::create(
                1ULL,
                0ULL);

    check(
        state,
        !zero_sequence_generator_result.has_value(),
        "Generator rejects a zero first sequence");

    check(
        state,
        !zero_sequence_generator_result.has_value() &&
            zero_sequence_generator_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Zero first sequence reports invalid_argument");

    Result<AssetIdGenerator> generator_result =
        AssetIdGenerator::create(
            0xABCDEFULL);

    check(
        state,
        generator_result.has_value(),
        "Generator accepts a non-zero catalog namespace");

    if (generator_result.has_value())
    {
        AssetIdGenerator generator{
            std::move(
                generator_result.value())
        };

        check(
            state,
            generator.catalog_namespace() ==
                0xABCDEFULL,
            "Generator preserves its catalog namespace");

        check(
            state,
            generator.next_sequence() == 1ULL,
            "Generator begins at the default sequence");

        check(
            state,
            !generator.exhausted(),
            "New generator is not exhausted");

        const Result<AssetId> first_result =
            generator.generate();

        check(
            state,
            first_result.has_value(),
            "Generator creates its first identity");

        check(
            state,
            first_result.has_value() &&
                first_result.value() ==
                    AssetId{
                        0xABCDEFULL,
                        1ULL
                    },
            "First generated asset ID is exact");

        check(
            state,
            generator.next_sequence() == 2ULL,
            "Generator advances after its first identity");

        const Result<AssetId> second_result =
            generator.generate();

        check(
            state,
            second_result.has_value(),
            "Generator creates its second identity");

        check(
            state,
            second_result.has_value() &&
                second_result.value() ==
                    AssetId{
                        0xABCDEFULL,
                        2ULL
                    },
            "Second generated asset ID is exact");

        check(
            state,
            generator.next_sequence() == 3ULL,
            "Generator advances after its second identity");

        check(
            state,
            !generator.exhausted(),
            "Ordinary generation does not exhaust the range");

        AssetIdGenerator copied_generator{
            generator
        };

        check(
            state,
            copied_generator.catalog_namespace() ==
                generator.catalog_namespace(),
            "Copied generator preserves its catalog");

        check(
            state,
            copied_generator.next_sequence() ==
                generator.next_sequence(),
            "Copied generator preserves its next sequence");

        const Result<AssetId> copied_result =
            copied_generator.generate();

        check(
            state,
            copied_result.has_value() &&
                copied_result.value() ==
                    AssetId{
                        0xABCDEFULL,
                        3ULL
                    },
            "Copied generator continues deterministically");

        check(
            state,
            generator.next_sequence() == 3ULL,
            "Copied generation does not mutate the source");
    }

    Result<AssetIdGenerator>
        boundary_generator_result =
            AssetIdGenerator::create(
                99ULL,
                maximum_value - 1ULL);

    check(
        state,
        boundary_generator_result.has_value(),
        "Generator accepts a near-maximum first sequence");

    if (boundary_generator_result.has_value())
    {
        AssetIdGenerator generator{
            std::move(
                boundary_generator_result.value())
        };

        check(
            state,
            generator.next_sequence() ==
                maximum_value - 1ULL,
            "Boundary generator preserves its first sequence");

        const Result<AssetId> penultimate_result =
            generator.generate();

        check(
            state,
            penultimate_result.has_value(),
            "Generator creates the penultimate sequence");

        check(
            state,
            penultimate_result.has_value() &&
                penultimate_result.value() ==
                    AssetId{
                        99ULL,
                        maximum_value - 1ULL
                    },
            "Penultimate generated sequence is exact");

        check(
            state,
            generator.next_sequence() ==
                maximum_value,
            "Generator advances to the maximum sequence");

        check(
            state,
            !generator.exhausted(),
            "Generator remains available before maximum");

        const Result<AssetId> maximum_result =
            generator.generate();

        check(
            state,
            maximum_result.has_value(),
            "Generator creates the maximum sequence");

        check(
            state,
            maximum_result.has_value() &&
                maximum_result.value() ==
                    AssetId{
                        99ULL,
                        maximum_value
                    },
            "Maximum generated sequence is exact");

        check(
            state,
            generator.exhausted(),
            "Generator becomes exhausted after maximum");

        check(
            state,
            generator.next_sequence() ==
                maximum_value,
            "Exhausted generator retains the maximum sequence");

        const Result<AssetId> exhausted_result =
            generator.generate();

        check(
            state,
            !exhausted_result.has_value(),
            "Exhausted generator rejects another identity");

        check(
            state,
            !exhausted_result.has_value() &&
                exhausted_result.error().code ==
                    ErrorCode::invalid_state,
            "Exhausted generator reports invalid_state");
    }

    Result<AssetIdGenerator>
        maximum_generator_result =
            AssetIdGenerator::create(
                101ULL,
                maximum_value);

    check(
        state,
        maximum_generator_result.has_value(),
        "Generator can begin at the maximum sequence");

    if (maximum_generator_result.has_value())
    {
        AssetIdGenerator generator{
            std::move(
                maximum_generator_result.value())
        };

        check(
            state,
            generator.next_sequence() ==
                maximum_value,
            "Maximum-start generator preserves its sequence");

        check(
            state,
            !generator.exhausted(),
            "Maximum-start generator begins available");

        const Result<AssetId> maximum_result =
            generator.generate();

        check(
            state,
            maximum_result.has_value() &&
                maximum_result.value() ==
                    AssetId{
                        101ULL,
                        maximum_value
                    },
            "Maximum-start generator creates one identity");

        check(
            state,
            generator.exhausted(),
            "Maximum-start generator then becomes exhausted");

        const Result<AssetId> exhausted_result =
            generator.generate();

        check(
            state,
            !exhausted_result.has_value(),
            "Maximum-start generator rejects a second identity");

        check(
            state,
            !exhausted_result.has_value() &&
                exhausted_result.error().code ==
                    ErrorCode::invalid_state,
            "Maximum-start exhaustion reports invalid_state");
    }

    std::cout
        << "\nAsset identity test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}