#pragma once

#include "oros/foundation/error.hpp"
#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_key.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>

namespace oros::streaming
{
    enum class WorldCellResidencyState :
        std::uint8_t
    {
        invalid = 0,
        unloaded,
        load_queued,
        loading,
        resident,
        unload_queued,
        unloading,
        failed
    };

    [[nodiscard]]
    constexpr bool
    is_residency_transitioning(
        const WorldCellResidencyState state)
        noexcept
    {
        return
            state ==
                WorldCellResidencyState::
                    load_queued ||
            state ==
                WorldCellResidencyState::
                    loading ||
            state ==
                WorldCellResidencyState::
                    unload_queued ||
            state ==
                WorldCellResidencyState::
                    unloading;
    }

    class WorldCellResidency final
    {
    public:
        [[nodiscard]]
        static foundation::Result<
            WorldCellResidency>
        create(
            WorldCellKey cell_key);

        WorldCellResidency(
            const WorldCellResidency&) =
                default;

        WorldCellResidency&
        operator=(
            const WorldCellResidency&) =
                default;

        WorldCellResidency(
            WorldCellResidency&&)
            noexcept = default;

        WorldCellResidency&
        operator=(
            WorldCellResidency&&)
            noexcept = default;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        const WorldCellKey&
        cell_key() const noexcept;

        [[nodiscard]]
        WorldCellResidencyState
        state() const noexcept;

        [[nodiscard]]
        const WorldCellRevisionId&
        target_revision() const noexcept;

        [[nodiscard]]
        const WorldCellRevisionId&
        resident_revision() const noexcept;

        [[nodiscard]]
        std::uint64_t
        active_request_id() const noexcept;

        [[nodiscard]]
        const foundation::Error&
        failure() const noexcept;

        [[nodiscard]]
        const WorldCellSnapshot*
        snapshot() const noexcept;

        [[nodiscard]]
        std::size_t
        resident_byte_count() const noexcept;

        [[nodiscard]]
        bool is_resident() const noexcept;

        [[nodiscard]]
        foundation::Status
        queue_load(
            WorldCellRevisionId target_revision,
            std::uint64_t request_id);

        [[nodiscard]]
        foundation::Status
        begin_load(
            std::uint64_t request_id);

        [[nodiscard]]
        foundation::Status
        complete_load(
            std::uint64_t request_id,
            WorldCellSnapshot snapshot);

        [[nodiscard]]
        foundation::Status
        fail_load(
            std::uint64_t request_id,
            foundation::Error error);

        [[nodiscard]]
        foundation::Status
        queue_unload();

        [[nodiscard]]
        foundation::Status
        begin_unload();

        [[nodiscard]]
        foundation::Status
        complete_unload();

        [[nodiscard]]
        foundation::Status
        clear_failure();

    private:
        explicit WorldCellResidency(
            WorldCellKey cell_key)
            noexcept;

        WorldCellKey
            cell_key_{};

        WorldCellResidencyState
            state_{
                WorldCellResidencyState::
                    invalid
            };

        WorldCellRevisionId
            target_revision_{};

        WorldCellRevisionId
            resident_revision_{};

        std::uint64_t
            active_request_id_{};

        foundation::Error
            failure_{};

        std::optional<
            WorldCellSnapshot>
            snapshot_{};
    };
}