#include "oros/physics/broad_phase.hpp"

#include "oros/foundation/error.hpp"

#include <iostream>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace
{
    using oros::physics::AxisAlignedBounds;
    using oros::physics::BroadPhasePair;
    using oros::physics::BroadPhaseProxy;
    using oros::physics::ColliderId;
    using oros::physics::PhysicsVector3;

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

    [[nodiscard]]
    std::optional<BroadPhaseProxy>
    make_proxy(
        const ColliderId collider,
        const PhysicsVector3 minimum,
        const PhysicsVector3 maximum)
    {
        const auto bounds_result =
            AxisAlignedBounds::create(
                minimum,
                maximum);

        if (!bounds_result.has_value())
        {
            return std::nullopt;
        }

        const auto proxy_result =
            BroadPhaseProxy::create(
                collider,
                bounds_result.value());

        if (!proxy_result.has_value())
        {
            return std::nullopt;
        }

        return proxy_result.value();
    }

    [[nodiscard]]
    bool contains_single_pair(
        const oros::foundation::Result<
            std::vector<BroadPhasePair>>& result,
        const BroadPhasePair& expected_pair)
    {
        return
            result.has_value() &&
            result.value().size() == 1U &&
            result.value().front() ==
                expected_pair;
    }

    [[nodiscard]]
    bool contains_no_pairs(
        const oros::foundation::Result<
            std::vector<BroadPhasePair>>& result)
    {
        return
            result.has_value() &&
            result.value().empty();
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nBroad-phase generation test summary: "
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
    using oros::world::EntityId;

    TestState state{};

    constexpr ColliderId collider_a{
        EntityId{
            1U,
            1U
        },
        1U
    };

    constexpr ColliderId collider_b{
        EntityId{
            1U,
            1U
        },
        2U
    };

    constexpr ColliderId collider_c{
        EntityId{
            1U,
            1U
        },
        3U
    };

    constexpr ColliderId collider_d{
        EntityId{
            2U,
            1U
        },
        1U
    };

    static_assert(
        collider_a.is_valid());

    static_assert(
        collider_b.is_valid());

    static_assert(
        collider_c.is_valid());

    static_assert(
        collider_d.is_valid());

    check(
        state,
        collider_a < collider_b &&
            collider_b < collider_c &&
            collider_c < collider_d,
        "Collider fixtures have deterministic identity order");

    const std::vector<BroadPhaseProxy>
        empty_proxies{};

    const auto empty_result =
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    empty_proxies});

    check(
        state,
        contains_no_pairs(
            empty_result),
        "Empty broad phase produces no pairs");

    const auto main_proxy_a =
        make_proxy(
            collider_a,
            PhysicsVector3{
                10.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                20.0,
                2.0,
                2.0
            });

    const auto main_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                15.0,
                2.0,
                2.0
            });

    const auto main_proxy_c =
        make_proxy(
            collider_c,
            PhysicsVector3{
                5.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                12.0,
                2.0,
                2.0
            });

    const auto main_proxy_d =
        make_proxy(
            collider_d,
            PhysicsVector3{
                30.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                31.0,
                1.0,
                1.0
            });

    check(
        state,
        main_proxy_a.has_value(),
        "Main A proxy fixture is valid");

    check(
        state,
        main_proxy_b.has_value(),
        "Main B proxy fixture is valid");

    check(
        state,
        main_proxy_c.has_value(),
        "Main C proxy fixture is valid");

    check(
        state,
        main_proxy_d.has_value(),
        "Main D proxy fixture is valid");

    if (!main_proxy_a.has_value() ||
        !main_proxy_b.has_value() ||
        !main_proxy_c.has_value() ||
        !main_proxy_d.has_value())
    {
        return finish(state);
    }

    const auto pair_ab_result =
        BroadPhasePair::create(
            collider_a,
            collider_b);

    const auto pair_ac_result =
        BroadPhasePair::create(
            collider_a,
            collider_c);

    const auto pair_bc_result =
        BroadPhasePair::create(
            collider_b,
            collider_c);

    check(
        state,
        pair_ab_result.has_value(),
        "A-B expected pair fixture is valid");

    check(
        state,
        pair_ac_result.has_value(),
        "A-C expected pair fixture is valid");

    check(
        state,
        pair_bc_result.has_value(),
        "B-C expected pair fixture is valid");

    if (!pair_ab_result.has_value() ||
        !pair_ac_result.has_value() ||
        !pair_bc_result.has_value())
    {
        return finish(state);
    }

    const BroadPhasePair pair_ab =
        pair_ab_result.value();

    const BroadPhasePair pair_ac =
        pair_ac_result.value();

    const BroadPhasePair pair_bc =
        pair_bc_result.value();

    const std::vector<BroadPhasePair>
        expected_main_pairs{
            pair_ab,
            pair_ac,
            pair_bc
        };

    const std::vector<BroadPhaseProxy>
        identity_order{
            main_proxy_a.value(),
            main_proxy_b.value(),
            main_proxy_c.value(),
            main_proxy_d.value()
        };

    const auto identity_order_result =
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    identity_order});

    check(
        state,
        identity_order_result.has_value(),
        "Broad phase accepts identity-ordered proxies");

    check(
        state,
        identity_order_result.has_value() &&
            identity_order_result.value() ==
                expected_main_pairs,
        "Broad phase returns all overlapping canonical pairs");

    check(
        state,
        identity_order_result.has_value() &&
            identity_order_result.
                value().
                size() ==
                3U,
        "Separated proxy does not produce a candidate pair");

    check(
        state,
        identity_order_result.has_value() &&
            identity_order_result.
                value().
                at(0U) ==
                pair_ab,
        "Pair output begins with A-B");

    check(
        state,
        identity_order_result.has_value() &&
            identity_order_result.
                value().
                at(1U) ==
                pair_ac,
        "Pair output places A-C second");

    check(
        state,
        identity_order_result.has_value() &&
            identity_order_result.
                value().
                at(2U) ==
                pair_bc,
        "Pair output places B-C third");

    const std::vector<BroadPhaseProxy>
        sweep_order{
            main_proxy_b.value(),
            main_proxy_c.value(),
            main_proxy_a.value(),
            main_proxy_d.value()
        };

    const auto sweep_order_before =
        sweep_order;

    const auto sweep_order_result =
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    sweep_order});

    check(
        state,
        sweep_order_result.has_value(),
        "Broad phase accepts spatial sweep order");

    check(
        state,
        sweep_order_result.has_value() &&
            sweep_order_result.value() ==
                expected_main_pairs,
        "Spatial generation order is normalized to identity order");

    check(
        state,
        sweep_order ==
            sweep_order_before,
        "Broad-phase generation does not mutate proxy input");

    const std::vector<BroadPhaseProxy>
        reversed_order{
            main_proxy_d.value(),
            main_proxy_c.value(),
            main_proxy_b.value(),
            main_proxy_a.value()
        };

    const auto reversed_order_result =
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    reversed_order});

    check(
        state,
        reversed_order_result.has_value(),
        "Broad phase accepts reversed proxy order");

    check(
        state,
        reversed_order_result.has_value() &&
            reversed_order_result.value() ==
                expected_main_pairs,
        "Reversed insertion order produces identical pairs");

    check(
        state,
        identity_order_result.has_value() &&
            sweep_order_result.has_value() &&
            reversed_order_result.has_value() &&
            identity_order_result.value() ==
                sweep_order_result.value() &&
            sweep_order_result.value() ==
                reversed_order_result.value(),
        "Candidate generation is deterministic across permutations");

    const std::vector<BroadPhaseProxy>
        singleton_proxies{
            main_proxy_a.value()
        };

    const auto singleton_result =
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    singleton_proxies});

    check(
        state,
        contains_no_pairs(
            singleton_result),
        "Single proxy produces no pairs");

    const std::vector<BroadPhaseProxy>
        duplicate_identity_proxies{
            main_proxy_a.value(),
            main_proxy_a.value()
        };

    check_failure(
        state,
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    duplicate_identity_proxies}),
        ErrorCode::invalid_argument,
        "Broad phase rejects duplicate collider identity");

    const auto duplicate_far_proxy =
        make_proxy(
            collider_a,
            PhysicsVector3{
                100.0,
                100.0,
                100.0
            },
            PhysicsVector3{
                101.0,
                101.0,
                101.0
            });

    check(
        state,
        duplicate_far_proxy.has_value(),
        "Duplicate identity alternate-bounds fixture is valid");

    if (!duplicate_far_proxy.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        duplicate_different_bounds{
            main_proxy_a.value(),
            duplicate_far_proxy.value()
        };

    check_failure(
        state,
        generate_broad_phase_pairs(
            std::span<
                const BroadPhaseProxy>{
                    duplicate_different_bounds}),
        ErrorCode::invalid_argument,
        "Duplicate identity is rejected regardless of bounds");

    const auto unit_proxy_a =
        make_proxy(
            collider_a,
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            });

    check(
        state,
        unit_proxy_a.has_value(),
        "Unit A proxy fixture is valid");

    if (!unit_proxy_a.has_value())
    {
        return finish(state);
    }

    const auto x_separated_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                3.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                4.0,
                2.0,
                2.0
            });

    check(
        state,
        x_separated_proxy_b.has_value(),
        "X-separated proxy fixture is valid");

    if (!x_separated_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        x_separated_proxies{
            unit_proxy_a.value(),
            x_separated_proxy_b.value()
        };

    check(
        state,
        contains_no_pairs(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        x_separated_proxies})),
        "X-axis separation produces no pair");

    const auto y_separated_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                1.0,
                3.0,
                1.0
            },
            PhysicsVector3{
                3.0,
                4.0,
                2.0
            });

    check(
        state,
        y_separated_proxy_b.has_value(),
        "Y-separated proxy fixture is valid");

    if (!y_separated_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        y_separated_proxies{
            unit_proxy_a.value(),
            y_separated_proxy_b.value()
        };

    check(
        state,
        contains_no_pairs(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        y_separated_proxies})),
        "Y-axis separation produces no pair");

    const auto z_separated_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                1.0,
                1.0,
                3.0
            },
            PhysicsVector3{
                3.0,
                2.0,
                4.0
            });

    check(
        state,
        z_separated_proxy_b.has_value(),
        "Z-separated proxy fixture is valid");

    if (!z_separated_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        z_separated_proxies{
            unit_proxy_a.value(),
            z_separated_proxy_b.value()
        };

    check(
        state,
        contains_no_pairs(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        z_separated_proxies})),
        "Z-axis separation produces no pair");

    const auto face_touching_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                2.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                4.0,
                2.0,
                2.0
            });

    check(
        state,
        face_touching_proxy_b.has_value(),
        "Face-touching proxy fixture is valid");

    if (!face_touching_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        face_touching_proxies{
            unit_proxy_a.value(),
            face_touching_proxy_b.value()
        };

    check(
        state,
        contains_single_pair(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        face_touching_proxies}),
            pair_ab),
        "Face-touching bounds produce a candidate pair");

    const auto edge_touching_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                2.0,
                2.0,
                0.0
            },
            PhysicsVector3{
                4.0,
                4.0,
                2.0
            });

    check(
        state,
        edge_touching_proxy_b.has_value(),
        "Edge-touching proxy fixture is valid");

    if (!edge_touching_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        edge_touching_proxies{
            unit_proxy_a.value(),
            edge_touching_proxy_b.value()
        };

    check(
        state,
        contains_single_pair(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        edge_touching_proxies}),
            pair_ab),
        "Edge-touching bounds produce a candidate pair");

    const auto corner_touching_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            },
            PhysicsVector3{
                4.0,
                4.0,
                4.0
            });

    check(
        state,
        corner_touching_proxy_b.has_value(),
        "Corner-touching proxy fixture is valid");

    if (!corner_touching_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        corner_touching_proxies{
            unit_proxy_a.value(),
            corner_touching_proxy_b.value()
        };

    check(
        state,
        contains_single_pair(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        corner_touching_proxies}),
            pair_ab),
        "Corner-touching bounds produce a candidate pair");

    const auto contained_point_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            },
            PhysicsVector3{
                1.0,
                1.0,
                1.0
            });

    check(
        state,
        contained_point_proxy_b.has_value(),
        "Contained point proxy fixture is valid");

    if (!contained_point_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        contained_point_proxies{
            unit_proxy_a.value(),
            contained_point_proxy_b.value()
        };

    check(
        state,
        contains_single_pair(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        contained_point_proxies}),
            pair_ab),
        "Contained point bounds produce a candidate pair");

    const auto outside_point_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                3.0,
                1.0,
                1.0
            },
            PhysicsVector3{
                3.0,
                1.0,
                1.0
            });

    check(
        state,
        outside_point_proxy_b.has_value(),
        "Outside point proxy fixture is valid");

    if (!outside_point_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        outside_point_proxies{
            unit_proxy_a.value(),
            outside_point_proxy_b.value()
        };

    check(
        state,
        contains_no_pairs(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        outside_point_proxies})),
        "Separated point bounds produce no pair");

    const auto identical_bounds_proxy_b =
        make_proxy(
            collider_b,
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            },
            PhysicsVector3{
                2.0,
                2.0,
                2.0
            });

    check(
        state,
        identical_bounds_proxy_b.has_value(),
        "Identical bounds proxy fixture is valid");

    if (!identical_bounds_proxy_b.has_value())
    {
        return finish(state);
    }

    const std::vector<BroadPhaseProxy>
        identical_bounds_proxies{
            unit_proxy_a.value(),
            identical_bounds_proxy_b.value()
        };

    check(
        state,
        contains_single_pair(
            generate_broad_phase_pairs(
                std::span<
                    const BroadPhaseProxy>{
                        identical_bounds_proxies}),
            pair_ab),
        "Identical bounds with distinct identities produce one pair");

    return finish(state);
}