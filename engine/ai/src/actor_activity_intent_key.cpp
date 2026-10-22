#include "oros/ai/actor_activity_intent_key.hpp"

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
        is_valid_intent_key_segment(
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
        ActorActivityIntentKey>
    ActorActivityIntentKey::create(
        const std::string_view
            intent_namespace,
        const std::string_view
            intent_name)
    {
        if (
            !is_valid_intent_key_segment(
                intent_namespace) ||
            !is_valid_intent_key_segment(
                intent_name))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Actor activity intent key "
                "segments must begin and end "
                "with lowercase ASCII letters "
                "or digits and may contain only "
                "lowercase ASCII letters, digits, "
                "periods, underscores, or hyphens.");
        }

        try
        {
            return ActorActivityIntentKey{
                std::string{
                    intent_namespace
                },
                std::string{
                    intent_name
                }
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor activity intent key "
                "could not allocate owned "
                "symbolic identity.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor activity intent key "
                "construction failed.");
        }
    }

    std::string_view
    ActorActivityIntentKey::
        intent_namespace()
        const noexcept
    {
        return intent_namespace_;
    }

    std::string_view
    ActorActivityIntentKey::
        intent_name()
        const noexcept
    {
        return intent_name_;
    }

    ActorActivityIntentKey::
        ActorActivityIntentKey(
            std::string intent_namespace,
            std::string intent_name)
            noexcept
        : intent_namespace_{
              std::move(intent_namespace)
          },
          intent_name_{
              std::move(intent_name)
          }
    {
    }
}