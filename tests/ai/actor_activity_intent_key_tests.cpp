#include "oros/ai/actor_activity_intent_key.hpp"

#include <iostream>
#include <string_view>
#include <type_traits>
#include <utility>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool condition,
        const std::string_view name)
    {
        ++state.checks;

        if (condition)
        {
            std::cout
                << "[pass] "
                << name
                << '\n';

            return;
        }

        ++state.failures;

        std::cerr
            << "[fail] "
            << name
            << '\n';
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;

    static_assert(
        !std::is_default_constructible_v<
            ActorActivityIntentKey>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorActivityIntentKey>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorActivityIntentKey>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorActivityIntentKey&>().
                    intent_namespace()),
            std::string_view>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const ActorActivityIntentKey&>().
                    intent_name()),
            std::string_view>);

    TestState state{};

    const Result<ActorActivityIntentKey>
        sleep_result =
            ActorActivityIntentKey::create(
                "oros",
                "sleep");

    check(
        state,
        sleep_result.has_value(),
        "Canonical engine activity-intent key constructs successfully");

    if (!sleep_result.has_value())
    {
        return 1;
    }

    const ActorActivityIntentKey&
        sleep =
            sleep_result.value();

    check(
        state,
        sleep.intent_namespace() ==
            "oros" &&
            sleep.intent_name() ==
                "sleep",
        "Activity-intent key preserves exact namespace and name");

    const Result<ActorActivityIntentKey>
        mod_result =
            ActorActivityIntentKey::create(
                "mod.example",
                "work_shift-2");

    check(
        state,
        mod_result.has_value(),
        "Namespaced mod activity-intent key constructs successfully");

    if (!mod_result.has_value())
    {
        return 1;
    }

    check(
        state,
        mod_result.value().
                intent_namespace() ==
            "mod.example" &&
            mod_result.value().
                intent_name() ==
            "work_shift-2",
        "Periods underscores and hyphens remain valid canonical interior characters");

    const Result<ActorActivityIntentKey>
        underscore_result =
            ActorActivityIntentKey::create(
                "mod_author",
                "work_shift");

    check(
        state,
        underscore_result.has_value(),
        "Underscore remains valid inside canonical key segments");

    const Result<ActorActivityIntentKey>
        same_sleep_result =
            ActorActivityIntentKey::create(
                "oros",
                "sleep");

    check(
        state,
        same_sleep_result.has_value() &&
            same_sleep_result.value() ==
                sleep,
        "Equal symbolic segments deterministically identify equal intents");

    const Result<ActorActivityIntentKey>
        different_name_result =
            ActorActivityIntentKey::create(
                "oros",
                "work");

    check(
        state,
        different_name_result.has_value() &&
            !(different_name_result.value() ==
                sleep),
        "Different intent names identify different activity intents");

    const Result<ActorActivityIntentKey>
        different_namespace_result =
            ActorActivityIntentKey::create(
                "mod.example",
                "sleep");

    check(
        state,
        different_namespace_result.has_value() &&
            !(different_namespace_result.value() ==
                sleep),
        "Different namespaces prevent cross-domain intent collisions");

    const Result<ActorActivityIntentKey>
        empty_namespace_result =
            ActorActivityIntentKey::create(
                "",
                "sleep");

    check(
        state,
        !empty_namespace_result.has_value() &&
            empty_namespace_result.error().code ==
                ErrorCode::invalid_argument,
        "Empty activity-intent namespace is rejected");

    const Result<ActorActivityIntentKey>
        empty_name_result =
            ActorActivityIntentKey::create(
                "oros",
                "");

    check(
        state,
        !empty_name_result.has_value() &&
            empty_name_result.error().code ==
                ErrorCode::invalid_argument,
        "Empty activity-intent name is rejected");

    const Result<ActorActivityIntentKey>
        uppercase_namespace_result =
            ActorActivityIntentKey::create(
                "Oros",
                "sleep");

    check(
        state,
        !uppercase_namespace_result.has_value() &&
            uppercase_namespace_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Uppercase namespace is rejected rather than normalized");

    const Result<ActorActivityIntentKey>
        uppercase_name_result =
            ActorActivityIntentKey::create(
                "oros",
                "Sleep");

    check(
        state,
        !uppercase_name_result.has_value() &&
            uppercase_name_result.error().code ==
                ErrorCode::invalid_argument,
        "Uppercase intent name is rejected rather than normalized");

    const Result<ActorActivityIntentKey>
        leading_punctuation_result =
            ActorActivityIntentKey::create(
                ".oros",
                "sleep");

    check(
        state,
        !leading_punctuation_result.has_value() &&
            leading_punctuation_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Leading punctuation is rejected");

    const Result<ActorActivityIntentKey>
        trailing_punctuation_result =
            ActorActivityIntentKey::create(
                "oros",
                "sleep-");

    check(
        state,
        !trailing_punctuation_result.has_value() &&
            trailing_punctuation_result.error().
                    code ==
                ErrorCode::invalid_argument,
        "Trailing punctuation is rejected");

    const Result<ActorActivityIntentKey>
        whitespace_result =
            ActorActivityIntentKey::create(
                "oros",
                "work shift");

    check(
        state,
        !whitespace_result.has_value() &&
            whitespace_result.error().code ==
                ErrorCode::invalid_argument,
        "Whitespace is rejected from canonical activity-intent keys");

    const Result<ActorActivityIntentKey>
        separator_result =
            ActorActivityIntentKey::create(
                "oros",
                "work/shift");

    check(
        state,
        !separator_result.has_value() &&
            separator_result.error().code ==
                ErrorCode::invalid_argument,
        "Slash separator is rejected from activity-intent key segments");

    const Result<ActorActivityIntentKey>
        colon_result =
            ActorActivityIntentKey::create(
                "oros",
                "work:shift");

    check(
        state,
        !colon_result.has_value() &&
            colon_result.error().code ==
                ErrorCode::invalid_argument,
        "Colon separator is rejected because namespace and name remain separate");

    const Result<ActorActivityIntentKey>
        non_ascii_result =
            ActorActivityIntentKey::create(
                "oros",
                "caf\xc3\xa9");

    check(
        state,
        !non_ascii_result.has_value() &&
            non_ascii_result.error().code ==
                ErrorCode::invalid_argument,
        "Non-ASCII bytes are rejected from canonical activity-intent keys");

    const ActorActivityIntentKey copied =
        sleep;

    check(
        state,
        copied ==
            sleep,
        "Activity-intent key copy preserves stable semantic identity");

    ActorActivityIntentKey assigned =
        mod_result.value();

    assigned =
        sleep;

    check(
        state,
        assigned ==
            sleep,
        "Activity-intent key assignment preserves stable semantic identity");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}