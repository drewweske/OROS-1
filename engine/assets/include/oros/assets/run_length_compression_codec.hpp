#pragma once

#include "oros/assets/asset_compression_codec.hpp"

#include <cstdint>
#include <string_view>

namespace oros::assets
{
    class RunLengthCompressionCodec final
        : public AssetCompressionCodec
    {
    public:
        RunLengthCompressionCodec() =
            default;

        ~RunLengthCompressionCodec()
            override = default;

        [[nodiscard]]
        std::string_view
        name() const noexcept override;

        [[nodiscard]]
        std::uint32_t
        version() const noexcept override;

        [[nodiscard]]
        foundation::Result<
            AssetCompressionResult>
        compress(
            const AssetCompressionRequest&
                request) const override;

        [[nodiscard]]
        foundation::Result<
            std::vector<std::byte>>
        decompress(
            const AssetDecompressionRequest&
                request) const override;
    };
}