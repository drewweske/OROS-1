#include "oros/streaming/world_cell_io_service.hpp"

#include "world_cell_io_shared_state.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <thread>
#include <utility>
#include <variant>

namespace oros::streaming
{
    namespace
    {
        void execute_request(
            WorldCellPersistenceStore& store,
            const std::shared_ptr<
                detail::WorldCellIoSharedState>&
                request)
        {
            try
            {
                switch (request->kind())
                {
                case WorldCellIoRequestKind::
                    load_snapshot:
                {
                    const auto* operation =
                        std::get_if<
                            detail::
                                WorldCellSnapshotLoadOperation>(
                                    &request->
                                        operation());

                    if (operation == nullptr)
                    {
                        request->complete_failure(
                            foundation::Error{
                                foundation::ErrorCode::
                                    internal_failure,
                                "The asynchronous snapshot "
                                "load request contains the "
                                "wrong operation type."
                            });

                        return;
                    }

                    foundation::Result<
                        WorldCellSnapshot>
                        result =
                            store.load_snapshot(
                                operation->
                                    revision_id);

                    if (!result.has_value())
                    {
                        request->complete_failure(
                            std::move(
                                result.error()));

                        return;
                    }

                    WorldCellSnapshot snapshot =
                        std::move(
                            result.value());

                    request->complete_success(
                        WorldCellIoResultValue{
                            std::in_place_type<
                                WorldCellSnapshot>,
                            std::move(
                                snapshot)
                        });

                    return;
                }

                case WorldCellIoRequestKind::
                    load_manifest:
                {
                    const auto* operation =
                        std::get_if<
                            detail::
                                WorldCellManifestLoadOperation>(
                                    &request->
                                        operation());

                    if (operation == nullptr)
                    {
                        request->complete_failure(
                            foundation::Error{
                                foundation::ErrorCode::
                                    internal_failure,
                                "The asynchronous manifest "
                                "load request contains the "
                                "wrong operation type."
                            });

                        return;
                    }

                    foundation::Result<
                        WorldCellSnapshotManifest>
                        result =
                            store.load_manifest(
                                operation->
                                    world_namespace,
                                operation->
                                    manifest_revision);

                    if (!result.has_value())
                    {
                        request->complete_failure(
                            std::move(
                                result.error()));

                        return;
                    }

                    WorldCellSnapshotManifest manifest =
                        std::move(
                            result.value());

                    request->complete_success(
                        WorldCellIoResultValue{
                            std::in_place_type<
                                WorldCellSnapshotManifest>,
                            std::move(
                                manifest)
                        });

                    return;
                }

                case WorldCellIoRequestKind::
                    load_current_manifest:
                {
                    const auto* operation =
                        std::get_if<
                            detail::
                                WorldCellCurrentManifestLoadOperation>(
                                    &request->
                                        operation());

                    if (operation == nullptr)
                    {
                        request->complete_failure(
                            foundation::Error{
                                foundation::ErrorCode::
                                    internal_failure,
                                "The asynchronous current "
                                "manifest load request "
                                "contains the wrong "
                                "operation type."
                            });

                        return;
                    }

                    foundation::Result<
                        WorldCellSnapshotManifest>
                        result =
                            store.load_current_manifest(
                                operation->
                                    world_namespace);

                    if (!result.has_value())
                    {
                        request->complete_failure(
                            std::move(
                                result.error()));

                        return;
                    }

                    WorldCellSnapshotManifest manifest =
                        std::move(
                            result.value());

                    request->complete_success(
                        WorldCellIoResultValue{
                            std::in_place_type<
                                WorldCellSnapshotManifest>,
                            std::move(
                                manifest)
                        });

                    return;
                }

                case WorldCellIoRequestKind::invalid:
                    request->complete_failure(
                        foundation::Error{
                            foundation::ErrorCode::
                                internal_failure,
                            "The asynchronous world cell "
                            "I/O service received an "
                            "invalid request kind."
                        });

                    return;
                }

                request->complete_failure(
                    foundation::Error{
                        foundation::ErrorCode::
                            internal_failure,
                        "The asynchronous world cell "
                        "I/O service received an "
                        "unknown request kind."
                    });
            }
            catch (const std::bad_alloc&)
            {
                request->complete_failure(
                    foundation::Error{
                        foundation::ErrorCode::
                            out_of_memory,
                        "Unable to allocate storage "
                        "while executing an "
                        "asynchronous world cell I/O "
                        "request."
                    });
            }
            catch (...)
            {
                request->complete_failure(
                    foundation::Error{
                        foundation::ErrorCode::
                            internal_failure,
                        "An unexpected failure occurred "
                        "while executing an "
                        "asynchronous world cell I/O "
                        "request."
                    });
            }
        }
    }

    class WorldCellIoService::Implementation final
    {
    public:
        Implementation(
            WorldCellPersistenceStore store,
            const WorldCellIoServiceLimits
                limits)
            : store_{
                  std::move(
                      store)
              },
              limits_{
                  limits
              }
        {
            valid_ =
                store_.is_valid() &&
                limits_.is_valid();

            if (!valid_)
            {
                return;
            }

            accepting_requests_ =
                true;

            worker_ =
                std::jthread{
                    [this]()
                    {
                        worker_loop();
                    }
                };
        }

        ~Implementation()
        {
            shutdown();
        }

        Implementation(
            const Implementation&) =
                delete;

        Implementation&
        operator=(
            const Implementation&) =
                delete;

        Implementation(
            Implementation&&) =
                delete;

        Implementation&
        operator=(
            Implementation&&) =
                delete;

        [[nodiscard]]
        bool is_valid() const noexcept
        {
            return valid_;
        }

        [[nodiscard]]
        bool is_accepting_requests()
            const noexcept
        {
            if (!valid_)
            {
                return false;
            }

            try
            {
                const std::scoped_lock lock{
                    mutex_
                };

                return accepting_requests_;
            }
            catch (...)
            {
                return false;
            }
        }

        [[nodiscard]]
        std::size_t
        queued_request_count() const noexcept
        {
            if (!valid_)
            {
                return 0U;
            }

            try
            {
                const std::scoped_lock lock{
                    mutex_
                };

                return queue_.size();
            }
            catch (...)
            {
                return 0U;
            }
        }

        [[nodiscard]]
        foundation::Result<
            std::shared_ptr<
                detail::WorldCellIoSharedState>>
        enqueue(
            const WorldCellIoRequestKind kind,
            detail::WorldCellIoOperation
                operation) noexcept
        {
            if (!valid_)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        invalid_state,
                    "Cannot enqueue work through an "
                    "invalid asynchronous world cell "
                    "I/O service.");
            }

            try
            {
                std::unique_lock lock{
                    mutex_
                };

                if (!accepting_requests_)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The asynchronous world cell "
                        "I/O service is no longer "
                        "accepting requests.");
                }

                if (queue_.size() >=
                    limits_.
                        maximum_queued_request_count)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The asynchronous world cell "
                        "I/O request queue has reached "
                        "its configured capacity.");
                }

                if (next_request_id_ == 0ULL)
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_state,
                        "The asynchronous world cell "
                        "I/O request identifier space "
                        "has been exhausted.");
                }

                const std::uint64_t request_id =
                    next_request_id_;

                if (next_request_id_ ==
                    std::numeric_limits<
                        std::uint64_t>::max())
                {
                    next_request_id_ =
                        0ULL;
                }
                else
                {
                    ++next_request_id_;
                }

                std::shared_ptr<
                    detail::WorldCellIoSharedState>
                    request =
                        std::make_shared<
                            detail::
                                WorldCellIoSharedState>(
                                    request_id,
                                    kind,
                                    std::move(
                                        operation));

                if (!request->is_valid())
                {
                    return foundation::fail(
                        foundation::ErrorCode::
                            invalid_argument,
                        "Cannot enqueue an invalid "
                        "asynchronous world cell I/O "
                        "operation.");
                }

                queue_.push_back(
                    request);

                lock.unlock();

                work_condition_.notify_one();

                return request;
            }
            catch (const std::bad_alloc&)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        out_of_memory,
                    "Unable to allocate storage for "
                    "an asynchronous world cell I/O "
                    "request.");
            }
            catch (...)
            {
                return foundation::fail(
                    foundation::ErrorCode::
                        internal_failure,
                    "An unexpected failure occurred "
                    "while enqueueing an asynchronous "
                    "world cell I/O request.");
            }
        }

        void shutdown() noexcept
        {
            try
            {
                const std::scoped_lock
                    shutdown_lock{
                        shutdown_mutex_
                    };

                {
                    const std::scoped_lock lock{
                        mutex_
                    };

                    accepting_requests_ =
                        false;

                    stopping_ =
                        true;

                    for (const auto& request :
                         queue_)
                    {
                        request->
                            cancel_for_shutdown();
                    }

                    queue_.clear();

                    if (active_request_ != nullptr)
                    {
                        active_request_->
                            cancel_for_shutdown();
                    }
                }

                work_condition_.notify_all();

                if (worker_.joinable())
                {
                    worker_.join();
                }
            }
            catch (...)
            {
            }
        }

    private:
        void worker_loop() noexcept
        {
            for (;;)
            {
                std::shared_ptr<
                    detail::WorldCellIoSharedState>
                    request{};

                try
                {
                    {
                        std::unique_lock lock{
                            mutex_
                        };

                        work_condition_.wait(
                            lock,
                            [this]()
                            {
                                return
                                    stopping_ ||
                                    !queue_.empty();
                            });

                        if (stopping_ &&
                            queue_.empty())
                        {
                            return;
                        }

                        request =
                            std::move(
                                queue_.front());

                        queue_.pop_front();

                        active_request_ =
                            request;
                    }

                    if (request != nullptr &&
                        request->begin_running())
                    {
                        execute_request(
                            store_,
                            request);
                    }

                    {
                        const std::scoped_lock lock{
                            mutex_
                        };

                        if (active_request_ ==
                            request)
                        {
                            active_request_.reset();
                        }
                    }
                }
                catch (...)
                {
                    if (request != nullptr)
                    {
                        try
                        {
                            request->complete_failure(
                                foundation::Error{
                                    foundation::
                                        ErrorCode::
                                            internal_failure,
                                    "The asynchronous world "
                                    "cell I/O worker "
                                    "encountered an "
                                    "unexpected failure."
                                });
                        }
                        catch (...)
                        {
                        }
                    }

                    try
                    {
                        const std::scoped_lock lock{
                            mutex_
                        };

                        if (active_request_ ==
                            request)
                        {
                            active_request_.reset();
                        }
                    }
                    catch (...)
                    {
                        return;
                    }
                }
            }
        }

        WorldCellPersistenceStore
            store_;

        WorldCellIoServiceLimits
            limits_{};

        bool valid_{};

        mutable std::mutex
            mutex_{};

        std::condition_variable
            work_condition_{};

        std::deque<
            std::shared_ptr<
                detail::WorldCellIoSharedState>>
            queue_{};

        std::shared_ptr<
            detail::WorldCellIoSharedState>
            active_request_{};

        std::uint64_t
            next_request_id_{
                1ULL
            };

        bool accepting_requests_{};

        bool stopping_{};

        std::jthread
            worker_{};

        std::mutex
            shutdown_mutex_{};
    };

    WorldCellIoService::
    WorldCellIoService(
        WorldCellPersistenceStore store,
        const WorldCellIoServiceLimits limits)
        : implementation_{
              std::make_unique<Implementation>(
                  std::move(
                      store),
                  limits)
          },
          limits_{
              limits
          }
    {
    }

    WorldCellIoService::~WorldCellIoService()
    {
        shutdown();
    }

    bool
    WorldCellIoService::is_valid()
        const noexcept
    {
        return
            implementation_ != nullptr &&
            implementation_->is_valid();
    }

    bool
    WorldCellIoService::
    is_accepting_requests() const noexcept
    {
        return
            implementation_ != nullptr &&
            implementation_->
                is_accepting_requests();
    }

    const WorldCellIoServiceLimits&
    WorldCellIoService::limits()
        const noexcept
    {
        return limits_;
    }

    std::size_t
    WorldCellIoService::
    queued_request_count() const noexcept
    {
        if (implementation_ == nullptr)
        {
            return 0U;
        }

        return
            implementation_->
                queued_request_count();
    }

    foundation::Result<
        WorldCellIoRequest>
    WorldCellIoService::
    request_snapshot_load(
        const WorldCellRevisionId&
            revision_id) noexcept
    {
        if (!revision_id.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot request an asynchronous load "
                "for an invalid world cell revision "
                "identity.");
        }

        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "service has no implementation.");
        }

        foundation::Result<
            std::shared_ptr<
                detail::WorldCellIoSharedState>>
            request_result =
                implementation_->enqueue(
                    WorldCellIoRequestKind::
                        load_snapshot,
                    detail::
                        WorldCellSnapshotLoadOperation{
                            revision_id
                        });

        if (!request_result.has_value())
        {
            return foundation::fail(
                request_result.error().code,
                request_result.error().message);
        }

        return WorldCellIoRequest{
            std::move(
                request_result.value())
        };
    }

    foundation::Result<
        WorldCellIoRequest>
    WorldCellIoService::
    request_manifest_load(
        const std::uint64_t world_namespace,
        const std::uint64_t manifest_revision)
        noexcept
    {
        if (world_namespace == 0ULL ||
            manifest_revision == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "World namespace and manifest "
                "revision must both be non-zero for "
                "an asynchronous manifest load.");
        }

        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "service has no implementation.");
        }

        foundation::Result<
            std::shared_ptr<
                detail::WorldCellIoSharedState>>
            request_result =
                implementation_->enqueue(
                    WorldCellIoRequestKind::
                        load_manifest,
                    detail::
                        WorldCellManifestLoadOperation{
                            world_namespace,
                            manifest_revision
                        });

        if (!request_result.has_value())
        {
            return foundation::fail(
                request_result.error().code,
                request_result.error().message);
        }

        return WorldCellIoRequest{
            std::move(
                request_result.value())
        };
    }

    foundation::Result<
        WorldCellIoRequest>
    WorldCellIoService::
    request_current_manifest_load(
        const std::uint64_t world_namespace)
        noexcept
    {
        if (world_namespace == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The world namespace must be "
                "non-zero for an asynchronous "
                "current manifest load.");
        }

        if (implementation_ == nullptr)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "The asynchronous world cell I/O "
                "service has no implementation.");
        }

        foundation::Result<
            std::shared_ptr<
                detail::WorldCellIoSharedState>>
            request_result =
                implementation_->enqueue(
                    WorldCellIoRequestKind::
                        load_current_manifest,
                    detail::
                        WorldCellCurrentManifestLoadOperation{
                            world_namespace
                        });

        if (!request_result.has_value())
        {
            return foundation::fail(
                request_result.error().code,
                request_result.error().message);
        }

        return WorldCellIoRequest{
            std::move(
                request_result.value())
        };
    }

    void
    WorldCellIoService::shutdown()
        noexcept
    {
        if (implementation_ != nullptr)
        {
            implementation_->shutdown();
        }
    }
}