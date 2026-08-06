#include "world_cell_io_shared_state.hpp"

#include <mutex>
#include <string>
#include <utility>
#include <variant>

namespace oros::streaming::detail
{
    namespace
    {
        [[nodiscard]]
        bool operation_matches_kind(
            const WorldCellIoRequestKind kind,
            const WorldCellIoOperation&
                operation) noexcept
        {
            switch (kind)
            {
            case WorldCellIoRequestKind::
                load_snapshot:
            {
                const auto* snapshot_operation =
                    std::get_if<
                        WorldCellSnapshotLoadOperation>(
                            &operation);

                return
                    snapshot_operation != nullptr &&
                    snapshot_operation->
                        revision_id.is_valid();
            }

            case WorldCellIoRequestKind::
                load_manifest:
            {
                const auto* manifest_operation =
                    std::get_if<
                        WorldCellManifestLoadOperation>(
                            &operation);

                return
                    manifest_operation != nullptr &&
                    manifest_operation->
                        world_namespace != 0ULL &&
                    manifest_operation->
                        manifest_revision != 0ULL;
            }

            case WorldCellIoRequestKind::
                load_current_manifest:
            {
                const auto* current_operation =
                    std::get_if<
                        WorldCellCurrentManifestLoadOperation>(
                            &operation);

                return
                    current_operation != nullptr &&
                    current_operation->
                        world_namespace != 0ULL;
            }

            case WorldCellIoRequestKind::invalid:
                return false;
            }

            return false;
        }

        [[nodiscard]]
        foundation::Error normalize_failure(
            foundation::Error error)
        {
            if (error.code !=
                foundation::ErrorCode::none)
            {
                return error;
            }

            return foundation::Error{
                foundation::ErrorCode::
                    internal_failure,
                "The asynchronous world cell I/O "
                "operation failed without an error "
                "code."
            };
        }
    }

    WorldCellIoSharedState::
    WorldCellIoSharedState(
        const std::uint64_t id,
        const WorldCellIoRequestKind kind,
        WorldCellIoOperation operation)
        : id_{
              id
          },
          kind_{
              kind
          },
          operation_{
              std::move(
                  operation)
          }
    {
        if (id_ != 0ULL &&
            kind_ !=
                WorldCellIoRequestKind::invalid &&
            operation_matches_kind(
                kind_,
                operation_))
        {
            state_ =
                WorldCellIoRequestState::queued;
        }
    }

    bool
    WorldCellIoSharedState::is_valid()
        const noexcept
    {
        return
            id_ != 0ULL &&
            kind_ !=
                WorldCellIoRequestKind::invalid &&
            operation_matches_kind(
                kind_,
                operation_);
    }

    std::uint64_t
    WorldCellIoSharedState::id()
        const noexcept
    {
        return id_;
    }

    WorldCellIoRequestKind
    WorldCellIoSharedState::kind()
        const noexcept
    {
        return kind_;
    }

    const WorldCellIoOperation&
    WorldCellIoSharedState::operation()
        const noexcept
    {
        return operation_;
    }

    WorldCellIoRequestState
    WorldCellIoSharedState::state() const
    {
        const std::scoped_lock lock{
            mutex_
        };

        return state_;
    }

    bool
    WorldCellIoSharedState::is_terminal()
        const
    {
        const std::scoped_lock lock{
            mutex_
        };

        return
            oros::streaming::is_terminal(
                state_);
    }

    bool
    WorldCellIoSharedState::
    cancellation_requested() const
    {
        const std::scoped_lock lock{
            mutex_
        };

        return cancellation_requested_;
    }

    bool
    WorldCellIoSharedState::
    request_cancellation()
    {
        std::unique_lock lock{
            mutex_
        };

        if (state_ ==
                WorldCellIoRequestState::invalid ||
            oros::streaming::is_terminal(
                state_) ||
            cancellation_requested_)
        {
            return false;
        }

        cancellation_requested_ =
            true;

        if (state_ ==
            WorldCellIoRequestState::queued)
        {
            state_ =
                WorldCellIoRequestState::
                    cancelled;

            lock.unlock();

            completion_condition_.
                notify_all();
        }

        return true;
    }

    bool
    WorldCellIoSharedState::begin_running()
    {
        std::unique_lock lock{
            mutex_
        };

        if (state_ !=
            WorldCellIoRequestState::queued)
        {
            return false;
        }

        if (cancellation_requested_)
        {
            state_ =
                WorldCellIoRequestState::
                    cancelled;

            lock.unlock();

            completion_condition_.
                notify_all();

            return false;
        }

        state_ =
            WorldCellIoRequestState::running;

        return true;
    }

    void
    WorldCellIoSharedState::complete_success(
        WorldCellIoResultValue value)
    {
        std::unique_lock lock{
            mutex_
        };

        if (state_ !=
            WorldCellIoRequestState::running)
        {
            return;
        }

        if (cancellation_requested_)
        {
            result_.reset();

            failure_ =
                foundation::Error{};

            state_ =
                WorldCellIoRequestState::
                    cancelled;

            lock.unlock();

            completion_condition_.
                notify_all();

            return;
        }

        result_.emplace(
            std::move(
                value));

        failure_ =
            foundation::Error{};

        state_ =
            WorldCellIoRequestState::
                succeeded;

        lock.unlock();

        completion_condition_.
            notify_all();
    }

    void
    WorldCellIoSharedState::complete_failure(
        foundation::Error error)
    {
        std::unique_lock lock{
            mutex_
        };

        if (state_ !=
            WorldCellIoRequestState::running)
        {
            return;
        }

        result_.reset();

        if (cancellation_requested_)
        {
            failure_ =
                foundation::Error{};

            state_ =
                WorldCellIoRequestState::
                    cancelled;
        }
        else
        {
            failure_ =
                normalize_failure(
                    std::move(
                        error));

            state_ =
                WorldCellIoRequestState::
                    failed;
        }

        lock.unlock();

        completion_condition_.
            notify_all();
    }

    void
    WorldCellIoSharedState::
    cancel_for_shutdown()
    {
        std::unique_lock lock{
            mutex_
        };

        if (state_ ==
                WorldCellIoRequestState::invalid ||
            oros::streaming::is_terminal(
                state_))
        {
            return;
        }

        cancellation_requested_ =
            true;

        if (state_ ==
            WorldCellIoRequestState::queued)
        {
            state_ =
                WorldCellIoRequestState::
                    cancelled;

            lock.unlock();

            completion_condition_.
                notify_all();
        }
    }

    foundation::Status
    WorldCellIoSharedState::
    wait_until_terminal() const
    {
        std::unique_lock lock{
            mutex_
        };

        if (!is_valid() ||
            state_ ==
                WorldCellIoRequestState::invalid)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot wait on an invalid "
                "asynchronous world cell I/O "
                "request.");
        }

        completion_condition_.wait(
            lock,
            [this]()
            {
                return
                    oros::streaming::
                        is_terminal(
                            state_);
            });

        switch (state_)
        {
        case WorldCellIoRequestState::
            succeeded:
            return foundation::Status{};

        case WorldCellIoRequestState::
            failed:
            if (failure_.code ==
                foundation::ErrorCode::none)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asynchronous world cell I/O "
                    "request failed without a stored "
                    "error.");
            }

            return foundation::fail(
                failure_.code,
                failure_.message);

        case WorldCellIoRequestState::
            cancelled:
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "request was cancelled.");

        case WorldCellIoRequestState::invalid:
        case WorldCellIoRequestState::queued:
        case WorldCellIoRequestState::running:
            break;
        }

        return foundation::fail(
            foundation::ErrorCode::
                internal_failure,
            "The asynchronous world cell I/O "
            "request reached an unknown completion "
            "state.");
    }

    foundation::Result<
        WorldCellIoResultValue>
    WorldCellIoSharedState::take_result()
    {
        std::scoped_lock lock{
            mutex_
        };

        if (!is_valid() ||
            state_ ==
                WorldCellIoRequestState::invalid)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot take a result from an "
                "invalid asynchronous world cell "
                "I/O request.");
        }

        if (!oros::streaming::is_terminal(
                state_))
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot take an asynchronous world "
                "cell I/O result before the request "
                "is complete.");
        }

        if (result_taken_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "result has already been consumed.");
        }

        result_taken_ =
            true;

        if (state_ ==
            WorldCellIoRequestState::cancelled)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "request was cancelled.");
        }

        if (state_ ==
            WorldCellIoRequestState::failed)
        {
            if (failure_.code ==
                foundation::ErrorCode::none)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "The asynchronous world cell I/O "
                    "request failed without a stored "
                    "error.");
            }

            return foundation::fail(
                failure_.code,
                failure_.message);
        }

        if (state_ !=
                WorldCellIoRequestState::
                    succeeded ||
            !result_.has_value())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "The completed asynchronous world "
                "cell I/O request does not contain "
                "a result.");
        }

        WorldCellIoResultValue value =
            std::move(
                result_.value());

        result_.reset();

        return value;
    }
}