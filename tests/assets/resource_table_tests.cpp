#include "oros/assets/resource_table.hpp"

#include <cstddef>
#include <cstdint>
#include <iostream>
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

    struct MoveOnlyResource final
    {
        int value{};

        explicit MoveOnlyResource(
            const int initial_value) noexcept
            : value{initial_value}
        {
        }

        MoveOnlyResource(
            const MoveOnlyResource&) = delete;

        MoveOnlyResource&
        operator=(
            const MoveOnlyResource&) = delete;

        MoveOnlyResource(
            MoveOnlyResource&& other) noexcept
            : value{
                std::exchange(
                    other.value,
                    -1)
            }
        {
        }

        MoveOnlyResource&
        operator=(
            MoveOnlyResource&& other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }

            value =
                std::exchange(
                    other.value,
                    -1);

            return *this;
        }

        ~MoveOnlyResource() noexcept = default;
    };

    using TestTable =
        oros::assets::ResourceTable<
            MoveOnlyResource>;

    static_assert(
        std::is_nothrow_move_constructible_v<
            MoveOnlyResource>);

    static_assert(
        std::is_nothrow_destructible_v<
            MoveOnlyResource>);

    static_assert(
        !std::is_copy_constructible_v<
            TestTable>);

    static_assert(
        !std::is_copy_assignable_v<
            TestTable>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            TestTable>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            TestTable>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    TestTable table{};

    check(
        state,
        table.empty(),
        "New resource table is empty");

    check(
        state,
        table.size() == 0U,
        "New resource table has zero resources");

    check(
        state,
        table.slot_count() == 0U,
        "New resource table has zero slots");

    check(
        state,
        !table.contains(
            invalid_resource_handle),
        "New table rejects the invalid handle");

    check(
        state,
        !table.contains(
            invalid_asset_id),
        "New table rejects the invalid asset identity");

    check(
        state,
        table.find(
            invalid_resource_handle) ==
            nullptr,
        "Invalid handle lookup returns null");

    check(
        state,
        table.handle_for(
            invalid_asset_id) ==
            invalid_resource_handle,
        "Invalid asset lookup returns an invalid handle");

    check(
        state,
        table.asset_for(
            invalid_resource_handle) ==
            invalid_asset_id,
        "Invalid handle returns an invalid asset identity");

    const Status invalid_erase_result =
        table.erase(
            invalid_resource_handle);

    check(
        state,
        !invalid_erase_result.has_value(),
        "Erasing an invalid handle fails");

    check(
        state,
        !invalid_erase_result.has_value() &&
            invalid_erase_result.error().code ==
                ErrorCode::not_found,
        "Invalid handle erase reports not_found");

    const Result<ResourceHandle>
        invalid_insert_result =
            table.insert(
                invalid_asset_id,
                MoveOnlyResource{5});

    check(
        state,
        !invalid_insert_result.has_value(),
        "Insertion rejects an invalid asset identity");

    check(
        state,
        !invalid_insert_result.has_value() &&
            invalid_insert_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid asset insertion reports invalid_argument");

    check(
        state,
        table.empty() &&
            table.slot_count() == 0U,
        "Rejected insertion does not mutate the table");

    const AssetId first_asset{
        1ULL,
        1ULL
    };

    const AssetId second_asset{
        1ULL,
        2ULL
    };

    const AssetId replacement_asset{
        1ULL,
        3ULL
    };

    const Result<ResourceHandle>
        first_insert_result =
            table.insert(
                first_asset,
                MoveOnlyResource{10});

    check(
        state,
        first_insert_result.has_value(),
        "Table inserts its first resource");

    ResourceHandle first_handle{};

    if (first_insert_result.has_value())
    {
        first_handle =
            first_insert_result.value();
    }

    check(
        state,
        first_handle ==
            ResourceHandle{
                1U,
                1U
            },
        "First resource receives slot one generation one");

    check(
        state,
        table.size() == 1U,
        "First insertion increases the resource count");

    check(
        state,
        table.slot_count() == 1U,
        "First insertion creates one slot");

    check(
        state,
        !table.empty(),
        "Table is non-empty after insertion");

    check(
        state,
        table.contains(first_asset),
        "Table contains the first asset identity");

    check(
        state,
        table.contains(first_handle),
        "Table contains the first resource handle");

    check(
        state,
        table.handle_for(first_asset) ==
            first_handle,
        "Asset lookup returns the first handle");

    check(
        state,
        table.asset_for(first_handle) ==
            first_asset,
        "Handle lookup returns the first asset");

    MoveOnlyResource* first_resource =
        table.find(first_handle);

    check(
        state,
        first_resource != nullptr,
        "Mutable lookup finds the first resource");

    check(
        state,
        first_resource != nullptr &&
            first_resource->value == 10,
        "First resource preserves its value");

    if (first_resource != nullptr)
    {
        first_resource->value = 11;
    }

    const TestTable& const_table =
        table;

    const MoveOnlyResource*
        const_first_resource =
            const_table.find(first_handle);

    check(
        state,
        const_first_resource != nullptr,
        "Const lookup finds the first resource");

    check(
        state,
        const_first_resource != nullptr &&
            const_first_resource->value == 11,
        "Const lookup observes mutable changes");

    const Result<ResourceHandle>
        duplicate_insert_result =
            table.insert(
                first_asset,
                MoveOnlyResource{99});

    check(
        state,
        !duplicate_insert_result.has_value(),
        "Table rejects a duplicate asset identity");

    check(
        state,
        !duplicate_insert_result.has_value() &&
            duplicate_insert_result.error().code ==
                ErrorCode::invalid_state,
        "Duplicate insertion reports invalid_state");

    check(
        state,
        table.size() == 1U &&
            table.slot_count() == 1U,
        "Duplicate insertion does not mutate table counts");

    check(
        state,
        table.find(first_handle) != nullptr &&
            table.find(first_handle)->value == 11,
        "Duplicate insertion preserves the original resource");

    const Result<ResourceHandle>
        second_insert_result =
            table.insert(
                second_asset,
                MoveOnlyResource{20});

    check(
        state,
        second_insert_result.has_value(),
        "Table inserts a second resource");

    ResourceHandle second_handle{};

    if (second_insert_result.has_value())
    {
        second_handle =
            second_insert_result.value();
    }

    check(
        state,
        second_handle ==
            ResourceHandle{
                2U,
                1U
            },
        "Second resource receives slot two generation one");

    check(
        state,
        table.size() == 2U &&
            table.slot_count() == 2U,
        "Second insertion appends a second slot");

    const Status first_erase_result =
        table.erase(first_handle);

    check(
        state,
        first_erase_result.has_value(),
        "Table erases the first resource");

    check(
        state,
        table.size() == 1U,
        "Erasing a resource decreases the count");

    check(
        state,
        table.slot_count() == 2U,
        "Erasing a resource preserves allocated slots");

    check(
        state,
        !table.contains(first_handle),
        "Erased handle is no longer contained");

    check(
        state,
        !table.contains(first_asset),
        "Erased asset identity is no longer contained");

    check(
        state,
        table.find(first_handle) ==
            nullptr,
        "Erased handle lookup returns null");

    check(
        state,
        table.handle_for(first_asset) ==
            invalid_resource_handle,
        "Erased asset lookup returns an invalid handle");

    check(
        state,
        table.asset_for(first_handle) ==
            invalid_asset_id,
        "Erased handle returns an invalid asset");

    check(
        state,
        table.contains(second_handle) &&
            table.contains(second_asset),
        "Erasing one resource preserves another resource");

    const Status repeated_erase_result =
        table.erase(first_handle);

    check(
        state,
        !repeated_erase_result.has_value(),
        "Erasing an already erased handle fails");

    check(
        state,
        !repeated_erase_result.has_value() &&
            repeated_erase_result.error().code ==
                ErrorCode::not_found,
        "Repeated erase reports not_found");

    const Result<ResourceHandle>
        replacement_insert_result =
            table.insert(
                replacement_asset,
                MoveOnlyResource{30});

    check(
        state,
        replacement_insert_result.has_value(),
        "Table inserts a replacement resource");

    ResourceHandle replacement_handle{};

    if (replacement_insert_result.has_value())
    {
        replacement_handle =
            replacement_insert_result.value();
    }

    check(
        state,
        replacement_handle.slot ==
            first_handle.slot,
        "Replacement resource reuses the erased slot");

    check(
        state,
        replacement_handle.generation ==
            first_handle.generation + 1U,
        "Reused slot advances its generation");

    check(
        state,
        replacement_handle !=
            first_handle,
        "Replacement handle differs from the stale handle");

    check(
        state,
        table.slot_count() == 2U,
        "Slot reuse does not grow the table");

    check(
        state,
        table.size() == 2U,
        "Slot reuse restores the live resource count");

    check(
        state,
        !table.contains(first_handle) &&
            table.find(first_handle) ==
                nullptr,
        "Stale handle remains rejected after slot reuse");

    check(
        state,
        table.asset_for(first_handle) ==
            invalid_asset_id,
        "Stale handle cannot resolve the replacement asset");

    check(
        state,
        table.contains(
            replacement_handle),
        "Replacement handle is contained");

    check(
        state,
        table.handle_for(
            replacement_asset) ==
            replacement_handle,
        "Replacement asset resolves to its new handle");

    check(
        state,
        table.asset_for(
            replacement_handle) ==
            replacement_asset,
        "Replacement handle resolves to its asset");

    check(
        state,
        table.find(
            replacement_handle) != nullptr &&
            table.find(
                replacement_handle)->value == 30,
        "Replacement handle finds the replacement resource");

    const Status stale_erase_result =
        table.erase(first_handle);

    check(
        state,
        !stale_erase_result.has_value(),
        "Stale generation cannot erase a replacement resource");

    check(
        state,
        !stale_erase_result.has_value() &&
            stale_erase_result.error().code ==
                ErrorCode::not_found,
        "Stale generation erase reports not_found");

    check(
        state,
        table.contains(
            replacement_handle),
        "Failed stale erase preserves the replacement");

    TestTable move_source{};

    const AssetId move_erased_asset{
        2ULL,
        1ULL
    };

    const AssetId move_live_asset{
        2ULL,
        2ULL
    };

    const Result<ResourceHandle>
        move_erased_insert_result =
            move_source.insert(
                move_erased_asset,
                MoveOnlyResource{40});

    const Result<ResourceHandle>
        move_live_insert_result =
            move_source.insert(
                move_live_asset,
                MoveOnlyResource{50});

    check(
        state,
        move_erased_insert_result.has_value() &&
            move_live_insert_result.has_value(),
        "Move source accepts two resources");

    ResourceHandle move_erased_handle{};
    ResourceHandle move_live_handle{};

    if (move_erased_insert_result.has_value())
    {
        move_erased_handle =
            move_erased_insert_result.value();
    }

    if (move_live_insert_result.has_value())
    {
        move_live_handle =
            move_live_insert_result.value();
    }

    const Status move_source_erase_result =
        move_source.erase(
            move_erased_handle);

    check(
        state,
        move_source_erase_result.has_value(),
        "Move source creates a reusable free slot");

    TestTable move_destination{
        std::move(move_source)
    };

    check(
        state,
        move_destination.size() == 1U &&
            move_destination.slot_count() == 2U,
        "Move construction transfers resources and slots");

    check(
        state,
        move_destination.contains(
            move_live_handle),
        "Move construction preserves live handles");

    check(
        state,
        move_destination.handle_for(
            move_live_asset) ==
            move_live_handle,
        "Move construction preserves asset lookup");

    check(
        state,
        move_destination.find(
            move_live_handle) != nullptr &&
            move_destination.find(
                move_live_handle)->value == 50,
        "Move construction preserves resource values");

    check(
        state,
        move_source.empty() &&
            move_source.size() == 0U &&
            move_source.slot_count() == 0U,
        "Move construction empties the source table");

    check(
        state,
        !move_source.contains(
            move_live_handle) &&
            !move_source.contains(
                move_live_asset),
        "Moved-from table no longer reports transferred resources");

    const AssetId moved_from_asset{
        2ULL,
        3ULL
    };

    const Result<ResourceHandle>
        moved_from_insert_result =
            move_source.insert(
                moved_from_asset,
                MoveOnlyResource{60});

    check(
        state,
        moved_from_insert_result.has_value(),
        "Moved-from table remains reusable");

    check(
        state,
        moved_from_insert_result.has_value() &&
            moved_from_insert_result.value() ==
                ResourceHandle{
                    1U,
                    1U
                },
        "Reused moved-from table starts with a fresh slot");

    const AssetId moved_replacement_asset{
        2ULL,
        4ULL
    };

    const Result<ResourceHandle>
        moved_replacement_result =
            move_destination.insert(
                moved_replacement_asset,
                MoveOnlyResource{70});

    check(
        state,
        moved_replacement_result.has_value(),
        "Moved-to table preserves its reusable free slot");

    check(
        state,
        moved_replacement_result.has_value() &&
            moved_replacement_result.value().slot ==
                move_erased_handle.slot &&
            moved_replacement_result.value().
                generation ==
                move_erased_handle.generation +
                    1U,
        "Moved-to table reuses the transferred slot generation");

    check(
        state,
        !move_destination.contains(
            move_erased_handle),
        "Moved-to table continues rejecting the stale handle");

    TestTable assignment_source{};

    const AssetId assignment_erased_asset{
        3ULL,
        1ULL
    };

    const AssetId assignment_live_asset{
        3ULL,
        2ULL
    };

    const Result<ResourceHandle>
        assignment_erased_insert_result =
            assignment_source.insert(
                assignment_erased_asset,
                MoveOnlyResource{80});

    const Result<ResourceHandle>
        assignment_live_insert_result =
            assignment_source.insert(
                assignment_live_asset,
                MoveOnlyResource{90});

    ResourceHandle assignment_erased_handle{};
    ResourceHandle assignment_live_handle{};

    if (assignment_erased_insert_result.has_value())
    {
        assignment_erased_handle =
            assignment_erased_insert_result.value();
    }

    if (assignment_live_insert_result.has_value())
    {
        assignment_live_handle =
            assignment_live_insert_result.value();
    }

    const Status assignment_erase_result =
        assignment_source.erase(
            assignment_erased_handle);

    check(
        state,
        assignment_erased_insert_result.has_value() &&
            assignment_live_insert_result.has_value() &&
            assignment_erase_result.has_value(),
        "Move-assignment source prepares live and free slots");

    TestTable assignment_destination{};

    const AssetId discarded_asset{
        4ULL,
        1ULL
    };

    const Result<ResourceHandle>
        discarded_insert_result =
            assignment_destination.insert(
                discarded_asset,
                MoveOnlyResource{100});

    check(
        state,
        discarded_insert_result.has_value(),
        "Move-assignment destination begins with a resource");

    assignment_destination =
        std::move(assignment_source);

    check(
        state,
        assignment_destination.size() == 1U &&
            assignment_destination.slot_count() ==
                2U,
        "Move assignment replaces destination ownership");

    check(
        state,
        !assignment_destination.contains(
            discarded_asset),
        "Move assignment discards the destination's old asset");

    check(
        state,
        assignment_destination.contains(
            assignment_live_asset) &&
            assignment_destination.contains(
                assignment_live_handle),
        "Move assignment transfers the source resource");

    check(
        state,
        assignment_destination.find(
            assignment_live_handle) != nullptr &&
            assignment_destination.find(
                assignment_live_handle)->value ==
                90,
        "Move assignment preserves the transferred value");

    check(
        state,
        assignment_source.empty() &&
            assignment_source.slot_count() == 0U,
        "Move assignment empties the source table");

    const AssetId assignment_replacement_asset{
        3ULL,
        3ULL
    };

    const Result<ResourceHandle>
        assignment_replacement_result =
            assignment_destination.insert(
                assignment_replacement_asset,
                MoveOnlyResource{110});

    check(
        state,
        assignment_replacement_result.has_value(),
        "Move-assigned table preserves the free list");

    check(
        state,
        assignment_replacement_result.has_value() &&
            assignment_replacement_result.value().
                slot ==
                assignment_erased_handle.slot &&
            assignment_replacement_result.value().
                generation ==
                assignment_erased_handle.generation +
                    1U,
        "Move-assigned table reuses the correct generation");

    assignment_destination =
        std::move(
            assignment_destination);

    check(
        state,
        assignment_destination.size() == 2U &&
            assignment_destination.contains(
                assignment_live_asset) &&
            assignment_destination.contains(
                assignment_replacement_asset),
        "Self move assignment preserves table ownership");

    const AssetId reused_assignment_source_asset{
        3ULL,
        4ULL
    };

    const Result<ResourceHandle>
        reused_assignment_source_result =
            assignment_source.insert(
                reused_assignment_source_asset,
                MoveOnlyResource{120});

    check(
        state,
        reused_assignment_source_result.has_value() &&
            reused_assignment_source_result.value() ==
                ResourceHandle{
                    1U,
                    1U
                },
        "Move-assignment source remains reusable");

    constexpr std::size_t growth_count{
        64U
    };

    TestTable growth_table{};

    std::vector<AssetId> growth_assets{};
    std::vector<ResourceHandle> growth_handles{};

    growth_assets.reserve(growth_count);
    growth_handles.reserve(growth_count);

    bool all_growth_insertions_succeeded{true};

    for (std::size_t index = 0U;
         index < growth_count;
         ++index)
    {
        const AssetId asset{
            10ULL,
            static_cast<std::uint64_t>(
                index + 1U)
        };

        const Result<ResourceHandle> result =
            growth_table.insert(
                asset,
                MoveOnlyResource{
                    static_cast<int>(
                        index * 3U)
                });

        if (!result.has_value())
        {
            all_growth_insertions_succeeded =
                false;

            break;
        }

        growth_assets.push_back(asset);
        growth_handles.push_back(
            result.value());
    }

    check(
        state,
        all_growth_insertions_succeeded,
        "Table grows through repeated insertions");

    check(
        state,
        growth_table.size() ==
            growth_count &&
            growth_table.slot_count() ==
                growth_count,
        "Growth creates one slot for each live resource");

    bool all_growth_handles_exact{true};
    bool all_growth_values_exact{true};

    for (std::size_t index = 0U;
         index < growth_handles.size();
         ++index)
    {
        const ResourceHandle expected_handle{
            static_cast<std::uint32_t>(
                index + 1U),
            1U
        };

        if (growth_handles[index] !=
            expected_handle)
        {
            all_growth_handles_exact = false;
        }

        const MoveOnlyResource* const resource =
            growth_table.find(
                growth_handles[index]);

        if (resource == nullptr ||
            resource->value !=
                static_cast<int>(
                    index * 3U))
        {
            all_growth_values_exact = false;
        }
    }

    check(
        state,
        all_growth_handles_exact,
        "Growth assigns sequential generation-one handles");

    check(
        state,
        all_growth_values_exact,
        "Growth preserves every resource value");

    bool all_even_erases_succeeded{true};

    for (std::size_t index = 0U;
         index < growth_handles.size();
         index += 2U)
    {
        const Status erase_result =
            growth_table.erase(
                growth_handles[index]);

        if (!erase_result.has_value())
        {
            all_even_erases_succeeded = false;
            break;
        }
    }

    check(
        state,
        all_even_erases_succeeded,
        "Table erases alternating resources");

    check(
        state,
        growth_table.size() ==
            growth_count / 2U &&
            growth_table.slot_count() ==
                growth_count,
        "Alternating erasure preserves allocated slots");

    bool stale_even_handles_rejected{true};
    bool odd_handles_preserved{true};

    for (std::size_t index = 0U;
         index < growth_handles.size();
         ++index)
    {
        if ((index % 2U) == 0U)
        {
            if (growth_table.contains(
                    growth_handles[index]) ||
                growth_table.find(
                    growth_handles[index]) !=
                    nullptr)
            {
                stale_even_handles_rejected =
                    false;
            }
        }
        else
        {
            const MoveOnlyResource* const resource =
                growth_table.find(
                    growth_handles[index]);

            if (resource == nullptr ||
                resource->value !=
                    static_cast<int>(
                        index * 3U))
            {
                odd_handles_preserved = false;
            }
        }
    }

    check(
        state,
        stale_even_handles_rejected,
        "Alternating erase invalidates every stale handle");

    check(
        state,
        odd_handles_preserved,
        "Alternating erase preserves every live handle");

    bool all_replacements_succeeded{true};
    bool replacements_reused_lifo{true};
    bool replacement_generations_exact{true};

    for (std::size_t index = 0U;
         index < growth_count / 2U;
         ++index)
    {
        const AssetId replacement{
            11ULL,
            static_cast<std::uint64_t>(
                index + 1U)
        };

        const Result<ResourceHandle> result =
            growth_table.insert(
                replacement,
                MoveOnlyResource{
                    static_cast<int>(
                        1'000U + index)
                });

        if (!result.has_value())
        {
            all_replacements_succeeded = false;
            break;
        }

        const std::uint32_t expected_slot =
            static_cast<std::uint32_t>(
                growth_count -
                1U -
                index * 2U);

        if (result.value().slot !=
            expected_slot)
        {
            replacements_reused_lifo = false;
        }

        if (result.value().generation !=
            2U)
        {
            replacement_generations_exact =
                false;
        }
    }

    check(
        state,
        all_replacements_succeeded,
        "Table fills every released slot");

    check(
        state,
        replacements_reused_lifo,
        "Released slots are reused in free-list order");

    check(
        state,
        replacement_generations_exact,
        "Every reused growth slot advances generation");

    check(
        state,
        growth_table.size() ==
            growth_count &&
            growth_table.slot_count() ==
                growth_count,
        "Refilling released slots avoids table growth");

    check(
        state,
        stale_even_handles_rejected &&
            odd_handles_preserved,
        "Growth and reuse preserve stable handle semantics");

    std::cout
        << "\nResource table test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}