#include "oros/streaming/world_cell_snapshot_manifest.hpp"

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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell snapshot manifest test "
            << "summary: "
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
        world_cell_snapshot_manifest_schema_version ==
            1U,
        "Snapshot manifest schema begins at version one");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000007ULL
        };

    const WorldCellKey cell_a{
        world_namespace,
        WorldCell{
            -10,
            0,
            5
        }
    };

    const WorldCellKey cell_b{
        world_namespace,
        WorldCell{
            0,
            0,
            0
        }
    };

    const WorldCellKey cell_c{
        world_namespace,
        WorldCell{
            12,
            -7,
            99
        }
    };

    const WorldCellRevisionId revision_a{
        cell_a,
        3ULL
    };

    const WorldCellRevisionId revision_b{
        cell_b,
        8ULL
    };

    const WorldCellRevisionId revision_c{
        cell_c,
        2ULL
    };

    const std::array<
        WorldCellRevisionId,
        3U>
        unsorted_entries{
            revision_c,
            revision_a,
            revision_b
        };

    const auto zero_namespace_result =
        WorldCellSnapshotManifest::create(
            0ULL,
            1ULL,
            std::span<
                const WorldCellRevisionId>{
                unsorted_entries
            });

    check(
        state,
        !zero_namespace_result.has_value(),
        "Zero world namespace is rejected");

    const auto zero_revision_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            0ULL,
            std::span<
                const WorldCellRevisionId>{
                unsorted_entries
            });

    check(
        state,
        !zero_revision_result.has_value(),
        "Zero manifest revision is rejected");

    const std::array<
        WorldCellRevisionId,
        1U>
        invalid_entries{
            invalid_world_cell_revision_id
        };

    const auto invalid_entry_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            1ULL,
            std::span<
                const WorldCellRevisionId>{
                invalid_entries
            });

    check(
        state,
        !invalid_entry_result.has_value(),
        "Invalid revision identity is rejected");

    const WorldCellKey other_world_cell{
        0x4F524F5300000008ULL,
        WorldCell{
            0,
            0,
            0
        }
    };

    const std::array<
        WorldCellRevisionId,
        1U>
        cross_namespace_entries{
            WorldCellRevisionId{
                other_world_cell,
                1ULL
            }
        };

    const auto cross_namespace_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            1ULL,
            std::span<
                const WorldCellRevisionId>{
                cross_namespace_entries
            });

    check(
        state,
        !cross_namespace_result.has_value(),
        "Cross-namespace entry is rejected");

    const std::array<
        WorldCellRevisionId,
        2U>
        duplicate_entries{
            WorldCellRevisionId{
                cell_a,
                3ULL
            },
            WorldCellRevisionId{
                cell_a,
                4ULL
            }
        };

    const auto duplicate_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            1ULL,
            std::span<
                const WorldCellRevisionId>{
                duplicate_entries
            });

    check(
        state,
        !duplicate_result.has_value(),
        "Multiple revisions for one cell are rejected");

    auto mutable_entries =
        unsorted_entries;

    const auto manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            std::span<
                const WorldCellRevisionId>{
                mutable_entries
            });

    check(
        state,
        manifest_result.has_value(),
        "Valid snapshot manifest is created");

    if (!manifest_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest& manifest =
        manifest_result.value();

    check(
        state,
        manifest.is_valid(),
        "Created snapshot manifest reports valid");

    check(
        state,
        manifest.world_namespace() ==
            world_namespace,
        "Manifest preserves its world namespace");

    check(
        state,
        manifest.schema_version() ==
            world_cell_snapshot_manifest_schema_version,
        "Manifest reports the current schema version");

    check(
        state,
        manifest.schema_version() == 1U,
        "Manifest schema version is explicitly one");

    check(
        state,
        manifest.manifest_revision() == 11ULL,
        "Manifest preserves its publication revision");

    check(
        state,
        manifest.size() == 3U,
        "Manifest reports its entry count");

    check(
        state,
        !manifest.empty(),
        "Populated manifest reports nonempty");

    check(
        state,
        manifest.entries().size() == 3U,
        "Manifest exposes all revision identities");

    check(
        state,
        manifest.entries()[0U] == revision_a,
        "Manifest sorts the first entry canonically");

    check(
        state,
        manifest.entries()[1U] == revision_b,
        "Manifest sorts the second entry canonically");

    check(
        state,
        manifest.entries()[2U] == revision_c,
        "Manifest sorts the third entry canonically");

    check(
        state,
        manifest.entries()[0U].cell_key <
            manifest.entries()[1U].cell_key &&
            manifest.entries()[1U].cell_key <
                manifest.entries()[2U].cell_key,
        "Manifest entries are strictly ordered by cell key");

    mutable_entries[0U] =
        WorldCellRevisionId{
            cell_a,
            999ULL
        };

    mutable_entries[1U] =
        invalid_world_cell_revision_id;

    check(
        state,
        manifest.entries()[0U] == revision_a &&
            manifest.entries()[1U] == revision_b &&
            manifest.entries()[2U] == revision_c,
        "Manifest owns entries independently of the caller");

    check(
        state,
        manifest.entries().data() !=
            mutable_entries.data(),
        "Manifest entries do not alias caller storage");

    const WorldCellRevisionId*
        found_a =
            manifest.find(
                cell_a);

    check(
        state,
        found_a != nullptr,
        "Manifest finds its first cell");

    check(
        state,
        found_a != nullptr &&
            *found_a == revision_a,
        "First cell lookup returns its exact revision");

    const WorldCellRevisionId*
        found_b =
            manifest.find(
                cell_b);

    check(
        state,
        found_b != nullptr,
        "Manifest finds its middle cell");

    check(
        state,
        found_b != nullptr &&
            *found_b == revision_b,
        "Middle cell lookup returns its exact revision");

    const WorldCellRevisionId*
        found_c =
            manifest.find(
                cell_c);

    check(
        state,
        found_c != nullptr,
        "Manifest finds its final cell");

    check(
        state,
        found_c != nullptr &&
            *found_c == revision_c,
        "Final cell lookup returns its exact revision");

    check(
        state,
        manifest.contains(
            cell_a),
        "Manifest reports its first cell present");

    check(
        state,
        manifest.contains(
            cell_b),
        "Manifest reports its middle cell present");

    check(
        state,
        manifest.contains(
            cell_c),
        "Manifest reports its final cell present");

    const WorldCellKey absent_cell{
        world_namespace,
        WorldCell{
            100,
            200,
            300
        }
    };

    check(
        state,
        manifest.find(
            absent_cell) == nullptr,
        "Manifest lookup rejects an absent cell");

    check(
        state,
        !manifest.contains(
            absent_cell),
        "Manifest reports an absent cell missing");

    check(
        state,
        manifest.find(
            invalid_world_cell_key) ==
            nullptr,
        "Manifest lookup rejects an invalid cell key");

    check(
        state,
        !manifest.contains(
            invalid_world_cell_key),
        "Manifest does not contain an invalid cell key");

    check(
        state,
        manifest.find(
            other_world_cell) ==
            nullptr,
        "Manifest lookup rejects another world namespace");

    check(
        state,
        !manifest.contains(
            other_world_cell),
        "Manifest does not contain another world namespace");

    const WorldCellSnapshotManifest
        copied_manifest{
            manifest
        };

    check(
        state,
        copied_manifest.is_valid(),
        "Copy-constructed manifest remains valid");

    check(
        state,
        copied_manifest == manifest,
        "Copy-constructed manifest compares equal");

    check(
        state,
        copied_manifest.entries().data() !=
            manifest.entries().data(),
        "Copy construction produces independent entry storage");

    const std::array<
        WorldCellRevisionId,
        1U>
        replacement_entries{
            revision_b
        };

    auto replacement_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            12ULL,
            std::span<
                const WorldCellRevisionId>{
                replacement_entries
            });

    check(
        state,
        replacement_result.has_value(),
        "Replacement manifest is created for assignment");

    if (!replacement_result.has_value())
    {
        return finish(state);
    }

    WorldCellSnapshotManifest assigned_manifest =
        std::move(
            replacement_result.value());

    assigned_manifest =
        manifest;

    check(
        state,
        assigned_manifest.is_valid(),
        "Copy-assigned manifest remains valid");

    check(
        state,
        assigned_manifest == manifest,
        "Copy assignment restores complete manifest state");

    check(
        state,
        assigned_manifest.entries().data() !=
            manifest.entries().data(),
        "Copy assignment produces independent entry storage");

    WorldCellSnapshotManifest moved_manifest{
        std::move(
            assigned_manifest)
    };

    check(
        state,
        moved_manifest.is_valid(),
        "Move-constructed manifest remains valid");

    check(
        state,
        moved_manifest == manifest,
        "Move construction preserves complete manifest state");

    auto move_target_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            13ULL,
            std::span<
                const WorldCellRevisionId>{
                replacement_entries
            });

    check(
        state,
        move_target_result.has_value(),
        "Move-assignment target manifest is created");

    if (!move_target_result.has_value())
    {
        return finish(state);
    }

    WorldCellSnapshotManifest
        move_assigned_manifest =
            std::move(
                move_target_result.value());

    move_assigned_manifest =
        std::move(
            moved_manifest);

    check(
        state,
        move_assigned_manifest.is_valid(),
        "Move-assigned manifest remains valid");

    check(
        state,
        move_assigned_manifest == manifest,
        "Move assignment preserves complete manifest state");

    const auto empty_manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            14ULL,
            std::span<
                const WorldCellRevisionId>{});

    check(
        state,
        empty_manifest_result.has_value(),
        "Empty snapshot manifest is permitted");

    if (!empty_manifest_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest&
        empty_manifest =
            empty_manifest_result.value();

    check(
        state,
        empty_manifest.is_valid(),
        "Empty manifest remains structurally valid");

    check(
        state,
        empty_manifest.empty(),
        "Empty manifest reports empty");

    check(
        state,
        empty_manifest.size() == 0U,
        "Empty manifest reports zero entries");

    check(
        state,
        empty_manifest.entries().empty(),
        "Empty manifest exposes an empty entry view");

    check(
        state,
        empty_manifest.find(
            cell_a) == nullptr,
        "Empty manifest lookup returns no entry");

    const auto maximum_revision_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            (std::numeric_limits<
                std::uint64_t>::max)(),
            std::span<
                const WorldCellRevisionId>{
                unsorted_entries
            });

    check(
        state,
        maximum_revision_result.has_value(),
        "Maximum manifest revision is accepted");

    check(
        state,
        maximum_revision_result.has_value() &&
            maximum_revision_result.value().
                manifest_revision() ==
            (std::numeric_limits<
                std::uint64_t>::max)(),
        "Maximum manifest revision is preserved exactly");

    const std::array<
        WorldCellRevisionId,
        3U>
        sorted_entries{
            revision_a,
            revision_b,
            revision_c
        };

    const auto sorted_manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            std::span<
                const WorldCellRevisionId>{
                sorted_entries
            });

    check(
        state,
        sorted_manifest_result.has_value(),
        "Already-sorted entries produce a manifest");

    check(
        state,
        sorted_manifest_result.has_value() &&
            sorted_manifest_result.value() ==
                manifest,
        "Input order does not affect canonical manifest state");

    const auto different_revision_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            15ULL,
            std::span<
                const WorldCellRevisionId>{
                sorted_entries
            });

    check(
        state,
        different_revision_result.has_value(),
        "Different publication revision manifest is created");

    check(
        state,
        different_revision_result.has_value() &&
            different_revision_result.value() !=
                manifest,
        "Manifest revision participates in equality");

    const std::array<
        WorldCellRevisionId,
        3U>
        changed_cell_revision_entries{
            revision_a,
            WorldCellRevisionId{
                cell_b,
                9ULL
            },
            revision_c
        };

    const auto changed_entry_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            std::span<
                const WorldCellRevisionId>{
                changed_cell_revision_entries
            });

    check(
        state,
        changed_entry_result.has_value(),
        "Changed cell revision manifest is created");

    check(
        state,
        changed_entry_result.has_value() &&
            changed_entry_result.value() !=
                manifest,
        "Cell revision entries participate in equality");

    return finish(state);
}