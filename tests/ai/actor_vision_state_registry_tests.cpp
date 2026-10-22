#include "oros/ai/actor_vision_state_registry.hpp"

#include "oros/foundation/error.hpp"
#include "oros/world/entity_id.hpp"

#include <iostream>
#include <string_view>
#include <type_traits>

namespace
{
    struct TestState
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        const bool success,
        std::string_view description)
    {
        ++state.checks;
        if (!success)
        {
            ++state.failures;
            std::cerr << "[fail] " << description << '\n';
        }
        else
        {
            std::cout << "[pass] " << description << '\n';
        }
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorVisionStateRegistry>);

    TestState test{};
    const EntityId actor_a{77ULL, 1ULL};
    const EntityId actor_b{77ULL, 2ULL};
    const EntityId actor_c{77ULL, 3ULL};
    const EntityId absent{77ULL, 99ULL};

    const auto east = ActorVisionDirection::create(
        WorldDisplacement{1.0, 0.0, 0.0});
    const auto north = ActorVisionDirection::create(
        WorldDisplacement{0.0, 0.0, 1.0});
    const auto short_sight = ActorVisionProfile::create(
        10.0, 80.0);
    const auto long_sight = ActorVisionProfile::create(
        200.0, 120.0);
    const auto full_confidence =
        ActorVisualObservationConfidence::create(1.0);
    const auto half_confidence =
        ActorVisualObservationConfidence::create(0.5);

    check(
        test,
        east.has_value() && north.has_value() &&
        short_sight.has_value() && long_sight.has_value() &&
        full_confidence.has_value() &&
        half_confidence.has_value(),
        "Validated multi-NPC fixture values");

    if (
        !east || !north || !short_sight || !long_sight ||
        !full_confidence || !half_confidence)
    {
        return 1;
    }

    const auto vision_a = ActorVisionState::create(
        actor_a, *east, *short_sight, *full_confidence);
    const auto vision_b = ActorVisionState::create(
        actor_b, *north, *long_sight, *half_confidence);
    const auto vision_c = ActorVisionState::create(
        actor_c, *east, *long_sight, *full_confidence);

    check(
        test,
        vision_a.has_value() && vision_b.has_value() &&
        vision_c.has_value(),
        "Distinct validated actor-owned vision states");
    if (!vision_a || !vision_b || !vision_c)
    {
        return 1;
    }

    ActorVisionStateRegistry registry{};
    check(
        test,
        registry.empty() && registry.size() == 0U &&
        registry.states_in_entity_order().empty(),
        "Registry starts empty");

    // Reverse insertion must still yield canonical EntityId order.
    check(
        test,
        registry.insert(*vision_c).has_value() &&
        registry.insert(*vision_a).has_value() &&
        registry.insert(*vision_b).has_value(),
        "Reverse-ordered insertions accepted");

    const auto sorted = registry.states_in_entity_order();
    check(
        test,
        registry.size() == 3U &&
        sorted.size() == 3U &&
        sorted[0].actor() == actor_a &&
        sorted[1].actor() == actor_b &&
        sorted[2].actor() == actor_c,
        "All vision states iterate in deterministic entity order");

    const auto duplicate = registry.insert(*vision_b);
    check(
        test,
        !duplicate.has_value() &&
        duplicate.error().code == ErrorCode::invalid_state &&
        registry.size() == 3U,
        "Duplicate actor rejected without mutating registry");

    ActorVisionState* a = registry.find(actor_a);
    ActorVisionState* b = registry.find(actor_b);

    check(
        test,
        a != nullptr && b != nullptr &&
        registry.find(absent) == nullptr &&
        registry.find(invalid_entity_id) == nullptr &&
        registry.contains(actor_c) &&
        !registry.contains(absent),
        "Find and contains agree for known and unknown actors");

    if (a == nullptr || b == nullptr)
    {
        return 1;
    }

    check(
        test,
        a->face(*north).has_value() &&
        a->set_profile(*long_sight).has_value() &&
        a->set_confidence(*half_confidence).has_value(),
        "One NPC can change its owned vision settings");

    const ActorVisionStateRegistry& const_registry = registry;
    const ActorVisionState* updated_a =
        const_registry.find(actor_a);
    const ActorVisionState* unchanged_b =
        const_registry.find(actor_b);

    check(
        test,
        updated_a != nullptr &&
        updated_a->forward() == *north &&
        updated_a->profile().maximum_range_meters() ==
            200.0 &&
        updated_a->confidence().value() == 0.5 &&
        unchanged_b != nullptr &&
        unchanged_b->actor() == actor_b &&
        unchanged_b->forward() == *north &&
        unchanged_b->profile().maximum_range_meters() ==
            200.0 &&
        unchanged_b->confidence().value() == 0.5 &&
        const_registry.find(actor_c)->forward() == *east,
        "Updates to one NPC leave other actors unchanged");

    const ActorVisionStateRegistry copy = registry;
    check(
        test,
        copy.size() == 3U &&
        copy.find(actor_a) != nullptr &&
        copy.find(actor_a)->actor() == actor_a,
        "Copy preserves independent actor vision snapshots");

    const auto invalid_removal = registry.remove(
        invalid_entity_id);
    const auto missing_removal = registry.remove(absent);
    check(
        test,
        !invalid_removal.has_value() &&
        invalid_removal.error().code ==
            ErrorCode::invalid_argument &&
        !missing_removal.has_value() &&
        missing_removal.error().code ==
            ErrorCode::not_found &&
        registry.size() == 3U,
        "Invalid or absent removals leave states intact");

    check(
        test,
        registry.remove(actor_b).has_value() &&
        registry.size() == 2U &&
        !registry.contains(actor_b) &&
        registry.states_in_entity_order()[0].actor() ==
            actor_a &&
        registry.states_in_entity_order()[1].actor() ==
            actor_c &&
        copy.contains(actor_b),
        "Removal keeps canonical order and does not alter copy");

    check(
        test,
        registry.insert(*vision_b).has_value() &&
        registry.size() == 3U &&
        registry.states_in_entity_order()[1].actor() ==
            actor_b,
        "Reinsertion restores deterministic ordering");

    std::cout << "\nActor vision registry checks: "
              << test.checks - test.failures << "/"
              << test.checks << " passed.\n";

    return test.failures == 0 ? 0 : 1;
}
