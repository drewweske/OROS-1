#include "oros/physics/physics_vector.hpp"

#include "oros/foundation/error.hpp"

#include <cmath>
#include <cstddef>
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
            << "\nPhysics vector test summary: "
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
        std::is_same_v<
            PhysicsScalar,
            double>);

    static_assert(
        std::is_trivially_copyable_v<
            PhysicsVector3>);

    static_assert(
        std::is_copy_constructible_v<
            PhysicsUnitVector3>);

    static_assert(
        std::is_move_constructible_v<
            PhysicsUnitVector3>);

    static_assert(
        physics_zero_vector ==
            PhysicsVector3{});

    static_assert(
        physics_positive_x +
            physics_positive_y ==
        PhysicsVector3{
            1.0,
            1.0,
            0.0
        });

    static_assert(
        physics_positive_x -
            physics_positive_y ==
        PhysicsVector3{
            1.0,
            -1.0,
            0.0
        });

    static_assert(
        -physics_positive_z ==
        PhysicsVector3{
            0.0,
            0.0,
            -1.0
        });

    static_assert(
        physics_positive_x *
            3.0 ==
        PhysicsVector3{
            3.0,
            0.0,
            0.0
        });

    static_assert(
        2.0 *
            physics_positive_y ==
        PhysicsVector3{
            0.0,
            2.0,
            0.0
        });

    static_assert(
        dot(
            physics_positive_x,
            physics_positive_y) ==
        0.0);

    static_assert(
        cross(
            physics_positive_x,
            physics_positive_y) ==
        physics_positive_z);

    TestState state{};

    check(
        state,
        is_valid_physics_tolerance(
            0.0),
        "Zero physics tolerance is valid");

    check(
        state,
        is_valid_physics_tolerance(
            physics_vector_zero_tolerance),
        "Default zero tolerance is valid");

    check(
        state,
        is_valid_physics_tolerance(
            physics_unit_length_tolerance),
        "Default unit-length tolerance is valid");

    check(
        state,
        !is_valid_physics_tolerance(
            -1.0),
        "Negative physics tolerance is rejected");

    check(
        state,
        !is_valid_physics_tolerance(
            std::numeric_limits<
                PhysicsScalar>::infinity()),
        "Infinite physics tolerance is rejected");

    check(
        state,
        !is_valid_physics_tolerance(
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN()),
        "NaN physics tolerance is rejected");

    check(
        state,
        physics_zero_vector.is_finite(),
        "Zero vector is finite");

    check(
        state,
        physics_positive_x.is_finite(),
        "Positive X vector is finite");

    const PhysicsVector3
        non_finite_x{
            std::numeric_limits<
                PhysicsScalar>::infinity(),
            0.0,
            0.0
        };

    check(
        state,
        !non_finite_x.is_finite(),
        "Infinite vector component is rejected");

    const PhysicsVector3
        non_finite_y{
            0.0,
            std::numeric_limits<
                PhysicsScalar>::quiet_NaN(),
            0.0
        };

    check(
        state,
        !non_finite_y.is_finite(),
        "NaN vector component is rejected");

    const PhysicsVector3
        three_four_vector{
            3.0,
            4.0,
            0.0
        };

    check(
        state,
        nearly_equal(
            three_four_vector.
                length_squared(),
            25.0),
        "Length squared is calculated correctly");

    check(
        state,
        nearly_equal(
            three_four_vector.length(),
            5.0),
        "Vector length is calculated correctly");

    check(
        state,
        physics_zero_vector.
            is_near_zero(),
        "Zero vector is near zero");

    check(
        state,
        PhysicsVector3{
            1.0e-13,
            0.0,
            0.0
        }.
            is_near_zero(),
        "Vector below default tolerance is near zero");

    check(
        state,
        !PhysicsVector3{
            1.0e-9,
            0.0,
            0.0
        }.
            is_near_zero(),
        "Vector above default tolerance is not near zero");

    check(
        state,
        PhysicsVector3{
            3.0,
            4.0,
            0.0
        }.
            is_near_zero(
                5.0),
        "Vector exactly at tolerance is near zero");

    check(
        state,
        !PhysicsVector3{
            3.0,
            4.0,
            0.0
        }.
            is_near_zero(
                4.999),
        "Vector above supplied tolerance is not near zero");

    check(
        state,
        !three_four_vector.
            is_near_zero(
                -1.0),
        "Near-zero query rejects negative tolerance");

    check(
        state,
        !non_finite_x.
            is_near_zero(),
        "Near-zero query rejects non-finite vectors");

    check_failure(
        state,
        physics_zero_vector.normalized(),
        ErrorCode::invalid_argument,
        "Zero vector cannot be normalized");

    check_failure(
        state,
        PhysicsVector3{
            1.0e-13,
            0.0,
            0.0
        }.
            normalized(),
        ErrorCode::invalid_argument,
        "Near-zero vector cannot be normalized");

    check_failure(
        state,
        three_four_vector.normalized(
            -1.0),
        ErrorCode::invalid_argument,
        "Normalization rejects negative tolerance");

    check_failure(
        state,
        three_four_vector.normalized(
            std::numeric_limits<
                PhysicsScalar>::infinity()),
        ErrorCode::invalid_argument,
        "Normalization rejects infinite tolerance");

    check_failure(
        state,
        non_finite_x.normalized(),
        ErrorCode::invalid_argument,
        "Normalization rejects infinite vector components");

    check_failure(
        state,
        non_finite_y.normalized(),
        ErrorCode::invalid_argument,
        "Normalization rejects NaN vector components");

    const auto normalized_result =
        three_four_vector.normalized();

    check(
        state,
        normalized_result.has_value(),
        "Finite non-zero vector is normalized");

    if (!normalized_result.has_value())
    {
        return finish(state);
    }

    const PhysicsVector3 normalized =
        normalized_result.value();

    check(
        state,
        normalized.is_finite(),
        "Normalized vector remains finite");

    check(
        state,
        nearly_equal(
            normalized,
            PhysicsVector3{
                0.6,
                0.8,
                0.0
            }),
        "Normalization preserves direction");

    check(
        state,
        nearly_equal(
            normalized.length(),
            1.0),
        "Normalized vector has unit length");

    const PhysicsScalar maximum =
        std::numeric_limits<
            PhysicsScalar>::max();

    const PhysicsVector3
        maximum_axis_vector{
            maximum,
            0.0,
            0.0
        };

    const auto
        maximum_axis_normalized_result =
            maximum_axis_vector.normalized();

    check(
        state,
        maximum_axis_normalized_result.
            has_value(),
        "Maximum finite axis vector is normalized");

    check(
        state,
        maximum_axis_normalized_result.
            has_value() &&
            nearly_equal(
                maximum_axis_normalized_result.
                    value(),
                physics_positive_x),
        "Maximum finite axis vector preserves direction");

    const PhysicsVector3
        maximum_diagonal_vector{
            maximum,
            maximum,
            0.0
        };

    const auto
        maximum_diagonal_normalized_result =
            maximum_diagonal_vector.
                normalized();

    check(
        state,
        maximum_diagonal_normalized_result.
            has_value(),
        "Maximum finite diagonal vector avoids intermediate overflow");

    check(
        state,
        maximum_diagonal_normalized_result.
            has_value() &&
            maximum_diagonal_normalized_result.
                value().
                is_finite(),
        "Maximum diagonal normalization remains finite");

    check(
        state,
        maximum_diagonal_normalized_result.
            has_value() &&
            nearly_equal(
                maximum_diagonal_normalized_result.
                    value().
                    length(),
                1.0),
        "Maximum diagonal normalization has unit length");

    const PhysicsScalar denormal =
        std::numeric_limits<
            PhysicsScalar>::denorm_min();

    const auto denormal_result =
        PhysicsVector3{
            denormal,
            0.0,
            0.0
        }.
            normalized(
                0.0);

    check(
        state,
        denormal_result.has_value(),
        "Positive denormal vector normalizes with zero tolerance");

    check(
        state,
        denormal_result.has_value() &&
            denormal_result.value() ==
                physics_positive_x,
        "Denormal normalization preserves axis direction");

    check(
        state,
        nearly_equal(
            1.0,
            1.0 +
                5.0e-10),
        "Scalar comparison accepts values within tolerance");

    check(
        state,
        !nearly_equal(
            1.0,
            1.0 +
                2.0e-9),
        "Scalar comparison rejects values outside tolerance");

    check(
        state,
        nearly_equal(
            2.0,
            2.0,
            0.0),
        "Scalar comparison accepts exact equality with zero tolerance");

    check(
        state,
        !nearly_equal(
            1.0,
            1.0,
            -1.0),
        "Scalar comparison rejects invalid tolerance");

    check(
        state,
        !nearly_equal(
            maximum,
            -maximum),
        "Scalar comparison safely rejects extreme opposite values");

    check(
        state,
        !nearly_equal(
            std::numeric_limits<
                PhysicsScalar>::infinity(),
            std::numeric_limits<
                PhysicsScalar>::infinity()),
        "Scalar comparison rejects infinities");

    check(
        state,
        nearly_equal(
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            },
            PhysicsVector3{
                1.0 +
                    1.0e-10,
                2.0 -
                    1.0e-10,
                3.0
            }),
        "Vector comparison accepts component differences within tolerance");

    check(
        state,
        !nearly_equal(
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            },
            PhysicsVector3{
                1.0,
                2.0,
                3.1
            }),
        "Vector comparison rejects an out-of-range component");

    check(
        state,
        dot(
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            },
            PhysicsVector3{
                4.0,
                5.0,
                6.0
            }) ==
            32.0,
        "Dot product is calculated correctly");

    check(
        state,
        cross(
            physics_positive_y,
            physics_positive_z) ==
            physics_positive_x,
        "Y cross Z produces positive X");

    check(
        state,
        cross(
            physics_positive_z,
            physics_positive_x) ==
            physics_positive_y,
        "Z cross X produces positive Y");

    check(
        state,
        cross(
            physics_positive_y,
            physics_positive_x) ==
            -physics_positive_z,
        "Cross product preserves handedness");

    check(
        state,
        dot(
            cross(
                physics_positive_x,
                physics_positive_y),
            physics_positive_x) ==
            0.0,
        "Cross product is perpendicular to its first operand");

    check(
        state,
        dot(
            cross(
                physics_positive_x,
                physics_positive_y),
            physics_positive_y) ==
            0.0,
        "Cross product is perpendicular to its second operand");

    const PhysicsUnitVector3 unit_x =
        PhysicsUnitVector3::positive_x();

    const PhysicsUnitVector3 unit_y =
        PhysicsUnitVector3::positive_y();

    const PhysicsUnitVector3 unit_z =
        PhysicsUnitVector3::positive_z();

    check(
        state,
        unit_x.is_valid(),
        "Positive X unit vector is valid");

    check(
        state,
        unit_y.is_valid(),
        "Positive Y unit vector is valid");

    check(
        state,
        unit_z.is_valid(),
        "Positive Z unit vector is valid");

    check(
        state,
        unit_x.vector() ==
            physics_positive_x,
        "Positive X unit vector preserves its direction");

    check(
        state,
        unit_y.vector() ==
            physics_positive_y,
        "Positive Y unit vector preserves its direction");

    check(
        state,
        unit_z.vector() ==
            physics_positive_z,
        "Positive Z unit vector preserves its direction");

    check_failure(
        state,
        PhysicsUnitVector3::create(
            physics_zero_vector),
        ErrorCode::invalid_argument,
        "Unit vector creation rejects zero direction");

    check_failure(
        state,
        PhysicsUnitVector3::create(
            non_finite_x),
        ErrorCode::invalid_argument,
        "Unit vector creation rejects non-finite direction");

    const auto unit_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                -2.0,
                0.0,
                0.0
            });

    check(
        state,
        unit_result.has_value(),
        "Unit vector is created from finite direction");

    if (!unit_result.has_value())
    {
        return finish(state);
    }

    const PhysicsUnitVector3
        negative_x_unit =
            unit_result.value();

    check(
        state,
        negative_x_unit.is_valid(),
        "Created unit vector satisfies its invariant");

    check(
        state,
        negative_x_unit.vector() ==
            PhysicsVector3{
                -1.0,
                0.0,
                0.0
            },
        "Created unit vector preserves normalized direction");

    const auto diagonal_unit_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            });

    check(
        state,
        diagonal_unit_result.has_value(),
        "Diagonal unit vector is created");

    check(
        state,
        diagonal_unit_result.has_value() &&
            diagonal_unit_result.value().
                is_valid(),
        "Diagonal unit vector satisfies unit-length invariant");

    check(
        state,
        diagonal_unit_result.has_value() &&
            nearly_equal(
                diagonal_unit_result.value().
                    vector().
                    length(),
                1.0),
        "Diagonal unit vector has unit magnitude");

    check(
        state,
        !unit_x.is_valid(
            -1.0),
        "Unit-vector validation rejects negative tolerance");

    check(
        state,
        PhysicsUnitVector3::create(
            physics_positive_x).
            has_value(),
        "Already normalized direction remains valid");

    PhysicsUnitVector3 copied_unit =
        unit_z;

    check(
        state,
        copied_unit ==
            unit_z,
        "Unit vector copy preserves identity");

    PhysicsUnitVector3 moved_unit =
        std::move(
            copied_unit);

    check(
        state,
        moved_unit ==
            unit_z,
        "Unit vector move preserves direction");

    return finish(state);
}