#include "oros/ai/actor_simulation_fidelity_state.hpp"

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
    using namespace oros::world;

    static_assert(
        std::is_nothrow_move_constructible_v<
            ActorSimulationFidelityState>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            ActorSimulationFidelityState>);

    TestState state{};

    check(
        state,
        !is_valid_actor_simulation_fidelity(
            ActorSimulationFidelity::invalid),
        "Invalid fidelity is rejected");

    check(
        state,
        is_valid_actor_simulation_fidelity(
            ActorSimulationFidelity::
                statistical_distant),
        "Statistical-distant fidelity is valid");

    check(
        state,
        is_valid_actor_simulation_fidelity(
            ActorSimulationFidelity::
                deep_local),
        "Deep-local fidelity is valid");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::invalid,
            ActorSimulationFidelity::deep_local) ==
            ActorSimulationFidelityTransition::
                invalid,
        "Invalid source fidelity cannot classify");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::deep_local,
            ActorSimulationFidelity::invalid) ==
            ActorSimulationFidelityTransition::
                invalid,
        "Invalid target fidelity cannot classify");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::
                statistical_distant,
            ActorSimulationFidelity::
                statistical_distant) ==
            ActorSimulationFidelityTransition::
                unchanged,
        "Equal distant fidelities classify unchanged");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::deep_local,
            ActorSimulationFidelity::deep_local) ==
            ActorSimulationFidelityTransition::
                unchanged,
        "Equal local fidelities classify unchanged");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::
                statistical_distant,
            ActorSimulationFidelity::deep_local) ==
            ActorSimulationFidelityTransition::
                promotion_to_deep_local,
        "Distant to local classifies promotion");

    check(
        state,
        classify_actor_simulation_fidelity_transition(
            ActorSimulationFidelity::deep_local,
            ActorSimulationFidelity::
                statistical_distant) ==
            ActorSimulationFidelityTransition::
                demotion_to_statistical_distant,
        "Local to distant classifies demotion");

    const Result<ActorSimulationFidelityState>
        invalid_actor_result =
            ActorSimulationFidelityState::create(
                invalid_entity_id,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        !invalid_actor_result.has_value(),
        "State creation rejects invalid actor identity");

    check(
        state,
        !invalid_actor_result.has_value() &&
            invalid_actor_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid actor reports invalid_argument");

    const EntityId actor{
        0x4F524F53ULL,
        42ULL
    };

    const Result<ActorSimulationFidelityState>
        invalid_fidelity_result =
            ActorSimulationFidelityState::create(
                actor,
                ActorSimulationFidelity::invalid);

    check(
        state,
        !invalid_fidelity_result.has_value(),
        "State creation rejects invalid fidelity");

    check(
        state,
        !invalid_fidelity_result.has_value() &&
            invalid_fidelity_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid fidelity reports invalid_argument");

    Result<ActorSimulationFidelityState>
        distant_result =
            ActorSimulationFidelityState::create(
                actor,
                ActorSimulationFidelity::
                    statistical_distant);

    check(
        state,
        distant_result.has_value(),
        "State accepts persistent actor identity");

    if (distant_result.has_value())
    {
        ActorSimulationFidelityState actor_state{
            std::move(distant_result.value())
        };

        check(
            state,
            actor_state.is_valid(),
            "Created actor state is valid");

        check(
            state,
            actor_state.actor() == actor,
            "Actor state preserves World EntityId");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant,
            "Actor begins in statistical-distant fidelity");

        const Result<
            ActorSimulationFidelityTransition>
            unchanged_result =
                actor_state.transition_to(
                    ActorSimulationFidelity::
                        statistical_distant);

        check(
            state,
            unchanged_result.has_value() &&
                unchanged_result.value() ==
                    ActorSimulationFidelityTransition::
                        unchanged,
            "Repeated fidelity transition is explicit");

        check(
            state,
            actor_state.actor() == actor,
            "Unchanged transition preserves actor identity");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant,
            "Unchanged transition preserves fidelity");

        const Result<
            ActorSimulationFidelityTransition>
            promotion_result =
                actor_state.transition_to(
                    ActorSimulationFidelity::
                        deep_local);

        check(
            state,
            promotion_result.has_value() &&
                promotion_result.value() ==
                    ActorSimulationFidelityTransition::
                        promotion_to_deep_local,
            "Distant actor promotes to deep-local");

        check(
            state,
            actor_state.actor() == actor,
            "Promotion preserves persistent actor identity");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    deep_local,
            "Promotion changes only simulation fidelity");

        const Result<
            ActorSimulationFidelityTransition>
            invalid_transition_result =
                actor_state.transition_to(
                    ActorSimulationFidelity::invalid);

        check(
            state,
            !invalid_transition_result.has_value(),
            "Transition rejects invalid target fidelity");

        check(
            state,
            !invalid_transition_result.has_value() &&
                invalid_transition_result.error().code ==
                    ErrorCode::invalid_argument,
            "Invalid transition reports invalid_argument");

        check(
            state,
            actor_state.actor() == actor,
            "Rejected transition preserves actor identity");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    deep_local,
            "Rejected transition preserves prior fidelity");

        const Result<
            ActorSimulationFidelityTransition>
            demotion_result =
                actor_state.transition_to(
                    ActorSimulationFidelity::
                        statistical_distant);

        check(
            state,
            demotion_result.has_value() &&
                demotion_result.value() ==
                    ActorSimulationFidelityTransition::
                        demotion_to_statistical_distant,
            "Deep-local actor demotes to statistical-distant");

        check(
            state,
            actor_state.actor() == actor,
            "Demotion preserves persistent actor identity");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    statistical_distant,
            "Demotion changes only simulation fidelity");

        check(
            state,
            actor_state.is_valid(),
            "Actor remains valid after round-trip transitions");
    }

    Result<ActorSimulationFidelityState>
        local_result =
            ActorSimulationFidelityState::create(
                EntityId{
                    99ULL,
                    7ULL
                },
                ActorSimulationFidelity::
                    deep_local);

    check(
        state,
        local_result.has_value(),
        "State can begin directly in deep-local fidelity");

    if (local_result.has_value())
    {
        const ActorSimulationFidelityState& actor_state =
            local_result.value();

        check(
            state,
            actor_state.actor() ==
                EntityId{
                    99ULL,
                    7ULL
                },
            "Independent actor identity is preserved");

        check(
            state,
            actor_state.fidelity() ==
                ActorSimulationFidelity::
                    deep_local,
            "Direct deep-local creation preserves fidelity");
    }

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}