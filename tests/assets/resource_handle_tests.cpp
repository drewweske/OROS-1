#include "oros/assets/resource_handle.hpp"

#include <cstdint>
#include <iostream>
#include <limits>
#include <string_view>
#include <unordered_set>

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
    using namespace oros::assets;

    TestState state{};

    check(
        state,
        sizeof(ResourceHandle) ==
            sizeof(std::uint64_t),
        "Resource handles occupy 64 bits");

    check(
        state,
        !invalid_resource_handle.is_valid(),
        "Default resource handle is invalid");

    check(
        state,
        !static_cast<bool>(
            invalid_resource_handle),
        "Invalid resource handle converts to false");

    const ResourceHandle missing_slot{
        0U,
        1U
    };

    check(
        state,
        !missing_slot.is_valid(),
        "Resource handle rejects a zero slot");

    check(
        state,
        !static_cast<bool>(
            missing_slot),
        "Zero-slot resource handle converts to false");

    const ResourceHandle missing_generation{
        1U,
        0U
    };

    check(
        state,
        !missing_generation.is_valid(),
        "Resource handle rejects a zero generation");

    check(
        state,
        !static_cast<bool>(
            missing_generation),
        "Zero-generation resource handle converts to false");

    const ResourceHandle valid_handle{
        1U,
        1U
    };

    check(
        state,
        valid_handle.is_valid(),
        "Resource handle accepts non-zero components");

    check(
        state,
        static_cast<bool>(
            valid_handle),
        "Valid resource handle converts to true");

    const ResourceHandle equal_handle{
        1U,
        1U
    };

    const ResourceHandle later_slot{
        2U,
        1U
    };

    const ResourceHandle later_generation{
        1U,
        2U
    };

    check(
        state,
        valid_handle ==
            equal_handle,
        "Equal resource handle components compare equal");

    check(
        state,
        valid_handle !=
            later_slot,
        "Different resource slots compare unequal");

    check(
        state,
        valid_handle !=
            later_generation,
        "Different resource generations compare unequal");

    check(
        state,
        valid_handle <
            later_slot,
        "Resource handles order by slot first");

    check(
        state,
        valid_handle <
            later_generation,
        "Resource handles order by generation within a slot");

    check(
        state,
        valid_handle.packed_value() ==
            0x0000000100000001ULL,
        "Simple resource handle packs exactly");

    check(
        state,
        later_slot.packed_value() ==
            0x0000000100000002ULL,
        "Resource slot occupies the lower 32 bits");

    check(
        state,
        later_generation.packed_value() ==
            0x0000000200000001ULL,
        "Resource generation occupies the upper 32 bits");

    const ResourceHandle patterned_handle{
        0x89ABCDEFU,
        0x01234567U
    };

    check(
        state,
        patterned_handle.packed_value() ==
            0x0123456789ABCDEFULL,
        "Patterned resource handle packs exactly");

    const ResourceHandle unpacked_pattern =
        ResourceHandle::from_packed_value(
            0x0123456789ABCDEFULL);

    check(
        state,
        unpacked_pattern ==
            patterned_handle,
        "Packed resource handle restores both components");

    check(
        state,
        unpacked_pattern.slot ==
            0x89ABCDEFU,
        "Unpacking restores the resource slot");

    check(
        state,
        unpacked_pattern.generation ==
            0x01234567U,
        "Unpacking restores the resource generation");

    const std::uint32_t maximum_component =
        (std::numeric_limits<
            std::uint32_t>::max)();

    const ResourceHandle maximum_handle{
        maximum_component,
        maximum_component
    };

    check(
        state,
        maximum_handle.is_valid(),
        "Maximum resource handle components are valid");

    check(
        state,
        maximum_handle.packed_value() ==
            0xFFFFFFFFFFFFFFFFULL,
        "Maximum resource handle packs to all one bits");

    const ResourceHandle unpacked_maximum =
        ResourceHandle::from_packed_value(
            0xFFFFFFFFFFFFFFFFULL);

    check(
        state,
        unpacked_maximum ==
            maximum_handle,
        "Maximum packed resource handle round-trips");

    const ResourceHandle unpacked_zero =
        ResourceHandle::from_packed_value(
            0ULL);

    check(
        state,
        unpacked_zero ==
            invalid_resource_handle,
        "Zero packed value restores the invalid handle");

    check(
        state,
        !unpacked_zero.is_valid(),
        "Zero packed value remains invalid");

    const ResourceHandle packed_missing_slot =
        ResourceHandle::from_packed_value(
            0x0000000100000000ULL);

    check(
        state,
        packed_missing_slot ==
            missing_slot,
        "Unpacking preserves a zero slot");

    check(
        state,
        !packed_missing_slot.is_valid(),
        "Unpacked zero-slot handle remains invalid");

    const ResourceHandle packed_missing_generation =
        ResourceHandle::from_packed_value(
            0x0000000000000001ULL);

    check(
        state,
        packed_missing_generation ==
            missing_generation,
        "Unpacking preserves a zero generation");

    check(
        state,
        !packed_missing_generation.is_valid(),
        "Unpacked zero-generation handle remains invalid");

    const ResourceHandle reused_slot{
        42U,
        7U
    };

    const ResourceHandle replacement_handle{
        42U,
        8U
    };

    check(
        state,
        reused_slot.slot ==
            replacement_handle.slot,
        "Reused resource handles can share a slot");

    check(
        state,
        reused_slot.generation !=
            replacement_handle.generation,
        "Reused slots receive a different generation");

    check(
        state,
        reused_slot !=
            replacement_handle,
        "Generation changes invalidate stale handles");

    check(
        state,
        reused_slot.packed_value() !=
            replacement_handle.packed_value(),
        "Stale and replacement handles pack differently");

    const ResourceHandleHash hash_function{};

    check(
        state,
        hash_function(valid_handle) ==
            hash_function(equal_handle),
        "Equal resource handles produce equal hashes");

    std::unordered_set<
        ResourceHandle,
        ResourceHandleHash
    > handle_set{};

    handle_set.insert(valid_handle);
    handle_set.insert(equal_handle);
    handle_set.insert(later_slot);
    handle_set.insert(later_generation);
    handle_set.insert(reused_slot);
    handle_set.insert(replacement_handle);

    check(
        state,
        handle_set.size() == 5U,
        "Resource-handle storage removes duplicates");

    check(
        state,
        handle_set.contains(
            valid_handle),
        "Resource-handle storage finds the first handle");

    check(
        state,
        handle_set.contains(
            later_slot),
        "Resource-handle storage finds another slot");

    check(
        state,
        handle_set.contains(
            later_generation),
        "Resource-handle storage finds another generation");

    check(
        state,
        handle_set.contains(
            reused_slot),
        "Resource-handle storage finds a stale-generation value");

    check(
        state,
        handle_set.contains(
            replacement_handle),
        "Resource-handle storage distinguishes a replacement generation");

    check(
        state,
        !handle_set.contains(
            ResourceHandle{
                42U,
                9U
            }),
        "Resource-handle storage rejects an absent generation");

    check(
        state,
        ResourceHandle::from_packed_value(
            valid_handle.packed_value()) ==
            valid_handle,
        "Simple handle survives pack and unpack");

    check(
        state,
        ResourceHandle::from_packed_value(
            reused_slot.packed_value()) ==
            reused_slot,
        "Stale-generation handle survives pack and unpack");

    check(
        state,
        ResourceHandle::from_packed_value(
            replacement_handle.packed_value()) ==
            replacement_handle,
        "Replacement handle survives pack and unpack");

    std::cout
        << "\nResource handle test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}