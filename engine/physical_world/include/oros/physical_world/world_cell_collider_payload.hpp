#pragma once

#include "oros/foundation/result.hpp"
#include "oros/physical_world/world_cell_collider_set.hpp"
#include "oros/streaming/world_cell_key.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oros::physical_world
{
    inline constexpr std::array<
        std::byte,
        8U>
        world_cell_collider_payload_magic{
            std::byte{0x4F},
            std::byte{0x52},
            std::byte{0x4F},
            std::byte{0x53},
            std::byte{0x43},
            std::byte{0x4F},
            std::byte{0x4C},
            std::byte{0x31}
        };

    inline constexpr std::uint32_t
        world_cell_collider_payload_format_version{
            1U
        };

    inline constexpr std::uint32_t
        world_cell_collider_payload_schema_version{
            2U
        };

    inline constexpr std::uint64_t
        world_cell_collider_payload_header_byte_count{
            64ULL
        };

    inline constexpr std::uint64_t
        world_cell_sphere_collider_record_byte_count{
            48ULL
        };

    inline constexpr std::uint64_t
        world_cell_box_collider_record_byte_count{
            64ULL
        };

    inline constexpr std::uint64_t
        world_cell_capsule_collider_record_byte_count{
            80ULL
        };

    [[nodiscard]]
    foundation::Result<
        std::vector<std::byte>>
    serialize_world_cell_collider_payload(
        const WorldCellColliderSet&
            collider_set) noexcept;

    [[nodiscard]]
    foundation::Result<
        WorldCellColliderSet>
    deserialize_world_cell_collider_payload(
        streaming::WorldCellKey
            expected_cell_key,
        std::span<const std::byte>
            bytes) noexcept;
}