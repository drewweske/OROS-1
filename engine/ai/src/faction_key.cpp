#include "oros/ai/faction_key.hpp"

#include <new>
#include <string>
#include <string_view>
#include <utility>

namespace oros::ai
{
    namespace
    {
        [[nodiscard]]
        bool
        is_ascii_lowercase_letter_or_digit(
            const char character)
            noexcept
        {
            return
                (
                    character >= 'a' &&
                    character <= 'z'
                ) ||
                (
                    character >= '0' &&
                    character <= '9'
                );
        }

        [[nodiscard]]
        bool
        is_valid_faction_key_segment(
            const std::string_view segment)
            noexcept
        {
            if (segment.empty())
            {
                return false;
            }

            if (
                !is_ascii_lowercase_letter_or_digit(
                    segment.front()) ||
                !is_ascii_lowercase_letter_or_digit(
                    segment.back()))
            {
                return false;
            }

            for (const char character : segment)
            {
                if (
                    !is_ascii_lowercase_letter_or_digit(
                        character) &&
                    character != '.' &&
                    character != '_' &&
                    character != '-')
                {
                    return false;
                }
            }

            return true;
        }
    }

    foundation::Result<
        FactionKey>
    FactionKey::create(
        const std::string_view
            faction_namespace,
        const std::string_view
            faction_name)
    {
        if (
            !is_valid_faction_key_segment(
                faction_namespace) ||
            !is_valid_faction_key_segment(
                faction_name))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Faction key segments must begin "
                "and end with lowercase ASCII "
                "letters or digits and may contain "
                "only lowercase ASCII letters, "
                "digits, periods, underscores, "
                "or hyphens.");
        }

        try
        {
            return FactionKey{
                std::string{
                    faction_namespace
                },
                std::string{
                    faction_name
                }
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Faction key could not allocate "
                "owned symbolic identity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Faction key construction failed.");
        }
    }

    std::string_view
    FactionKey::
        faction_namespace()
        const noexcept
    {
        return faction_namespace_;
    }

    std::string_view
    FactionKey::
        faction_name()
        const noexcept
    {
        return faction_name_;
    }

    FactionKey::
        FactionKey(
            std::string faction_namespace,
            std::string faction_name)
            noexcept
        : faction_namespace_{
              std::move(faction_namespace)
          },
          faction_name_{
              std::move(faction_name)
          }
    {
    }
}
