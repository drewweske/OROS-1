#pragma once

#include "oros/foundation/error.hpp"
#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_io_request.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <optional>
#include <variant>

namespace oros::streaming::detail
{
    struct WorldCellSnapshotLoadOperation final
    {
        WorldCellRevisionId revision_id{};
    };

    struct WorldCellManifestLoadOperation final
    {
        std::uint64_t world_namespace{};
        std::uint64_t manifest_revision{};
    };

    struct WorldCellCurrentManifestLoadOperation final
    {
        std::uint64_t world_namespace{};
    };

    using WorldCellIoOperation =
        std::variant<
            WorldCellSnapshotLoadOperation,
            WorldCellManifestLoadOperation,
            WorldCellCurrentManifestLoadOperation>;

    class WorldCellIoSharedState final
    {
    public:
        WorldCellIoSharedState(
            std::uint64_t id,
            WorldCellIoRequestKind kind,
            WorldCellIoOperation operation);

        ~WorldCellIoSharedState() = default;

        WorldCellIoSharedState(
            const WorldCellIoSharedState&) =
                delete;

        WorldCellIoSharedState&
        operator=(
            const WorldCellIoSharedState&) =
                delete;

        WorldCellIoSharedState(
            WorldCellIoSharedState&&) =
                delete;

        WorldCellIoSharedState&
        operator=(
            WorldCellIoSharedState&&) =
                delete;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        std::uint64_t id() const noexcept;

        [[nodiscard]]
        WorldCellIoRequestKind
        kind() const noexcept;

        [[nodiscard]]
        const WorldCellIoOperation&
        operation() const noexcept;

        [[nodiscard]]
        WorldCellIoRequestState
        state() const;

        [[nodiscard]]
        bool is_terminal() const;

        [[nodiscard]]
        bool cancellation_requested() const;

        [[nodiscard]]
        bool request_cancellation();

        [[nodiscard]]
        bool begin_running();

        void complete_success(
            WorldCellIoResultValue value);

        void complete_failure(
            foundation::Error error);

        void cancel_for_shutdown();

        [[nodiscard]]
        foundation::Status
        wait_until_terminal() const;

        [[nodiscard]]
        foundation::Result<
            WorldCellIoResultValue>
        take_result();

    private:
        std::uint64_t id_{};

        WorldCellIoRequestKind
            kind_{
                WorldCellIoRequestKind::
                    invalid
            };

        WorldCellIoOperation
            operation_{};

        mutable std::mutex
            mutex_{};

        mutable std::condition_variable
            completion_condition_{};

        WorldCellIoRequestState
            state_{
                WorldCellIoRequestState::
                    invalid
            };

        bool cancellation_requested_{};

        bool result_taken_{};

        std::optional<
            WorldCellIoResultValue>
            result_{};

        foundation::Error
            failure_{};
    };
}