#include "oros/physical_world/world_cell_collider_registry.hpp"

#include "oros/foundation/error.hpp"
#include "oros/physical_world/world_cell_collider_payload.hpp"
#include "oros/physics/collider_geometry.hpp"
#include "oros/physics/collider_id.hpp"
#include "oros/physics/collision_shape.hpp"
#include "oros/physics/physics_vector.hpp"
#include "oros/streaming/world_cell_residency.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/world/entity_id.hpp"
#include "oros/world/world_position.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <span>
#include <string_view>
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

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell collider registry test "
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

    [[nodiscard]]
    bool make_resident(
        oros::streaming::WorldCellResidency&
            residency,
        oros::streaming::WorldCellSnapshot
            snapshot,
        const std::uint64_t request_id)
    {
        const oros::streaming::
            WorldCellRevisionId
            revision{
                snapshot.key(),
                snapshot.revision()
            };

        if (!residency.queue_load(
                revision,
                request_id).
                has_value())
        {
            return false;
        }

        if (!residency.begin_load(
                request_id).
                has_value())
        {
            return false;
        }

        return
            residency.complete_load(
                request_id,
                std::move(
                    snapshot)).
                has_value();
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

    WorldCellColliderRegistry registry{};

    check(
        state,
        registry.is_valid(),
        "Default collider registry is valid");

    check(
        state,
        registry.empty(),
        "Default collider registry is empty");

    check(
        state,
        registry.active_cell_count() ==
            0U,
        "Default registry has zero active cells");

    check(
        state,
        registry.collider_count() ==
            0U,
        "Default registry has zero colliders");

    check(
        state,
        registry.find_cell(
            invalid_world_cell_key) ==
            nullptr,
        "Invalid cell lookup returns null");

    check(
        state,
        registry.find(
            invalid_collider_id) ==
            nullptr,
        "Invalid collider lookup returns null");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000008ULL
        };

    const WorldCellKey cell_a{
        world_namespace,
        WorldCell{
            10,
            20,
            30
        }
    };

    const WorldCellKey cell_b{
        world_namespace,
        WorldCell{
            11,
            20,
            30
        }
    };

    const ColliderId collider_a{
        EntityId{
            world_namespace,
            100ULL
        },
        1U
    };

    const ColliderId collider_b{
        EntityId{
            world_namespace,
            200ULL
        },
        1U
    };

    const auto sphere_a_result =
        SphereShape::create(
            1.25);

    const auto sphere_b_result =
        SphereShape::create(
            2.5);

    check(
        state,
        sphere_a_result.has_value(),
        "Cell A sphere shape is created");

    check(
        state,
        sphere_b_result.has_value(),
        "Cell B sphere shape is created");

    if (!sphere_a_result.has_value() ||
        !sphere_b_result.has_value())
    {
        return finish(state);
    }

    const auto geometry_a_result =
        ColliderGeometry::create(
            collider_a,
            sphere_a_result.value(),
            PhysicsVector3{
                1.0,
                2.0,
                3.0
            });

    const auto geometry_b_result =
        ColliderGeometry::create(
            collider_b,
            sphere_b_result.value(),
            PhysicsVector3{
                -4.0,
                5.0,
                -6.0
            });

    check(
        state,
        geometry_a_result.has_value(),
        "Cell A collider geometry is created");

    check(
        state,
        geometry_b_result.has_value(),
        "Cell B collider geometry is created");

    if (!geometry_a_result.has_value() ||
        !geometry_b_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        cell_a_colliders{
            geometry_a_result.value()
        };

    const std::array<
        ColliderGeometry,
        1U>
        cell_b_colliders{
            geometry_b_result.value()
        };

    const auto set_a_result =
        WorldCellColliderSet::create(
            cell_a,
            std::span<
                const ColliderGeometry>{
                    cell_a_colliders
                });

    const auto set_b_result =
        WorldCellColliderSet::create(
            cell_b,
            std::span<
                const ColliderGeometry>{
                    cell_b_colliders
                });

    check(
        state,
        set_a_result.has_value(),
        "Cell A collider set is created");

    check(
        state,
        set_b_result.has_value(),
        "Cell B collider set is created");

    if (!set_a_result.has_value() ||
        !set_b_result.has_value())
    {
        return finish(state);
    }

    const auto payload_a_result =
        serialize_world_cell_collider_payload(
            set_a_result.value());

    const auto payload_b_result =
        serialize_world_cell_collider_payload(
            set_b_result.value());

    check(
        state,
        payload_a_result.has_value(),
        "Cell A collider payload serializes");

    check(
        state,
        payload_b_result.has_value(),
        "Cell B collider payload serializes");

    if (!payload_a_result.has_value() ||
        !payload_b_result.has_value())
    {
        return finish(state);
    }

    const auto snapshot_a_result =
        WorldCellSnapshot::create(
            cell_a,
            7ULL,
            std::span<const std::byte>{
                payload_a_result.value()
            });

    const auto snapshot_b_result =
        WorldCellSnapshot::create(
            cell_b,
            8ULL,
            std::span<const std::byte>{
                payload_b_result.value()
            });

    check(
        state,
        snapshot_a_result.has_value(),
        "Cell A snapshot is created");

    check(
        state,
        snapshot_b_result.has_value(),
        "Cell B snapshot is created");

    if (!snapshot_a_result.has_value() ||
        !snapshot_b_result.has_value())
    {
        return finish(state);
    }

    auto residency_a_result =
        WorldCellResidency::create(
            cell_a);

    check(
        state,
        residency_a_result.has_value(),
        "Cell A residency is created");

    if (!residency_a_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency residency_a =
        std::move(
            residency_a_result.value());

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Unloaded cell synchronizes successfully");

    check(
        state,
        registry.empty(),
        "Unloaded cell does not activate colliders");

    const WorldCellRevisionId
        revision_a{
            cell_a,
            7ULL
        };

    check(
        state,
        residency_a.queue_load(
            revision_a,
            101ULL).
            has_value(),
        "Cell A load is queued");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Load-queued cell synchronizes successfully");

    check(
        state,
        registry.empty(),
        "Load-queued cell remains physically inactive");

    check(
        state,
        residency_a.begin_load(
            101ULL).
            has_value(),
        "Cell A load begins");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Loading cell synchronizes successfully");

    check(
        state,
        registry.empty(),
        "Loading cell remains physically inactive");

    check(
        state,
        residency_a.complete_load(
            101ULL,
            snapshot_a_result.value()).
            has_value(),
        "Cell A load completes");

    check(
        state,
        residency_a.state() ==
            WorldCellResidencyState::
                resident,
        "Cell A becomes stream-resident");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Resident cell activates successfully");

    check(
        state,
        registry.is_valid(),
        "Registry remains valid after activation");

    check(
        state,
        registry.active_cell_count() ==
            1U,
        "Resident cell becomes physically active");

    check(
        state,
        registry.collider_count() ==
            1U,
        "Resident cell contributes one collider");

    check(
        state,
        registry.contains_cell(
            cell_a),
        "Registry contains active cell A");

    check(
        state,
        registry.contains(
            collider_a),
        "Registry contains cell A collider");

    check(
        state,
        registry.find_cell(
            cell_a) !=
            nullptr,
        "Active cell A can be found");

    check(
        state,
        registry.find(
            collider_a) !=
            nullptr,
        "Active collider A can be found");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Repeated synchronization is idempotent");

    check(
        state,
        registry.active_cell_count() ==
                1U &&
            registry.collider_count() ==
                1U,
        "Repeated synchronization does not duplicate data");

    check(
        state,
        residency_a.queue_unload().
            has_value(),
        "Cell A unload is queued");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Unload-queued cell synchronizes successfully");

    check(
        state,
        registry.contains(
            collider_a),
        "Unload-queued cell remains physically active");

    check(
        state,
        residency_a.begin_unload().
            has_value(),
        "Cell A begins unloading");

    check(
        state,
        residency_a.is_resident(),
        "Streaming still owns snapshot while unloading");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Unloading cell synchronizes successfully");

    check(
        state,
        !registry.contains_cell(
            cell_a) &&
            !registry.contains(
                collider_a),
        "Unloading removes physical colliders immediately");

    check(
        state,
        registry.empty(),
        "Registry is empty before streaming destroys snapshot");

    check(
        state,
        residency_a.complete_unload().
            has_value(),
        "Cell A unload completes");

    check(
        state,
        registry.synchronize(
            residency_a).
            has_value(),
        "Completed unload synchronizes successfully");

    check(
        state,
        registry.empty(),
        "Completed unload remains physically inactive");

    auto residency_a_multi_result =
        WorldCellResidency::create(
            cell_a);

    auto residency_b_multi_result =
        WorldCellResidency::create(
            cell_b);

    check(
        state,
        residency_a_multi_result.has_value(),
        "Multi-cell residency A is created");

    check(
        state,
        residency_b_multi_result.has_value(),
        "Multi-cell residency B is created");

    if (!residency_a_multi_result.has_value() ||
        !residency_b_multi_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency residency_a_multi =
        std::move(
            residency_a_multi_result.value());

    WorldCellResidency residency_b_multi =
        std::move(
            residency_b_multi_result.value());

    check(
        state,
        make_resident(
            residency_a_multi,
            snapshot_a_result.value(),
            201ULL),
        "Multi-cell residency A becomes resident");

    check(
        state,
        make_resident(
            residency_b_multi,
            snapshot_b_result.value(),
            202ULL),
        "Multi-cell residency B becomes resident");

    check(
        state,
        registry.synchronize(
            residency_b_multi).
            has_value(),
        "Cell B activates before cell A");

    check(
        state,
        registry.synchronize(
            residency_a_multi).
            has_value(),
        "Cell A activates after cell B");

    check(
        state,
        registry.is_valid(),
        "Registry canonical ordering survives reverse activation");

    check(
        state,
        registry.active_cell_count() ==
            2U,
        "Two cells are physically active");

    check(
        state,
        registry.collider_count() ==
            2U,
        "Two active cells contribute two colliders");

    check(
        state,
        registry.contains(
            collider_a) &&
            registry.contains(
                collider_b),
        "Global lookup finds colliders from both cells");

    WorldCellColliderRegistry
        duplicate_registry{};

    const ColliderId shared_collider{
        EntityId{
            world_namespace,
            900ULL
        },
        1U
    };

    const auto shared_geometry_a_result =
        ColliderGeometry::create(
            shared_collider,
            sphere_a_result.value(),
            PhysicsVector3{
                0.0,
                0.0,
                0.0
            });

    const auto shared_geometry_b_result =
        ColliderGeometry::create(
            shared_collider,
            sphere_b_result.value(),
            PhysicsVector3{
                1.0,
                0.0,
                0.0
            });

    check(
        state,
        shared_geometry_a_result.has_value(),
        "First shared-identity geometry is created");

    check(
        state,
        shared_geometry_b_result.has_value(),
        "Second shared-identity geometry is created");

    if (!shared_geometry_a_result.has_value() ||
        !shared_geometry_b_result.has_value())
    {
        return finish(state);
    }

    const std::array<
        ColliderGeometry,
        1U>
        shared_a_colliders{
            shared_geometry_a_result.value()
        };

    const std::array<
        ColliderGeometry,
        1U>
        shared_b_colliders{
            shared_geometry_b_result.value()
        };

    const auto shared_set_a_result =
        WorldCellColliderSet::create(
            cell_a,
            std::span<
                const ColliderGeometry>{
                    shared_a_colliders
                });

    const auto shared_set_b_result =
        WorldCellColliderSet::create(
            cell_b,
            std::span<
                const ColliderGeometry>{
                    shared_b_colliders
                });

    check(
        state,
        shared_set_a_result.has_value() &&
            shared_set_b_result.has_value(),
        "Shared-identity collider sets are individually valid");

    if (!shared_set_a_result.has_value() ||
        !shared_set_b_result.has_value())
    {
        return finish(state);
    }

    const auto shared_payload_a_result =
        serialize_world_cell_collider_payload(
            shared_set_a_result.value());

    const auto shared_payload_b_result =
        serialize_world_cell_collider_payload(
            shared_set_b_result.value());

    check(
        state,
        shared_payload_a_result.has_value() &&
            shared_payload_b_result.has_value(),
        "Shared-identity payloads serialize independently");

    if (!shared_payload_a_result.has_value() ||
        !shared_payload_b_result.has_value())
    {
        return finish(state);
    }

    const auto shared_snapshot_a_result =
        WorldCellSnapshot::create(
            cell_a,
            17ULL,
            std::span<const std::byte>{
                shared_payload_a_result.value()
            });

    const auto shared_snapshot_b_result =
        WorldCellSnapshot::create(
            cell_b,
            18ULL,
            std::span<const std::byte>{
                shared_payload_b_result.value()
            });

    check(
        state,
        shared_snapshot_a_result.has_value() &&
            shared_snapshot_b_result.has_value(),
        "Shared-identity snapshots are created");

    if (!shared_snapshot_a_result.has_value() ||
        !shared_snapshot_b_result.has_value())
    {
        return finish(state);
    }

    auto duplicate_a_result =
        WorldCellResidency::create(
            cell_a);

    auto duplicate_b_result =
        WorldCellResidency::create(
            cell_b);

    check(
        state,
        duplicate_a_result.has_value() &&
            duplicate_b_result.has_value(),
        "Duplicate test residency records are created");

    if (!duplicate_a_result.has_value() ||
        !duplicate_b_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency duplicate_a =
        std::move(
            duplicate_a_result.value());

    WorldCellResidency duplicate_b =
        std::move(
            duplicate_b_result.value());

    check(
        state,
        make_resident(
            duplicate_a,
            shared_snapshot_a_result.value(),
            301ULL),
        "First shared-identity cell becomes resident");

    check(
        state,
        make_resident(
            duplicate_b,
            shared_snapshot_b_result.value(),
            302ULL),
        "Second shared-identity cell becomes resident");

    check(
        state,
        duplicate_registry.synchronize(
            duplicate_a).
            has_value(),
        "First shared collider identity activates");

    const auto duplicate_activation_result =
        duplicate_registry.synchronize(
            duplicate_b);

    check(
        state,
        !duplicate_activation_result.has_value() &&
            duplicate_activation_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Second active copy of persistent collider identity is rejected");

    check(
        state,
        duplicate_registry.active_cell_count() ==
                1U &&
            duplicate_registry.collider_count() ==
                1U,
        "Rejected duplicate activation leaves registry unchanged");

    check(
        state,
        duplicate_registry.is_valid(),
        "Registry remains valid after duplicate rejection");

    WorldCellColliderRegistry
        corrupted_registry{};

    std::vector<std::byte>
        corrupted_payload =
            payload_a_result.value();

    corrupted_payload[
        corrupted_payload.size() - 1U] ^=
            std::byte{0x01};

    const auto corrupted_snapshot_result =
        WorldCellSnapshot::create(
            cell_a,
            27ULL,
            std::span<const std::byte>{
                corrupted_payload
            });

    check(
        state,
        corrupted_snapshot_result.has_value(),
        "Opaque streaming snapshot accepts corrupted physics payload bytes");

    if (!corrupted_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto corrupted_residency_result =
        WorldCellResidency::create(
            cell_a);

    check(
        state,
        corrupted_residency_result.has_value(),
        "Corrupted-payload residency is created");

    if (!corrupted_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency corrupted_residency =
        std::move(
            corrupted_residency_result.value());

    check(
        state,
        make_resident(
            corrupted_residency,
            corrupted_snapshot_result.value(),
            401ULL),
        "Streaming can resident-own opaque corrupted physics payload");

    const auto corrupted_sync_result =
        corrupted_registry.synchronize(
            corrupted_residency);

    check(
        state,
        !corrupted_sync_result.has_value() &&
            corrupted_sync_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Physical registry rejects corrupted collider payload");

    check(
        state,
        corrupted_registry.empty(),
        "Failed payload activation leaves registry empty");

    WorldCellColliderRegistry
        wrong_cell_registry{};

    const auto wrong_cell_snapshot_result =
        WorldCellSnapshot::create(
            cell_b,
            37ULL,
            std::span<const std::byte>{
                payload_a_result.value()
            });

    check(
        state,
        wrong_cell_snapshot_result.has_value(),
        "Streaming snapshot can hold payload bound to another cell");

    if (!wrong_cell_snapshot_result.has_value())
    {
        return finish(state);
    }

    auto wrong_cell_residency_result =
        WorldCellResidency::create(
            cell_b);

    check(
        state,
        wrong_cell_residency_result.has_value(),
        "Wrong-cell payload residency is created");

    if (!wrong_cell_residency_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency wrong_cell_residency =
        std::move(
            wrong_cell_residency_result.value());

    check(
        state,
        make_resident(
            wrong_cell_residency,
            wrong_cell_snapshot_result.value(),
            501ULL),
        "Wrong-cell opaque payload becomes stream-resident");

    const auto wrong_cell_sync_result =
        wrong_cell_registry.synchronize(
            wrong_cell_residency);

    check(
        state,
        !wrong_cell_sync_result.has_value() &&
            wrong_cell_sync_result.
                error().
                code ==
            ErrorCode::invalid_argument,
        "Physical registry rejects payload bound to another cell");

    check(
        state,
        wrong_cell_registry.empty(),
        "Wrong-cell payload failure leaves registry empty");

    WorldCellColliderRegistry
        revision_registry{};

    auto revision_old_residency_result =
        WorldCellResidency::create(
            cell_a);

    auto revision_new_residency_result =
        WorldCellResidency::create(
            cell_a);

    const auto revision_new_snapshot_result =
        WorldCellSnapshot::create(
            cell_a,
            47ULL,
            std::span<const std::byte>{
                payload_a_result.value()
            });

    check(
        state,
        revision_old_residency_result.has_value() &&
            revision_new_residency_result.has_value() &&
            revision_new_snapshot_result.has_value(),
        "Revision-change fixtures are created");

    if (!revision_old_residency_result.has_value() ||
        !revision_new_residency_result.has_value() ||
        !revision_new_snapshot_result.has_value())
    {
        return finish(state);
    }

    WorldCellResidency revision_old_residency =
        std::move(
            revision_old_residency_result.value());

    WorldCellResidency revision_new_residency =
        std::move(
            revision_new_residency_result.value());

    check(
        state,
        make_resident(
            revision_old_residency,
            snapshot_a_result.value(),
            601ULL),
        "Old cell revision becomes resident");

    check(
        state,
        make_resident(
            revision_new_residency,
            revision_new_snapshot_result.value(),
            602ULL),
        "New cell revision becomes resident independently");

    check(
        state,
        revision_registry.synchronize(
            revision_old_residency).
            has_value(),
        "Old cell revision activates");

    const auto revision_change_result =
        revision_registry.synchronize(
            revision_new_residency);

    check(
        state,
        !revision_change_result.has_value() &&
            revision_change_result.
                error().
                code ==
            ErrorCode::invalid_state,
        "Active cell revision cannot change without deactivation");

    check(
        state,
        revision_registry.active_cell_count() ==
                1U &&
            revision_registry.collider_count() ==
                1U &&
            revision_registry.is_valid(),
        "Rejected revision replacement preserves registry state");

    return finish(state);
}