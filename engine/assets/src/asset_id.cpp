#include "oros/assets/asset_id.hpp"

#include <bit>
#include <charconv>
#include <limits>
#include <string_view>
#include <system_error>

namespace oros::assets
{
    namespace
    {
        inline constexpr std::size_t
            component_hex_length{16U};

        inline constexpr std::size_t
            serialized_length{
                component_hex_length * 2U + 1U
            };

        inline constexpr char separator{':'};

        inline constexpr char hex_digits[]{
            "0123456789abcdef"
        };

        void write_hex_component(
            const std::uint64_t value,
            char* const output) noexcept
        {
            for (std::size_t index = 0U;
                 index < component_hex_length;
                 ++index)
            {
                const unsigned shift =
                    static_cast<unsigned>(
                        (component_hex_length -
                            1U -
                            index) *
                        4U);

                const std::uint64_t digit =
                    (value >> shift) &
                    0xFULL;

                output[index] =
                    hex_digits[
                        static_cast<std::size_t>(
                            digit)];
            }
        }

        [[nodiscard]]
        bool parse_hex_component(
            const std::string_view text,
            std::uint64_t& value) noexcept
        {
            if (text.size() !=
                component_hex_length)
            {
                return false;
            }

            std::uint64_t parsed_value{};

            const char* const begin =
                text.data();

            const char* const end =
                begin + text.size();

            const std::from_chars_result result =
                std::from_chars(
                    begin,
                    end,
                    parsed_value,
                    16);

            if (result.ec != std::errc{} ||
                result.ptr != end)
            {
                return false;
            }

            value = parsed_value;
            return true;
        }

        [[nodiscard]]
        constexpr std::uint64_t mix_hash(
            std::uint64_t value) noexcept
        {
            value +=
                0x9E3779B97F4A7C15ULL;

            value =
                (value ^ (value >> 30U)) *
                0xBF58476D1CE4E5B9ULL;

            value =
                (value ^ (value >> 27U)) *
                0x94D049BB133111EBULL;

            return value ^ (value >> 31U);
        }
    }

    std::size_t
    AssetIdHash::operator()(
        const AssetId asset) const noexcept
    {
        const std::uint64_t mixed_namespace =
            mix_hash(
                asset.catalog_namespace);

        const std::uint64_t mixed_sequence =
            std::rotl(
                mix_hash(
                    asset.asset_sequence),
                32);

        const std::uint64_t combined =
            mixed_namespace ^
            mixed_sequence;

        if constexpr (
            sizeof(std::size_t) >=
            sizeof(std::uint64_t))
        {
            return static_cast<std::size_t>(
                combined);
        }
        else
        {
            return static_cast<std::size_t>(
                combined ^
                (combined >> 32U));
        }
    }

    std::string
    to_string(
        const AssetId asset)
    {
        std::string text(
            serialized_length,
            '0');

        write_hex_component(
            asset.catalog_namespace,
            text.data());

        text[component_hex_length] =
            separator;

        write_hex_component(
            asset.asset_sequence,
            text.data() +
                component_hex_length +
                1U);

        return text;
    }

    foundation::Result<AssetId>
    parse_asset_id(
        const std::string_view text)
    {
        if (text.size() != serialized_length ||
            text[component_hex_length] !=
                separator)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Asset ID text must contain two "
                "16-digit hexadecimal components "
                "separated by a colon.");
        }

        std::uint64_t catalog_namespace{};
        std::uint64_t asset_sequence{};

        const bool namespace_parsed =
            parse_hex_component(
                text.substr(
                    0U,
                    component_hex_length),
                catalog_namespace);

        const bool sequence_parsed =
            parse_hex_component(
                text.substr(
                    component_hex_length + 1U,
                    component_hex_length),
                asset_sequence);

        if (!namespace_parsed ||
            !sequence_parsed)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Asset ID text contains an invalid "
                "hexadecimal component.");
        }

        const AssetId asset{
            catalog_namespace,
            asset_sequence
        };

        if (!asset.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Asset ID catalog namespace and sequence "
                "must both be non-zero.");
        }

        return asset;
    }

    foundation::Result<AssetIdGenerator>
    AssetIdGenerator::create(
        const std::uint64_t catalog_namespace,
        const std::uint64_t first_sequence)
    {
        if (catalog_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Asset ID catalog namespace must be "
                "non-zero.");
        }

        if (first_sequence == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Asset ID first sequence must be "
                "non-zero.");
        }

        return AssetIdGenerator{
            catalog_namespace,
            first_sequence
        };
    }

    AssetIdGenerator::AssetIdGenerator(
        const std::uint64_t catalog_namespace,
        const std::uint64_t first_sequence) noexcept
        : catalog_namespace_{
            catalog_namespace
        },
          next_sequence_{
            first_sequence
        }
    {
    }

    foundation::Result<AssetId>
    AssetIdGenerator::generate()
    {
        if (exhausted_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Asset ID generator has exhausted "
                "its sequence range.");
        }

        const AssetId asset{
            catalog_namespace_,
            next_sequence_
        };

        if (next_sequence_ ==
            (std::numeric_limits<
                std::uint64_t>::max)())
        {
            exhausted_ = true;
        }
        else
        {
            ++next_sequence_;
        }

        return asset;
    }
}