#include "oros/world/entity_id.hpp"

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
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        !invalid_entity_id.is_valid(),
        "Default entity ID is invalid");

    check(
        state,
        !static_cast<bool>(invalid_entity_id),
        "Invalid entity ID converts to false");

    const EntityId missing_namespace{
        0ULL,
        1ULL
    };

    check(
        state,
        !missing_namespace.is_valid(),
        "Entity ID rejects a zero world namespace");

    const EntityId missing_sequence{
        1ULL,
        0ULL
    };

    check(
        state,
        !missing_sequence.is_valid(),
        "Entity ID rejects a zero entity sequence");

    const EntityId valid_id{
        1ULL,
        1ULL
    };

    check(
        state,
        valid_id.is_valid(),
        "Entity ID accepts non-zero components");

    check(
        state,
        static_cast<bool>(valid_id),
        "Valid entity ID converts to true");

    const EntityId equal_id{
        1ULL,
        1ULL
    };

    const EntityId later_sequence{
        1ULL,
        2ULL
    };

    const EntityId later_namespace{
        2ULL,
        1ULL
    };

    check(
        state,
        valid_id == equal_id,
        "Equal entity ID components compare equal");

    check(
        state,
        valid_id != later_sequence,
        "Different entity sequences compare unequal");

    check(
        state,
        valid_id < later_sequence,
        "Entity IDs order by sequence inside a namespace");

    check(
        state,
        later_sequence < later_namespace,
        "Entity IDs order by namespace before sequence");

    const std::string simple_text =
        to_string(valid_id);

    check(
        state,
        simple_text ==
            "0000000000000001:0000000000000001",
        "Entity ID serializes with fixed-width components");

    check(
        state,
        simple_text.size() == 33U,
        "Serialized entity ID contains 33 characters");

    const EntityId patterned_id{
        0x0123456789ABCDEFULL,
        0xFEDCBA9876543210ULL
    };

    const std::string patterned_text =
        to_string(patterned_id);

    check(
        state,
        patterned_text ==
            "0123456789abcdef:fedcba9876543210",
        "Entity ID serialization uses lowercase hexadecimal");

    const EntityId maximum_id{
        (std::numeric_limits<
            std::uint64_t>::max)(),
        (std::numeric_limits<
            std::uint64_t>::max)()
    };

    check(
        state,
        to_string(maximum_id) ==
            "ffffffffffffffff:ffffffffffffffff",
        "Entity ID serializes maximum component values");

    const Result<EntityId> simple_parse_result =
        parse_entity_id(simple_text);

    check(
        state,
        simple_parse_result.has_value(),
        "Parser accepts a serialized entity ID");

    check(
        state,
        simple_parse_result.has_value() &&
            simple_parse_result.value() == valid_id,
        "Parser restores serialized entity ID components");

    const Result<EntityId> patterned_parse_result =
        parse_entity_id(patterned_text);

    check(
        state,
        patterned_parse_result.has_value(),
        "Parser accepts hexadecimal alphabetic digits");

    check(
        state,
        patterned_parse_result.has_value() &&
            patterned_parse_result.value() ==
                patterned_id,
        "Patterned entity ID round-trips exactly");

    const Result<EntityId> uppercase_parse_result =
        parse_entity_id(
            "0123456789ABCDEF:FEDCBA9876543210");

    check(
        state,
        uppercase_parse_result.has_value(),
        "Parser accepts uppercase hexadecimal digits");

    check(
        state,
        uppercase_parse_result.has_value() &&
            uppercase_parse_result.value() ==
                patterned_id,
        "Uppercase parsing preserves component values");

    const Result<EntityId> maximum_parse_result =
        parse_entity_id(
            "ffffffffffffffff:ffffffffffffffff");

    check(
        state,
        maximum_parse_result.has_value(),
        "Parser accepts maximum component values");

    check(
        state,
        maximum_parse_result.has_value() &&
            maximum_parse_result.value() ==
                maximum_id,
        "Maximum entity ID round-trips exactly");

    const Result<EntityId> empty_parse_result =
        parse_entity_id("");

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

    const Result<EntityId> short_parse_result =
        parse_entity_id(
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
        "Short entity ID reports invalid_argument");

    const Result<EntityId> long_parse_result =
        parse_entity_id(
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
        "Long entity ID reports invalid_argument");

    const Result<EntityId> separator_parse_result =
        parse_entity_id(
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

    const Result<EntityId> namespace_digit_result =
        parse_entity_id(
            "000000000000000g:0000000000000001");

    check(
        state,
        !namespace_digit_result.has_value(),
        "Parser rejects an invalid namespace digit");

    check(
        state,
        !namespace_digit_result.has_value() &&
            namespace_digit_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid namespace digit reports invalid_argument");

    const Result<EntityId> sequence_digit_result =
        parse_entity_id(
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

    const Result<EntityId> zero_namespace_result =
        parse_entity_id(
            "0000000000000000:0000000000000001");

    check(
        state,
        !zero_namespace_result.has_value(),
        "Parser rejects a zero world namespace");

    check(
        state,
        !zero_namespace_result.has_value() &&
            zero_namespace_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero parsed namespace reports invalid_argument");

    const Result<EntityId> zero_sequence_result =
        parse_entity_id(
            "0000000000000001:0000000000000000");

    check(
        state,
        !zero_sequence_result.has_value(),
        "Parser rejects a zero entity sequence");

    check(
        state,
        !zero_sequence_result.has_value() &&
            zero_sequence_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero parsed sequence reports invalid_argument");

    std::unordered_set<EntityId, EntityIdHash>
        identity_set{};

    identity_set.insert(valid_id);
    identity_set.insert(equal_id);
    identity_set.insert(later_sequence);
    identity_set.insert(later_namespace);

    check(
        state,
        identity_set.size() == 3U,
        "Entity ID hash storage removes duplicates");

    check(
        state,
        identity_set.contains(valid_id),
        "Entity ID hash storage finds the first identity");

    check(
        state,
        identity_set.contains(later_sequence),
        "Entity ID hash storage finds a later sequence");

    check(
        state,
        identity_set.contains(later_namespace),
        "Entity ID hash storage finds another namespace");

    check(
        state,
        !identity_set.contains(
            EntityId{3ULL, 1ULL}),
        "Entity ID hash storage rejects an absent identity");

    const Result<EntityIdGenerator>
        zero_namespace_generator_result =
            EntityIdGenerator::create(0ULL);

    check(
        state,
        !zero_namespace_generator_result.has_value(),
        "Generator rejects a zero world namespace");

    check(
        state,
        !zero_namespace_generator_result.has_value() &&
            zero_namespace_generator_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Zero generator namespace reports invalid_argument");

    const Result<EntityIdGenerator>
        zero_sequence_generator_result =
            EntityIdGenerator::create(
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

    Result<EntityIdGenerator> generator_result =
        EntityIdGenerator::create(
            0xABCDEFULL);

    check(
        state,
        generator_result.has_value(),
        "Generator accepts a non-zero namespace");

    if (generator_result.has_value())
    {
        EntityIdGenerator generator{
            std::move(generator_result.value())
        };

        check(
            state,
            generator.world_namespace() ==
                0xABCDEFULL,
            "Generator preserves its world namespace");

        check(
            state,
            !generator.is_exhausted(),
            "New generator is not exhausted");

        const Result<EntityId> first_result =
            generator.generate();

        check(
            state,
            first_result.has_value(),
            "Generator creates its first identity");

        check(
            state,
            first_result.has_value() &&
                first_result.value() ==
                    EntityId{
                        0xABCDEFULL,
                        1ULL
                    },
            "Generator begins with the default sequence");

        const Result<EntityId> second_result =
            generator.generate();

        check(
            state,
            second_result.has_value(),
            "Generator creates its second identity");

        check(
            state,
            second_result.has_value() &&
                second_result.value() ==
                    EntityId{
                        0xABCDEFULL,
                        2ULL
                    },
            "Generator increments the entity sequence");

        check(
            state,
            !generator.is_exhausted(),
            "Ordinary generation does not exhaust the range");
    }

    const std::uint64_t maximum_sequence =
        (std::numeric_limits<
            std::uint64_t>::max)();

    Result<EntityIdGenerator>
        boundary_generator_result =
            EntityIdGenerator::create(
                99ULL,
                maximum_sequence - 1ULL);

    check(
        state,
        boundary_generator_result.has_value(),
        "Generator accepts a near-maximum first sequence");

    if (boundary_generator_result.has_value())
    {
        EntityIdGenerator generator{
            std::move(
                boundary_generator_result.value())
        };

        const Result<EntityId> penultimate_result =
            generator.generate();

        check(
            state,
            penultimate_result.has_value(),
            "Generator creates the penultimate sequence");

        check(
            state,
            penultimate_result.has_value() &&
                penultimate_result.value() ==
                    EntityId{
                        99ULL,
                        maximum_sequence - 1ULL
                    },
            "Penultimate generated sequence is exact");

        check(
            state,
            !generator.is_exhausted(),
            "Generator remains available before maximum");

        const Result<EntityId> maximum_result =
            generator.generate();

        check(
            state,
            maximum_result.has_value(),
            "Generator creates the maximum sequence");

        check(
            state,
            maximum_result.has_value() &&
                maximum_result.value() ==
                    EntityId{
                        99ULL,
                        maximum_sequence
                    },
            "Maximum generated sequence is exact");

        check(
            state,
            generator.is_exhausted(),
            "Generator becomes exhausted after maximum");

        const Result<EntityId> exhausted_result =
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

    Result<EntityIdGenerator>
        maximum_generator_result =
            EntityIdGenerator::create(
                101ULL,
                maximum_sequence);

    check(
        state,
        maximum_generator_result.has_value(),
        "Generator can begin at the maximum sequence");

    if (maximum_generator_result.has_value())
    {
        EntityIdGenerator generator{
            std::move(
                maximum_generator_result.value())
        };

        check(
            state,
            !generator.is_exhausted(),
            "Maximum-start generator begins available");

        const Result<EntityId> maximum_result =
            generator.generate();

        check(
            state,
            maximum_result.has_value() &&
                maximum_result.value() ==
                    EntityId{
                        101ULL,
                        maximum_sequence
                    },
            "Maximum-start generator creates one identity");

        check(
            state,
            generator.is_exhausted(),
            "Maximum-start generator then becomes exhausted");

        const Result<EntityId> exhausted_result =
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
        << "\nEntity identity test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}