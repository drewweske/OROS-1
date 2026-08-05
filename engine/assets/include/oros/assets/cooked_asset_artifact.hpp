#pragma once

#include "oros/assets/asset_record.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace oros::assets
{
    inline constexpr std::uint32_t
        cooked_asset_artifact_format_version{
            1U
        };

    struct CookedAssetArtifact final
    {
        std::uint32_t format_version{
            cooked_asset_artifact_format_version
        };

        AssetRecord record{};

        std::string cooker_name{};
        std::uint32_t cooker_version{};

        std::string compression_codec_name{};
        std::uint32_t compression_codec_version{};

        std::uint64_t uncompressed_byte_count{};
        std::vector<std::byte> payload_bytes{};

        [[nodiscard]]
        bool
        is_compressed() const noexcept
        {
            return
                !compression_codec_name.empty();
        }

        [[nodiscard]]
        bool
        has_payload_data() const noexcept
        {
            return
                !payload_bytes.empty();
        }

        [[nodiscard]]
        std::uint64_t
        encoded_byte_count() const noexcept
        {
            return
                static_cast<std::uint64_t>(
                    payload_bytes.size());
        }

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            if (format_version !=
                    cooked_asset_artifact_format_version ||
                !record.is_valid() ||
                !record.is_cooked() ||
                cooker_name.empty() ||
                cooker_version == 0U)
            {
                return false;
            }

            const bool has_codec_name =
                !compression_codec_name.empty();

            const bool has_codec_version =
                compression_codec_version != 0U;

            if (has_codec_name !=
                has_codec_version)
            {
                return false;
            }

            if (!has_codec_name)
            {
                return
                    encoded_byte_count() ==
                    uncompressed_byte_count;
            }

            return
                has_payload_data() ||
                uncompressed_byte_count == 0U;
        }
    };
}