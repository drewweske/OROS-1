#include "oros/streaming/world_cell_persistence_store.hpp"

#include "oros/foundation/error.hpp"

#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    int finish(
        const TestState& state)
    {
        std::cout
            << "\nWorld cell persistence store test "
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

    class TemporaryDirectory final
    {
    public:
        TemporaryDirectory()
        {
            static std::atomic<std::uint64_t>
                sequence{
                    0ULL
                };

            const auto timestamp =
                std::chrono::steady_clock::now().
                    time_since_epoch().
                    count();

            const std::uint64_t suffix =
                sequence.fetch_add(
                    1ULL,
                    std::memory_order_relaxed);

            path_ =
                std::filesystem::
                    temp_directory_path() /
                (
                    "oros-world-cell-persistence-tests-" +
                    std::to_string(
                        timestamp) +
                    "-" +
                    std::to_string(
                        suffix)
                );

            std::error_code
                create_error{};

            std::filesystem::create_directories(
                path_,
                create_error);

            valid_ =
                !create_error;
        }

        ~TemporaryDirectory()
        {
            std::error_code
                remove_error{};

            std::filesystem::remove_all(
                path_,
                remove_error);
        }

        TemporaryDirectory(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory&
        operator=(
            const TemporaryDirectory&) =
                delete;

        TemporaryDirectory(
            TemporaryDirectory&&) =
                delete;

        TemporaryDirectory&
        operator=(
            TemporaryDirectory&&) =
                delete;

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return valid_;
        }

        [[nodiscard]]
        const std::filesystem::path&
        path() const noexcept
        {
            return path_;
        }

    private:
        std::filesystem::path
            path_{};

        bool valid_{};
    };

    [[nodiscard]]
    bool corrupt_byte(
        const std::filesystem::path& path,
        const std::size_t offset)
    {
        std::fstream stream{
            path,
            std::ios::binary |
                std::ios::in |
                std::ios::out
        };

        if (!stream.is_open())
        {
            return false;
        }

        const std::streamoff stream_offset =
            static_cast<std::streamoff>(
                offset);

        stream.seekg(
            stream_offset,
            std::ios::beg);

        if (!stream)
        {
            return false;
        }

        char value{};

        stream.read(
            &value,
            1);

        if (!stream)
        {
            return false;
        }

        const unsigned char original =
            static_cast<unsigned char>(
                value);

        value =
            static_cast<char>(
                original ^
                static_cast<unsigned char>(
                    0x01U));

        stream.seekp(
            stream_offset,
            std::ios::beg);

        if (!stream)
        {
            return false;
        }

        stream.write(
            &value,
            1);

        stream.close();

        return
            !stream.fail();
    }

    [[nodiscard]]
    bool copy_file_to(
        const std::filesystem::path& source,
        const std::filesystem::path& destination)
    {
        std::error_code
            directory_error{};

        std::filesystem::create_directories(
            destination.parent_path(),
            directory_error);

        if (directory_error)
        {
            return false;
        }

        std::error_code
            copy_error{};

        std::filesystem::copy_file(
            source,
            destination,
            std::filesystem::
                copy_options::
                    overwrite_existing,
            copy_error);

        return
            !copy_error;
    }

    [[nodiscard]]
    bool has_temporary_files(
        const std::filesystem::path& root)
    {
        std::error_code
            exists_error{};

        if (!std::filesystem::exists(
                root,
                exists_error) ||
            exists_error)
        {
            return false;
        }

        std::error_code
            iterator_error{};

        std::filesystem::
            recursive_directory_iterator iterator{
                root,
                iterator_error
            };

        const std::filesystem::
            recursive_directory_iterator end{};

        while (!iterator_error &&
               iterator != end)
        {
            const std::string filename =
                iterator->path().
                    filename().
                    string();

            if (filename.find(
                    ".tmp-") !=
                std::string::npos)
            {
                return true;
            }

            iterator.increment(
                iterator_error);
        }

        return false;
    }
}

int main()
{
    using namespace oros::streaming;
    using namespace oros::world;
    using oros::foundation::ErrorCode;

    TestState state{};

    check(
        state,
        world_cell_snapshot_file_format_version ==
            1U,
        "Snapshot file format begins at version one");

    check(
        state,
        world_cell_manifest_file_format_version ==
            1U,
        "Manifest file format begins at version one");

    check(
        state,
        world_cell_manifest_pointer_format_version ==
            1U,
        "Manifest pointer format begins at version one");

    const WorldCellPersistenceLimits
        default_limits{};

    check(
        state,
        default_limits.is_valid(),
        "Default persistence limits are valid");

    check(
        state,
        default_limits.
            maximum_snapshot_payload_byte_count ==
            64ULL *
                1024ULL *
                1024ULL,
        "Default snapshot payload limit is 64 MiB");

    check(
        state,
        default_limits.
            maximum_manifest_entry_count ==
            1'000'000ULL,
        "Default manifest entry limit is one million");

    const WorldCellPersistenceLimits
        zero_payload_limits{
            0ULL,
            1ULL
        };

    check(
        state,
        !zero_payload_limits.is_valid(),
        "Zero snapshot payload limit is invalid");

    const WorldCellPersistenceLimits
        zero_manifest_limits{
            1ULL,
            0ULL
        };

    check(
        state,
        !zero_manifest_limits.is_valid(),
        "Zero manifest entry limit is invalid");

    TemporaryDirectory
        temporary_directory{};

    check(
        state,
        temporary_directory.is_valid(),
        "Temporary persistence directory is created");

    if (!temporary_directory.is_valid())
    {
        return finish(state);
    }

    WorldCellPersistenceStore invalid_store{
        std::filesystem::path{}
    };

    check(
        state,
        !invalid_store.is_valid(),
        "Store with an empty root is invalid");

    check_failure(
        state,
        invalid_store.snapshot_path_for(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_state,
        "Invalid store rejects snapshot path generation");

    check_failure(
        state,
        invalid_store.manifest_path_for(
            1ULL,
            1ULL),
        ErrorCode::invalid_state,
        "Invalid store rejects manifest path generation");

    check_failure(
        state,
        invalid_store.
            current_manifest_pointer_path_for(
                1ULL),
        ErrorCode::invalid_state,
        "Invalid store rejects pointer path generation");

    check_failure(
        state,
        invalid_store.load_snapshot(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_state,
        "Invalid store rejects snapshot loading");

    check_failure(
        state,
        invalid_store.contains_snapshot(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_state,
        "Invalid store rejects snapshot inspection");

    check_failure(
        state,
        invalid_store.load_manifest(
            1ULL,
            1ULL),
        ErrorCode::invalid_state,
        "Invalid store rejects manifest loading");

    check_failure(
        state,
        invalid_store.contains_manifest(
            1ULL,
            1ULL),
        ErrorCode::invalid_state,
        "Invalid store rejects manifest inspection");

    check_failure(
        state,
        invalid_store.load_current_manifest(
            1ULL),
        ErrorCode::invalid_state,
        "Invalid store rejects current manifest loading");

    WorldCellPersistenceStore
        invalid_limits_store{
            temporary_directory.path() /
                "invalid-limits",
            zero_payload_limits
        };

    check(
        state,
        !invalid_limits_store.is_valid(),
        "Store with invalid persistence limits is invalid");

    const std::filesystem::path store_root =
        temporary_directory.path() /
        "store";

    WorldCellPersistenceStore store{
        store_root
    };

    check(
        state,
        store.is_valid(),
        "Store with a root and valid limits is valid");

    check(
        state,
        store.root_directory() ==
            store_root,
        "Store preserves its root directory");

    check(
        state,
        store.limits().is_valid(),
        "Store preserves valid persistence limits");

    WorldCellPersistenceStore moved_store{
        std::move(
            store)
    };

    check(
        state,
        moved_store.is_valid(),
        "Move construction preserves a valid store");

    check(
        state,
        !store.is_valid(),
        "Move construction invalidates the source store");

    WorldCellPersistenceStore
        assigned_store{
            temporary_directory.path() /
                "unused"
        };

    assigned_store =
        std::move(
            moved_store);

    check(
        state,
        assigned_store.is_valid(),
        "Move assignment preserves a valid store");

    check(
        state,
        !moved_store.is_valid(),
        "Move assignment invalidates the source store");

    assigned_store =
        std::move(
            assigned_store);

    check(
        state,
        assigned_store.is_valid(),
        "Self move assignment preserves the store");

    constexpr std::uint64_t
        world_namespace{
            0x4F524F5300000007ULL
        };

    constexpr std::uint64_t
        other_world_namespace{
            0x4F524F5300000008ULL
        };

    const WorldCellKey cell_a{
        world_namespace,
        WorldCell{
            -10,
            20,
            -30
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
            99,
            -50,
            25
        }
    };

    const WorldCellRevisionId revision_a{
        cell_a,
        7ULL
    };

    const WorldCellRevisionId revision_b{
        cell_b,
        4ULL
    };

    const WorldCellRevisionId revision_c{
        cell_c,
        2ULL
    };

    check_failure(
        state,
        assigned_store.snapshot_path_for(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_argument,
        "Valid store rejects an invalid snapshot identity");

    check_failure(
        state,
        assigned_store.manifest_path_for(
            0ULL,
            1ULL),
        ErrorCode::invalid_argument,
        "Manifest path rejects a zero namespace");

    check_failure(
        state,
        assigned_store.manifest_path_for(
            world_namespace,
            0ULL),
        ErrorCode::invalid_argument,
        "Manifest path rejects a zero revision");

    check_failure(
        state,
        assigned_store.
            current_manifest_pointer_path_for(
                0ULL),
        ErrorCode::invalid_argument,
        "Pointer path rejects a zero namespace");

    check_failure(
        state,
        assigned_store.load_snapshot(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_argument,
        "Snapshot loading rejects an invalid identity");

    check_failure(
        state,
        assigned_store.contains_snapshot(
            invalid_world_cell_revision_id),
        ErrorCode::invalid_argument,
        "Snapshot inspection rejects an invalid identity");

    check_failure(
        state,
        assigned_store.load_manifest(
            0ULL,
            1ULL),
        ErrorCode::invalid_argument,
        "Manifest loading rejects a zero namespace");

    check_failure(
        state,
        assigned_store.load_manifest(
            world_namespace,
            0ULL),
        ErrorCode::invalid_argument,
        "Manifest loading rejects a zero revision");

    check_failure(
        state,
        assigned_store.load_current_manifest(
            0ULL),
        ErrorCode::invalid_argument,
        "Current manifest loading rejects a zero namespace");

    const auto snapshot_path_result =
        assigned_store.snapshot_path_for(
            revision_a);

    check(
        state,
        snapshot_path_result.has_value(),
        "Snapshot path is generated");

    const std::filesystem::path
        expected_snapshot_path =
            store_root /
            "worlds" /
            "4f524f5300000007" /
            "snapshots" /
            "-10" /
            "20" /
            "-30@7.orossnapshot";

    check(
        state,
        snapshot_path_result.has_value() &&
            snapshot_path_result.value() ==
                expected_snapshot_path,
        "Snapshot path is deterministic");

    const auto manifest_path_result =
        assigned_store.manifest_path_for(
            world_namespace,
            10ULL);

    check(
        state,
        manifest_path_result.has_value(),
        "Manifest path is generated");

    const std::filesystem::path
        expected_manifest_path =
            store_root /
            "worlds" /
            "4f524f5300000007" /
            "manifests" /
            "10.orosmanifest";

    check(
        state,
        manifest_path_result.has_value() &&
            manifest_path_result.value() ==
                expected_manifest_path,
        "Manifest path is deterministic");

    const auto pointer_path_result =
        assigned_store.
            current_manifest_pointer_path_for(
                world_namespace);

    check(
        state,
        pointer_path_result.has_value(),
        "Current manifest pointer path is generated");

    const std::filesystem::path
        expected_pointer_path =
            store_root /
            "worlds" /
            "4f524f5300000007" /
            "current.orosmanifestref";

    check(
        state,
        pointer_path_result.has_value() &&
            pointer_path_result.value() ==
                expected_pointer_path,
        "Current manifest pointer path is deterministic");

    const WorldCellRevisionId
        missing_revision{
            cell_c,
            999ULL
        };

    const auto missing_snapshot_contains =
        assigned_store.contains_snapshot(
            missing_revision);

    check(
        state,
        missing_snapshot_contains.has_value() &&
            !missing_snapshot_contains.value(),
        "Missing snapshot is not contained");

    check_failure(
        state,
        assigned_store.load_snapshot(
            missing_revision),
        ErrorCode::not_found,
        "Loading a missing snapshot reports not found");

    const auto missing_manifest_contains =
        assigned_store.contains_manifest(
            world_namespace,
            999ULL);

    check(
        state,
        missing_manifest_contains.has_value() &&
            !missing_manifest_contains.value(),
        "Missing manifest is not contained");

    check_failure(
        state,
        assigned_store.load_manifest(
            world_namespace,
            999ULL),
        ErrorCode::not_found,
        "Loading a missing manifest reports not found");

    check_failure(
        state,
        assigned_store.load_current_manifest(
            world_namespace),
        ErrorCode::not_found,
        "Loading an unpublished current manifest reports not found");

    const std::array<std::byte, 5U>
        payload_a{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50}
        };

    const std::array<std::byte, 4U>
        payload_b{
            std::byte{0xAA},
            std::byte{0xBB},
            std::byte{0xCC},
            std::byte{0xDD}
        };

    const std::array<std::byte, 3U>
        payload_c{
            std::byte{0x01},
            std::byte{0x02},
            std::byte{0x03}
        };

    const auto snapshot_a_result =
        WorldCellSnapshot::create(
            cell_a,
            revision_a.revision,
            std::span<const std::byte>{
                payload_a
            });

    const auto snapshot_b_result =
        WorldCellSnapshot::create(
            cell_b,
            revision_b.revision,
            std::span<const std::byte>{
                payload_b
            });

    const auto snapshot_c_result =
        WorldCellSnapshot::create(
            cell_c,
            revision_c.revision,
            std::span<const std::byte>{
                payload_c
            });

    check(
        state,
        snapshot_a_result.has_value(),
        "First snapshot fixture is created");

    check(
        state,
        snapshot_b_result.has_value(),
        "Second snapshot fixture is created");

    check(
        state,
        snapshot_c_result.has_value(),
        "Third snapshot fixture is created");

    if (!snapshot_a_result.has_value() ||
        !snapshot_b_result.has_value() ||
        !snapshot_c_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshot snapshot_a =
        snapshot_a_result.value();

    const WorldCellSnapshot snapshot_b =
        snapshot_b_result.value();

    const WorldCellSnapshot snapshot_c =
        snapshot_c_result.value();

    const auto store_snapshot_a_result =
        assigned_store.store_snapshot(
            snapshot_a);

    check(
        state,
        store_snapshot_a_result.has_value(),
        "First snapshot is persisted");

    check(
        state,
        store_snapshot_a_result.has_value() &&
            store_snapshot_a_result.value() ==
                revision_a,
        "Snapshot persistence returns its revision identity");

    if (snapshot_path_result.has_value())
    {
        std::error_code
            regular_file_error{};

        check(
            state,
            std::filesystem::is_regular_file(
                snapshot_path_result.value(),
                regular_file_error) &&
                !regular_file_error,
            "Persisted snapshot is a regular file");
    }
    else
    {
        check(
            state,
            false,
            "Persisted snapshot is a regular file");
    }

    check(
        state,
        !has_temporary_files(
            store_root),
        "Snapshot publication leaves no temporary files");

    const auto contains_snapshot_a_result =
        assigned_store.contains_snapshot(
            revision_a);

    check(
        state,
        contains_snapshot_a_result.has_value() &&
            contains_snapshot_a_result.value(),
        "Store contains the persisted snapshot");

    const auto load_snapshot_a_result =
        assigned_store.load_snapshot(
            revision_a);

    check(
        state,
        load_snapshot_a_result.has_value(),
        "Persisted snapshot loads");

    check(
        state,
        load_snapshot_a_result.has_value() &&
            load_snapshot_a_result.value() ==
                snapshot_a,
        "Loaded snapshot exactly matches the original");

    const auto duplicate_snapshot_result =
        assigned_store.store_snapshot(
            snapshot_a);

    check(
        state,
        duplicate_snapshot_result.has_value() &&
            duplicate_snapshot_result.value() ==
                revision_a,
        "Persisting identical snapshot bytes is idempotent");

    const std::array<std::byte, 5U>
        conflicting_payload{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x51}
        };

    const auto conflicting_snapshot_result =
        WorldCellSnapshot::create(
            cell_a,
            revision_a.revision,
            std::span<const std::byte>{
                conflicting_payload
            });

    check(
        state,
        conflicting_snapshot_result.has_value(),
        "Conflicting snapshot fixture is created");

    if (conflicting_snapshot_result.has_value())
    {
        check_failure(
            state,
            assigned_store.store_snapshot(
                conflicting_snapshot_result.value()),
            ErrorCode::invalid_state,
            "Immutable snapshot path rejects different bytes");
    }
    else
    {
        check(
            state,
            false,
            "Immutable snapshot path rejects different bytes");
    }

    const auto store_snapshot_b_result =
        assigned_store.store_snapshot(
            snapshot_b);

    check(
        state,
        store_snapshot_b_result.has_value() &&
            store_snapshot_b_result.value() ==
                revision_b,
        "Second snapshot is persisted");

    const std::array<
        WorldCellRevisionId,
        2U>
        manifest_10_entries{
            revision_b,
            revision_a
        };

    const auto manifest_10_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            10ULL,
            std::span<
                const WorldCellRevisionId>{
                    manifest_10_entries
                });

    check(
        state,
        manifest_10_result.has_value(),
        "First manifest fixture is created");

    if (!manifest_10_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest
        manifest_10 =
            manifest_10_result.value();

    const auto store_manifest_10_status =
        assigned_store.store_manifest(
            manifest_10);

    check(
        state,
        store_manifest_10_status.has_value(),
        "First manifest is stored immutably");

    const auto contains_manifest_10_result =
        assigned_store.contains_manifest(
            world_namespace,
            10ULL);

    check(
        state,
        contains_manifest_10_result.has_value() &&
            contains_manifest_10_result.value(),
        "Store contains the persisted manifest");

    const auto load_manifest_10_result =
        assigned_store.load_manifest(
            world_namespace,
            10ULL);

    check(
        state,
        load_manifest_10_result.has_value(),
        "Persisted manifest loads");

    check(
        state,
        load_manifest_10_result.has_value() &&
            load_manifest_10_result.value() ==
                manifest_10,
        "Loaded manifest exactly matches the original");

    const auto duplicate_manifest_status =
        assigned_store.store_manifest(
            manifest_10);

    check(
        state,
        duplicate_manifest_status.has_value(),
        "Persisting identical manifest bytes is idempotent");

    const std::array<
        WorldCellRevisionId,
        2U>
        conflicting_manifest_entries{
            revision_a,
            WorldCellRevisionId{
                cell_b,
                999ULL
            }
        };

    const auto conflicting_manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            10ULL,
            std::span<
                const WorldCellRevisionId>{
                    conflicting_manifest_entries
                });

    check(
        state,
        conflicting_manifest_result.has_value(),
        "Conflicting manifest fixture is created");

    if (conflicting_manifest_result.has_value())
    {
        check_failure(
            state,
            assigned_store.store_manifest(
                conflicting_manifest_result.value()),
            ErrorCode::invalid_state,
            "Immutable manifest path rejects different bytes");
    }
    else
    {
        check(
            state,
            false,
            "Immutable manifest path rejects different bytes");
    }

    const std::array<
        WorldCellRevisionId,
        2U>
        missing_manifest_entries{
            revision_a,
            missing_revision
        };

    const auto missing_reference_manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            std::span<
                const WorldCellRevisionId>{
                    missing_manifest_entries
                });

    check(
        state,
        missing_reference_manifest_result.
            has_value(),
        "Missing-reference manifest fixture is created");

    if (missing_reference_manifest_result.
            has_value())
    {
        check_failure(
            state,
            assigned_store.publish_manifest(
                missing_reference_manifest_result.
                    value()),
            ErrorCode::not_found,
            "Manifest publication rejects a missing snapshot");
    }
    else
    {
        check(
            state,
            false,
            "Manifest publication rejects a missing snapshot");
    }

    check_failure(
        state,
        assigned_store.load_current_manifest(
            world_namespace),
        ErrorCode::not_found,
        "Failed publication does not create a current pointer");

    const auto publish_manifest_10_status =
        assigned_store.publish_manifest(
            manifest_10);

    check(
        state,
        publish_manifest_10_status.has_value(),
        "First complete manifest is published");

    if (pointer_path_result.has_value())
    {
        std::error_code
            regular_file_error{};

        check(
            state,
            std::filesystem::is_regular_file(
                pointer_path_result.value(),
                regular_file_error) &&
                !regular_file_error,
            "Published current pointer is a regular file");
    }
    else
    {
        check(
            state,
            false,
            "Published current pointer is a regular file");
    }

    const auto current_manifest_10_result =
        assigned_store.load_current_manifest(
            world_namespace);

    check(
        state,
        current_manifest_10_result.has_value(),
        "Current manifest loads after publication");

    check(
        state,
        current_manifest_10_result.has_value() &&
            current_manifest_10_result.value() ==
                manifest_10,
        "Current pointer resolves to the published manifest");

    const auto store_snapshot_c_result =
        assigned_store.store_snapshot(
            snapshot_c);

    check(
        state,
        store_snapshot_c_result.has_value() &&
            store_snapshot_c_result.value() ==
                revision_c,
        "Third snapshot is persisted");

    const std::array<
        WorldCellRevisionId,
        3U>
        manifest_11_entries{
            revision_c,
            revision_a,
            revision_b
        };

    const auto manifest_11_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            11ULL,
            std::span<
                const WorldCellRevisionId>{
                    manifest_11_entries
                });

    check(
        state,
        manifest_11_result.has_value(),
        "Second complete manifest fixture is created");

    if (!manifest_11_result.has_value())
    {
        return finish(state);
    }

    const WorldCellSnapshotManifest
        manifest_11 =
            manifest_11_result.value();

    const auto publish_manifest_11_status =
        assigned_store.publish_manifest(
            manifest_11);

    check(
        state,
        publish_manifest_11_status.has_value(),
        "Second complete manifest is published");

    const auto current_manifest_11_result =
        assigned_store.load_current_manifest(
            world_namespace);

    check(
        state,
        current_manifest_11_result.has_value() &&
            current_manifest_11_result.value() ==
                manifest_11,
        "Atomic publication advances the current manifest");

    const auto old_manifest_result =
        assigned_store.load_manifest(
            world_namespace,
            10ULL);

    check(
        state,
        old_manifest_result.has_value() &&
            old_manifest_result.value() ==
                manifest_10,
        "Previous immutable manifest remains available");

    check(
        state,
        !has_temporary_files(
            store_root),
        "Manifest publication leaves no temporary files");

    const auto empty_manifest_result =
        WorldCellSnapshotManifest::create(
            world_namespace,
            12ULL,
            std::span<
                const WorldCellRevisionId>{});

    check(
        state,
        empty_manifest_result.has_value(),
        "Empty manifest fixture is created");

    if (empty_manifest_result.has_value())
    {
        const auto publish_empty_status =
            assigned_store.publish_manifest(
                empty_manifest_result.value());

        check(
            state,
            publish_empty_status.has_value(),
            "Empty manifest can be published");

        const auto current_empty_result =
            assigned_store.load_current_manifest(
                world_namespace);

        check(
            state,
            current_empty_result.has_value() &&
                current_empty_result.value() ==
                    empty_manifest_result.value(),
            "Current pointer resolves to an empty manifest");
    }
    else
    {
        check(
            state,
            false,
            "Empty manifest can be published");

        check(
            state,
            false,
            "Current pointer resolves to an empty manifest");
    }

    const auto other_world_manifest_result =
        WorldCellSnapshotManifest::create(
            other_world_namespace,
            1ULL,
            std::span<
                const WorldCellRevisionId>{});

    check(
        state,
        other_world_manifest_result.has_value(),
        "Second-world manifest fixture is created");

    if (other_world_manifest_result.has_value())
    {
        const auto publish_other_world_status =
            assigned_store.publish_manifest(
                other_world_manifest_result.value());

        check(
            state,
            publish_other_world_status.has_value(),
            "Second world publishes independently");

        const auto other_world_current_result =
            assigned_store.load_current_manifest(
                other_world_namespace);

        check(
            state,
            other_world_current_result.has_value() &&
                other_world_current_result.value() ==
                    other_world_manifest_result.value(),
            "Second world resolves its own current manifest");

        const auto first_world_current_result =
            assigned_store.load_current_manifest(
                world_namespace);

        check(
            state,
            first_world_current_result.has_value() &&
                first_world_current_result.value().
                    manifest_revision() ==
                12ULL,
            "Second-world publication does not alter the first world");
    }
    else
    {
        check(
            state,
            false,
            "Second world publishes independently");

        check(
            state,
            false,
            "Second world resolves its own current manifest");

        check(
            state,
            false,
            "Second-world publication does not alter the first world");
    }

    WorldCellPersistenceStore
        limited_store{
            temporary_directory.path() /
                "limited-store",
            WorldCellPersistenceLimits{
                4ULL,
                1ULL
            }
        };

    check(
        state,
        limited_store.is_valid(),
        "Store with small non-zero limits is valid");

    check_failure(
        state,
        limited_store.store_snapshot(
            snapshot_a),
        ErrorCode::invalid_argument,
        "Snapshot exceeding payload limit is rejected");

    check_failure(
        state,
        limited_store.store_manifest(
            manifest_10),
        ErrorCode::invalid_argument,
        "Manifest exceeding entry limit is rejected");

    WorldCellPersistenceStore
        mismatch_store{
            temporary_directory.path() /
                "identity-mismatch-store"
        };

    const auto mismatch_store_status =
        mismatch_store.store_snapshot(
            snapshot_a);

    check(
        state,
        mismatch_store_status.has_value(),
        "Identity-mismatch fixture snapshot is stored");

    const WorldCellRevisionId
        mismatched_revision{
            cell_b,
            44ULL
        };

    const auto mismatch_source_path =
        mismatch_store.snapshot_path_for(
            revision_a);

    const auto mismatch_target_path =
        mismatch_store.snapshot_path_for(
            mismatched_revision);

    bool copied_to_mismatched_path{};

    if (mismatch_source_path.has_value() &&
        mismatch_target_path.has_value())
    {
        copied_to_mismatched_path =
            copy_file_to(
                mismatch_source_path.value(),
                mismatch_target_path.value());
    }

    check(
        state,
        copied_to_mismatched_path,
        "Snapshot bytes are copied to a mismatched path");

    check_failure(
        state,
        mismatch_store.load_snapshot(
            mismatched_revision),
        ErrorCode::invalid_state,
        "Snapshot identity must match its storage path");

    WorldCellPersistenceStore
        corrupted_snapshot_store{
            temporary_directory.path() /
                "corrupted-snapshot-store"
        };

    const auto corrupted_snapshot_store_status =
        corrupted_snapshot_store.store_snapshot(
            snapshot_a);

    check(
        state,
        corrupted_snapshot_store_status.has_value(),
        "Corruption-test snapshot is stored");

    const auto corrupted_snapshot_path =
        corrupted_snapshot_store.snapshot_path_for(
            revision_a);

    bool snapshot_corrupted{};

    if (corrupted_snapshot_path.has_value())
    {
        snapshot_corrupted =
            corrupt_byte(
                corrupted_snapshot_path.value(),
                76U);
    }

    check(
        state,
        snapshot_corrupted,
        "Persisted snapshot payload is corrupted");

    check_failure(
        state,
        corrupted_snapshot_store.load_snapshot(
            revision_a),
        ErrorCode::invalid_argument,
        "Snapshot checksum detects corruption");

    check_failure(
        state,
        corrupted_snapshot_store.contains_snapshot(
            revision_a),
        ErrorCode::invalid_argument,
        "Snapshot containment validates file integrity");

    WorldCellPersistenceStore
        corrupted_manifest_store{
            temporary_directory.path() /
                "corrupted-manifest-store"
        };

    const auto corruption_snapshot_a_status =
        corrupted_manifest_store.store_snapshot(
            snapshot_a);

    const auto corruption_snapshot_b_status =
        corrupted_manifest_store.store_snapshot(
            snapshot_b);

    check(
        state,
        corruption_snapshot_a_status.has_value() &&
            corruption_snapshot_b_status.has_value(),
        "Manifest-corruption snapshots are stored");

    const auto corruption_manifest_status =
        corrupted_manifest_store.publish_manifest(
            manifest_10);

    check(
        state,
        corruption_manifest_status.has_value(),
        "Manifest-corruption fixture is published");

    const auto corrupted_manifest_path =
        corrupted_manifest_store.manifest_path_for(
            world_namespace,
            10ULL);

    bool manifest_corrupted{};

    if (corrupted_manifest_path.has_value())
    {
        manifest_corrupted =
            corrupt_byte(
                corrupted_manifest_path.value(),
                111U);
    }

    check(
        state,
        manifest_corrupted,
        "Persisted manifest entry is corrupted");

    check_failure(
        state,
        corrupted_manifest_store.load_manifest(
            world_namespace,
            10ULL),
        ErrorCode::invalid_argument,
        "Manifest checksum detects corruption");

    check_failure(
        state,
        corrupted_manifest_store.contains_manifest(
            world_namespace,
            10ULL),
        ErrorCode::invalid_argument,
        "Manifest containment validates file integrity");

    check_failure(
        state,
        corrupted_manifest_store.
            load_current_manifest(
                world_namespace),
        ErrorCode::invalid_argument,
        "Current manifest loading detects manifest corruption");

    WorldCellPersistenceStore
        corrupted_pointer_store{
            temporary_directory.path() /
                "corrupted-pointer-store"
        };

    const auto pointer_snapshot_a_status =
        corrupted_pointer_store.store_snapshot(
            snapshot_a);

    const auto pointer_snapshot_b_status =
        corrupted_pointer_store.store_snapshot(
            snapshot_b);

    check(
        state,
        pointer_snapshot_a_status.has_value() &&
            pointer_snapshot_b_status.has_value(),
        "Pointer-corruption snapshots are stored");

    const auto pointer_manifest_status =
        corrupted_pointer_store.publish_manifest(
            manifest_10);

    check(
        state,
        pointer_manifest_status.has_value(),
        "Pointer-corruption fixture is published");

    const auto corrupted_pointer_path =
        corrupted_pointer_store.
            current_manifest_pointer_path_for(
                world_namespace);

    bool pointer_corrupted{};

    if (corrupted_pointer_path.has_value())
    {
        pointer_corrupted =
            corrupt_byte(
                corrupted_pointer_path.value(),
                39U);
    }

    check(
        state,
        pointer_corrupted,
        "Current manifest pointer checksum is corrupted");

    check_failure(
        state,
        corrupted_pointer_store.
            load_current_manifest(
                world_namespace),
        ErrorCode::invalid_argument,
        "Manifest pointer checksum detects corruption");

    check(
        state,
        !has_temporary_files(
            temporary_directory.path()),
        "Persistence lifecycle leaves no temporary files");

    return finish(state);
}