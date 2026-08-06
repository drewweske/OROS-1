#include "oros/streaming/world_cell_snapshot.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <limits>
#include <span>
#include <string_view>
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

    [[nodiscard]]
    bool bytes_equal(
        const std::span<const std::byte> left,
        const std::span<const std::byte> right)
        noexcept
    {
        if (left.size() != right.size())
        {
            return false;
        }

        for (std::size_t index = 0U;
             index < left.size();
             ++index)
        {
            if (left[index] != right[index])
            {
                return false;
            }
        }

        return true;
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell snapshot test summary: "
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
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        world_cell_snapshot_schema_version == 1U,
        "World cell snapshot schema begins at version one");

    const WorldCellKey valid_key{
        0x4F524F5300000007ULL,
        WorldCell{
            -42,
            17,
            9001
        }
    };

    const std::array<std::byte, 8U>
        original_payload{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60},
            std::byte{0x70},
            std::byte{0x80}
        };

    const auto invalid_key_snapshot =
        WorldCellSnapshot::create(
            invalid_world_cell_key,
            1ULL,
            std::span<const std::byte>{
                original_payload
            });

    check(
        state,
        !invalid_key_snapshot.has_value(),
        "Invalid world cell key is rejected");

    const auto zero_revision_snapshot =
        WorldCellSnapshot::create(
            valid_key,
            0ULL,
            std::span<const std::byte>{
                original_payload
            });

    check(
        state,
        !zero_revision_snapshot.has_value(),
        "Zero snapshot revision is rejected");

    auto mutable_payload =
        original_payload;

    const auto created_snapshot =
        WorldCellSnapshot::create(
            valid_key,
            7ULL,
            std::span<const std::byte>{
                mutable_payload
            });

    check(
        state,
        created_snapshot.has_value(),
        "Valid world cell snapshot is created");

    if (!created_snapshot.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot& snapshot =
        created_snapshot.value();

    check(
        state,
        snapshot.is_valid(),
        "Created snapshot reports valid");

    check(
        state,
        snapshot.key() == valid_key,
        "Snapshot preserves its persistent cell key");

    check(
        state,
        snapshot.schema_version() ==
            world_cell_snapshot_schema_version,
        "Snapshot reports the current schema version");

    check(
        state,
        snapshot.schema_version() == 1U,
        "Snapshot schema version is explicitly one");

    check(
        state,
        snapshot.revision() == 7ULL,
        "Snapshot preserves its revision");

    check(
        state,
        snapshot.byte_count() ==
            original_payload.size(),
        "Snapshot reports its payload byte count");

    check(
        state,
        !snapshot.empty(),
        "Nonempty snapshot reports nonempty");

    check(
        state,
        bytes_equal(
            snapshot.payload(),
            std::span<const std::byte>{
                original_payload
            }),
        "Snapshot preserves every payload byte");

    mutable_payload[0U] =
        std::byte{0xFF};

    mutable_payload[7U] =
        std::byte{0xEE};

    check(
        state,
        bytes_equal(
            snapshot.payload(),
            std::span<const std::byte>{
                original_payload
            }),
        "Snapshot owns bytes independently of the caller");

    check(
        state,
        snapshot.payload().data() !=
            mutable_payload.data(),
        "Snapshot payload does not alias caller storage");

    const WorldCellSnapshot copied_snapshot{
        snapshot
    };

    check(
        state,
        copied_snapshot.is_valid(),
        "Copy-constructed snapshot remains valid");

    check(
        state,
        copied_snapshot == snapshot,
        "Copy-constructed snapshot compares equal");

    check(
        state,
        bytes_equal(
            copied_snapshot.payload(),
            snapshot.payload()),
        "Copy construction preserves every payload byte");

    check(
        state,
        copied_snapshot.payload().data() !=
            snapshot.payload().data(),
        "Copy construction produces independent payload storage");

    const WorldCellKey replacement_key{
        99ULL,
        WorldCell{
            1,
            2,
            3
        }
    };

    const std::array<std::byte, 3U>
        replacement_payload{
            std::byte{0xAA},
            std::byte{0xBB},
            std::byte{0xCC}
        };

    auto replacement_result =
        WorldCellSnapshot::create(
            replacement_key,
            3ULL,
            std::span<const std::byte>{
                replacement_payload
            });

    check(
        state,
        replacement_result.has_value(),
        "Replacement snapshot is created for assignment");

    if (!replacement_result.has_value())
    {
        return finish(state);
    }

    WorldCellSnapshot assigned_snapshot =
        std::move(
            replacement_result.value());

    assigned_snapshot =
        snapshot;

    check(
        state,
        assigned_snapshot.is_valid(),
        "Copy-assigned snapshot remains valid");

    check(
        state,
        assigned_snapshot == snapshot,
        "Copy assignment restores complete snapshot state");

    check(
        state,
        assigned_snapshot.payload().data() !=
            snapshot.payload().data(),
        "Copy assignment produces independent payload storage");

    WorldCellSnapshot moved_snapshot{
        std::move(
            assigned_snapshot)
    };

    check(
        state,
        moved_snapshot.is_valid(),
        "Move-constructed snapshot remains valid");

    check(
        state,
        moved_snapshot == snapshot,
        "Move construction preserves complete snapshot state");

    check(
        state,
        bytes_equal(
            moved_snapshot.payload(),
            std::span<const std::byte>{
                original_payload
            }),
        "Move construction preserves every payload byte");

    auto move_target_result =
        WorldCellSnapshot::create(
            replacement_key,
            4ULL,
            std::span<const std::byte>{
                replacement_payload
            });

    check(
        state,
        move_target_result.has_value(),
        "Move-assignment target snapshot is created");

    if (!move_target_result.has_value())
    {
        return finish(state);
    }

    WorldCellSnapshot move_assigned_snapshot =
        std::move(
            move_target_result.value());

    move_assigned_snapshot =
        std::move(
            moved_snapshot);

    check(
        state,
        move_assigned_snapshot.is_valid(),
        "Move-assigned snapshot remains valid");

    check(
        state,
        move_assigned_snapshot == snapshot,
        "Move assignment preserves complete snapshot state");

    check(
        state,
        bytes_equal(
            move_assigned_snapshot.payload(),
            std::span<const std::byte>{
                original_payload
            }),
        "Move assignment preserves every payload byte");

    const auto empty_snapshot_result =
        WorldCellSnapshot::create(
            valid_key,
            8ULL,
            std::span<const std::byte>{});

    check(
        state,
        empty_snapshot_result.has_value(),
        "Empty world cell snapshot is permitted");

    if (!empty_snapshot_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot& empty_snapshot =
        empty_snapshot_result.value();

    check(
        state,
        empty_snapshot.is_valid(),
        "Empty snapshot remains structurally valid");

    check(
        state,
        empty_snapshot.empty(),
        "Empty snapshot reports empty");

    check(
        state,
        empty_snapshot.byte_count() == 0U,
        "Empty snapshot reports zero bytes");

    check(
        state,
        empty_snapshot.payload().empty(),
        "Empty snapshot exposes an empty payload view");

    check(
        state,
        empty_snapshot.revision() == 8ULL,
        "Empty snapshot preserves its revision");

    const auto maximum_revision_result =
        WorldCellSnapshot::create(
            valid_key,
            (std::numeric_limits<
                std::uint64_t>::max)(),
            std::span<const std::byte>{
                original_payload
            });

    check(
        state,
        maximum_revision_result.has_value(),
        "Maximum nonzero revision is accepted");

    check(
        state,
        maximum_revision_result.has_value() &&
            maximum_revision_result.value().
                revision() ==
            (std::numeric_limits<
                std::uint64_t>::max)(),
        "Maximum revision is preserved exactly");

    const auto different_revision_result =
        WorldCellSnapshot::create(
            valid_key,
            9ULL,
            std::span<const std::byte>{
                original_payload
            });

    check(
        state,
        different_revision_result.has_value(),
        "Different-revision snapshot is created");

    check(
        state,
        different_revision_result.has_value() &&
            different_revision_result.value() !=
                snapshot,
        "Revision participates in snapshot equality");

    const auto different_key_result =
        WorldCellSnapshot::create(
            replacement_key,
            7ULL,
            std::span<const std::byte>{
                original_payload
            });

    check(
        state,
        different_key_result.has_value(),
        "Different-key snapshot is created");

    check(
        state,
        different_key_result.has_value() &&
            different_key_result.value() !=
                snapshot,
        "Persistent cell key participates in equality");

    const std::array<std::byte, 8U>
        different_payload{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60},
            std::byte{0x70},
            std::byte{0x81}
        };

    const auto different_payload_result =
        WorldCellSnapshot::create(
            valid_key,
            7ULL,
            std::span<const std::byte>{
                different_payload
            });

    check(
        state,
        different_payload_result.has_value(),
        "Different-payload snapshot is created");

    check(
        state,
        different_payload_result.has_value() &&
            different_payload_result.value() !=
                snapshot,
        "Payload bytes participate in snapshot equality");

    check(
        state,
        snapshot.key().world_namespace ==
            0x4F524F5300000007ULL,
        "Snapshot exposes its original world namespace");

    check(
        state,
        snapshot.key().cell ==
            WorldCell{
                -42,
                17,
                9001
            },
        "Snapshot exposes its original signed cell coordinates");

    return finish(state);
}