#include "oros/streaming/world_cell_key.hpp"

#include <bit>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <system_error>

namespace oros::streaming
{
    namespace
    {
        inline constexpr std::size_t
            namespace_hex_length{16U};

        inline constexpr char separator{':'};

        inline constexpr char hex_digits[]{
            "0123456789abcdef"
        };

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

            return
                value ^
                (value >> 31U);
        }

        [[nodiscard]]
        constexpr std::uint64_t
        signed_bits(
            const std::int64_t value) noexcept
        {
            return std::bit_cast<std::uint64_t>(
                value);
        }

        void write_hex_namespace(
            const std::uint64_t value,
            char* const output) noexcept
        {
            for (std::size_t index = 0U;
                 index < namespace_hex_length;
                 ++index)
            {
                const unsigned shift =
                    static_cast<unsigned>(
                        (
                            namespace_hex_length -
                            1U -
                            index
                        ) *
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
        bool parse_namespace(
            const std::string_view text,
            std::uint64_t& value) noexcept
        {
            if (text.size() !=
                namespace_hex_length)
            {
                return false;
            }

            std::uint64_t parsed_value{};

            const char* const begin =
                text.data();

            const char* const end =
                begin +
                text.size();

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
        bool parse_coordinate(
            const std::string_view text,
            std::int64_t& value) noexcept
        {
            if (text.empty())
            {
                return false;
            }

            std::int64_t parsed_value{};

            const char* const begin =
                text.data();

            const char* const end =
                begin +
                text.size();

            const std::from_chars_result result =
                std::from_chars(
                    begin,
                    end,
                    parsed_value,
                    10);

            if (result.ec != std::errc{} ||
                result.ptr != end)
            {
                return false;
            }

            value = parsed_value;
            return true;
        }
    }

    std::size_t
    WorldCellKeyHash::operator()(
        const WorldCellKey key) const noexcept
    {
        const std::uint64_t namespace_hash =
            mix_hash(
                key.world_namespace);

        const std::uint64_t x_hash =
            std::rotl(
                mix_hash(
                    signed_bits(
                        key.cell.x)),
                13);

        const std::uint64_t y_hash =
            std::rotl(
                mix_hash(
                    signed_bits(
                        key.cell.y)),
                29);

        const std::uint64_t z_hash =
            std::rotl(
                mix_hash(
                    signed_bits(
                        key.cell.z)),
                47);

        const std::uint64_t combined =
            namespace_hash ^
            x_hash ^
            y_hash ^
            z_hash;

        if constexpr (
            sizeof(std::size_t) >=
            sizeof(std::uint64_t))
        {
            return
                static_cast<std::size_t>(
                    combined);
        }
        else
        {
            return
                static_cast<std::size_t>(
                    combined ^
                    (combined >> 32U));
        }
    }

    std::string to_string(
        const WorldCellKey key)
    {
        std::string text(
            namespace_hex_length,
            '0');

        write_hex_namespace(
            key.world_namespace,
            text.data());

        text.push_back(
            separator);

        text +=
            std::to_string(
                key.cell.x);

        text.push_back(
            separator);

        text +=
            std::to_string(
                key.cell.y);

        text.push_back(
            separator);

        text +=
            std::to_string(
                key.cell.z);

        return text;
    }

    foundation::Result<WorldCellKey>
    parse_world_cell_key(
        const std::string_view text)
    {
        const std::size_t first_separator =
            text.find(
                separator);

        const std::size_t second_separator =
            first_separator ==
                    std::string_view::npos
                ? std::string_view::npos
                : text.find(
                    separator,
                    first_separator + 1U);

        const std::size_t third_separator =
            second_separator ==
                    std::string_view::npos
                ? std::string_view::npos
                : text.find(
                    separator,
                    second_separator + 1U);

        const std::size_t fourth_separator =
            third_separator ==
                    std::string_view::npos
                ? std::string_view::npos
                : text.find(
                    separator,
                    third_separator + 1U);

        if (first_separator !=
                namespace_hex_length ||
            second_separator ==
                std::string_view::npos ||
            third_separator ==
                std::string_view::npos ||
            fourth_separator !=
                std::string_view::npos)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell key text must contain a "
                "16-digit hexadecimal world namespace "
                "followed by three signed decimal "
                "coordinates separated by colons.");
        }

        std::uint64_t world_namespace{};
        std::int64_t x{};
        std::int64_t y{};
        std::int64_t z{};

        const bool namespace_parsed =
            parse_namespace(
                text.substr(
                    0U,
                    first_separator),
                world_namespace);

        const bool x_parsed =
            parse_coordinate(
                text.substr(
                    first_separator + 1U,
                    second_separator -
                        first_separator -
                        1U),
                x);

        const bool y_parsed =
            parse_coordinate(
                text.substr(
                    second_separator + 1U,
                    third_separator -
                        second_separator -
                        1U),
                y);

        const bool z_parsed =
            parse_coordinate(
                text.substr(
                    third_separator + 1U),
                z);

        if (!namespace_parsed ||
            !x_parsed ||
            !y_parsed ||
            !z_parsed)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell key text contains an "
                "invalid namespace or coordinate.");
        }

        const WorldCellKey key{
            world_namespace,
            world::WorldCell{
                x,
                y,
                z
            }
        };

        if (!key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell key namespace must be "
                "non-zero.");
        }

        if (to_string(key) != text)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell key text is not in its "
                "canonical serialized form.");
        }

        return key;
    }
}