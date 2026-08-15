#include "oros/physical_world/world_segment_query.hpp"

#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/physics_vector.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>

namespace oros::physical_world
{
    namespace
    {
        inline constexpr std::size_t
            maximum_residency_cells_per_segment{
                65'536U
            };

        struct AxisTraversal final
        {
            int step{};

            physics::PhysicsScalar
                next_fraction{
                    (std::numeric_limits<
                        physics::PhysicsScalar>::
                        infinity)()
                };

            physics::PhysicsScalar
                delta_fraction{
                    (std::numeric_limits<
                        physics::PhysicsScalar>::
                        infinity)()
                };
        };

        struct UnavailableCell final
        {
            streaming::WorldCellKey
                cell_key{};

            physics::PhysicsScalar
                entry_fraction{};
        };

        [[nodiscard]]
        foundation::Result<
            AxisTraversal>
        create_axis_traversal(
            const physics::PhysicsScalar
                start_local,
            const physics::PhysicsScalar
                direction)
        {
            if (
                !std::isfinite(start_local) ||
                !std::isfinite(direction))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World segment axis traversal "
                    "requires finite coordinates.");
            }

            if (direction == 0.0)
            {
                return AxisTraversal{};
            }

            const int step =
                direction > 0.0
                    ? 1
                    : -1;

            const physics::PhysicsScalar
                boundary =
                    step > 0
                        ? world::
                            world_cell_half_extent_meters
                        : -world::
                            world_cell_half_extent_meters;

            const physics::PhysicsScalar
                boundary_distance =
                    boundary -
                    start_local;

            const physics::PhysicsScalar
                next_fraction =
                    boundary_distance /
                    direction;

            const physics::PhysicsScalar
                delta_fraction =
                    world::
                        world_cell_extent_meters /
                    std::abs(direction);

            if (
                !std::isfinite(next_fraction) ||
                !std::isfinite(delta_fraction) ||
                next_fraction < 0.0 ||
                delta_fraction <= 0.0)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World segment axis traversal "
                    "cannot be represented as finite "
                    "segment fractions.");
            }

            return AxisTraversal{
                step,
                next_fraction,
                delta_fraction
            };
        }

        [[nodiscard]]
        foundation::Status
        validate_axis_target(
            const std::int64_t start_cell,
            const std::int64_t target_cell,
            const AxisTraversal& traversal)
        {
            if (
                start_cell < target_cell &&
                traversal.step != 1)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "World segment positive cell "
                    "movement disagrees with its "
                    "physical displacement.");
            }

            if (
                start_cell > target_cell &&
                traversal.step != -1)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "World segment negative cell "
                    "movement disagrees with its "
                    "physical displacement.");
            }

            return foundation::Status{};
        }

        [[nodiscard]]
        foundation::Status
        advance_axis(
            std::int64_t& current_cell,
            const std::int64_t target_cell,
            AxisTraversal& traversal)
        {
            if (traversal.step > 0)
            {
                if (current_cell >= target_cell)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "World segment traversal "
                        "attempted to advance beyond "
                        "its positive target cell.");
                }

                ++current_cell;
            }
            else if (traversal.step < 0)
            {
                if (current_cell <= target_cell)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "World segment traversal "
                        "attempted to advance beyond "
                        "its negative target cell.");
                }

                --current_cell;
            }
            else
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "World segment traversal "
                    "attempted to advance a stationary "
                    "axis.");
            }

            if (current_cell == target_cell)
            {
                traversal.next_fraction =
                    (std::numeric_limits<
                        physics::PhysicsScalar>::
                        infinity)();

                return foundation::Status{};
            }

            traversal.next_fraction +=
                traversal.delta_fraction;

            if (!std::isfinite(
                    traversal.next_fraction))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World segment cell traversal "
                    "exceeds the finite fraction "
                    "range.");
            }

            return foundation::Status{};
        }

        [[nodiscard]]
        foundation::Result<
            std::optional<
                UnavailableCell>>
        find_first_unavailable_cell(
            const WorldCellColliderRegistry& registry,
            const std::uint64_t world_namespace,
            const world::WorldPosition&
                segment_start,
            const world::WorldPosition&
                segment_end,
            const physics::PhysicsVector3
                direction)
        {
            const auto x_result =
                create_axis_traversal(
                    segment_start.local().x,
                    direction.x);

            if (!x_result.has_value())
            {
                return foundation::fail(
                    x_result.error().code,
                    x_result.error().message);
            }

            const auto y_result =
                create_axis_traversal(
                    segment_start.local().y,
                    direction.y);

            if (!y_result.has_value())
            {
                return foundation::fail(
                    y_result.error().code,
                    y_result.error().message);
            }

            const auto z_result =
                create_axis_traversal(
                    segment_start.local().z,
                    direction.z);

            if (!z_result.has_value())
            {
                return foundation::fail(
                    z_result.error().code,
                    z_result.error().message);
            }

            AxisTraversal x =
                x_result.value();

            AxisTraversal y =
                y_result.value();

            AxisTraversal z =
                z_result.value();

            const world::WorldCell
                target_cell =
                    segment_end.cell();

            world::WorldCell current_cell =
                segment_start.cell();

            const auto x_target_status =
                validate_axis_target(
                    current_cell.x,
                    target_cell.x,
                    x);

            if (!x_target_status.has_value())
            {
                return foundation::fail(
                    x_target_status.error().code,
                    x_target_status.error().message);
            }

            const auto y_target_status =
                validate_axis_target(
                    current_cell.y,
                    target_cell.y,
                    y);

            if (!y_target_status.has_value())
            {
                return foundation::fail(
                    y_target_status.error().code,
                    y_target_status.error().message);
            }

            const auto z_target_status =
                validate_axis_target(
                    current_cell.z,
                    target_cell.z,
                    z);

            if (!z_target_status.has_value())
            {
                return foundation::fail(
                    z_target_status.error().code,
                    z_target_status.error().message);
            }

            physics::PhysicsScalar
                entry_fraction{0.0};

            std::size_t visited_cells{};

            for (;;)
            {
                ++visited_cells;

                if (
                    visited_cells >
                    maximum_residency_cells_per_segment)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World segment crosses more "
                        "residency cells than the "
                        "current Physical World query "
                        "limit permits.");
                }

                const streaming::WorldCellKey
                    current_key{
                        world_namespace,
                        current_cell
                    };

                if (!registry.contains_cell(
                        current_key))
                {
                    return std::optional<
                        UnavailableCell>{
                            UnavailableCell{
                                current_key,
                                entry_fraction
                            }
                        };
                }

                if (current_cell == target_cell)
                {
                    return std::optional<
                        UnavailableCell>{};
                }

                const physics::PhysicsScalar
                    next_fraction =
                        (std::min)({
                            x.next_fraction,
                            y.next_fraction,
                            z.next_fraction
                        });

                if (
                    !std::isfinite(next_fraction) ||
                    next_fraction <
                        entry_fraction ||
                    next_fraction > 1.0)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "World segment residency "
                        "traversal could not reach its "
                        "target cell inside the closed "
                        "segment.");
                }

                bool advanced{};

                if (
                    x.next_fraction ==
                    next_fraction)
                {
                    const auto status =
                        advance_axis(
                            current_cell.x,
                            target_cell.x,
                            x);

                    if (!status.has_value())
                    {
                        return foundation::fail(
                            status.error().code,
                            status.error().message);
                    }

                    advanced = true;
                }

                if (
                    y.next_fraction ==
                    next_fraction)
                {
                    const auto status =
                        advance_axis(
                            current_cell.y,
                            target_cell.y,
                            y);

                    if (!status.has_value())
                    {
                        return foundation::fail(
                            status.error().code,
                            status.error().message);
                    }

                    advanced = true;
                }

                if (
                    z.next_fraction ==
                    next_fraction)
                {
                    const auto status =
                        advance_axis(
                            current_cell.z,
                            target_cell.z,
                            z);

                    if (!status.has_value())
                    {
                        return foundation::fail(
                            status.error().code,
                            status.error().message);
                    }

                    advanced = true;
                }

                if (!advanced)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            internal_failure,
                        "World segment residency "
                        "traversal made no progress.");
                }

                entry_fraction =
                    next_fraction;
            }
        }
    }

    foundation::Result<
        WorldSegmentQueryFilter>
    WorldSegmentQueryFilter::
    create_excluding_owner(
        const world::EntityId excluded_owner)
    {
        if (!excluded_owner.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment query owner exclusion "
                "requires a valid EntityId.");
        }

        return WorldSegmentQueryFilter{
            excluded_owner
        };
    }

    bool
    WorldSegmentQueryFilter::is_valid()
        const noexcept
    {
        return
            !excluded_owner_.has_value() ||
            excluded_owner_->is_valid();
    }

    const std::optional<
        world::EntityId>&
    WorldSegmentQueryFilter::
    excluded_owner() const noexcept
    {
        return excluded_owner_;
    }

    WorldSegmentQueryFilter::
    WorldSegmentQueryFilter(
        const world::EntityId excluded_owner)
        noexcept
        : excluded_owner_{
              excluded_owner
          }
    {
    }

    WorldSegmentQueryResult::
    WorldSegmentQueryResult(
        const WorldSegmentQueryState state,
        std::optional<
            physics::ColliderSegmentHit> hit,
        std::optional<
            streaming::WorldCellKey>
            unavailable_cell)
        noexcept
        : state_{state},
          hit_{
              std::move(hit)
          },
          unavailable_cell_{
              std::move(unavailable_cell)
          }
    {
    }

    foundation::Result<
        WorldSegmentQueryResult>
    WorldSegmentQueryResult::create_clear()
    {
        return WorldSegmentQueryResult{
            WorldSegmentQueryState::clear,
            std::nullopt,
            std::nullopt
        };
    }

    foundation::Result<
        WorldSegmentQueryResult>
    WorldSegmentQueryResult::
    create_blocked(
        const physics::ColliderSegmentHit hit)
    {
        if (
            !hit.collider().is_valid() ||
            !std::isfinite(
                hit.segment_fraction()) ||
            hit.segment_fraction() < 0.0 ||
            hit.segment_fraction() > 1.0)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Blocked world segment query result "
                "requires a valid finite collider "
                "segment hit.");
        }

        return WorldSegmentQueryResult{
            WorldSegmentQueryState::blocked,
            std::optional<
                physics::ColliderSegmentHit>{
                    hit
                },
            std::nullopt
        };
    }

    foundation::Result<
        WorldSegmentQueryResult>
    WorldSegmentQueryResult::
    create_unavailable(
        const streaming::WorldCellKey
            unavailable_cell)
    {
        if (!unavailable_cell.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Unavailable world segment query "
                "result requires a valid world cell "
                "identity.");
        }

        return WorldSegmentQueryResult{
            WorldSegmentQueryState::unavailable,
            std::nullopt,
            std::optional<
                streaming::WorldCellKey>{
                    unavailable_cell
                }
        };
    }

    bool
    WorldSegmentQueryResult::is_valid()
        const noexcept
    {
        switch (state_)
        {
        case WorldSegmentQueryState::clear:
            return
                !hit_.has_value() &&
                !unavailable_cell_.has_value();

        case WorldSegmentQueryState::blocked:
            return
                hit_.has_value() &&
                hit_->collider().is_valid() &&
                std::isfinite(
                    hit_->segment_fraction()) &&
                hit_->segment_fraction() >= 0.0 &&
                hit_->segment_fraction() <= 1.0 &&
                !unavailable_cell_.has_value();

        case WorldSegmentQueryState::unavailable:
            return
                !hit_.has_value() &&
                unavailable_cell_.has_value() &&
                unavailable_cell_->is_valid();

        case WorldSegmentQueryState::invalid:
        default:
            return false;
        }
    }

    WorldSegmentQueryState
    WorldSegmentQueryResult::state()
        const noexcept
    {
        return state_;
    }

    const std::optional<
        physics::ColliderSegmentHit>&
    WorldSegmentQueryResult::hit()
        const noexcept
    {
        return hit_;
    }

    const std::optional<
        streaming::WorldCellKey>&
    WorldSegmentQueryResult::
    unavailable_cell()
        const noexcept
    {
        return unavailable_cell_;
    }

    foundation::Result<
        WorldSegmentQueryResult>
    query_world_segment(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const world::WorldPosition&
            segment_start,
        const world::WorldPosition&
            segment_end)
    {
        return query_world_segment(
            registry,
            world_namespace,
            segment_start,
            segment_end,
            WorldSegmentQueryFilter{});
    }

    foundation::Result<
        WorldSegmentQueryResult>
    query_world_segment(
        const WorldCellColliderRegistry& registry,
        const std::uint64_t world_namespace,
        const world::WorldPosition&
            segment_start,
        const world::WorldPosition&
            segment_end,
        const WorldSegmentQueryFilter& filter)
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment query requires a "
                "valid world namespace.");
        }

        if (!registry.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "World segment query requires a "
                "valid physical world collider "
                "registry.");
        }

        if (!filter.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment query requires a "
                "valid collider filter.");
        }

        if (
            filter.excluded_owner().has_value() &&
            filter.excluded_owner()->
                    world_namespace !=
                world_namespace)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment query excluded owner "
                "must belong to the queried world "
                "namespace.");
        }

        if (
            !segment_start.is_normalized() ||
            !segment_end.is_normalized())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment query endpoints must "
                "be normalized WorldPosition values.");
        }

        const auto displacement_result =
            segment_start.displacement_to(
                segment_end);

        if (!displacement_result.has_value())
        {
            return foundation::fail(
                displacement_result.error().code,
                displacement_result.error().message);
        }

        const world::WorldDisplacement&
            displacement =
                displacement_result.value();

        const physics::PhysicsVector3
            segment_direction{
                displacement.x,
                displacement.y,
                displacement.z
            };

        if (!segment_direction.is_finite())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World segment displacement exceeds "
                "the finite Physics query range.");
        }

        const auto unavailable_result =
            find_first_unavailable_cell(
                registry,
                world_namespace,
                segment_start,
                segment_end,
                segment_direction);

        if (!unavailable_result.has_value())
        {
            return foundation::fail(
                unavailable_result.error().code,
                unavailable_result.error().message);
        }

        const std::optional<
            UnavailableCell>
            first_unavailable =
                unavailable_result.value();

        if (
            first_unavailable.has_value() &&
            first_unavailable->
                    entry_fraction ==
                0.0)
        {
            return
                WorldSegmentQueryResult::
                    create_unavailable(
                        first_unavailable->
                            cell_key);
        }

        const auto relative_colliders_result =
            registry.colliders_relative_to(
                world_namespace,
                segment_start);

        if (!relative_colliders_result.has_value())
        {
            return foundation::fail(
                relative_colliders_result.
                    error().code,
                relative_colliders_result.
                    error().message);
        }

        std::optional<
            physics::ColliderSegmentHit>
            earliest_hit{};

        for (
            const physics::ColliderGeometry&
                geometry :
            relative_colliders_result.value())
        {
            if (
                filter.excluded_owner().
                    has_value() &&
                geometry.collider().owner ==
                    filter.excluded_owner().
                        value())
            {
                continue;
            }

            const auto hit_result =
                physics::
                    query_collider_segment_hit(
                        physics::
                            physics_zero_vector,
                        segment_direction,
                        geometry);

            if (!hit_result.has_value())
            {
                return foundation::fail(
                    hit_result.error().code,
                    hit_result.error().message);
            }

            if (!hit_result.value().has_value())
            {
                continue;
            }

            const physics::ColliderSegmentHit&
                hit =
                    hit_result.value().value();

            if (
                !earliest_hit.has_value() ||
                hit.segment_fraction() <
                    earliest_hit->
                        segment_fraction())
            {
                earliest_hit =
                    hit;
            }
        }

        if (first_unavailable.has_value())
        {
            if (
                !earliest_hit.has_value() ||
                first_unavailable->
                        entry_fraction <=
                    earliest_hit->
                        segment_fraction())
            {
                return
                    WorldSegmentQueryResult::
                        create_unavailable(
                            first_unavailable->
                                cell_key);
            }
        }

        if (earliest_hit.has_value())
        {
            return
                WorldSegmentQueryResult::
                    create_blocked(
                        earliest_hit.value());
        }

        return
            WorldSegmentQueryResult::
                create_clear();
    }
}
