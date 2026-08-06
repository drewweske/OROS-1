#include "oros/physics/axis_aligned_bounds.hpp"

#include "oros/foundation/error.hpp"

#include <iostream>
#include <limits>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nAxis-aligned bounds test summary: "
            << state.checks - state.failures
            << '/'
            << state.checks
            << " passed.\n";

        return
            state.failures == 0
                ? 0
                : 1;
    }
}

int main()
{
    using namespace oros::physics;
    using oros::foundation::ErrorCode;

    static_assert(
        !std::is_default_constructible_v<
            AxisAlignedBounds>);

    static_assert(
        std::is_copy_constructible_v<
            AxisAlignedBounds>);

    static_assert(
        std::is_move_constructible_v<
            AxisAlignedBounds>);

    static_assert(
        std::is_copy_assignable_v<
            AxisAlignedBounds>);

    static_assert(
        std::is_move_assignable_v<
            AxisAlignedBounds>);

    TestState state{};

    const PhysicsScalar infinity =
        std::numeric_limits<
            PhysicsScalar>::infinity();

    const PhysicsScalar quiet_nan =
        std::numeric_limits<
            PhysicsScalar>::quiet_NaN();

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    check_failure(
        state,
        AxisAlignedBounds::create(
            PhysicsVector3{
                infinity,
                0.0,
                0.0
            },
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Bounds reject infinite minimum");

    check_failure(
        state,
        AxisAlignedBounds::create(
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                1.0,
                quiet_nan,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Bounds reject NaN maximum");

    check_failure(
        state,
        AxisAlignedBounds::create(
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Bounds reject reversed X endpoints");

    check_failure(
        state,
        AxisAlignedBounds::create(
            PhysicsVector3{
                0.0,
                2.0,
                0.0
            },
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Bounds reject reversed Y endpoints");

    check_failure(
        state,
        AxisAlignedBounds::create(
            PhysicsVector3{
                0.0,
                0.0,
                2.0
            },
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            }),
        ErrorCode::invalid_argument,
        "Bounds reject reversed Z endpoints");

    const PhysicsVector3 minimum{
        -2.0,
        -4.0,
        -6.0
    };

    const PhysicsVector3 maximum_point{
        2.0,
        4.0,
        6.0
    };

    const auto bounds_result =
        AxisAlignedBounds::create(
            minimum,
            maximum_point);

    check(
        state,
        bounds_result.has_value(),
        "Bounds accept finite ordered endpoints");

    if (!bounds_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds bounds =
        bounds_result.value();

    check(
        state,
        bounds.minimum() ==
            minimum,
        "Bounds preserve minimum endpoint");

    check(
        state,
        bounds.maximum() ==
            maximum_point,
        "Bounds preserve maximum endpoint");

    check(
        state,
        bounds.center() ==
            physics_zero_vector,
        "Symmetric bounds calculate zero center");

    check(
        state,
        bounds.half_extents() ==
            PhysicsVector3{
                2.0,
                4.0,
                6.0
            },
        "Bounds calculate half extents");

    const auto degenerate_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                3.0,
                3.0,
                3.0
            },
            PhysicsVector3{
                3.0,
                3.0,
                3.0
            });

    check(
        state,
        degenerate_result.has_value(),
        "Bounds accept a degenerate point volume");

    check(
        state,
        degenerate_result.has_value() &&
            degenerate_result.
                value().
                half_extents() ==
                physics_zero_vector,
        "Degenerate bounds have zero half extents");

    check_failure(
        state,
        AxisAlignedBounds::
            from_center_and_half_extents(
                PhysicsVector3{
                    infinity,
                    0.0,
                    0.0
                },
                PhysicsVector3{
                    1.0,
                    1.0,
                    1.0
                }),
        ErrorCode::invalid_argument,
        "Center construction rejects infinite center");

    check_failure(
        state,
        AxisAlignedBounds::
            from_center_and_half_extents(
                physics_zero_vector,
                PhysicsVector3{
                    1.0,
                    quiet_nan,
                    1.0
                }),
        ErrorCode::invalid_argument,
        "Center construction rejects NaN half extent");

    check_failure(
        state,
        AxisAlignedBounds::
            from_center_and_half_extents(
                physics_zero_vector,
                PhysicsVector3{
                    -1.0,
                    1.0,
                    1.0
                }),
        ErrorCode::invalid_argument,
        "Center construction rejects negative half extent");

    check_failure(
        state,
        AxisAlignedBounds::
            from_center_and_half_extents(
                PhysicsVector3{
                    maximum,
                    0.0,
                    0.0
                },
                PhysicsVector3{
                    maximum,
                    0.0,
                    0.0
                }),
        ErrorCode::invalid_argument,
        "Center construction rejects overflowing endpoints");

    const PhysicsVector3 center{
        10.0,
        20.0,
        30.0
    };

    const PhysicsVector3 half_extents{
        1.0,
        2.0,
        3.0
    };

    const auto centered_result =
        AxisAlignedBounds::
            from_center_and_half_extents(
                center,
                half_extents);

    check(
        state,
        centered_result.has_value(),
        "Bounds accept valid center and half extents");

    if (!centered_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds centered =
        centered_result.value();

    check(
        state,
        centered.minimum() ==
            PhysicsVector3{
                9.0,
                18.0,
                27.0
            },
        "Center construction calculates minimum");

    check(
        state,
        centered.maximum() ==
            PhysicsVector3{
                11.0,
                22.0,
                33.0
            },
        "Center construction calculates maximum");

    check(
        state,
        centered.center() ==
            center,
        "Center construction preserves center");

    check(
        state,
        centered.half_extents() ==
            half_extents,
        "Center construction preserves half extents");

    const auto zero_extent_result =
        AxisAlignedBounds::
            from_center_and_half_extents(
                center,
                physics_zero_vector);

    check(
        state,
        zero_extent_result.has_value(),
        "Center construction accepts zero half extents");

    check(
        state,
        zero_extent_result.has_value() &&
            zero_extent_result.
                value().
                minimum() ==
                center &&
            zero_extent_result.
                value().
                maximum() ==
                center,
        "Zero half extents produce point bounds");

    const auto extreme_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                -maximum,
                -maximum,
                -maximum
            },
            PhysicsVector3{
                maximum,
                maximum,
                maximum
            });

    check(
        state,
        extreme_result.has_value(),
        "Bounds accept full finite scalar range");

    check(
        state,
        extreme_result.has_value() &&
            extreme_result.
                value().
                center() ==
                physics_zero_vector,
        "Extreme symmetric bounds calculate finite center");

    check(
        state,
        extreme_result.has_value() &&
            extreme_result.
                value().
                half_extents() ==
                PhysicsVector3{
                    maximum,
                    maximum,
                    maximum
                },
        "Extreme bounds calculate finite half extents");

    check(
        state,
        bounds.contains(
            physics_zero_vector),
        "Bounds contain an interior point");

    check(
        state,
        bounds.contains(
            minimum),
        "Bounds contain minimum boundary");

    check(
        state,
        bounds.contains(
            maximum_point),
        "Bounds contain maximum boundary");

    check(
        state,
        !bounds.contains(
            PhysicsVector3{
                2.001,
                0.0,
                0.0
            }),
        "Bounds reject point beyond X maximum");

    check(
        state,
        !bounds.contains(
            PhysicsVector3{
                0.0,
                -4.001,
                0.0
            }),
        "Bounds reject point below Y minimum");

    check(
        state,
        !bounds.contains(
            PhysicsVector3{
                0.0,
                0.0,
                infinity
            }),
        "Bounds reject non-finite point");

    const auto inner_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                -1.0,
                -2.0,
                -3.0
            },
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            });

    if (!inner_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds inner =
        inner_result.value();

    check(
        state,
        bounds.contains(
            inner),
        "Bounds contain an interior volume");

    check(
        state,
        bounds.contains(
            bounds),
        "Bounds contain themselves");

    check(
        state,
        !inner.contains(
            bounds),
        "Interior volume does not contain outer bounds");

    const auto touching_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                2.0,
                -1.0,
                -1.0
            },
            PhysicsVector3{
                4.0,
                1.0,
                1.0
            });

    if (!touching_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds touching =
        touching_result.value();

    check(
        state,
        bounds.overlaps(
            touching),
        "Bounds overlap when faces touch");

    check(
        state,
        touching.overlaps(
            bounds),
        "Overlap query is symmetric");

    const auto separated_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                2.001,
                -1.0,
                -1.0
            },
            PhysicsVector3{
                4.0,
                1.0,
                1.0
            });

    if (!separated_result.has_value())
    {
        return finish(state);
    }

    const AxisAlignedBounds separated =
        separated_result.value();

    check(
        state,
        !bounds.overlaps(
            separated),
        "Bounds reject separated volume");

    check(
        state,
        bounds.overlaps(
            inner),
        "Bounds overlap contained volume");

    check(
        state,
        bounds.overlaps(
            bounds),
        "Bounds overlap themselves");

    const AxisAlignedBounds merged =
        bounds.merged_with(
            separated);

    check(
        state,
        merged.minimum() ==
            minimum,
        "Merged bounds preserve component minima");

    check(
        state,
        merged.maximum() ==
            PhysicsVector3{
                4.0,
                4.0,
                6.0
            },
        "Merged bounds preserve component maxima");

    check(
        state,
        merged.contains(
            bounds) &&
            merged.contains(
                separated),
        "Merged bounds contain both source volumes");

    check(
        state,
        bounds.merged_with(
            bounds) ==
            bounds,
        "Merging bounds with themselves preserves value");

    check_failure(
        state,
        bounds.translated(
            PhysicsVector3{
                quiet_nan,
                0.0,
                0.0
            }),
        ErrorCode::invalid_argument,
        "Translation rejects non-finite offset");

    const auto translated_result =
        bounds.translated(
            PhysicsVector3{
                10.0,
                -10.0,
                5.0
            });

    check(
        state,
        translated_result.has_value(),
        "Bounds accept finite translation");

    check(
        state,
        translated_result.has_value() &&
            translated_result.
                value().
                minimum() ==
                PhysicsVector3{
                    8.0,
                    -14.0,
                    -1.0
                },
        "Translation offsets minimum");

    check(
        state,
        translated_result.has_value() &&
            translated_result.
                value().
                maximum() ==
                PhysicsVector3{
                    12.0,
                    -6.0,
                    11.0
                },
        "Translation offsets maximum");

    check(
        state,
        translated_result.has_value() &&
            translated_result.
                value().
                half_extents() ==
                bounds.half_extents(),
        "Translation preserves half extents");

    const auto maximum_point_result =
        AxisAlignedBounds::create(
            PhysicsVector3{
                maximum,
                0.0,
                0.0
            },
            PhysicsVector3{
                maximum,
                0.0,
                0.0
            });

    if (!maximum_point_result.has_value())
    {
        return finish(state);
    }

    check_failure(
        state,
        maximum_point_result.
            value().
            translated(
                PhysicsVector3{
                    maximum,
                    0.0,
                    0.0
                }),
        ErrorCode::invalid_argument,
        "Translation rejects scalar overflow");

    check_failure(
        state,
        bounds.expanded(
            -1.0),
        ErrorCode::invalid_argument,
        "Expansion rejects negative margin");

    check_failure(
        state,
        bounds.expanded(
            infinity),
        ErrorCode::invalid_argument,
        "Expansion rejects infinite margin");

    check_failure(
        state,
        bounds.expanded(
            quiet_nan),
        ErrorCode::invalid_argument,
        "Expansion rejects NaN margin");

    const auto unchanged_result =
        bounds.expanded(
            0.0);

    check(
        state,
        unchanged_result.has_value() &&
            unchanged_result.value() ==
                bounds,
        "Zero expansion preserves bounds");

    const auto expanded_result =
        bounds.expanded(
            2.0);

    check(
        state,
        expanded_result.has_value(),
        "Bounds accept finite expansion");

    check(
        state,
        expanded_result.has_value() &&
            expanded_result.
                value().
                minimum() ==
                PhysicsVector3{
                    -4.0,
                    -6.0,
                    -8.0
                },
        "Expansion decreases minimum");

    check(
        state,
        expanded_result.has_value() &&
            expanded_result.
                value().
                maximum() ==
                PhysicsVector3{
                    4.0,
                    6.0,
                    8.0
                },
        "Expansion increases maximum");

    check(
        state,
        expanded_result.has_value() &&
            expanded_result.
                value().
                contains(
                    bounds),
        "Expanded bounds contain original bounds");

    check_failure(
        state,
        maximum_point_result.
            value().
            expanded(
                maximum),
        ErrorCode::invalid_argument,
        "Expansion rejects scalar overflow");

    AxisAlignedBounds copied_bounds =
        bounds;

    check(
        state,
        copied_bounds ==
            bounds,
        "Bounds copy preserves value");

    AxisAlignedBounds moved_bounds =
        std::move(
            copied_bounds);

    check(
        state,
        moved_bounds ==
            bounds,
        "Bounds move preserves value");

    return finish(state);
}