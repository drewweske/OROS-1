#pragma once

#include "oros/assets/cooked_asset_artifact.hpp"
#include "oros/foundation/result.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oros::assets
{
    inline constexpr std::array<
        std::byte,
        8U
    > cooked_asset_artifact_magic{
        std::byte{0x4F},
        std::byte{0x52},
        std::byte{0x4F},
        std::byte{0x53},
        std::byte{0x41},
        std::byte{0x52},
        std::byte{0x54},
        std::byte{0x46}
    };

    inline constexpr std::uint64_t
        cooked_asset_artifact_header_byte_count{
            160ULL
        };

    inline constexpr std::uint32_t
        cooked_asset_artifact_compressed_flag{
            1U
        };

    struct CookedAssetArtifactSerializationLimits final
    {
        std::uint64_t max_artifact_byte_count{
            2ULL * 1024ULL * 1024ULL * 1024ULL
        };

        std::uint64_t max_payload_byte_count{
            1ULL * 1024ULL * 1024ULL * 1024ULL
        };

        std::uint64_t max_uncompressed_byte_count{
            1ULL * 1024ULL * 1024ULL * 1024ULL
        };

        std::uint32_t max_source_path_byte_count{
            4096U
        };

        std::uint32_t max_name_byte_count{
            255U
        };

        std::uint32_t max_dependency_count{
            65535U
        };

        [[nodiscard]]
        bool
        is_valid() const noexcept
        {
            return
                max_artifact_byte_count >=
                    cooked_asset_artifact_header_byte_count &&
                max_payload_byte_count <=
                    max_artifact_byte_count &&
                max_source_path_byte_count != 0U &&
                max_name_byte_count != 0U;
        }
    };

    [[nodiscard]]
    foundation::Result<std::vector<std::byte>>
    serialize_cooked_asset_artifact(
        const CookedAssetArtifact& artifact,
        CookedAssetArtifactSerializationLimits
            limits = {}) noexcept;

    [[nodiscard]]
    foundation::Result<CookedAssetArtifact>
    deserialize_cooked_asset_artifact(
        std::span<const std::byte> bytes,
        CookedAssetArtifactSerializationLimits
            limits = {}) noexcept;
}