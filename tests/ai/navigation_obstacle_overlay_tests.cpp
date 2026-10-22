#include "oros/ai/navigation_obstacle_overlay.hpp"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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

    bool is_invalid_argument(
        const oros::foundation::Result<
            oros::ai::
                NavigationObstacleOverlay>&
            result)
    {
        return
            !result.has_value() &&
            result.error().code ==
                oros::foundation::
                    ErrorCode::invalid_argument;
    }
}

int main()
{
    using namespace oros::ai;
    using namespace oros::foundation;
    using namespace oros::world;

    static_assert(
        std::is_default_constructible_v<
            NavigationObstacleOverlay>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationObstacleOverlay>);

    static_assert(
        std::is_copy_assignable_v<
            NavigationObstacleOverlay>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationObstacleOverlay>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            NavigationObstacleOverlay>);

    static_assert(
        std::is_copy_constructible_v<
            NavigationTraversalBlock>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            NavigationTraversalBlock>);

    static_assert(
        std::is_same_v<
            decltype(
                NavigationObstacleOverlay::
                    create(
                        std::declval<
                            std::span<
                                const NavigationTraversalBlock>>())),
            Result<
                NavigationObstacleOverlay>>);

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationObstacleOverlay&>().
                    blocks(
                        std::declval<
                            NavigationPortalId>(),
                        std::declval<
                            NavigationPortalTraversalDirection>())),
            bool>);

    static_assert(
        noexcept(
            std::declval<
                const NavigationObstacleOverlay&>().
                blocks(
                    std::declval<
                        NavigationPortalId>(),
                    std::declval<
                        NavigationPortalTraversalDirection>())));

    static_assert(
        std::is_same_v<
            decltype(
                std::declval<
                    const NavigationObstacleOverlay&>().
                    blocks_in_canonical_order()),
            std::span<
                const NavigationTraversalBlock>>);

    TestState state{};

    const WorldCell namespace_cell{
        100,
        0,
        0
    };

    const NavigationPortalId portal_1{
        namespace_cell,
        10ULL
    };

    const NavigationPortalId portal_2{
        namespace_cell,
        20ULL
    };

    const NavigationPortalId portal_3{
        namespace_cell,
        30ULL
    };

    const NavigationPortalId
        currently_unknown_portal{
            WorldCell{
                999,
                999,
                999
            },
            77ULL
        };

    const auto first =
        NavigationPortalTraversalDirection::
            first_to_second;

    const auto second =
        NavigationPortalTraversalDirection::
            second_to_first;

    const auto invalid_direction =
        static_cast<
            NavigationPortalTraversalDirection>(
                255);

    NavigationObstacleOverlay
        default_overlay{};

    check(
        state,
        default_overlay.empty() &&
            default_overlay.size() == 0 &&
            default_overlay.
                blocks_in_canonical_order().
                empty(),
        "Default overlay is valid and empty");

    const auto empty_result =
        NavigationObstacleOverlay::create(
            std::span<
                const NavigationTraversalBlock>{});

    check(
        state,
        empty_result.has_value() &&
            empty_result.value() ==
                default_overlay,
        "Create empty equals default overlay");

    const std::vector<
        NavigationTraversalBlock>
        invalid_portal_blocks{
            NavigationTraversalBlock{
                invalid_navigation_portal_id,
                first
            }
        };

    const auto invalid_portal_result =
        NavigationObstacleOverlay::create(
            invalid_portal_blocks);

    check(
        state,
        is_invalid_argument(
            invalid_portal_result),
        "Invalid portal identity is rejected");

    const std::vector<
        NavigationTraversalBlock>
        invalid_direction_blocks{
            NavigationTraversalBlock{
                portal_1,
                invalid_direction
            }
        };

    const auto invalid_direction_result =
        NavigationObstacleOverlay::create(
            invalid_direction_blocks);

    check(
        state,
        is_invalid_argument(
            invalid_direction_result),
        "Invalid direction enum is rejected");

    const std::vector<
        NavigationTraversalBlock>
        first_direction_blocks{
            NavigationTraversalBlock{
                portal_1,
                first
            }
        };

    const auto first_direction_result =
        NavigationObstacleOverlay::create(
            first_direction_blocks);

    check(
        state,
        first_direction_result.has_value() &&
            first_direction_result.value().
                blocks(
                    portal_1,
                    first),
        "First-to-second block is accepted");

    const std::vector<
        NavigationTraversalBlock>
        second_direction_blocks{
            NavigationTraversalBlock{
                portal_1,
                second
            }
        };

    const auto second_direction_result =
        NavigationObstacleOverlay::create(
            second_direction_blocks);

    check(
        state,
        second_direction_result.has_value() &&
            second_direction_result.value().
                blocks(
                    portal_1,
                    second),
        "Second-to-first block is accepted");

    const std::vector<
        NavigationTraversalBlock>
        both_direction_blocks{
            NavigationTraversalBlock{
                portal_1,
                second
            },
            NavigationTraversalBlock{
                portal_1,
                first
            }
        };

    const auto both_direction_result =
        NavigationObstacleOverlay::create(
            both_direction_blocks);

    check(
        state,
        both_direction_result.has_value() &&
            both_direction_result.value().
                size() == 2 &&
            both_direction_result.value().
                blocks(
                    portal_1,
                    first) &&
            both_direction_result.value().
                blocks(
                    portal_1,
                    second),
        "Same portal may block both directions");

    const std::vector<
        NavigationTraversalBlock>
        duplicates{
            NavigationTraversalBlock{
                portal_2,
                first
            },
            NavigationTraversalBlock{
                portal_2,
                first
            },
            NavigationTraversalBlock{
                portal_2,
                first
            }
        };

    const auto duplicate_result =
        NavigationObstacleOverlay::create(
            duplicates);

    check(
        state,
        duplicate_result.has_value() &&
            duplicate_result.value().
                size() == 1,
        "Duplicate exact blocks collapse idempotently");

    const std::vector<
        NavigationTraversalBlock>
        unordered_blocks{
            NavigationTraversalBlock{
                portal_3,
                first
            },
            NavigationTraversalBlock{
                portal_1,
                second
            },
            NavigationTraversalBlock{
                portal_2,
                second
            },
            NavigationTraversalBlock{
                portal_1,
                first
            },
            NavigationTraversalBlock{
                portal_2,
                first
            },
            NavigationTraversalBlock{
                portal_3,
                second
            },
            NavigationTraversalBlock{
                portal_1,
                first
            }
        };

    const auto canonical_result =
        NavigationObstacleOverlay::create(
            unordered_blocks);

    if (!canonical_result.has_value())
    {
        return 1;
    }

    const auto canonical =
        canonical_result.value().
            blocks_in_canonical_order();

    check(
        state,
        canonical.size() == 6,
        "Producer duplicates do not survive canonicalization");

    check(
        state,
        canonical.size() == 6 &&
            canonical[0] ==
                NavigationTraversalBlock{
                    portal_1,
                    first
                } &&
            canonical[1] ==
                NavigationTraversalBlock{
                    portal_1,
                    second
                } &&
            canonical[2] ==
                NavigationTraversalBlock{
                    portal_2,
                    first
                } &&
            canonical[3] ==
                NavigationTraversalBlock{
                    portal_2,
                    second
                } &&
            canonical[4] ==
                NavigationTraversalBlock{
                    portal_3,
                    first
                } &&
            canonical[5] ==
                NavigationTraversalBlock{
                    portal_3,
                    second
                },
        "Producer order is canonicalized exactly");

    check(
        state,
        canonical[1].portal <
            canonical[2].portal,
        "Canonical portal order dominates direction order");

    check(
        state,
        canonical[0].portal ==
            canonical[1].portal &&
            canonical[0].direction ==
                first &&
            canonical[1].direction ==
                second,
        "First-to-second precedes second-to-first for one portal");

    check(
        state,
        canonical_result.value().
            blocks(
                portal_2,
                first),
        "Blocks lookup finds existing first direction");

    check(
        state,
        canonical_result.value().
            blocks(
                portal_2,
                second),
        "Blocks lookup finds existing second direction");

    check(
        state,
        !canonical_result.value().
            blocks(
                NavigationPortalId{
                    namespace_cell,
                    999ULL
                },
                first),
        "Blocks lookup is false for absent key");

    check(
        state,
        !canonical_result.value().
            blocks(
                invalid_navigation_portal_id,
                first),
        "Blocks lookup is false for invalid portal");

    check(
        state,
        !canonical_result.value().
            blocks(
                portal_1,
                invalid_direction),
        "Blocks lookup is false for invalid direction");

    const std::vector<
        NavigationTraversalBlock>
        unknown_portal_blocks{
            NavigationTraversalBlock{
                currently_unknown_portal,
                second
            }
        };

    const auto unknown_portal_result =
        NavigationObstacleOverlay::create(
            unknown_portal_blocks);

    check(
        state,
        unknown_portal_result.has_value() &&
            unknown_portal_result.value().
                blocks(
                    currently_unknown_portal,
                    second),
        "Block for currently unknown portal is accepted");

    const std::vector<
        NavigationTraversalBlock>
        canonical_input{
            NavigationTraversalBlock{
                portal_1,
                first
            },
            NavigationTraversalBlock{
                portal_1,
                second
            },
            NavigationTraversalBlock{
                portal_2,
                first
            },
            NavigationTraversalBlock{
                portal_2,
                second
            },
            NavigationTraversalBlock{
                portal_3,
                first
            },
            NavigationTraversalBlock{
                portal_3,
                second
            }
        };

    const auto canonical_input_result =
        NavigationObstacleOverlay::create(
            canonical_input);

    check(
        state,
        canonical_input_result.has_value() &&
            canonical_input_result.value() ==
                canonical_result.value(),
        "Equality is independent of producer order and duplicates");

    const NavigationObstacleOverlay copied{
        canonical_result.value()
    };

    check(
        state,
        copied ==
            canonical_result.value(),
        "NavigationObstacleOverlay supports copy construction");

    NavigationObstacleOverlay assigned{};

    assigned =
        canonical_result.value();

    check(
        state,
        assigned ==
            canonical_result.value(),
        "NavigationObstacleOverlay supports copy assignment");

    const auto assigned_blocks =
        assigned.
            blocks_in_canonical_order();

    const auto canonical_blocks =
        canonical_result.value().
            blocks_in_canonical_order();

    check(
        state,
        assigned_blocks.size() ==
                canonical_blocks.size() &&
            std::equal(
                assigned_blocks.begin(),
                assigned_blocks.end(),
                canonical_blocks.begin(),
                canonical_blocks.end()),
        "Exact canonical block span is exposed");

    std::cout
        << state.checks
        << " checks, "
        << state.failures
        << " failures\n";

    return state.failures == 0
        ? 0
        : 1;
}
