#include "oros/streaming/world_cell_io_request.hpp"

#include "world_cell_io_shared_state.hpp"

#include <new>
#include <utility>

namespace oros::streaming
{
    WorldCellIoRequest::~WorldCellIoRequest() =
        default;

    WorldCellIoRequest::
    WorldCellIoRequest(
        std::shared_ptr<
            detail::WorldCellIoSharedState>
            shared_state) noexcept
        : shared_state_{
              std::move(
                  shared_state)
          }
    {
    }

    WorldCellIoRequest::
    WorldCellIoRequest(
        WorldCellIoRequest&& other)
        noexcept
        : shared_state_{
              std::move(
                  other.shared_state_)
          }
    {
    }

    WorldCellIoRequest&
    WorldCellIoRequest::operator=(
        WorldCellIoRequest&& other)
        noexcept
    {
        if (this == &other)
        {
            return *this;
        }

        shared_state_ =
            std::move(
                other.shared_state_);

        return *this;
    }

    bool
    WorldCellIoRequest::is_valid()
        const noexcept
    {
        return
            shared_state_ != nullptr &&
            shared_state_->is_valid();
    }

    WorldCellIoRequest::
    operator bool() const noexcept
    {
        return is_valid();
    }

    std::uint64_t
    WorldCellIoRequest::id()
        const noexcept
    {
        if (!is_valid())
        {
            return 0ULL;
        }

        return shared_state_->id();
    }

    WorldCellIoRequestKind
    WorldCellIoRequest::kind()
        const noexcept
    {
        if (!is_valid())
        {
            return
                WorldCellIoRequestKind::
                    invalid;
        }

        return shared_state_->kind();
    }

    WorldCellIoRequestState
    WorldCellIoRequest::state()
        const noexcept
    {
        if (!is_valid())
        {
            return
                WorldCellIoRequestState::
                    invalid;
        }

        try
        {
            return shared_state_->state();
        }
        catch (...)
        {
            return
                WorldCellIoRequestState::
                    invalid;
        }
    }

    bool
    WorldCellIoRequest::is_terminal()
        const noexcept
    {
        if (!is_valid())
        {
            return false;
        }

        try
        {
            return
                shared_state_->
                    is_terminal();
        }
        catch (...)
        {
            return false;
        }
    }

    bool
    WorldCellIoRequest::
    cancellation_requested() const noexcept
    {
        if (!is_valid())
        {
            return false;
        }

        try
        {
            return
                shared_state_->
                    cancellation_requested();
        }
        catch (...)
        {
            return false;
        }
    }

    bool
    WorldCellIoRequest::cancel()
        noexcept
    {
        if (!is_valid())
        {
            return false;
        }

        try
        {
            return
                shared_state_->
                    request_cancellation();
        }
        catch (...)
        {
            return false;
        }
    }

    foundation::Status
    WorldCellIoRequest::wait()
        const noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot wait on an invalid "
                "asynchronous world cell I/O "
                "request handle.");
        }

        try
        {
            return
                shared_state_->
                    wait_until_terminal();
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate storage while "
                "waiting for an asynchronous world "
                "cell I/O request.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while waiting for an asynchronous "
                "world cell I/O request.");
        }
    }

    foundation::Result<
        WorldCellIoResultValue>
    WorldCellIoRequest::take_result()
        noexcept
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot take a result from an "
                "invalid asynchronous world cell "
                "I/O request handle.");
        }

        try
        {
            return
                shared_state_->
                    take_result();
        }
        catch (const std::bad_alloc&)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    out_of_memory,
                "Unable to allocate storage while "
                "taking an asynchronous world cell "
                "I/O result.");
        }
        catch (...)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    internal_failure,
                "An unexpected failure occurred "
                "while taking an asynchronous world "
                "cell I/O result.");
        }
    }
}