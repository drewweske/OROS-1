#include "oros/assets/content_hash.hpp"

#include <array>
#include <cstddef>
#include <iostream>
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

    [[nodiscard]]
    bool hash_matches(
        const oros::assets::ContentHash& hash,
        const std::string_view expected)
    {
        return
            oros::assets::to_string(hash) ==
            expected;
    }
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    check(
        state,
        content_hash_byte_count == 32U,
        "Content hashes contain 32 bytes");

    check(
        state,
        content_hash_text_length == 64U,
        "Serialized content hashes contain 64 digits");

    const ContentHash empty_hash =
        hash_text("");

    check(
        state,
        hash_matches(
            empty_hash,
            "e3b0c44298fc1c149afbf4c8996fb924"
            "27ae41e4649b934ca495991b7852b855"),
        "Empty input matches the SHA-256 reference vector");

    const ContentHash abc_hash =
        hash_text("abc");

    check(
        state,
        hash_matches(
            abc_hash,
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad"),
        "abc matches the SHA-256 reference vector");

    const ContentHash quick_brown_fox_hash =
        hash_text(
            "The quick brown fox jumps over the lazy dog");

    check(
        state,
        hash_matches(
            quick_brown_fox_hash,
            "d7a8fbb307d7809469ca9abcb0082e4f"
            "8d5651e46d3cdb762d02d0bf37c9e592"),
        "Quick brown fox text matches its reference vector");

    const ContentHash quick_brown_fox_period_hash =
        hash_text(
            "The quick brown fox jumps over the lazy dog.");

    check(
        state,
        hash_matches(
            quick_brown_fox_period_hash,
            "ef537f25c895bfa782526529a9b63d97"
            "aa631564d5d789c2b765448c8635fb6c"),
        "A one-character change produces the reference hash");

    check(
        state,
        quick_brown_fox_hash !=
            quick_brown_fox_period_hash,
        "Different content produces different hashes");

    const std::string fifty_five_as(
        55U,
        'a');

    const std::string fifty_six_as(
        56U,
        'a');

    const std::string sixty_three_as(
        63U,
        'a');

    const std::string sixty_four_as(
        64U,
        'a');

    const std::string sixty_five_as(
        65U,
        'a');

    check(
        state,
        hash_matches(
            hash_text(fifty_five_as),
            "9f4390f8d30c2dd92ec9f095b65e2b9a"
            "e9b0a925a5258e241c9f1e910f734318"),
        "55-byte input uses one final SHA-256 block");

    check(
        state,
        hash_matches(
            hash_text(fifty_six_as),
            "b35439a4ac6f0948b6d6f9e3c6af0f5"
            "f590ce20f1bde7090ef7970686ec6738a"),
        "56-byte input uses two final SHA-256 blocks");

    check(
        state,
        hash_matches(
            hash_text(sixty_three_as),
            "7d3e74a05d7db15bce4ad9ec0658ea98"
            "e3f06eeecf16b4c6fff2da457ddc2f34"),
        "63-byte input handles the padding boundary");

    check(
        state,
        hash_matches(
            hash_text(sixty_four_as),
            "ffe054fe7ae0cb6dc65c3af9b61d5209"
            "f439851db43d0ba5997337df154668eb"),
        "64-byte input processes one complete data block");

    check(
        state,
        hash_matches(
            hash_text(sixty_five_as),
            "635361c48bb9eab14198e76ea8ab7f1a"
            "41685d6ad62aa9146d301d4f17eb0ae0"),
        "65-byte input processes data across two blocks");

    const std::string million_as(
        1'000'000U,
        'a');

    check(
        state,
        hash_matches(
            hash_text(million_as),
            "cdc76e5c9914fb9281a1c7e284d73e67"
            "f1809a48a497200e046d39ccc7112cd0"),
        "One million characters match the SHA-256 vector");

    const std::array<std::byte, 6U>
        binary_payload{
            std::byte{0x00U},
            std::byte{0x01U},
            std::byte{0x02U},
            std::byte{0xFFU},
            std::byte{0x80U},
            std::byte{0x7FU}
        };

    const ContentHash binary_hash =
        hash_bytes(binary_payload);

    check(
        state,
        hash_matches(
            binary_hash,
            "76d3f9e801976304b3d22e470369c39a"
            "1b01b044016366d49222f641c66f7176"),
        "Binary input hashes without text conversion");

    const std::array<std::byte, 0U>
        empty_bytes{};

    check(
        state,
        hash_bytes(empty_bytes) ==
            empty_hash,
        "Empty binary and empty text inputs hash equally");

    const ContentHash repeated_abc_hash =
        hash_text("abc");

    check(
        state,
        repeated_abc_hash ==
            abc_hash,
        "Repeated hashing is deterministic");

    const std::string abc_text =
        to_string(abc_hash);

    check(
        state,
        abc_text.size() ==
            content_hash_text_length,
        "Serialized hash has the required length");

    check(
        state,
        abc_text ==
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad",
        "Hash serialization uses lowercase hexadecimal");

    const Result<ContentHash> abc_parse_result =
        parse_content_hash(abc_text);

    check(
        state,
        abc_parse_result.has_value(),
        "Parser accepts a serialized content hash");

    check(
        state,
        abc_parse_result.has_value() &&
            abc_parse_result.value() ==
                abc_hash,
        "Parsed content hash restores every byte");

    const Result<ContentHash>
        uppercase_parse_result =
            parse_content_hash(
                "BA7816BF8F01CFEA414140DE5DAE2223"
                "B00361A396177A9CB410FF61F20015AD");

    check(
        state,
        uppercase_parse_result.has_value(),
        "Parser accepts uppercase hexadecimal digits");

    check(
        state,
        uppercase_parse_result.has_value() &&
            uppercase_parse_result.value() ==
                abc_hash,
        "Uppercase parsing preserves the hash value");

    const Result<ContentHash>
        mixed_case_parse_result =
            parse_content_hash(
                "Ba7816bF8f01CfEa414140de5dae2223"
                "B00361a396177A9Cb410fF61F20015aD");

    check(
        state,
        mixed_case_parse_result.has_value(),
        "Parser accepts mixed-case hexadecimal digits");

    check(
        state,
        mixed_case_parse_result.has_value() &&
            mixed_case_parse_result.value() ==
                abc_hash,
        "Mixed-case parsing preserves the hash value");

    const Result<ContentHash> empty_parse_result =
        parse_content_hash("");

    check(
        state,
        !empty_parse_result.has_value(),
        "Parser rejects empty content-hash text");

    check(
        state,
        !empty_parse_result.has_value() &&
            empty_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Empty content-hash text reports invalid_argument");

    const Result<ContentHash> short_parse_result =
        parse_content_hash(
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015a");

    check(
        state,
        !short_parse_result.has_value(),
        "Parser rejects a 63-digit content hash");

    check(
        state,
        !short_parse_result.has_value() &&
            short_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Short content hash reports invalid_argument");

    const Result<ContentHash> long_parse_result =
        parse_content_hash(
            "ba7816bf8f01cfea414140de5dae2223"
            "b00361a396177a9cb410ff61f20015ad0");

    check(
        state,
        !long_parse_result.has_value(),
        "Parser rejects a 65-digit content hash");

    check(
        state,
        !long_parse_result.has_value() &&
            long_parse_result.error().code ==
                ErrorCode::invalid_argument,
        "Long content hash reports invalid_argument");

    const Result<ContentHash>
        invalid_high_digit_result =
            parse_content_hash(
                "ga7816bf8f01cfea414140de5dae2223"
                "b00361a396177a9cb410ff61f20015ad");

    check(
        state,
        !invalid_high_digit_result.has_value(),
        "Parser rejects an invalid high hexadecimal digit");

    check(
        state,
        !invalid_high_digit_result.has_value() &&
            invalid_high_digit_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid high digit reports invalid_argument");

    const Result<ContentHash>
        invalid_low_digit_result =
            parse_content_hash(
                "bg7816bf8f01cfea414140de5dae2223"
                "b00361a396177a9cb410ff61f20015ad");

    check(
        state,
        !invalid_low_digit_result.has_value(),
        "Parser rejects an invalid low hexadecimal digit");

    check(
        state,
        !invalid_low_digit_result.has_value() &&
            invalid_low_digit_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid low digit reports invalid_argument");

    const ContentHashHash hash_function{};

    check(
        state,
        hash_function(abc_hash) ==
            hash_function(repeated_abc_hash),
        "Equal content hashes produce equal container hashes");

    std::unordered_set<
        ContentHash,
        ContentHashHash
    > hash_set{};

    hash_set.insert(empty_hash);
    hash_set.insert(abc_hash);
    hash_set.insert(repeated_abc_hash);
    hash_set.insert(quick_brown_fox_hash);

    check(
        state,
        hash_set.size() == 3U,
        "Content-hash storage removes duplicate values");

    check(
        state,
        hash_set.contains(empty_hash),
        "Content-hash storage finds the empty-input hash");

    check(
        state,
        hash_set.contains(abc_hash),
        "Content-hash storage finds the abc hash");

    check(
        state,
        hash_set.contains(
            quick_brown_fox_hash),
        "Content-hash storage finds a longer-input hash");

    check(
        state,
        !hash_set.contains(
            quick_brown_fox_period_hash),
        "Content-hash storage rejects an absent hash");

    ContentHash zero_hash{};

    check(
        state,
        to_string(zero_hash) ==
            "00000000000000000000000000000000"
            "00000000000000000000000000000000",
        "A zero-filled content hash serializes exactly");

    const Result<ContentHash> zero_parse_result =
        parse_content_hash(
            "00000000000000000000000000000000"
            "00000000000000000000000000000000");

    check(
        state,
        zero_parse_result.has_value(),
        "Parser accepts a zero-filled content hash");

    check(
        state,
        zero_parse_result.has_value() &&
            zero_parse_result.value() ==
                zero_hash,
        "Zero-filled content hash round-trips exactly");

    std::cout
        << "\nContent hash test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}