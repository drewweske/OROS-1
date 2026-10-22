#include "oros/ai/faction_key.hpp"

#include <iostream>
#include <string>
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

    oros::foundation::Result<
        oros::ai::FactionKey>
    make_key_from_ephemeral_sources()
    {
        std::string
            faction_namespace{
                "mod.example"
            };

        std::string
            faction_name{
                "merchant-guild"
            };

        auto result =
            oros::ai::FactionKey::create(
                faction_namespace,
                faction_name);

        faction_namespace.front() =
            'x';

        faction_name.front() =
            'x';

        return result;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;

    static_assert(
        !std::is_default_constructible_v<
            FactionKey>);

    static_assert(
        std::is_copy_constructible_v<
            FactionKey>);

    static_assert(
        std::is_copy_assignable_v<
            FactionKey>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            FactionKey>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            FactionKey>);

    static_assert(
        std::is_same_v<
            decltype(
                FactionKey::create(
                    std::declval<
                        std::string_view>(),
                    std::declval<
                        std::string_view>())),
            Result<FactionKey>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionKey&>().
                    faction_namespace()),
            std::string_view>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const FactionKey&>().
                    faction_name()),
            std::string_view>);

    TestState state{};

    const Result<FactionKey>
        citizens_result =
            FactionKey::create(
                "oros",
                "citizens");

    check(
        state,
        citizens_result.has_value(),
        "Canonical engine faction key constructs successfully");

    if (!citizens_result.has_value())
    {
        return 1;
    }

    const FactionKey&
        citizens =
            citizens_result.value();

    check(
        state,
        citizens.faction_namespace() ==
                "oros" &&
            citizens.faction_name() ==
                "citizens",
        "Faction key preserves exact namespace and name");

    const Result<FactionKey>
        mod_result =
            FactionKey::create(
                "mod.example",
                "merchant-guild");

    check(
        state,
        mod_result.has_value(),
        "Namespaced mod faction key constructs successfully");

    if (!mod_result.has_value())
    {
        return 1;
    }

    check(
        state,
        mod_result.value().
                faction_namespace() ==
                "mod.example" &&
            mod_result.value().
                faction_name() ==
                "merchant-guild",
        "Periods and hyphens remain valid canonical interior characters");

    const Result<FactionKey>
        underscore_result =
            FactionKey::create(
                "mod_author",
                "merchant_guild");

    check(
        state,
        underscore_result.has_value(),
        "Underscore remains valid inside canonical faction key segments");

    const Result<FactionKey>
        ephemeral_result =
            make_key_from_ephemeral_sources();

    check(
        state,
        ephemeral_result.has_value() &&
            ephemeral_result.value().
                faction_namespace() ==
                "mod.example" &&
            ephemeral_result.value().
                faction_name() ==
                "merchant-guild",
        "Faction key owns namespace and name independently of source strings");

    const Result<FactionKey>
        same_citizens_result =
            FactionKey::create(
                "oros",
                "citizens");

    check(
        state,
        same_citizens_result.has_value() &&
            same_citizens_result.value() ==
                citizens,
        "Equal symbolic segments deterministically identify equal factions");

    const Result<FactionKey>
        different_namespace_result =
            FactionKey::create(
                "mod.example",
                "citizens");

    check(
        state,
        different_namespace_result.
                has_value() &&
            !(
                different_namespace_result.
                    value() ==
                citizens
            ),
        "Different faction namespaces identify different factions");

    const Result<FactionKey>
        different_name_result =
            FactionKey::create(
                "oros",
                "raiders");

    check(
        state,
        different_name_result.has_value() &&
            !(
                different_name_result.value() ==
                citizens
            ),
        "Different faction names identify different factions");

    const Result<FactionKey>
        empty_namespace_result =
            FactionKey::create(
                "",
                "citizens");

    check(
        state,
        !empty_namespace_result.has_value() &&
            empty_namespace_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Empty faction namespace is rejected");

    const Result<FactionKey>
        empty_name_result =
            FactionKey::create(
                "oros",
                "");

    check(
        state,
        !empty_name_result.has_value() &&
            empty_name_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Empty faction name is rejected");

    const Result<FactionKey>
        uppercase_namespace_result =
            FactionKey::create(
                "Oros",
                "citizens");

    check(
        state,
        !uppercase_namespace_result.
                has_value() &&
            uppercase_namespace_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Uppercase faction namespace is rejected rather than normalized");

    const Result<FactionKey>
        uppercase_name_result =
            FactionKey::create(
                "oros",
                "Citizens");

    check(
        state,
        !uppercase_name_result.has_value() &&
            uppercase_name_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Uppercase faction name is rejected rather than normalized");

    const Result<FactionKey>
        whitespace_result =
            FactionKey::create(
                "oros",
                "city guard");

    check(
        state,
        !whitespace_result.has_value() &&
            whitespace_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Whitespace is rejected from canonical faction keys");

    const Result<FactionKey>
        slash_namespace_result =
            FactionKey::create(
                "mod/example",
                "citizens");

    check(
        state,
        !slash_namespace_result.
                has_value() &&
            slash_namespace_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Slash is rejected from faction namespace segments");

    const Result<FactionKey>
        slash_name_result =
            FactionKey::create(
                "oros",
                "city/guard");

    check(
        state,
        !slash_name_result.has_value() &&
            slash_name_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Slash is rejected from faction name segments");

    const Result<FactionKey>
        leading_punctuation_result =
            FactionKey::create(
                ".oros",
                "citizens");

    check(
        state,
        !leading_punctuation_result.
                has_value() &&
            leading_punctuation_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Leading punctuation is rejected from faction key segments");

    const Result<FactionKey>
        trailing_punctuation_result =
            FactionKey::create(
                "oros",
                "citizens-");

    check(
        state,
        !trailing_punctuation_result.
                has_value() &&
            trailing_punctuation_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Trailing punctuation is rejected from faction key segments");

    const Result<FactionKey>
        colon_result =
            FactionKey::create(
                "oros",
                "city:guard");

    check(
        state,
        !colon_result.has_value() &&
            colon_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Colon is rejected because faction namespace and name remain separate");

    const Result<FactionKey>
        non_ascii_result =
            FactionKey::create(
                "oros",
                "caf\xc3\xa9");

    check(
        state,
        !non_ascii_result.has_value() &&
            non_ascii_result.
                error().code ==
                ErrorCode::invalid_argument,
        "Non-ASCII bytes are rejected from canonical faction keys");

    const FactionKey copied =
        citizens;

    check(
        state,
        copied ==
            citizens,
        "Faction key copy preserves stable semantic identity");

    FactionKey assigned =
        mod_result.value();

    assigned =
        citizens;

    check(
        state,
        assigned ==
            citizens,
        "Faction key assignment preserves stable semantic identity");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
