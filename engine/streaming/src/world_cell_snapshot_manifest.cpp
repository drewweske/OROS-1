#include "oros/streaming/world_cell_snapshot_manifest.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::streaming
{
    foundation::Result<
        WorldCellSnapshotManifest>
    WorldCellSnapshotManifest::create(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision,
        const std::span<
            const WorldCellRevisionId>
            entries)
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest "
                "namespace must be non-zero.");
        }

        if (manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot manifest "
                "revision must be non-zero.");
        }

        for (const WorldCellRevisionId&
                 entry : entries)
        {
            if (!entry.is_valid())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell snapshot manifest "
                    "contains an invalid revision "
                    "identity.");
            }

            if (entry.cell_key.world_namespace !=
                world_namespace)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell snapshot manifest "
                    "entry belongs to a different "
                    "world namespace.");
            }
        }

        try
        {
            std::vector<
                WorldCellRevisionId>
                owned_entries{
                    entries.begin(),
                    entries.end()
                };

            std::sort(
                owned_entries.begin(),
                owned_entries.end(),
                [](
                    const WorldCellRevisionId& left,
                    const WorldCellRevisionId& right)
                {
                    return
                        left.cell_key <
                        right.cell_key;
                });

            const auto duplicate_iterator =
                std::adjacent_find(
                    owned_entries.begin(),
                    owned_entries.end(),
                    [](
                        const WorldCellRevisionId& left,
                        const WorldCellRevisionId& right)
                    {
                        return
                            left.cell_key ==
                            right.cell_key;
                    });

            if (duplicate_iterator !=
                owned_entries.end())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell snapshot manifest "
                    "contains more than one revision "
                    "for the same world cell.");
            }

            return WorldCellSnapshotManifest{
                world_namespace,
                manifest_revision,
                std::move(
                    owned_entries)
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "snapshot manifest storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while creating a world cell "
                "snapshot manifest.");
        }
    }

    WorldCellSnapshotManifest::
    WorldCellSnapshotManifest(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision,
        std::vector<
            WorldCellRevisionId>
            entries) noexcept
        : world_namespace_{
              world_namespace
          },
          manifest_revision_{
              manifest_revision
          },
          entries_{
              std::move(
                  entries)
          }
    {
    }

    bool
    WorldCellSnapshotManifest::is_valid()
        const noexcept
    {
        if (world_namespace_ == 0ULL ||
            schema_version_ !=
                world_cell_snapshot_manifest_schema_version ||
            manifest_revision_ == 0ULL)
        {
            return false;
        }

        WorldCellKey previous_key{};
        bool has_previous_key{};

        for (const WorldCellRevisionId&
                 entry : entries_)
        {
            if (!entry.is_valid() ||
                entry.cell_key.world_namespace !=
                    world_namespace_)
            {
                return false;
            }

            if (has_previous_key &&
                !(previous_key <
                    entry.cell_key))
            {
                return false;
            }

            previous_key =
                entry.cell_key;

            has_previous_key = true;
        }

        return true;
    }

    std::uint64_t
    WorldCellSnapshotManifest::
    world_namespace() const noexcept
    {
        return world_namespace_;
    }

    std::uint32_t
    WorldCellSnapshotManifest::
    schema_version() const noexcept
    {
        return schema_version_;
    }

    std::uint64_t
    WorldCellSnapshotManifest::
    manifest_revision() const noexcept
    {
        return manifest_revision_;
    }

    std::span<
        const WorldCellRevisionId>
    WorldCellSnapshotManifest::entries()
        const noexcept
    {
        return std::span<
            const WorldCellRevisionId>{
                entries_
            };
    }

    const WorldCellRevisionId*
    WorldCellSnapshotManifest::find(
        const WorldCellKey cell_key)
        const noexcept
    {
        if (!cell_key.is_valid() ||
            cell_key.world_namespace !=
                world_namespace_)
        {
            return nullptr;
        }

        const auto iterator =
            std::lower_bound(
                entries_.begin(),
                entries_.end(),
                cell_key,
                [](
                    const WorldCellRevisionId& entry,
                    const WorldCellKey key)
                {
                    return
                        entry.cell_key <
                        key;
                });

        if (iterator == entries_.end() ||
            iterator->cell_key != cell_key)
        {
            return nullptr;
        }

        return &(*iterator);
    }

    bool
    WorldCellSnapshotManifest::contains(
        const WorldCellKey cell_key)
        const noexcept
    {
        return find(cell_key) != nullptr;
    }

    std::size_t
    WorldCellSnapshotManifest::size()
        const noexcept
    {
        return entries_.size();
    }

    bool
    WorldCellSnapshotManifest::empty()
        const noexcept
    {
        return entries_.empty();
    }
}