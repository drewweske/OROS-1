#include "oros/assets/asset_compression_codec.hpp"

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

    class PassthroughCompressionCodec final
        : public oros::assets::
              AssetCompressionCodec
    {
    public:
        [[nodiscard]]
        std::string_view
        name() const noexcept override
        {
            return
                "oros.test.passthrough";
        }

        [[nodiscard]]
        std::uint32_t
        version() const noexcept override
        {
            return 1U;
        }

        [[nodiscard]]
        oros::foundation::Result<
            oros::assets::
                AssetCompressionResult>
        compress(
            const oros::assets::
                AssetCompressionRequest&
                    request) const override
        {
            using namespace oros::assets;

            if (!request.is_valid())
            {
                return oros::foundation::fail(
                    oros::foundation::ErrorCode::
                        invalid_argument,
                    "The compression request is "
                    "invalid.");
            }

            AssetCompressionResult result{};

            result.bytes.assign(
                request.source_bytes.begin(),
                request.source_bytes.end());

            result.original_byte_count =
                request.source_bytes.size();

            return result;
        }

        [[nodiscard]]
        oros::foundation::Result<
            std::vector<std::byte>>
        decompress(
            const oros::assets::
                AssetDecompressionRequest&
                    request) const override
        {
            if (!request.is_valid())
            {
                return oros::foundation::fail(
                    oros::foundation::ErrorCode::
                        invalid_argument,
                    "The decompression request is "
                    "invalid.");
            }

            if (request.compressed_bytes.size() !=
                request.expected_byte_count)
            {
                return oros::foundation::fail(
                    oros::foundation::ErrorCode::
                        invalid_argument,
                    "The passthrough payload size "
                    "does not match the expected "
                    "byte count.");
            }

            return std::vector<std::byte>{
                request.compressed_bytes.begin(),
                request.compressed_bytes.end()
            };
        }
    };

    static_assert(
        std::is_abstract_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        std::has_virtual_destructor_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                AssetCompressionCodec>);

    static_assert(
        std::is_final_v<
            PassthroughCompressionCodec>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const AssetCompressionRequest
        default_compression_request{};

    check(
        state,
        default_compression_request.is_valid(),
        "Default compression request is valid");

    check(
        state,
        default_compression_request.
            source_bytes.empty(),
        "Default compression request has no source bytes");

    const std::vector<std::byte>
        source_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

    const AssetCompressionRequest
        populated_compression_request{
            std::span<const std::byte>{
                source_bytes
            }
        };

    check(
        state,
        populated_compression_request.
            is_valid(),
        "Populated compression request is valid");

    check(
        state,
        populated_compression_request.
            source_bytes.size() ==
            source_bytes.size(),
        "Compression request preserves its byte count");

    check(
        state,
        populated_compression_request.
            source_bytes.front() ==
            source_bytes.front(),
        "Compression request preserves its first byte");

    check(
        state,
        populated_compression_request.
            source_bytes.back() ==
            source_bytes.back(),
        "Compression request preserves its final byte");

    const AssetDecompressionRequest
        default_decompression_request{};

    check(
        state,
        default_decompression_request.
            is_valid(),
        "Default decompression request is valid");

    check(
        state,
        default_decompression_request.
            compressed_bytes.empty(),
        "Default decompression request has no compressed bytes");

    check(
        state,
        default_decompression_request.
            expected_byte_count == 0U,
        "Default decompression request expects zero bytes");

    const AssetDecompressionRequest
        invalid_empty_decompression_request{
            {},
            4U
        };

    check(
        state,
        !invalid_empty_decompression_request.
            is_valid(),
        "Empty compressed data cannot represent non-empty output");

    const AssetDecompressionRequest
        populated_decompression_request{
            std::span<const std::byte>{
                source_bytes
            },
            source_bytes.size()
        };

    check(
        state,
        populated_decompression_request.
            is_valid(),
        "Populated decompression request is valid");

    check(
        state,
        populated_decompression_request.
            expected_byte_count ==
            source_bytes.size(),
        "Decompression request preserves its expected size");

    const AssetCompressionResult
        default_compression_result{};

    check(
        state,
        default_compression_result.is_valid(),
        "Default compression result is valid");

    check(
        state,
        !default_compression_result.
            has_data(),
        "Default compression result has no data");

    check(
        state,
        default_compression_result.
            original_byte_count == 0U,
        "Default compression result represents empty input");

    AssetCompressionResult
        invalid_compression_result{};

    invalid_compression_result.
        original_byte_count = 4U;

    check(
        state,
        !invalid_compression_result.
            is_valid(),
        "Empty compressed output cannot represent non-empty input");

    AssetCompressionResult
        encoded_empty_result{};

    encoded_empty_result.bytes = {
        std::byte{0x00}
    };

    check(
        state,
        encoded_empty_result.is_valid(),
        "A codec may encode empty input with format data");

    check(
        state,
        encoded_empty_result.has_data(),
        "Encoded empty input reports compressed data");

    check(
        state,
        encoded_empty_result.
            original_byte_count == 0U,
        "Encoded empty input preserves its original size");

    const PassthroughCompressionCodec
        codec{};

    check(
        state,
        codec.name() ==
            "oros.test.passthrough",
        "Compression codec exposes a stable name");

    check(
        state,
        codec.version() == 1U,
        "Compression codec exposes a nonzero version");

    const Result<AssetCompressionResult>
        empty_compression_result =
            codec.compress(
                default_compression_request);

    check(
        state,
        empty_compression_result.has_value(),
        "Codec accepts empty compression input");

    check(
        state,
        empty_compression_result.has_value() &&
            empty_compression_result.value().
                is_valid(),
        "Empty compression produces a valid result");

    check(
        state,
        empty_compression_result.has_value() &&
            empty_compression_result.value().
                original_byte_count == 0U,
        "Empty compression records zero original bytes");

    check(
        state,
        empty_compression_result.has_value() &&
            !empty_compression_result.value().
                has_data(),
        "Passthrough empty compression produces no data");

    const Result<std::vector<std::byte>>
        empty_decompression_result =
            codec.decompress(
                default_decompression_request);

    check(
        state,
        empty_decompression_result.has_value(),
        "Codec accepts empty decompression input");

    check(
        state,
        empty_decompression_result.has_value() &&
            empty_decompression_result.value().
                empty(),
        "Empty decompression produces empty output");

    const Result<AssetCompressionResult>
        populated_compression_result =
            codec.compress(
                populated_compression_request);

    check(
        state,
        populated_compression_result.
            has_value(),
        "Codec accepts populated compression input");

    check(
        state,
        populated_compression_result.
                has_value() &&
            populated_compression_result.value().
                is_valid(),
        "Populated compression produces a valid result");

    check(
        state,
        populated_compression_result.
                has_value() &&
            populated_compression_result.value().
                has_data(),
        "Populated compression reports data");

    check(
        state,
        populated_compression_result.
                has_value() &&
            populated_compression_result.value().
                original_byte_count ==
                source_bytes.size(),
        "Compression records the original byte count");

    check(
        state,
        populated_compression_result.
                has_value() &&
            populated_compression_result.value().
                bytes ==
                source_bytes,
        "Passthrough compression preserves every byte");

    AssetDecompressionRequest
        round_trip_request{};

    if (populated_compression_result.
            has_value())
    {
        round_trip_request.
            compressed_bytes =
                std::span<const std::byte>{
                    populated_compression_result.
                        value().bytes
                };

        round_trip_request.
            expected_byte_count =
                populated_compression_result.
                    value().
                    original_byte_count;
    }

    check(
        state,
        round_trip_request.is_valid(),
        "Compressed output forms a valid decompression request");

    const Result<std::vector<std::byte>>
        round_trip_result =
            codec.decompress(
                round_trip_request);

    check(
        state,
        round_trip_result.has_value(),
        "Codec decompresses its own output");

    check(
        state,
        round_trip_result.has_value() &&
            round_trip_result.value() ==
                source_bytes,
        "Compression round trip restores every source byte");

    const AssetDecompressionRequest
        mismatched_size_request{
            std::span<const std::byte>{
                source_bytes
            },
            source_bytes.size() + 1U
        };

    check(
        state,
        mismatched_size_request.is_valid(),
        "Mismatched populated request remains structurally valid");

    const Result<std::vector<std::byte>>
        mismatched_size_result =
            codec.decompress(
                mismatched_size_request);

    check(
        state,
        !mismatched_size_result.has_value(),
        "Codec rejects an incorrect expected byte count");

    check(
        state,
        !mismatched_size_result.has_value() &&
            mismatched_size_result.error().code ==
                ErrorCode::invalid_argument,
        "Incorrect expected size reports invalid_argument");

    const Result<AssetCompressionResult>
        repeated_compression_result =
            codec.compress(
                populated_compression_request);

    check(
        state,
        repeated_compression_result.has_value(),
        "Codec accepts repeated compression");

    check(
        state,
        repeated_compression_result.
                has_value() &&
            populated_compression_result.
                has_value() &&
            repeated_compression_result.value().
                bytes ==
                populated_compression_result.
                    value().bytes,
        "Repeated compression is deterministic");

    check(
        state,
        source_bytes ==
            std::vector<std::byte>{
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x40}
            },
        "Compression and decompression preserve source storage");

    std::cout
        << "\nAsset compression codec test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}