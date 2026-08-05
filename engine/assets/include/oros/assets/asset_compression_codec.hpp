#pragma once

#include "oros/foundation/result.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace oros::assets
{
    struct AssetCompressionRequest final
    {
        std::span<const std::byte>
            source_bytes{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return true;
        }
    };

    struct AssetDecompressionRequest final
    {
        std::span<const std::byte>
            compressed_bytes{};

        std::size_t
            expected_byte_count{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                !compressed_bytes.empty() ||
                expected_byte_count == 0U;
        }
    };

    struct AssetCompressionResult final
    {
        std::vector<std::byte>
            bytes{};

        std::size_t
            original_byte_count{};

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return
                !bytes.empty() ||
                original_byte_count == 0U;
        }

        [[nodiscard]]
        bool has_data() const noexcept
        {
            return !bytes.empty();
        }
    };

    class AssetCompressionCodec
    {
    public:
        AssetCompressionCodec() = default;

        virtual ~AssetCompressionCodec() =
            default;

        AssetCompressionCodec(
            const AssetCompressionCodec&) =
                delete;

        AssetCompressionCodec&
        operator=(
            const AssetCompressionCodec&) =
                delete;

        AssetCompressionCodec(
            AssetCompressionCodec&&) =
                delete;

        AssetCompressionCodec&
        operator=(
            AssetCompressionCodec&&) =
                delete;

        [[nodiscard]]
        virtual std::string_view
        name() const noexcept = 0;

        [[nodiscard]]
        virtual std::uint32_t
        version() const noexcept = 0;

        [[nodiscard]]
        virtual foundation::Result<
            AssetCompressionResult>
        compress(
            const AssetCompressionRequest&
                request) const = 0;

        [[nodiscard]]
        virtual foundation::Result<
            std::vector<std::byte>>
        decompress(
            const AssetDecompressionRequest&
                request) const = 0;
    };
}