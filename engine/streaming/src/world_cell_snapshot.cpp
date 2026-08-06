#include "oros/streaming/world_cell_snapshot.hpp"

#include <cstddef>
#include <cstdint>
#include <new>
#include <span>
#include <utility>
#include <vector>

namespace oros::streaming
{
    foundation::Result<WorldCellSnapshot>
    WorldCellSnapshot::create(
        const WorldCellKey key,
        const std::uint64_t revision,
        const std::span<const std::byte>
            payload)
    {
        if (!key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot key must be "
                "valid.");
        }

        if (revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell snapshot revision must "
                "be non-zero.");
        }

        try
        {
            std::vector<std::byte>
                owned_payload{
                    payload.begin(),
                    payload.end()
                };

            return WorldCellSnapshot{
                key,
                revision,
                std::move(
                    owned_payload)
            };
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "snapshot payload storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while creating a world cell "
                "snapshot.");
        }
    }

    WorldCellSnapshot::WorldCellSnapshot(
        const WorldCellKey key,
        const std::uint64_t revision,
        std::vector<std::byte>
            payload) noexcept
        : key_{
              key
          },
          revision_{
              revision
          },
          payload_{
              std::move(
                  payload)
          }
    {
    }

    bool
    WorldCellSnapshot::is_valid()
        const noexcept
    {
        return
            key_.is_valid() &&
            schema_version_ ==
                world_cell_snapshot_schema_version &&
            revision_ != 0ULL;
    }

    const WorldCellKey&
    WorldCellSnapshot::key()
        const noexcept
    {
        return key_;
    }

    std::uint32_t
    WorldCellSnapshot::schema_version()
        const noexcept
    {
        return schema_version_;
    }

    std::uint64_t
    WorldCellSnapshot::revision()
        const noexcept
    {
        return revision_;
    }

    std::span<const std::byte>
    WorldCellSnapshot::payload()
        const noexcept
    {
        return std::span<const std::byte>{
            payload_
        };
    }

    std::size_t
    WorldCellSnapshot::byte_count()
        const noexcept
    {
        return payload_.size();
    }

    bool
    WorldCellSnapshot::empty()
        const noexcept
    {
        return payload_.empty();
    }
}