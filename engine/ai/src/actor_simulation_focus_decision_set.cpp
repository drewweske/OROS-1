#include "oros/ai/actor_simulation_focus_decision_set.hpp"

#include <cstddef>
#include <new>
#include <utility>
#include <vector>

namespace oros::ai
{
    foundation::Result<
        ActorSimulationFocusDecisionSet>
    ActorSimulationFocusDecisionSet::evaluate(
        const ActorSimulationFocusPolicy& policy,
        const ActorSimulationFidelityRegistry&
            fidelity_registry,
        const ActorSimulationPositionSnapshotSet&
            actor_positions,
        const ActorSimulationFocusSourceRegistry&
            focus_sources)
    {
        const std::span<
            const ActorSimulationFidelityState>
            states =
                fidelity_registry.
                    states_in_entity_order();

        const std::span<
            const ActorSimulationPositionSnapshot>
            positions =
                actor_positions.
                    snapshots_in_entity_order();

        std::vector<
            ActorSimulationFocusDecision>
            decisions{};

        try
        {
            decisions.reserve(
                states.size());
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Actor simulation focus decision "
                "set could not allocate output "
                "storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "Actor simulation focus decision "
                "set failed while reserving "
                "output storage.");
        }

        std::size_t position_index = 0U;

        for (
            const ActorSimulationFidelityState&
                state :
            states)
        {
            while (
                position_index <
                    positions.size() &&
                positions[position_index].
                        actor() <
                    state.actor())
            {
                ++position_index;
            }

            if (
                position_index >=
                    positions.size() ||
                positions[position_index].
                        actor() !=
                    state.actor())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        not_found,
                    "Actor simulation focus "
                    "evaluation requires a "
                    "position snapshot for every "
                    "registered actor.");
            }

            foundation::Result<
                ActorSimulationFocusDecision>
                decision_result =
                    ActorSimulationFocusDecision::
                        evaluate(
                            policy,
                            state,
                            positions[
                                position_index].
                                position(),
                            focus_sources);

            if (!decision_result.has_value())
            {
                return foundation::fail(
                    decision_result.error().code,
                    decision_result.error().message);
            }

            try
            {
                decisions.emplace_back(
                    std::move(
                        decision_result.value()));
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Actor simulation focus "
                    "decision set could not append "
                    "an evaluated decision.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Actor simulation focus "
                    "decision set failed while "
                    "recording an evaluated "
                    "decision.");
            }

            ++position_index;
        }

        return ActorSimulationFocusDecisionSet{
            std::move(decisions)
        };
    }

    std::size_t
    ActorSimulationFocusDecisionSet::size()
        const noexcept
    {
        return decisions_.size();
    }

    bool
    ActorSimulationFocusDecisionSet::empty()
        const noexcept
    {
        return decisions_.empty();
    }

    std::span<
        const ActorSimulationFocusDecision>
    ActorSimulationFocusDecisionSet::
        decisions_in_entity_order()
        const noexcept
    {
        return decisions_;
    }

    ActorSimulationFocusDecisionSet::
        ActorSimulationFocusDecisionSet(
            std::vector<
                ActorSimulationFocusDecision>
                    decisions)
            noexcept
        : decisions_{std::move(decisions)}
    {
    }
}