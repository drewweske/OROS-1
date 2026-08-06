#pragma once

#include "oros/foundation/result.hpp"
#include "oros/streaming/world_cell_io_request.hpp"
#include "oros/streaming/world_cell_persistence_store.hpp"
#include "oros/streaming/world_cell_revision_id.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

namespace oros::streaming
{
    struct WorldCellIoServiceLimits final
    {
        std::size_t
            maximum_queued_request_count{
                256U
            };

        [[nodiscard]]
        constexpr bool is_valid() const noexcept
        {
            return
                maximum_queued_request_count >
                    0U;
        }
    };

    class WorldCellIoService final
    {
    public:
        explicit WorldCellIoService(
            WorldCellPersistenceStore store,
            WorldCellIoServiceLimits limits = {});

        ~WorldCellIoService();

        WorldCellIoService(
            const WorldCellIoService&) =
                delete;

        WorldCellIoService&
        operator=(
            const WorldCellIoService&) =
                delete;

        WorldCellIoService(
            WorldCellIoService&&) =
                delete;

        WorldCellIoService&
        operator=(
            WorldCellIoService&&) =
                delete;

        [[nodiscard]]
        bool is_valid() const noexcept;

        [[nodiscard]]
        bool is_accepting_requests()
            const noexcept;

        [[nodiscard]]
        const WorldCellIoServiceLimits&
        limits() const noexcept;

        [[nodiscard]]
        std::size_t
        queued_request_count() const noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellIoRequest>
        request_snapshot_load(
            const WorldCellRevisionId&
                revision_id) noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellIoRequest>
        request_manifest_load(
            std::uint64_t world_namespace,
            std::uint64_t manifest_revision)
            noexcept;

        [[nodiscard]]
        foundation::Result<
            WorldCellIoRequest>
        request_current_manifest_load(
            std::uint64_t world_namespace)
            noexcept;

        void shutdown() noexcept;

    private:
        class Implementation;

        std::unique_ptr<Implementation>
            implementation_{};

        WorldCellIoServiceLimits
            limits_{};
    };
}