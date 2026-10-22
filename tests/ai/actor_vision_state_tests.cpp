#include "oros/ai/actor_vision_state.hpp"
#include "oros/foundation/error.hpp"
#include "oros/world/entity_id.hpp"

#include <iostream>
#include <string_view>
#include <type_traits>

namespace
{
    struct TestState final
    {
        int checks{};
        int failures{};
    };

    void check(
        TestState& state,
        bool condition,
        std::string_view name)
    {
        ++state.checks;
        if (condition)
        {
            std::cout << "[pass] " << name << '\n';
            return;
        }

        ++state.failures;
        std::cerr << "[fail] " << name << '\n';
    }

    int finish(const TestState& state)
    {
        std::cout << "\nActor vision state: "
                  << state.checks - state.failures
                  << "/" << state.checks
                  << " checks passed.\n";
        return state.failures == 0 ? 0 : 1;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(!std::is_default_constructible_v<ActorVisionState>);
    static_assert(std::is_copy_constructible_v<ActorVisionState>);

    TestState state{};
    const EntityId actor{77ULL, 1ULL};
    const EntityId second_actor{77ULL, 2ULL};

    const auto east = ActorVisionDirection::create(
        WorldDisplacement{1.0, 0.0, 0.0});
    const auto north = ActorVisionDirection::create(
        WorldDisplacement{0.0, 1.0, 0.0});
    const auto original_profile = ActorVisionProfile::create(
        200.0, 120.0);
    const auto narrower_profile = ActorVisionProfile::create(
        80.0, 60.0);
    const auto full_confidence =
        ActorVisualObservationConfidence::create(1.0);
    const auto half_confidence =
        ActorVisualObservationConfidence::create(0.5);

    check(state,
        east.has_value() && north.has_value() &&
        original_profile.has_value() && narrower_profile.has_value() &&
        full_confidence.has_value() && half_confidence.has_value(),
        "Validated vision fixtures construct");
    if (!east || !north || !original_profile || !narrower_profile ||
        !full_confidence || !half_confidence)
    {
        return finish(state);
    }

    const auto invalid_actor = ActorVisionState::create(
        invalid_entity_id, *east, *original_profile, *full_confidence);
    check(state,
        !invalid_actor &&
        invalid_actor.error().code == ErrorCode::invalid_argument,
        "Invalid actor identity is rejected");

    auto initial = ActorVisionState::create(
        actor, *east, *original_profile, *full_confidence);
    check(state, initial.has_value() && initial->is_valid(),
        "Valid actor state constructs");
    if (!initial)
    {
        return finish(state);
    }

    ActorVisionState vision = *initial;
    const ActorVisionState copy = vision;

    check(state,
        vision.actor() == actor &&
        vision.forward() == *east &&
        vision.profile().maximum_range_meters() == 200.0 &&
        vision.profile().field_of_view_degrees() == 120.0 &&
        vision.confidence().value() == 1.0,
        "Initial validated values are preserved");

    const auto turn = vision.face(*north);
    check(state,
        turn.has_value() && vision.forward() == *north,
        "Actor can turn");
    check(state,
        vision.actor() == actor &&
        vision.profile().maximum_range_meters() == 200.0 &&
        vision.confidence().value() == 1.0,
        "Turning preserves identity and capabilities");
    check(state, copy.forward() == *east,
        "Copy remains independent after turning");

    const auto profile_update = vision.set_profile(*narrower_profile);
    check(state,
        profile_update.has_value() &&
        vision.profile().maximum_range_meters() == 80.0 &&
        vision.profile().field_of_view_degrees() == 60.0,
        "Range and FOV can change together");
    check(state,
        vision.actor() == actor && vision.forward() == *north &&
        vision.confidence().value() == 1.0,
        "Profile update preserves identity, facing and confidence");

    const auto confidence_update = vision.set_confidence(*half_confidence);
    check(state,
        confidence_update.has_value() &&
        vision.confidence().value() == 0.5,
        "Observation confidence is configurable");
    check(state,
        vision.is_valid() && vision.actor() == actor &&
        vision.forward() == *north &&
        vision.profile().maximum_range_meters() == 80.0,
        "Confidence update preserves other settings");
    check(state,
        copy.actor() == actor && copy.forward() == *east &&
        copy.profile().maximum_range_meters() == 200.0 &&
        copy.confidence().value() == 1.0,
        "Independent copy retains original settings");

    const auto another = ActorVisionState::create(
        second_actor, *north, *narrower_profile, *half_confidence);
    check(state,
        another.has_value() && another->actor() == second_actor &&
        another->is_valid(),
        "Separate actors have distinct vision states");

    const auto invalid_direction =
        ActorVisionDirection::create(WorldDisplacement{});
    const auto invalid_profile = ActorVisionProfile::create(-1.0, 60.0);
    const auto invalid_confidence =
        ActorVisualObservationConfidence::create(1.5);
    check(state,
        !invalid_direction &&
        invalid_direction.error().code == ErrorCode::invalid_argument,
        "Invalid facing vector rejected by validated factory");
    check(state,
        !invalid_profile &&
        invalid_profile.error().code == ErrorCode::invalid_argument,
        "Negative visual range rejected by validated factory");
    check(state,
        !invalid_confidence &&
        invalid_confidence.error().code == ErrorCode::invalid_argument,
        "Out of range confidence rejected by validated factory");
    check(state,
        vision.is_valid() && vision.actor() == actor,
        "Rejected raw settings leave actor state intact");

    return finish(state);
}
