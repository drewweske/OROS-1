#include "oros/physical_world/world_cell_collider_payload.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode expected_code,
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
        const std::span<const std::byte> bytes)
        noexcept
    {
        constexpr std::size_t
            checksum_offset{
                56U
            };

        constexpr std::size_t
            checksum_count{
                8U
            };

        std::uint64_t hash{
            14695981039346656037ULL
        };

        for (std::size_t index = 0U;
             index < bytes.size();
             ++index)
        {
            const bool excluded =
                index >= checksum_offset &&
                index - checksum_offset <
                    checksum_count;

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

    void patch_u32(
        std::vector<std::byte>& bytes,
        const std::size_t offset,
        const std::uint32_t value)
    {
        for (std::size_t index = 0U;
             index < 4U;
             ++index)
        {
            const unsigned int shift =
                static_cast<unsigned int>(
                    index * 8U);

            const std::uint8_t component =
                static_cast<std::uint8_t>(
                    (value >> shift) &
                    0xFFU);

            bytes[offset + index] =
                static_cast<std::byte>(
                    component);
        }
    }

    void patch_u64(
        std::vector<std::byte>& bytes,
        const std::size_t offset,
        const std::uint64_t value)
    {
        for (std::size_t index = 0U;
             index < 8U;
             ++index)
        {
            const unsigned int shift =
                static_cast<unsigned int>(
                    index * 8U);

            const std::uint8_t component =
                static_cast<std::uint8_t>(
                    (value >> shift) &
                    0xFFULL);

            bytes[offset + index] =
                static_cast<std::byte>(
                    component);
        }
    }

    void patch_f64(
        std::vector<std::byte>& bytes,
        const std::size_t offset,
        const double value)
    {
        patch_u64(
            bytes,
            offset,
            std::bit_cast<
                std::uint64_t>(
                    value));
    }

    void refresh_checksum(
        std::vector<std::byte>& bytes)
    {
        patch_u64(
            bytes,
            56U,
            calculate_checksum(
                std::span<const std::byte>{
                    bytes
                }));
    }

    [[nodiscard]]
    bool byte_equals(
        const std::vector<std::byte>& bytes,
        const std::size_t index,
        const std::uint8_t value)
        noexcept
    {
        return
            index < bytes.size() &&
            std::to_integer<std::uint8_t>(
                bytes[index]) ==
                value;
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell collider payload test "
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
    using namespace oros::foundation;
    using namespace oros::physical_world;
    using namespace oros::physics;
    using namespace oros::streaming;
    using namespace oros::world;

    TestState state{};

    check(
        state,
        world_cell_collider_payload_format_version ==
            1U,
        "Collider payload format begins at version one");

    check(
        state,
        world_cell_collider_payload_schema_version ==
            2U,
        "Collider payload schema advances to version two");

    check(
        state,
        world_cell_collider_payload_header_byte_count ==
            64ULL,
        "Collider payload header is 64 bytes");

    check(
        state,
        world_cell_sphere_collider_record_byte_count ==
            48ULL,
        "Sphere collider record is 48 bytes");

    check(
        state,
        world_cell_box_collider_record_byte_count ==
            64ULL,
        "Box collider record is 64 bytes");

    check(
        state,
        world_cell_capsule_collider_record_byte_count ==
            80ULL,
        "Capsule collider record is 80 bytes");

    constexpr std::uint64_t world_namespace{
        0x0102030405060708ULL
    };

    const WorldCellKey cell_key{
        world_namespace,
        WorldCell{
            -11,
            22,
            -33
        }
    };

    const auto empty_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{});

    check(
        state,
        empty_set_result.has_value(),
        "Empty collider set fixture is created");

    if (!empty_set_result.has_value())
    {
        return finish(state);
    }

    const auto empty_payload_result =
        serialize_world_cell_collider_payload(
            empty_set_result.value());

    check(
        state,
        empty_payload_result.has_value(),
        "Empty collider set serializes");

    if (!empty_payload_result.has_value())
    {
        return finish(state);
    }

    check(
        state,
        empty_payload_result.value().size() ==
            static_cast<std::size_t>(
                world_cell_collider_payload_header_byte_count),
        "Empty collider payload contains only its header");

    const auto empty_round_trip_result =
        deserialize_world_cell_collider_payload(
            cell_key,
            std::span<const std::byte>{
                empty_payload_result.value()
            });

    check(
        state,
        empty_round_trip_result.has_value(),
        "Empty collider payload deserializes");

    check(
        state,
        empty_round_trip_result.has_value() &&
            empty_round_trip_result.value() ==
                empty_set_result.value(),
        "Empty collider payload round trip is exact");

    const auto sphere_shape_result =
        SphereShape::create(
            2.25);

    const auto box_shape_result =
        BoxShape::create(
            PhysicsVector3{
                3.5,
                4.5,
                5.5
            });

    const auto capsule_axis_result =
        PhysicsUnitVector3::create(
            PhysicsVector3{
                0.0,
                0.6,
                0.8
            });

    check(
        state,
        sphere_shape_result.has_value(),
        "Sphere fixture is created");

    check(
        state,
        box_shape_result.has_value(),
        "Box fixture is created");

    check(
        state,
        capsule_axis_result.has_value(),
        "Capsule axis fixture is created");

    if (!sphere_shape_result.has_value() ||
        !box_shape_result.has_value() ||
        !capsule_axis_result.has_value())
    {
        return finish(state);
    }

    const auto capsule_shape_result =
        CapsuleShape::create(
            1.25,
            2.75,
            capsule_axis_result.value());

    check(
        state,
        capsule_shape_result.has_value(),
        "Capsule fixture is created");

    if (!capsule_shape_result.has_value())
    {
        return finish(state);
    }

    const ColliderId sphere_id{
        EntityId{
            world_namespace,
            10ULL
        },
        1U
    };

    const ColliderId box_id{
        EntityId{
            world_namespace,
            20ULL
        },
        2U
    };

    const ColliderId capsule_id{
        EntityId{
            world_namespace,
            30ULL
        },
        3U
    };

    const auto sphere_geometry_result =
        ColliderGeometry::create(
            sphere_id,
            sphere_shape_result.value(),
            PhysicsVector3{
                0.125,
                -0.25,
                0.5
            });

    const auto box_geometry_result =
        ColliderGeometry::create(
            box_id,
            box_shape_result.value(),
            PhysicsVector3{
                -100.5,
                200.25,
                -300.125
            });

    const auto capsule_geometry_result =
        ColliderGeometry::create(
            capsule_id,
            capsule_shape_result.value(),
            PhysicsVector3{
                100.5,
                -100.0,
                0.75
            });

    check(
        state,
        sphere_geometry_result.has_value(),
        "Sphere geometry fixture is created");

    check(
        state,
        box_geometry_result.has_value(),
        "Box geometry fixture is created");

    check(
        state,
        capsule_geometry_result.has_value(),
        "Capsule geometry fixture is created");

    if (!sphere_geometry_result.has_value() ||
        !box_geometry_result.has_value() ||
        !capsule_geometry_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        3U>
        unsorted_colliders{
            capsule_geometry_result.value(),
            sphere_geometry_result.value(),
            box_geometry_result.value()
        };

    const auto collider_set_result =
        WorldCellColliderSet::create(
            cell_key,
            std::span<
                const ColliderGeometry>{
                    unsorted_colliders
                });

    check(
        state,
        collider_set_result.has_value(),
        "Populated collider set fixture is created");

    if (!collider_set_result.has_value())
    {
        return finish(state);
    }

    const WorldCellColliderSet&
        collider_set =
            collider_set_result.value();

    const auto payload_result =
        serialize_world_cell_collider_payload(
            collider_set);

    check(
        state,
        payload_result.has_value(),
        "Populated collider set serializes");

    if (!payload_result.has_value())
    {
        return finish(state);
    }

    const std::vector<std::byte>& payload =
        payload_result.value();

    check(
        state,
        payload.size() ==
            256U,
        "Mixed collider payload has exact expected size");

    check(
        state,
        byte_equals(
            payload,
            0U,
            0x4FU) &&
        byte_equals(
            payload,
            1U,
            0x52U) &&
        byte_equals(
            payload,
            2U,
            0x4FU) &&
        byte_equals(
            payload,
            3U,
            0x53U) &&
        byte_equals(
            payload,
            4U,
            0x43U) &&
        byte_equals(
            payload,
            5U,
            0x4FU) &&
        byte_equals(
            payload,
            6U,
            0x4CU) &&
        byte_equals(
            payload,
            7U,
            0x31U),
        "Payload writes OROSCOL1 magic exactly");

    check(
        state,
        byte_equals(
            payload,
            8U,
            0x01U) &&
        byte_equals(
            payload,
            9U,
            0x00U) &&
        byte_equals(
            payload,
            10U,
            0x00U) &&
        byte_equals(
            payload,
            11U,
            0x00U),
        "Payload writes format version little-endian");

    check(
        state,
        byte_equals(
            payload,
            16U,
            0x08U) &&
        byte_equals(
            payload,
            17U,
            0x07U) &&
        byte_equals(
            payload,
            18U,
            0x06U) &&
        byte_equals(
            payload,
            19U,
            0x05U) &&
        byte_equals(
            payload,
            20U,
            0x04U) &&
        byte_equals(
            payload,
            21U,
            0x03U) &&
        byte_equals(
            payload,
            22U,
            0x02U) &&
        byte_equals(
            payload,
            23U,
            0x01U),
        "Payload writes world namespace little-endian");

    const auto round_trip_result =
        deserialize_world_cell_collider_payload(
            cell_key,
            std::span<const std::byte>{
                payload
            });

    check(
        state,
        round_trip_result.has_value(),
        "Populated collider payload deserializes");

    check(
        state,
        round_trip_result.has_value() &&
            round_trip_result.value() ==
                collider_set,
        "All collider identities and geometry round trip exactly");

    const auto second_payload_result =
        serialize_world_cell_collider_payload(
            collider_set);

    check(
        state,
        second_payload_result.has_value(),
        "Same collider set serializes a second time");

    check(
        state,
        second_payload_result.has_value() &&
            second_payload_result.value() ==
                payload,
        "Serialization is byte-for-byte deterministic");

    check_failure(
        state,
        deserialize_world_cell_collider_payload(
            invalid_world_cell_key,
            std::span<const std::byte>{
                payload
            }),
        ErrorCode::invalid_argument,
        "Deserialization rejects invalid expected cell key");

    const WorldCellKey wrong_cell_key{
        world_namespace,
        WorldCell{
            -11,
            22,
            -32
        }
    };

    check_failure(
        state,
        deserialize_world_cell_collider_payload(
            wrong_cell_key,
            std::span<const std::byte>{
                payload
            }),
        ErrorCode::invalid_argument,
        "Deserialization rejects wrong expected cell");

    check_failure(
        state,
        deserialize_world_cell_collider_payload(
            cell_key,
            std::span<const std::byte>{
                payload.data(),
                63U
            }),
        ErrorCode::invalid_argument,
        "Deserialization rejects truncated header");

    {
        std::vector<std::byte> corrupted =
            payload;

        corrupted[0U] =
            std::byte{0x00};

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects wrong magic");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u32(
            corrupted,
            8U,
            2U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects unsupported format version");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u32(
            corrupted,
            12U,
            1U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects prior collider schema without migration");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u32(
            corrupted,
            12U,
            3U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects unsupported future schema version");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u64(
            corrupted,
            16U,
            world_namespace + 1ULL);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects payload from another namespace");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        corrupted[80U] ^=
            std::byte{0x01};

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects checksum corruption");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u64(
            corrupted,
            48U,
            100ULL);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects collider count that cannot fit payload");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u64(
            corrupted,
            64U,
            0ULL);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects zero entity sequence");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u32(
            corrupted,
            72U,
            0U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects zero shape slot");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u32(
            corrupted,
            76U,
            99U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects unsupported shape kind");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_f64(
            corrupted,
            104U,
            0.0);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects invalid sphere radius");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_f64(
            corrupted,
            152U,
            0.0);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects invalid box half extent");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_f64(
            corrupted,
            232U,
            0.0);

        patch_f64(
            corrupted,
            240U,
            0.0);

        patch_f64(
            corrupted,
            248U,
            0.0);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects invalid capsule axis");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u64(
            corrupted,
            112U,
            5ULL);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects noncanonical collider order");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_u64(
            corrupted,
            112U,
            10ULL);

        patch_u32(
            corrupted,
            120U,
            1U);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects duplicate collider identity");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_f64(
            corrupted,
            80U,
            world_cell_half_extent_meters);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects noncanonical local center");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        patch_f64(
            corrupted,
            80U,
            world_cell_half_extent_meters -
                1.0);

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects canonical-center collider bounds crossing cell boundary");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        corrupted.pop_back();

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects truncated collider data");
    }

    {
        std::vector<std::byte> corrupted =
            payload;

        corrupted.push_back(
            std::byte{0xAA});

        refresh_checksum(
            corrupted);

        check_failure(
            state,
            deserialize_world_cell_collider_payload(
                cell_key,
                std::span<const std::byte>{
                    corrupted
                }),
            ErrorCode::invalid_argument,
            "Deserialization rejects trailing bytes");
    }

    return finish(state);
}