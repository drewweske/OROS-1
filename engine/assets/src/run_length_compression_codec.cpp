#include "oros/assets/run_length_compression_codec.hpp"

#include <cstddef>
#include <new>
#include <span>
#include <vector>

namespace oros::assets
{
    std::string_view
    RunLengthCompressionCodec::name()
        const noexcept
    {
        return "oros.rle";
    }

    std::uint32_t
    RunLengthCompressionCodec::version()
        const noexcept
    {
        return 1U;
    }

    foundation::Result<
        AssetCompressionResult>
    RunLengthCompressionCodec::compress(
        const AssetCompressionRequest&
            request) const
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The run-length compression "
                "request is invalid.");
        }

        try
        {
            AssetCompressionResult result{};

            result.original_byte_count =
                request.source_bytes.size();

            if (request.source_bytes.empty())
            {
                return result;
            }

            std::size_t source_index{};

            while (source_index <
                   request.source_bytes.size())
            {
                const std::byte run_value =
                    request.source_bytes[
                        source_index];

                std::size_t run_length =
                    1U;

                while (
                    source_index + run_length <
                        request.source_bytes.
                            size() &&
                    run_length < 255U &&
                    request.source_bytes[
                        source_index +
                        run_length] ==
                        run_value)
                {
                    ++run_length;
                }

                result.bytes.push_back(
                    std::byte{
                        static_cast<
                            unsigned char>(
                            run_length)
                    });

                result.bytes.push_back(
                    run_value);

                source_index +=
                    run_length;
            }

            if (!result.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Run-length compression "
                    "produced an invalid result.");
            }

            return result;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate memory while "
                "compressing asset data.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while compressing asset data.");
        }
    }

    foundation::Result<
        std::vector<std::byte>>
    RunLengthCompressionCodec::decompress(
        const AssetDecompressionRequest&
            request) const
    {
        if (!request.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The run-length decompression "
                "request is invalid.");
        }

        if (request.compressed_bytes.empty())
        {
            return std::vector<std::byte>{};
        }

        if (request.compressed_bytes.size() %
                2U !=
            0U)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Run-length compressed data must "
                "contain complete count-value "
                "pairs.");
        }

        try
        {
            std::vector<std::byte>
                decompressed_bytes{};

            decompressed_bytes.reserve(
                request.expected_byte_count);

            std::size_t decoded_byte_count{};

            for (std::size_t index{};
                 index <
                 request.compressed_bytes.size();
                 index += 2U)
            {
                const unsigned int
                    encoded_run_length =
                        std::to_integer<
                            unsigned int>(
                            request.
                                compressed_bytes[
                                    index]);

                if (encoded_run_length == 0U)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Run-length compressed data "
                        "contains a zero-length run.");
                }

                const std::size_t run_length =
                    static_cast<std::size_t>(
                        encoded_run_length);

                if (decoded_byte_count >
                        request.
                            expected_byte_count ||
                    run_length >
                        request.
                            expected_byte_count -
                            decoded_byte_count)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Run-length compressed data "
                        "expands beyond the expected "
                        "byte count.");
                }

                const std::byte run_value =
                    request.compressed_bytes[
                        index + 1U];

                decompressed_bytes.insert(
                    decompressed_bytes.end(),
                    run_length,
                    run_value);

                decoded_byte_count +=
                    run_length;
            }

            if (decoded_byte_count !=
                request.expected_byte_count)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "Run-length compressed data "
                    "does not produce the expected "
                    "byte count.");
            }

            return decompressed_bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate memory while "
                "decompressing asset data.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while decompressing asset data.");
        }
    }
}