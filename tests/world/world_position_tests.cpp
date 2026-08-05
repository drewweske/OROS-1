#include "oros/world/world_position.hpp"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>

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

    [[nodiscard]] bool nearly_equal(
        const double left,
        const double right,
        const double tolerance = 0.000'000'001)
        noexcept
    {
        return std::abs(left - right) <= tolerance;
    }

    [[nodiscard]] bool local_equal(
        const oros::world::LocalPosition& left,
        const oros::world::LocalPosition& right)
        noexcept
    {
        return
            nearly_equal(left.x, right.x) &&
            nearly_equal(left.y, right.y) &&
            nearly_equal(left.z, right.z);
    }

    [[nodiscard]] bool displacement_equal(
        const oros::world::WorldDisplacement& left,
        const oros::world::WorldDisplacement& right)
        noexcept
    {
        return
            nearly_equal(left.x, right.x) &&
            nearly_equal(left.y, right.y) &&
            nearly_equal(left.z, right.z);
    }
}

int main()
{
    using namespace oros::foundation;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        nearly_equal(
            world_cell_extent_meters,
            1024.0),
        "World cell extent is 1024 meters");

    check(
        state,
        nearly_equal(
            world_cell_half_extent_meters,
            512.0),
        "World cell half extent is 512 meters");

    const Result<WorldPosition> origin_result =
        WorldPosition::origin();

    check(
        state,
        origin_result.has_value(),
        "World origin initializes successfully");

    if (origin_result.has_value())
    {
        const WorldPosition& origin =
            origin_result.value();

        check(
            state,
            origin.cell() == WorldCell{},
            "World origin uses the zero cell");

        check(
            state,
            local_equal(
                origin.local(),
                LocalPosition{}),
            "World origin uses zero local coordinates");

        check(
            state,
            origin.is_normalized(),
            "World origin is normalized");
    }

    const Result<WorldPosition> canonical_result =
        WorldPosition::create(
            WorldCell{
                10,
                -20,
                30
            },
            LocalPosition{
                100.0,
                -200.0,
                300.0
            });

    check(
        state,
        canonical_result.has_value(),
        "Canonical coordinates initialize successfully");

    if (canonical_result.has_value())
    {
        const WorldPosition& position =
            canonical_result.value();

        check(
            state,
            position.cell() ==
                WorldCell{
                    10,
                    -20,
                    30
                },
            "Canonical coordinates preserve their cell");

        check(
            state,
            local_equal(
                position.local(),
                LocalPosition{
                    100.0,
                    -200.0,
                    300.0
                }),
            "Canonical coordinates preserve local values");

        check(
            state,
            position.is_normalized(),
            "Canonical coordinates report normalized");
    }

    const Result<WorldPosition>
        positive_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    512.0,
                    0.0,
                    0.0
                });

    check(
        state,
        positive_boundary_result.has_value(),
        "Positive cell boundary normalizes successfully");

    if (positive_boundary_result.has_value())
    {
        check(
            state,
            positive_boundary_result.value().cell() ==
                WorldCell{
                    1,
                    0,
                    0
                },
            "Positive boundary advances the cell");

        check(
            state,
            local_equal(
                positive_boundary_result.value().local(),
                LocalPosition{
                    -512.0,
                    0.0,
                    0.0
                }),
            "Positive boundary becomes negative half extent");

        check(
            state,
            positive_boundary_result.value().
                is_normalized(),
            "Positive boundary result is normalized");
    }

    const Result<WorldPosition>
        negative_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -512.0,
                    0.0,
                    0.0
                });

    check(
        state,
        negative_boundary_result.has_value(),
        "Negative half extent is accepted");

    if (negative_boundary_result.has_value())
    {
        check(
            state,
            negative_boundary_result.value().cell() ==
                WorldCell{},
            "Negative half extent remains in its cell");

        check(
            state,
            local_equal(
                negative_boundary_result.value().local(),
                LocalPosition{
                    -512.0,
                    0.0,
                    0.0
                }),
            "Negative half extent remains unchanged");

        check(
            state,
            negative_boundary_result.value().
                is_normalized(),
            "Negative half extent is normalized");
    }

    const Result<WorldPosition>
        positive_extent_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    1024.0,
                    0.0,
                    0.0
                });

    check(
        state,
        positive_extent_result.has_value(),
        "One positive cell extent normalizes");

    if (positive_extent_result.has_value())
    {
        check(
            state,
            positive_extent_result.value().cell() ==
                WorldCell{
                    1,
                    0,
                    0
                },
            "One positive extent advances one cell");

        check(
            state,
            local_equal(
                positive_extent_result.value().local(),
                LocalPosition{}),
            "One positive extent produces zero local offset");
    }

    const Result<WorldPosition>
        negative_extent_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -1024.0,
                    0.0,
                    0.0
                });

    check(
        state,
        negative_extent_result.has_value(),
        "One negative cell extent normalizes");

    if (negative_extent_result.has_value())
    {
        check(
            state,
            negative_extent_result.value().cell() ==
                WorldCell{
                    -1,
                    0,
                    0
                },
            "One negative extent retreats one cell");

        check(
            state,
            local_equal(
                negative_extent_result.value().local(),
                LocalPosition{}),
            "One negative extent produces zero local offset");
    }

    const Result<WorldPosition>
        multi_axis_result =
            WorldPosition::create(
                WorldCell{
                    5,
                    -5,
                    10
                },
                LocalPosition{
                    2048.25,
                    -1536.5,
                    512.0
                });

    check(
        state,
        multi_axis_result.has_value(),
        "All axes normalize independently");

    if (multi_axis_result.has_value())
    {
        check(
            state,
            multi_axis_result.value().cell() ==
                WorldCell{
                    7,
                    -7,
                    11
                },
            "Multi-axis normalization updates every cell");

        check(
            state,
            local_equal(
                multi_axis_result.value().local(),
                LocalPosition{
                    0.25,
                    511.5,
                    -512.0
                }),
            "Multi-axis normalization preserves remainders");

        check(
            state,
            multi_axis_result.value().is_normalized(),
            "Multi-axis result is normalized");
    }

    const Result<WorldPosition>
        below_negative_boundary_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -512.25,
                    0.0,
                    0.0
                });

    check(
        state,
        below_negative_boundary_result.has_value(),
        "Value below negative boundary normalizes");

    if (below_negative_boundary_result.has_value())
    {
        check(
            state,
            below_negative_boundary_result.value().cell() ==
                WorldCell{
                    -1,
                    0,
                    0
                },
            "Value below negative boundary retreats a cell");

        check(
            state,
            local_equal(
                below_negative_boundary_result.value().
                    local(),
                LocalPosition{
                    511.75,
                    0.0,
                    0.0
                }),
            "Negative overflow becomes a positive remainder");
    }

    const Result<WorldPosition>
        equivalent_first_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    512.0,
                    0.0,
                    0.0
                });

    const Result<WorldPosition>
        equivalent_second_result =
            WorldPosition::create(
                WorldCell{
                    1,
                    0,
                    0
                },
                LocalPosition{
                    -512.0,
                    0.0,
                    0.0
                });

    check(
        state,
        equivalent_first_result.has_value() &&
            equivalent_second_result.has_value(),
        "Equivalent representations initialize");

    check(
        state,
        equivalent_first_result.has_value() &&
            equivalent_second_result.has_value() &&
            equivalent_first_result.value() ==
                equivalent_second_result.value(),
        "Equivalent positions normalize identically");

    const double quiet_nan =
        (std::numeric_limits<double>::
            quiet_NaN)();

    const double infinity =
        (std::numeric_limits<double>::
            infinity)();

    const Result<WorldPosition> nan_x_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                quiet_nan,
                0.0,
                0.0
            });

    check(
        state,
        !nan_x_result.has_value(),
        "Creation rejects NaN on the X axis");

    check(
        state,
        !nan_x_result.has_value() &&
            nan_x_result.error().code ==
                ErrorCode::invalid_argument,
        "NaN creation reports invalid_argument");

    const Result<WorldPosition> nan_y_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                quiet_nan,
                0.0
            });

    check(
        state,
        !nan_y_result.has_value(),
        "Creation rejects NaN on the Y axis");

    const Result<WorldPosition> nan_z_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                0.0,
                0.0,
                quiet_nan
            });

    check(
        state,
        !nan_z_result.has_value(),
        "Creation rejects NaN on the Z axis");

    const Result<WorldPosition> infinity_result =
        WorldPosition::create(
            WorldCell{},
            LocalPosition{
                infinity,
                0.0,
                0.0
            });

    check(
        state,
        !infinity_result.has_value(),
        "Creation rejects positive infinity");

    check(
        state,
        !infinity_result.has_value() &&
            infinity_result.error().code ==
                ErrorCode::invalid_argument,
        "Infinite creation reports invalid_argument");

    const Result<WorldPosition>
        negative_infinity_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    -infinity,
                    0.0,
                    0.0
                });

    check(
        state,
        !negative_infinity_result.has_value(),
        "Creation rejects negative infinity");

    const double maximum_double =
        (std::numeric_limits<double>::max)();

    const Result<WorldPosition>
        enormous_local_result =
            WorldPosition::create(
                WorldCell{},
                LocalPosition{
                    maximum_double,
                    0.0,
                    0.0
                });

    check(
        state,
        !enormous_local_result.has_value(),
        "Creation rejects an unrepresentable cell offset");

    check(
        state,
        !enormous_local_result.has_value() &&
            enormous_local_result.error().code ==
                ErrorCode::invalid_argument,
        "Unrepresentable offset reports invalid_argument");

    const std::int64_t maximum_cell =
        (std::numeric_limits<std::int64_t>::max)();

    const std::int64_t minimum_cell =
        (std::numeric_limits<std::int64_t>::min)();

    const Result<WorldPosition>
        maximum_cell_valid_result =
            WorldPosition::create(
                WorldCell{
                    maximum_cell,
                    0,
                    0
                },
                LocalPosition{
                    511.0,
                    0.0,
                    0.0
                });

    check(
        state,
        maximum_cell_valid_result.has_value(),
        "Maximum cell accepts a canonical local value");

    const Result<WorldPosition>
        positive_overflow_result =
            WorldPosition::create(
                WorldCell{
                    maximum_cell,
                    0,
                    0
                },
                LocalPosition{
                    512.0,
                    0.0,
                    0.0
                });

    check(
        state,
        !positive_overflow_result.has_value(),
        "Normalization rejects positive cell overflow");

    check(
        state,
        !positive_overflow_result.has_value() &&
            positive_overflow_result.error().code ==
                ErrorCode::invalid_argument,
        "Positive cell overflow reports invalid_argument");

    const Result<WorldPosition>
        minimum_cell_valid_result =
            WorldPosition::create(
                WorldCell{
                    minimum_cell,
                    0,
                    0
                },
                LocalPosition{
                    -512.0,
                    0.0,
                    0.0
                });

    check(
        state,
        minimum_cell_valid_result.has_value(),
        "Minimum cell accepts negative half extent");

    const Result<WorldPosition>
        negative_overflow_result =
            WorldPosition::create(
                WorldCell{
                    minimum_cell,
                    0,
                    0
                },
                LocalPosition{
                    -513.0,
                    0.0,
                    0.0
                });

    check(
        state,
        !negative_overflow_result.has_value(),
        "Normalization rejects negative cell overflow");

    check(
        state,
        !negative_overflow_result.has_value() &&
            negative_overflow_result.error().code ==
                ErrorCode::invalid_argument,
        "Negative cell overflow reports invalid_argument");

    const Result<WorldPosition> base_result =
        WorldPosition::create(
            WorldCell{
                4,
                -3,
                2
            },
            LocalPosition{
                100.0,
                -200.0,
                300.0
            });

    check(
        state,
        base_result.has_value(),
        "Translation base position initializes");

    if (base_result.has_value())
    {
        const WorldPosition& base =
            base_result.value();

        const Result<WorldPosition>
            zero_translation_result =
                base.translated(
                    WorldDisplacement{});

        check(
            state,
            zero_translation_result.has_value(),
            "Zero translation succeeds");

        check(
            state,
            zero_translation_result.has_value() &&
                zero_translation_result.value() == base,
            "Zero translation preserves the position");

        const Result<WorldPosition>
            local_translation_result =
                base.translated(
                    WorldDisplacement{
                        10.0,
                        20.0,
                        -30.0
                    });

        check(
            state,
            local_translation_result.has_value(),
            "Translation inside one cell succeeds");

        if (local_translation_result.has_value())
        {
            check(
                state,
                local_translation_result.value().cell() ==
                    base.cell(),
                "In-cell translation preserves the cell");

            check(
                state,
                local_equal(
                    local_translation_result.value().
                        local(),
                    LocalPosition{
                        110.0,
                        -180.0,
                        270.0
                    }),
                "In-cell translation updates local values");
        }

        const Result<WorldPosition>
            crossing_translation_result =
                base.translated(
                    WorldDisplacement{
                        500.0,
                        -400.0,
                        300.0
                    });

        check(
            state,
            crossing_translation_result.has_value(),
            "Translation across cell boundaries succeeds");

        if (crossing_translation_result.has_value())
        {
            check(
                state,
                crossing_translation_result.value().cell() ==
                    WorldCell{
                        5,
                        -4,
                        3
                    },
                "Cross-boundary translation updates cells");

            check(
                state,
                local_equal(
                    crossing_translation_result.value().
                        local(),
                    LocalPosition{
                        -424.0,
                        424.0,
                        -424.0
                    }),
                "Cross-boundary translation keeps remainders");

            check(
                state,
                crossing_translation_result.value().
                    is_normalized(),
                "Cross-boundary translation is normalized");
        }

        const Result<WorldPosition>
            nonfinite_translation_result =
                base.translated(
                    WorldDisplacement{
                        infinity,
                        0.0,
                        0.0
                    });

        check(
            state,
            !nonfinite_translation_result.has_value(),
            "Translation rejects infinite displacement");

        check(
            state,
            !nonfinite_translation_result.has_value() &&
                nonfinite_translation_result.error().code ==
                    ErrorCode::invalid_argument,
            "Infinite translation reports invalid_argument");
    }

    if (maximum_cell_valid_result.has_value())
    {
        const Result<WorldPosition>
            translation_overflow_result =
                maximum_cell_valid_result.value().
                    translated(
                        WorldDisplacement{
                            1.0,
                            0.0,
                            0.0
                        });

        check(
            state,
            !translation_overflow_result.has_value(),
            "Translation rejects positive cell overflow");

        check(
            state,
            !translation_overflow_result.has_value() &&
                translation_overflow_result.error().code ==
                    ErrorCode::invalid_argument,
            "Translation overflow reports invalid_argument");
    }

    if (minimum_cell_valid_result.has_value())
    {
        const Result<WorldPosition>
            translation_underflow_result =
                minimum_cell_valid_result.value().
                    translated(
                        WorldDisplacement{
                            -1.0,
                            0.0,
                            0.0
                        });

        check(
            state,
            !translation_underflow_result.has_value(),
            "Translation rejects negative cell overflow");
    }

    const Result<WorldPosition> source_result =
        WorldPosition::create(
            WorldCell{
                0,
                10,
                -5
            },
            LocalPosition{
                500.0,
                -100.0,
                250.0
            });

    const Result<WorldPosition> destination_result =
        WorldPosition::create(
            WorldCell{
                1,
                8,
                -2
            },
            LocalPosition{
                -500.0,
                300.0,
                -250.0
            });

    check(
        state,
        source_result.has_value() &&
            destination_result.has_value(),
        "Displacement endpoints initialize");

    if (source_result.has_value() &&
        destination_result.has_value())
    {
        const WorldPosition& source =
            source_result.value();

        const WorldPosition& destination =
            destination_result.value();

        const Result<WorldDisplacement>
            zero_displacement_result =
                source.displacement_to(source);

        check(
            state,
            zero_displacement_result.has_value(),
            "Position computes displacement to itself");

        check(
            state,
            zero_displacement_result.has_value() &&
                displacement_equal(
                    zero_displacement_result.value(),
                    WorldDisplacement{}),
            "Self displacement is zero");

        const Result<WorldDisplacement>
            forward_result =
                source.displacement_to(destination);

        check(
            state,
            forward_result.has_value(),
            "Forward displacement calculates successfully");

        check(
            state,
            forward_result.has_value() &&
                displacement_equal(
                    forward_result.value(),
                    WorldDisplacement{
                        24.0,
                        -1648.0,
                        2572.0
                    }),
            "Forward displacement includes cells and locals");

        const Result<WorldDisplacement>
            reverse_result =
                destination.displacement_to(source);

        check(
            state,
            reverse_result.has_value(),
            "Reverse displacement calculates successfully");

        check(
            state,
            reverse_result.has_value() &&
                displacement_equal(
                    reverse_result.value(),
                    WorldDisplacement{
                        -24.0,
                        1648.0,
                        -2572.0
                    }),
            "Reverse displacement negates every axis");

        if (forward_result.has_value())
        {
            const Result<WorldPosition>
                reconstructed_result =
                    source.translated(
                        forward_result.value());

            check(
                state,
                reconstructed_result.has_value(),
                "Displacement can reconstruct destination");

            check(
                state,
                reconstructed_result.has_value() &&
                    reconstructed_result.value() ==
                        destination,
                "Translation by displacement reaches destination");
        }
    }

    const Result<WorldPosition> distant_source_result =
        WorldPosition::create(
            WorldCell{
                -1'000'000,
                2'000'000,
                -3'000'000
            },
            LocalPosition{
                -100.0,
                200.0,
                -300.0
            });

    const Result<WorldPosition>
        distant_destination_result =
            WorldPosition::create(
                WorldCell{
                    1'000'000,
                    -2'000'000,
                    3'000'000
                },
                LocalPosition{
                    100.0,
                    -200.0,
                    300.0
                });

    check(
        state,
        distant_source_result.has_value() &&
            distant_destination_result.has_value(),
        "Distant world positions initialize");

    if (distant_source_result.has_value() &&
        distant_destination_result.has_value())
    {
        const Result<WorldDisplacement>
            distant_displacement_result =
                distant_source_result.value().
                    displacement_to(
                        distant_destination_result.value());

        check(
            state,
            distant_displacement_result.has_value(),
            "Distant displacement calculates successfully");

        check(
            state,
            distant_displacement_result.has_value() &&
                displacement_equal(
                    distant_displacement_result.value(),
                    WorldDisplacement{
                        2'048'000'200.0,
                        -4'096'000'400.0,
                        6'144'000'600.0
                    }),
            "Distant displacement preserves meter values");
    }

    std::cout
        << "\nWorld position test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}