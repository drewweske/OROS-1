#include "oros/streaming/world_cell_revision_id.hpp"

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
        inline constexpr char
            revision_separator{'@'};

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
        bool parse_revision(
            const std::string_view text,
            std::uint64_t& revision) noexcept
        {
            if (text.empty())
            {
                return false;
            }

            std::uint64_t parsed_revision{};

            const char* const begin =
                text.data();

            const char* const end =
                begin +
                text.size();

            const std::from_chars_result result =
                std::from_chars(
                    begin,
                    end,
                    parsed_revision,
                    10);

            if (result.ec != std::errc{} ||
                result.ptr != end)
            {
                return false;
            }

            revision = parsed_revision;
            return true;
        }
    }

    std::size_t
    WorldCellRevisionIdHash::operator()(
        const WorldCellRevisionId&
            revision_id) const noexcept
    {
        const std::uint64_t cell_hash =
            mix_hash(
                static_cast<std::uint64_t>(
                    WorldCellKeyHash{}(
                        revision_id.cell_key)));

        const std::uint64_t revision_hash =
            std::rotl(
                mix_hash(
                    revision_id.revision),
                31);

        const std::uint64_t combined =
            cell_hash ^
            revision_hash;

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
        const WorldCellRevisionId&
            revision_id)
    {
        std::string text =
            to_string(
                revision_id.cell_key);

        text.push_back(
            revision_separator);

        text +=
            std::to_string(
                revision_id.revision);

        return text;
    }

    foundation::Result<WorldCellRevisionId>
    parse_world_cell_revision_id(
        const std::string_view text)
    {
        const std::size_t separator_position =
            text.find(
                revision_separator);

        const std::size_t second_separator =
            separator_position ==
                    std::string_view::npos
                ? std::string_view::npos
                : text.find(
                    revision_separator,
                    separator_position + 1U);

        if (separator_position ==
                std::string_view::npos ||
            separator_position == 0U ||
            separator_position + 1U >=
                text.size() ||
            second_separator !=
                std::string_view::npos)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell revision identity must "
                "contain one canonical world cell "
                "key followed by an at sign and a "
                "non-zero decimal revision.");
        }

        const auto parsed_cell_key =
            parse_world_cell_key(
                text.substr(
                    0U,
                    separator_position));

        if (!parsed_cell_key.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell revision identity "
                "contains an invalid world cell "
                "key.");
        }

        std::uint64_t revision{};

        const bool revision_parsed =
            parse_revision(
                text.substr(
                    separator_position + 1U),
                revision);

        if (!revision_parsed ||
            revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell revision identity "
                "contains an invalid revision.");
        }

        const WorldCellRevisionId revision_id{
            parsed_cell_key.value(),
            revision
        };

        if (!revision_id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell revision identity must "
                "be valid.");
        }

        if (to_string(revision_id) != text)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell revision identity text "
                "is not in its canonical serialized "
                "form.");
        }

        return revision_id;
    }
}