#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_snapshot.hpp"
#include "oros/streaming/world_cell_snapshot_manifest.hpp"

#include <cstdint>
#include <memory>
#include <variant>

namespace oros::streaming
{
    enum class WorldCellIoRequestKind :
        std::uint8_t
    {
        invalid = 0,
        load_snapshot,
        load_manifest,
        load_current_manifest
    };

    enum class WorldCellIoRequestState :
        std::uint8_t
    {
        invalid = 0,
        queued,
        running,
        succeeded,
        failed,
        cancelled
    };

    using WorldCellIoResultValue =
        std::variant<
            WorldCellSnapshot,
            WorldCellSnapshotManifest>;

    [[nodiscard]]
    constexpr bool
    is_terminal(
        const WorldCellIoRequestState state)
        noexcept
    {
        return
            state ==
                WorldCellIoRequestState::
                    succeeded ||
            state ==
                WorldCellIoRequestState::
                    failed ||
            state ==
                WorldCellIoRequestState::
                    cancelled;
    }

    namespace detail
    {
        class WorldCellIoSharedState;
    }

    class WorldCellIoService;

    class WorldCellIoRequest final
    {
    public:
        WorldCellIoRequest() noexcept =
            default;

        ~WorldCellIoRequest();

        WorldCellIoRequest(
            const WorldCellIoRequest&) =
                delete;

        WorldCellIoRequest&
        operator=(
            const WorldCellIoRequest&) =
                delete;

        WorldCellIoRequest(
            WorldCellIoRequest&& other)
            noexcept;

        WorldCellIoRequest&
        operator=(
            WorldCellIoRequest&& other)
            noexcept;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        explicit operator bool() const noexcept;

        [[nodiscard]]
        std::uint64_t id() const noexcept;

        [[nodiscard]]
        WorldCellIoRequestKind
        kind() const noexcept;

        [[nodiscard]]
        WorldCellIoRequestState
        state() const noexcept;

        [[nodiscard]]
        bool is_terminal() const noexcept;

        [[nodiscard]]
        bool cancellation_requested()
            const noexcept;

        [[nodiscard]]
        bool cancel() noexcept;

        [[nodiscard]]
        foundation::Status
        wait() const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellIoResultValue>
        take_result() noexcept;

    private:
        explicit WorldCellIoRequest(
            std::shared_ptr<
                detail::WorldCellIoSharedState>
                shared_state) noexcept;

        std::shared_ptr<
            detail::WorldCellIoSharedState>
            shared_state_{};

        friend class WorldCellIoService;
    };
}