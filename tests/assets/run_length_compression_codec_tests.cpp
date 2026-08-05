#include "oros/assets/run_length_compression_codec.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <vector>

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
    oros::assets::AssetCompressionRequest
    make_compression_request(
        const std::vector<std::byte>& bytes)
    {
        return {
            std::span<const std::byte>{
                bytes
            }
        };
    }

    [[nodiscard]]
    oros::assets::AssetDecompressionRequest
    make_decompression_request(
        const std::vector<std::byte>& bytes,
        const std::size_t expected_byte_count)
    {
        return {
            std::span<const std::byte>{
                bytes
            },
            expected_byte_count
        };
    }

    static_assert(
        std::is_final_v<
            oros::assets::
                RunLengthCompressionCodec>);

    static_assert(
        std::is_base_of_v<
            oros::assets::
                AssetCompressionCodec,
            oros::assets::
                RunLengthCompressionCodec>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                RunLengthCompressionCodec>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                RunLengthCompressionCodec>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                RunLengthCompressionCodec>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                RunLengthCompressionCodec>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const RunLengthCompressionCodec
        codec{};

    check(
        state,
        codec.name() ==
            "oros.rle",
        "Run-length codec exposes its stable name");

    check(
        state,
        codec.version() == 1U,
        "Run-length codec exposes version one");

    const std::vector<std::byte>
        empty_source{};

    const Result<AssetCompressionResult>
        empty_compression =
            codec.compress(
                make_compression_request(
                    empty_source));

    check(
        state,
        empty_compression.has_value(),
        "Run-length codec accepts empty input");

    check(
        state,
        empty_compression.has_value() &&
            empty_compression.value().
                is_valid(),
        "Empty compression produces a valid result");

    check(
        state,
        empty_compression.has_value() &&
            empty_compression.value().
                original_byte_count == 0U,
        "Empty compression records zero source bytes");

    check(
        state,
        empty_compression.has_value() &&
            empty_compression.value().
                bytes.empty(),
        "Empty compression produces no encoded pairs");

    const Result<std::vector<std::byte>>
        empty_decompression =
            codec.decompress(
                make_decompression_request(
                    empty_source,
                    0U));

    check(
        state,
        empty_decompression.has_value(),
        "Run-length codec accepts empty encoded input");

    check(
        state,
        empty_decompression.has_value() &&
            empty_decompression.value().
                empty(),
        "Empty encoded input produces empty output");

    const std::vector<std::byte>
        single_source{
            std::byte{0x7A}
        };

    const Result<AssetCompressionResult>
        single_compression =
            codec.compress(
                make_compression_request(
                    single_source));

    check(
        state,
        single_compression.has_value(),
        "Run-length codec compresses one byte");

    check(
        state,
        single_compression.has_value() &&
            single_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0x01},
                    std::byte{0x7A}
                },
        "One byte produces one count-value pair");

    check(
        state,
        single_compression.has_value() &&
            single_compression.value().
                original_byte_count == 1U,
        "Single-byte compression records its source size");

    const Result<std::vector<std::byte>>
        single_decompression =
            codec.decompress(
                make_decompression_request(
                    single_compression.value().
                        bytes,
                    single_source.size()));

    check(
        state,
        single_decompression.has_value(),
        "Run-length codec decompresses one pair");

    check(
        state,
        single_decompression.has_value() &&
            single_decompression.value() ==
                single_source,
        "One-pair round trip restores the source byte");

    const std::vector<std::byte>
        repeated_source{
            std::byte{0x42},
            std::byte{0x42},
            std::byte{0x42},
            std::byte{0x42}
        };

    const Result<AssetCompressionResult>
        repeated_compression =
            codec.compress(
                make_compression_request(
                    repeated_source));

    check(
        state,
        repeated_compression.has_value(),
        "Run-length codec compresses a repeated run");

    check(
        state,
        repeated_compression.has_value() &&
            repeated_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0x04},
                    std::byte{0x42}
                },
        "Repeated bytes collapse into one pair");

    check(
        state,
        repeated_compression.has_value() &&
            repeated_compression.value().
                bytes.size() <
                repeated_source.size(),
        "Repeated run produces smaller encoded data");

    const Result<std::vector<std::byte>>
        repeated_decompression =
            codec.decompress(
                make_decompression_request(
                    repeated_compression.value().
                        bytes,
                    repeated_source.size()));

    check(
        state,
        repeated_decompression.has_value() &&
            repeated_decompression.value() ==
                repeated_source,
        "Repeated run round trip restores every byte");

    const std::vector<std::byte>
        mixed_source{
            std::byte{0x10},
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x30},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x40}
        };

    const std::vector<std::byte>
        expected_mixed_encoding{
            std::byte{0x02},
            std::byte{0x10},
            std::byte{0x01},
            std::byte{0x20},
            std::byte{0x03},
            std::byte{0x30},
            std::byte{0x02},
            std::byte{0x40}
        };

    const Result<AssetCompressionResult>
        mixed_compression =
            codec.compress(
                make_compression_request(
                    mixed_source));

    check(
        state,
        mixed_compression.has_value(),
        "Run-length codec compresses mixed runs");

    check(
        state,
        mixed_compression.has_value() &&
            mixed_compression.value().
                bytes ==
                expected_mixed_encoding,
        "Mixed runs produce the expected pair sequence");

    check(
        state,
        mixed_compression.has_value() &&
            mixed_compression.value().
                original_byte_count ==
                mixed_source.size(),
        "Mixed compression records the source byte count");

    const Result<std::vector<std::byte>>
        mixed_decompression =
            codec.decompress(
                make_decompression_request(
                    expected_mixed_encoding,
                    mixed_source.size()));

    check(
        state,
        mixed_decompression.has_value(),
        "Run-length codec decompresses mixed runs");

    check(
        state,
        mixed_decompression.has_value() &&
            mixed_decompression.value() ==
                mixed_source,
        "Mixed run round trip restores every byte");

    const std::vector<std::byte>
        alternating_source{
            std::byte{0x00},
            std::byte{0xFF},
            std::byte{0x00},
            std::byte{0xFF}
        };

    const Result<AssetCompressionResult>
        alternating_compression =
            codec.compress(
                make_compression_request(
                    alternating_source));

    check(
        state,
        alternating_compression.has_value(),
        "Run-length codec accepts incompressible data");

    check(
        state,
        alternating_compression.has_value() &&
            alternating_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0x01},
                    std::byte{0x00},
                    std::byte{0x01},
                    std::byte{0xFF},
                    std::byte{0x01},
                    std::byte{0x00},
                    std::byte{0x01},
                    std::byte{0xFF}
                },
        "Alternating bytes become individual runs");

    check(
        state,
        alternating_compression.has_value() &&
            alternating_compression.value().
                bytes.size() ==
                alternating_source.size() *
                    2U,
        "Individual runs use two encoded bytes each");

    const Result<std::vector<std::byte>>
        alternating_decompression =
            codec.decompress(
                make_decompression_request(
                    alternating_compression.value().
                        bytes,
                    alternating_source.size()));

    check(
        state,
        alternating_decompression.has_value() &&
            alternating_decompression.value() ==
                alternating_source,
        "Incompressible data still round trips losslessly");

    const std::vector<std::byte>
        run_255(
            255U,
            std::byte{0xA1});

    const Result<AssetCompressionResult>
        run_255_compression =
            codec.compress(
                make_compression_request(
                    run_255));

    check(
        state,
        run_255_compression.has_value(),
        "Run-length codec compresses a 255-byte run");

    check(
        state,
        run_255_compression.has_value() &&
            run_255_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0xFF},
                    std::byte{0xA1}
                },
        "A 255-byte run fits in one encoded pair");

    const Result<std::vector<std::byte>>
        run_255_decompression =
            codec.decompress(
                make_decompression_request(
                    run_255_compression.value().
                        bytes,
                    run_255.size()));

    check(
        state,
        run_255_decompression.has_value() &&
            run_255_decompression.value() ==
                run_255,
        "A 255-byte run round trips losslessly");

    const std::vector<std::byte>
        run_256(
            256U,
            std::byte{0xB2});

    const Result<AssetCompressionResult>
        run_256_compression =
            codec.compress(
                make_compression_request(
                    run_256));

    check(
        state,
        run_256_compression.has_value(),
        "Run-length codec compresses a 256-byte run");

    check(
        state,
        run_256_compression.has_value() &&
            run_256_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0xFF},
                    std::byte{0xB2},
                    std::byte{0x01},
                    std::byte{0xB2}
                },
        "A 256-byte run splits at the format limit");

    const Result<std::vector<std::byte>>
        run_256_decompression =
            codec.decompress(
                make_decompression_request(
                    run_256_compression.value().
                        bytes,
                    run_256.size()));

    check(
        state,
        run_256_decompression.has_value() &&
            run_256_decompression.value() ==
                run_256,
        "A split 256-byte run round trips losslessly");

    const std::vector<std::byte>
        run_600(
            600U,
            std::byte{0xC3});

    const Result<AssetCompressionResult>
        run_600_compression =
            codec.compress(
                make_compression_request(
                    run_600));

    check(
        state,
        run_600_compression.has_value(),
        "Run-length codec compresses a 600-byte run");

    check(
        state,
        run_600_compression.has_value() &&
            run_600_compression.value().
                bytes ==
                std::vector<std::byte>{
                    std::byte{0xFF},
                    std::byte{0xC3},
                    std::byte{0xFF},
                    std::byte{0xC3},
                    std::byte{0x5A},
                    std::byte{0xC3}
                },
        "A 600-byte run splits into 255, 255, and 90");

    const Result<std::vector<std::byte>>
        run_600_decompression =
            codec.decompress(
                make_decompression_request(
                    run_600_compression.value().
                        bytes,
                    run_600.size()));

    check(
        state,
        run_600_decompression.has_value() &&
            run_600_decompression.value() ==
                run_600,
        "A multi-pair long run round trips losslessly");

    const std::vector<std::byte>
        binary_source{
            std::byte{0x00},
            std::byte{0x00},
            std::byte{0x7F},
            std::byte{0x80},
            std::byte{0x80},
            std::byte{0xFF},
            std::byte{0xFF},
            std::byte{0xFF}
        };

    const Result<AssetCompressionResult>
        binary_compression =
            codec.compress(
                make_compression_request(
                    binary_source));

    check(
        state,
        binary_compression.has_value(),
        "Run-length codec accepts arbitrary binary values");

    const Result<std::vector<std::byte>>
        binary_decompression =
            codec.decompress(
                make_decompression_request(
                    binary_compression.value().
                        bytes,
                    binary_source.size()));

    check(
        state,
        binary_decompression.has_value() &&
            binary_decompression.value() ==
                binary_source,
        "Binary values round trip without sign conversion");

    const Result<AssetCompressionResult>
        repeated_mixed_compression =
            codec.compress(
                make_compression_request(
                    mixed_source));

    check(
        state,
        repeated_mixed_compression.has_value(),
        "Run-length codec accepts repeated compression");

    check(
        state,
        repeated_mixed_compression.has_value() &&
            mixed_compression.has_value() &&
            repeated_mixed_compression.value().
                bytes ==
                mixed_compression.value().
                    bytes,
        "Run-length compression is deterministic");

    check(
        state,
        mixed_source ==
            std::vector<std::byte>{
                std::byte{0x10},
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x30},
                std::byte{0x30},
                std::byte{0x40},
                std::byte{0x40}
            },
        "Compression preserves source storage");

    const std::vector<std::byte>
        odd_encoded_data{
            std::byte{0x02},
            std::byte{0x44},
            std::byte{0x01}
        };

    const Result<std::vector<std::byte>>
        odd_encoded_result =
            codec.decompress(
                make_decompression_request(
                    odd_encoded_data,
                    3U));

    check(
        state,
        !odd_encoded_result.has_value(),
        "Run-length codec rejects an incomplete pair");

    check(
        state,
        !odd_encoded_result.has_value() &&
            odd_encoded_result.error().code ==
                ErrorCode::invalid_argument,
        "Incomplete pair reports invalid_argument");

    const std::vector<std::byte>
        zero_run_data{
            std::byte{0x00},
            std::byte{0x55}
        };

    const Result<std::vector<std::byte>>
        zero_run_result =
            codec.decompress(
                make_decompression_request(
                    zero_run_data,
                    0U));

    check(
        state,
        !zero_run_result.has_value(),
        "Run-length codec rejects a zero-length run");

    check(
        state,
        !zero_run_result.has_value() &&
            zero_run_result.error().code ==
                ErrorCode::invalid_argument,
        "Zero-length run reports invalid_argument");

    const std::vector<std::byte>
        encoded_four_bytes{
            std::byte{0x04},
            std::byte{0x77}
        };

    const Result<std::vector<std::byte>>
        expected_too_small_result =
            codec.decompress(
                make_decompression_request(
                    encoded_four_bytes,
                    3U));

    check(
        state,
        !expected_too_small_result.has_value(),
        "Run-length codec rejects expansion beyond the expected size");

    check(
        state,
        !expected_too_small_result.
                has_value() &&
            expected_too_small_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Oversized expansion reports invalid_argument");

    const Result<std::vector<std::byte>>
        expected_too_large_result =
            codec.decompress(
                make_decompression_request(
                    encoded_four_bytes,
                    5U));

    check(
        state,
        !expected_too_large_result.has_value(),
        "Run-length codec rejects output shorter than expected");

    check(
        state,
        !expected_too_large_result.
                has_value() &&
            expected_too_large_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Undersized expansion reports invalid_argument");

    const AssetDecompressionRequest
        empty_nonzero_request{
            {},
            4U
        };

    check(
        state,
        !empty_nonzero_request.is_valid(),
        "Empty encoded input with nonzero output is invalid");

    const Result<std::vector<std::byte>>
        empty_nonzero_result =
            codec.decompress(
                empty_nonzero_request);

    check(
        state,
        !empty_nonzero_result.has_value(),
        "Run-length codec rejects an invalid empty request");

    check(
        state,
        !empty_nonzero_result.has_value() &&
            empty_nonzero_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid empty request reports invalid_argument");

    const AssetDecompressionRequest
        nonempty_zero_expected_request =
            make_decompression_request(
                encoded_four_bytes,
                0U);

    check(
        state,
        nonempty_zero_expected_request.
            is_valid(),
        "Nonempty encoded data with zero expected bytes is structurally valid");

    const Result<std::vector<std::byte>>
        nonempty_zero_expected_result =
            codec.decompress(
                nonempty_zero_expected_request);

    check(
        state,
        !nonempty_zero_expected_result.
            has_value(),
        "Run-length codec rejects nonempty expansion when zero bytes are expected");

    check(
        state,
        !nonempty_zero_expected_result.
                has_value() &&
            nonempty_zero_expected_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Unexpected nonempty expansion reports invalid_argument");

    std::cout
        << "\nRun-length compression codec test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}