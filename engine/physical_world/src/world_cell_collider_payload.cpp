#include "oros/physical_world/world_cell_collider_payload.hpp"

#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/world/entity_id.hpp"

#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <new>
#include <optional>
#include <span>
#include <utility>
#include <variant>
#include <vector>

namespace oros::physical_world
{
    namespace
    {
        inline constexpr std::size_t
            collider_payload_checksum_offset{
                56U
            };

        inline constexpr std::size_t
            checksum_byte_count{
                8U
            };

        [[nodiscard]]
        constexpr std::uint64_t mix_hash(
            std::uint64_t value) noexcept
        {
            value +=
                0x9E3779B97F4A7C15ULL;

            value =
                (value ^ (value >> 30U)) *
                0xBF58476D1CE4E5B9ULL;

            value =
                (value ^ (value >> 27U)) *
                0x94D049BB133111EBULL;

            return
                value ^
                (value >> 31U);
        }

        [[nodiscard]]
        std::uint64_t calculate_checksum(
            const std::span<const std::byte> bytes,
            const std::size_t excluded_offset,
            const std::size_t excluded_count)
            noexcept
        {
            std::uint64_t hash{
                14695981039346656037ULL
            };

            for (std::size_t index = 0U;
                 index < bytes.size();
                 ++index)
            {
                const bool excluded =
                    index >= excluded_offset &&
                    index - excluded_offset <
                        excluded_count;

                if (excluded)
                {
                    continue;
                }

                hash ^=
                    static_cast<std::uint64_t>(
                        std::to_integer<
                            std::uint8_t>(
                                bytes[index]));

                hash *=
                    1099511628211ULL;
            }

            hash ^=
                static_cast<std::uint64_t>(
                    bytes.size());

            return mix_hash(
                hash);
        }

        class ByteWriter final
        {
        public:
            explicit ByteWriter(
                const std::span<std::byte>
                    bytes) noexcept
                : bytes_{
                      bytes
                  }
            {
            }

            [[nodiscard]]
            bool write_bytes(
                const std::span<
                    const std::byte>
                    values) noexcept
            {
                if (!can_write(
                        values.size()))
                {
                    return false;
                }

                for (const std::byte value :
                     values)
                {
                    bytes_[offset_] =
                        value;

                    ++offset_;
                }

                return true;
            }

            [[nodiscard]]
            bool write_u32(
                const std::uint32_t value)
                noexcept
            {
                for (std::size_t index = 0U;
                     index < 4U;
                     ++index)
                {
                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    const std::uint8_t component =
                        static_cast<
                            std::uint8_t>(
                                (
                                    value >>
                                    shift
                                ) &
                                0xFFU);

                    if (!write_byte(
                            static_cast<
                                std::byte>(
                                    component)))
                    {
                        return false;
                    }
                }

                return true;
            }

            [[nodiscard]]
            bool write_u64(
                const std::uint64_t value)
                noexcept
            {
                for (std::size_t index = 0U;
                     index < 8U;
                     ++index)
                {
                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    const std::uint8_t component =
                        static_cast<
                            std::uint8_t>(
                                (
                                    value >>
                                    shift
                                ) &
                                0xFFULL);

                    if (!write_byte(
                            static_cast<
                                std::byte>(
                                    component)))
                    {
                        return false;
                    }
                }

                return true;
            }

            [[nodiscard]]
            bool write_i64(
                const std::int64_t value)
                noexcept
            {
                return write_u64(
                    std::bit_cast<
                        std::uint64_t>(
                            value));
            }

            [[nodiscard]]
            bool write_f64(
                const double value)
                noexcept
            {
                return write_u64(
                    std::bit_cast<
                        std::uint64_t>(
                            value));
            }

            [[nodiscard]]
            bool complete() const noexcept
            {
                return
                    offset_ ==
                    bytes_.size();
            }

        private:
            [[nodiscard]]
            bool write_byte(
                const std::byte value)
                noexcept
            {
                if (!can_write(
                        1U))
                {
                    return false;
                }

                bytes_[offset_] =
                    value;

                ++offset_;

                return true;
            }

            [[nodiscard]]
            bool can_write(
                const std::size_t count)
                const noexcept
            {
                return
                    offset_ <=
                        bytes_.size() &&
                    count <=
                        bytes_.size() -
                            offset_;
            }

            std::span<std::byte>
                bytes_{};

            std::size_t offset_{};
        };

        class ByteReader final
        {
        public:
            explicit ByteReader(
                const std::span<
                    const std::byte>
                    bytes) noexcept
                : bytes_{
                      bytes
                  }
            {
            }

            [[nodiscard]]
            bool read_magic(
                const std::span<
                    const std::byte>
                    expected) noexcept
            {
                if (!can_read(
                        expected.size()))
                {
                    return false;
                }

                for (const std::byte value :
                     expected)
                {
                    if (bytes_[offset_] !=
                        value)
                    {
                        return false;
                    }

                    ++offset_;
                }

                return true;
            }

            [[nodiscard]]
            bool read_u32(
                std::uint32_t& value)
                noexcept
            {
                value = 0U;

                for (std::size_t index = 0U;
                     index < 4U;
                     ++index)
                {
                    std::byte component{};

                    if (!read_byte(
                            component))
                    {
                        return false;
                    }

                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    value |=
                        static_cast<
                            std::uint32_t>(
                                std::to_integer<
                                    std::uint8_t>(
                                        component))
                        << shift;
                }

                return true;
            }

            [[nodiscard]]
            bool read_u64(
                std::uint64_t& value)
                noexcept
            {
                value = 0ULL;

                for (std::size_t index = 0U;
                     index < 8U;
                     ++index)
                {
                    std::byte component{};

                    if (!read_byte(
                            component))
                    {
                        return false;
                    }

                    const unsigned int shift =
                        static_cast<
                            unsigned int>(
                                index *
                                8U);

                    value |=
                        static_cast<
                            std::uint64_t>(
                                std::to_integer<
                                    std::uint8_t>(
                                        component))
                        << shift;
                }

                return true;
            }

            [[nodiscard]]
            bool read_i64(
                std::int64_t& value)
                noexcept
            {
                std::uint64_t bits{};

                if (!read_u64(
                        bits))
                {
                    return false;
                }

                value =
                    std::bit_cast<
                        std::int64_t>(
                            bits);

                return true;
            }

            [[nodiscard]]
            bool read_f64(
                double& value)
                noexcept
            {
                std::uint64_t bits{};

                if (!read_u64(
                        bits))
                {
                    return false;
                }

                value =
                    std::bit_cast<
                        double>(
                            bits);

                return true;
            }

            [[nodiscard]]
            bool complete() const noexcept
            {
                return
                    offset_ ==
                    bytes_.size();
            }

        private:
            [[nodiscard]]
            bool read_byte(
                std::byte& value)
                noexcept
            {
                if (!can_read(
                        1U))
                {
                    return false;
                }

                value =
                    bytes_[offset_];

                ++offset_;

                return true;
            }

            [[nodiscard]]
            bool can_read(
                const std::size_t count)
                const noexcept
            {
                return
                    offset_ <=
                        bytes_.size() &&
                    count <=
                        bytes_.size() -
                            offset_;
            }

            std::span<
                const std::byte>
                bytes_{};

            std::size_t offset_{};
        };

        [[nodiscard]]
        bool patch_u64(
            const std::span<std::byte> bytes,
            const std::size_t offset,
            const std::uint64_t value)
            noexcept
        {
            if (offset > bytes.size() ||
                checksum_byte_count >
                    bytes.size() -
                        offset)
            {
                return false;
            }

            for (std::size_t index = 0U;
                 index < checksum_byte_count;
                 ++index)
            {
                const unsigned int shift =
                    static_cast<
                        unsigned int>(
                            index *
                            8U);

                const std::uint8_t component =
                    static_cast<
                        std::uint8_t>(
                            (
                                value >>
                                shift
                            ) &
                            0xFFULL);

                bytes[offset + index] =
                    static_cast<std::byte>(
                        component);
            }

            return true;
        }

        [[nodiscard]]
        std::uint64_t collider_record_byte_count(
            const physics::CollisionShape&
                shape) noexcept
        {
            switch (
                physics::collision_shape_kind(
                    shape))
            {
            case physics::CollisionShapeKind::
                sphere:
                return
                    world_cell_sphere_collider_record_byte_count;

            case physics::CollisionShapeKind::
                box:
                return
                    world_cell_box_collider_record_byte_count;

            case physics::CollisionShapeKind::
                capsule:
                return
                    world_cell_capsule_collider_record_byte_count;
            }

            return 0ULL;
        }
    }

    foundation::Result<
        std::vector<std::byte>>
    serialize_world_cell_collider_payload(
        const WorldCellColliderSet&
            collider_set) noexcept
    {
        if (!collider_set.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot serialize an invalid world "
                "cell collider set.");
        }

        if (!std::in_range<std::uint64_t>(
                collider_set.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider count cannot "
                "be represented by the payload "
                "format.");
        }

        const std::uint64_t collider_count =
            static_cast<std::uint64_t>(
                collider_set.size());

        const std::uint64_t maximum =
            (std::numeric_limits<
                std::uint64_t>::max)();

        std::uint64_t byte_count =
            world_cell_collider_payload_header_byte_count;

        for (const physics::ColliderGeometry&
                 geometry :
             collider_set.colliders())
        {
            const std::uint64_t record_byte_count =
                collider_record_byte_count(
                    geometry.shape());

            if (record_byte_count == 0ULL ||
                byte_count >
                    maximum -
                        record_byte_count)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider payload "
                    "size would overflow.");
            }

            byte_count +=
                record_byte_count;
        }

        if (!std::in_range<std::size_t>(
                byte_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload size "
                "cannot be represented on this "
                "platform.");
        }

        try
        {
            std::vector<std::byte> bytes(
                static_cast<std::size_t>(
                    byte_count));

            ByteWriter writer{
                std::span<std::byte>{
                    bytes
                }
            };

            const streaming::WorldCellKey&
                cell_key =
                    collider_set.cell_key();

            bool wrote_payload =
                writer.write_bytes(
                    std::span<
                        const std::byte>{
                            world_cell_collider_payload_magic
                        }) &&
                writer.write_u32(
                    world_cell_collider_payload_format_version) &&
                writer.write_u32(
                    world_cell_collider_payload_schema_version) &&
                writer.write_u64(
                    cell_key.world_namespace) &&
                writer.write_i64(
                    cell_key.cell.x) &&
                writer.write_i64(
                    cell_key.cell.y) &&
                writer.write_i64(
                    cell_key.cell.z) &&
                writer.write_u64(
                    collider_count) &&
                writer.write_u64(
                    0ULL);

            for (const physics::ColliderGeometry&
                     geometry :
                 collider_set.colliders())
            {
                const physics::ColliderId collider =
                    geometry.collider();

                const physics::CollisionShape&
                    shape =
                        geometry.shape();

                const physics::PhysicsVector3 center =
                    geometry.center();

                const physics::CollisionShapeKind kind =
                    physics::collision_shape_kind(
                        shape);

                wrote_payload =
                    wrote_payload &&
                    writer.write_u64(
                        collider.owner.
                            entity_sequence) &&
                    writer.write_u32(
                        collider.shape_slot) &&
                    writer.write_u32(
                        static_cast<
                            std::uint32_t>(
                                kind)) &&
                    writer.write_f64(
                        center.x) &&
                    writer.write_f64(
                        center.y) &&
                    writer.write_f64(
                        center.z);

                switch (kind)
                {
                case physics::CollisionShapeKind::
                    sphere:
                {
                    const physics::SphereShape&
                        sphere =
                            std::get<
                                physics::SphereShape>(
                                    shape);

                    wrote_payload =
                        wrote_payload &&
                        writer.write_f64(
                            sphere.radius());

                    break;
                }

                case physics::CollisionShapeKind::
                    box:
                {
                    const physics::BoxShape&
                        box =
                            std::get<
                                physics::BoxShape>(
                                    shape);

                    const physics::PhysicsVector3&
                        half_extents =
                            box.half_extents();

                    wrote_payload =
                        wrote_payload &&
                        writer.write_f64(
                            half_extents.x) &&
                        writer.write_f64(
                            half_extents.y) &&
                        writer.write_f64(
                            half_extents.z);

                    break;
                }

                case physics::CollisionShapeKind::
                    capsule:
                {
                    const physics::CapsuleShape&
                        capsule =
                            std::get<
                                physics::CapsuleShape>(
                                    shape);

                    const physics::PhysicsVector3&
                        axis =
                            capsule.
                                axis().
                                vector();

                    wrote_payload =
                        wrote_payload &&
                        writer.write_f64(
                            capsule.radius()) &&
                        writer.write_f64(
                            capsule.
                                half_segment_length()) &&
                        writer.write_f64(
                            axis.x) &&
                        writer.write_f64(
                            axis.y) &&
                        writer.write_f64(
                            axis.z);

                    break;
                }
                }
            }

            if (!wrote_payload ||
                !writer.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to construct the complete "
                    "world cell collider payload.");
            }

            const std::uint64_t checksum =
                calculate_checksum(
                    std::span<
                        const std::byte>{
                            bytes
                        },
                    collider_payload_checksum_offset,
                    checksum_byte_count);

            if (!patch_u64(
                    std::span<std::byte>{
                        bytes
                    },
                    collider_payload_checksum_offset,
                    checksum))
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "Unable to write the world cell "
                    "collider payload checksum.");
            }

            return bytes;
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate world cell "
                "collider payload storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while serializing a world cell "
                "collider payload.");
        }
    }

    foundation::Result<
        WorldCellColliderSet>
    deserialize_world_cell_collider_payload(
        const streaming::WorldCellKey
            expected_cell_key,
        const std::span<
            const std::byte>
            bytes) noexcept
    {
        if (!expected_cell_key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Expected world cell key must be "
                "valid.");
        }

        if (!std::in_range<std::uint64_t>(
                bytes.size()))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload size "
                "cannot be represented by the "
                "payload format.");
        }

        const std::uint64_t byte_count =
            static_cast<std::uint64_t>(
                bytes.size());

        if (byte_count <
            world_cell_collider_payload_header_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload is "
                "truncated.");
        }

        ByteReader reader{
            bytes
        };

        std::uint32_t format_version{};
        std::uint32_t schema_version{};
        std::uint64_t world_namespace{};
        std::int64_t cell_x{};
        std::int64_t cell_y{};
        std::int64_t cell_z{};
        std::uint64_t collider_count{};
        std::uint64_t stored_checksum{};

        const bool read_header =
            reader.read_magic(
                std::span<
                    const std::byte>{
                        world_cell_collider_payload_magic
                    }) &&
            reader.read_u32(
                format_version) &&
            reader.read_u32(
                schema_version) &&
            reader.read_u64(
                world_namespace) &&
            reader.read_i64(
                cell_x) &&
            reader.read_i64(
                cell_y) &&
            reader.read_i64(
                cell_z) &&
            reader.read_u64(
                collider_count) &&
            reader.read_u64(
                stored_checksum);

        if (!read_header)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload header "
                "is invalid or truncated.");
        }

        if (format_version !=
            world_cell_collider_payload_format_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload format "
                "version is unsupported.");
        }

        if (schema_version !=
            world_cell_collider_payload_schema_version)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload schema "
                "version is unsupported.");
        }

        const streaming::WorldCellKey payload_cell_key{
            world_namespace,
            world::WorldCell{
                cell_x,
                cell_y,
                cell_z
            }
        };

        if (!payload_cell_key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload contains "
                "an invalid cell identity.");
        }

        if (payload_cell_key !=
            expected_cell_key)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload belongs "
                "to a different world cell.");
        }

        const std::uint64_t actual_checksum =
            calculate_checksum(
                bytes,
                collider_payload_checksum_offset,
                checksum_byte_count);

        if (actual_checksum !=
            stored_checksum)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider payload checksum "
                "does not match its contents.");
        }

        const std::uint64_t
            remaining_byte_count =
                byte_count -
                world_cell_collider_payload_header_byte_count;

        if (collider_count >
            remaining_byte_count /
                world_cell_sphere_collider_record_byte_count)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider count cannot fit "
                "inside the payload.");
        }

        if (!std::in_range<std::size_t>(
                collider_count))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World cell collider count cannot "
                "be represented on this platform.");
        }

        try
        {
            std::vector<
                physics::ColliderGeometry>
                colliders{};

            colliders.reserve(
                static_cast<std::size_t>(
                    collider_count));

            physics::ColliderId
                previous_collider{};

            bool has_previous_collider{};

            for (std::uint64_t index = 0ULL;
                 index < collider_count;
                 ++index)
            {
                std::uint64_t entity_sequence{};
                std::uint32_t shape_slot{};
                std::uint32_t shape_kind_value{};
                double center_x{};
                double center_y{};
                double center_z{};

                const bool read_common_record =
                    reader.read_u64(
                        entity_sequence) &&
                    reader.read_u32(
                        shape_slot) &&
                    reader.read_u32(
                        shape_kind_value) &&
                    reader.read_f64(
                        center_x) &&
                    reader.read_f64(
                        center_y) &&
                    reader.read_f64(
                        center_z);

                if (!read_common_record)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell collider record "
                        "is truncated.");
                }

                const physics::ColliderId collider{
                    world::EntityId{
                        world_namespace,
                        entity_sequence
                    },
                    shape_slot
                };

                if (!collider.is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell collider payload "
                        "contains an invalid collider "
                        "identity.");
                }

                if (has_previous_collider &&
                    !(previous_collider <
                        collider))
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell collider records "
                        "are not in canonical collider "
                        "identity order.");
                }

                const physics::PhysicsVector3 center{
                    center_x,
                    center_y,
                    center_z
                };

                std::optional<
                    physics::CollisionShape>
                    shape{};

                if (shape_kind_value ==
                    static_cast<std::uint32_t>(
                        physics::CollisionShapeKind::
                            sphere))
                {
                    double radius{};

                    if (!reader.read_f64(
                            radius))
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell sphere collider "
                            "record is truncated.");
                    }

                    const auto shape_result =
                        physics::SphereShape::create(
                            radius);

                    if (!shape_result.has_value())
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell collider payload "
                            "contains an invalid sphere "
                            "shape.");
                    }

                    shape.emplace(
                        shape_result.value());
                }
                else if (
                    shape_kind_value ==
                    static_cast<std::uint32_t>(
                        physics::CollisionShapeKind::
                            box))
                {
                    double half_extent_x{};
                    double half_extent_y{};
                    double half_extent_z{};

                    const bool read_box =
                        reader.read_f64(
                            half_extent_x) &&
                        reader.read_f64(
                            half_extent_y) &&
                        reader.read_f64(
                            half_extent_z);

                    if (!read_box)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell box collider "
                            "record is truncated.");
                    }

                    const auto shape_result =
                        physics::BoxShape::create(
                            physics::PhysicsVector3{
                                half_extent_x,
                                half_extent_y,
                                half_extent_z
                            });

                    if (!shape_result.has_value())
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell collider payload "
                            "contains an invalid box "
                            "shape.");
                    }

                    shape.emplace(
                        shape_result.value());
                }
                else if (
                    shape_kind_value ==
                    static_cast<std::uint32_t>(
                        physics::CollisionShapeKind::
                            capsule))
                {
                    double radius{};
                    double half_segment_length{};
                    double axis_x{};
                    double axis_y{};
                    double axis_z{};

                    const bool read_capsule =
                        reader.read_f64(
                            radius) &&
                        reader.read_f64(
                            half_segment_length) &&
                        reader.read_f64(
                            axis_x) &&
                        reader.read_f64(
                            axis_y) &&
                        reader.read_f64(
                            axis_z);

                    if (!read_capsule)
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell capsule collider "
                            "record is truncated.");
                    }

                    const auto axis_result =
                        physics::
                            PhysicsUnitVector3::
                            create(
                                physics::
                                    PhysicsVector3{
                                        axis_x,
                                        axis_y,
                                        axis_z
                                    });

                    if (!axis_result.has_value())
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell collider payload "
                            "contains an invalid capsule "
                            "axis.");
                    }

                    const auto shape_result =
                        physics::CapsuleShape::create(
                            radius,
                            half_segment_length,
                            axis_result.value());

                    if (!shape_result.has_value())
                    {
                        return foundation::fail(
                            foundation::ErrorCode::
                                invalid_argument,
                            "World cell collider payload "
                            "contains an invalid capsule "
                            "shape.");
                    }

                    shape.emplace(
                        shape_result.value());
                }
                else
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell collider payload "
                        "contains an unsupported shape "
                        "kind.");
                }

                const auto geometry_result =
                    physics::ColliderGeometry::create(
                        collider,
                        std::move(
                            shape.value()),
                        center);

                if (!geometry_result.has_value())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "World cell collider payload "
                        "contains invalid collider "
                        "geometry.");
                }

                colliders.push_back(
                    geometry_result.value());

                previous_collider =
                    collider;

                has_previous_collider =
                    true;
            }

            if (!reader.complete())
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_argument,
                    "World cell collider payload "
                    "contains unexpected trailing "
                    "bytes.");
            }

            return
                WorldCellColliderSet::create(
                    payload_cell_key,
                    std::span<
                        const physics::
                            ColliderGeometry>{
                                colliders
                            });
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate decoded world "
                "cell collider storage.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while deserializing a world cell "
                "collider payload.");
        }
    }
}