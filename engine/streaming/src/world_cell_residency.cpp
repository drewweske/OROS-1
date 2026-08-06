#include "oros/streaming/world_cell_residency.hpp"

#include <utility>

namespace oros::streaming
{
    namespace
    {
        [[nodiscard]]
        bool revision_matches_cell(
            const WorldCellRevisionId&
                revision_id,
            const WorldCellKey&
                cell_key) noexcept
        {
            return
                revision_id.is_valid() &&
                revision_id.cell_key ==
                    cell_key;
        }

        [[nodiscard]]
        foundation::Error
        normalize_failure(
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
                "The world cell residency load "
                "failed without an error code."
            };
        }
    }

    foundation::Result<
        WorldCellResidency>
    WorldCellResidency::create(
        const WorldCellKey cell_key)
    {
        if (!cell_key.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot create residency state for "
                "an invalid world cell key.");
        }

        return WorldCellResidency{
            cell_key
        };
    }

    WorldCellResidency::
    WorldCellResidency(
        const WorldCellKey cell_key)
        noexcept
        : cell_key_{
              cell_key
          },
          state_{
              cell_key.is_valid()
                  ? WorldCellResidencyState::
                        unloaded
                  : WorldCellResidencyState::
                        invalid
          }
    {
    }

    bool
    WorldCellResidency::is_valid()
        const noexcept
    {
        if (!cell_key_.is_valid() ||
            state_ ==
                WorldCellResidencyState::
                    invalid)
        {
            return false;
        }

        switch (state_)
        {
        case WorldCellResidencyState::
            unloaded:
            return
                !target_revision_.is_valid() &&
                !resident_revision_.is_valid() &&
                active_request_id_ == 0ULL &&
                failure_.code ==
                    foundation::ErrorCode::none &&
                !snapshot_.has_value();

        case WorldCellResidencyState::
            load_queued:
        case WorldCellResidencyState::
            loading:
            return
                revision_matches_cell(
                    target_revision_,
                    cell_key_) &&
                !resident_revision_.is_valid() &&
                active_request_id_ != 0ULL &&
                failure_.code ==
                    foundation::ErrorCode::none &&
                !snapshot_.has_value();

        case WorldCellResidencyState::
            resident:
        case WorldCellResidencyState::
            unload_queued:
        case WorldCellResidencyState::
            unloading:
            return
                !target_revision_.is_valid() &&
                revision_matches_cell(
                    resident_revision_,
                    cell_key_) &&
                active_request_id_ == 0ULL &&
                failure_.code ==
                    foundation::ErrorCode::none &&
                snapshot_.has_value() &&
                snapshot_->is_valid() &&
                snapshot_->key() ==
                    cell_key_ &&
                snapshot_->revision() ==
                    resident_revision_.revision;

        case WorldCellResidencyState::
            failed:
            return
                revision_matches_cell(
                    target_revision_,
                    cell_key_) &&
                !resident_revision_.is_valid() &&
                active_request_id_ == 0ULL &&
                failure_.code !=
                    foundation::ErrorCode::none &&
                !snapshot_.has_value();

        case WorldCellResidencyState::
            invalid:
            return false;
        }

        return false;
    }

    const WorldCellKey&
    WorldCellResidency::cell_key()
        const noexcept
    {
        return cell_key_;
    }

    WorldCellResidencyState
    WorldCellResidency::state()
        const noexcept
    {
        return state_;
    }

    const WorldCellRevisionId&
    WorldCellResidency::target_revision()
        const noexcept
    {
        return target_revision_;
    }

    const WorldCellRevisionId&
    WorldCellResidency::resident_revision()
        const noexcept
    {
        return resident_revision_;
    }

    std::uint64_t
    WorldCellResidency::active_request_id()
        const noexcept
    {
        return active_request_id_;
    }

    const foundation::Error&
    WorldCellResidency::failure()
        const noexcept
    {
        return failure_;
    }

    const WorldCellSnapshot*
    WorldCellResidency::snapshot()
        const noexcept
    {
        if (!snapshot_.has_value())
        {
            return nullptr;
        }

        return &snapshot_.value();
    }

    std::size_t
    WorldCellResidency::
    resident_byte_count() const noexcept
    {
        if (!snapshot_.has_value())
        {
            return 0U;
        }

        return snapshot_->byte_count();
    }

    bool
    WorldCellResidency::is_resident()
        const noexcept
    {
        return
            snapshot_.has_value() &&
            resident_revision_.is_valid();
    }

    foundation::Status
    WorldCellResidency::queue_load(
        const WorldCellRevisionId
            target_revision,
        const std::uint64_t request_id)
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot queue a load through invalid "
                "world cell residency state.");
        }

        if (state_ !=
            WorldCellResidencyState::unloaded)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell load can only be "
                "queued from the unloaded state.");
        }

        if (!target_revision.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot queue a load for an invalid "
                "world cell revision identity.");
        }

        if (target_revision.cell_key !=
            cell_key_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The requested world cell revision "
                "does not match this residency "
                "record.");
        }

        if (request_id == 0ULL)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "A queued world cell load requires "
                "a non-zero request identifier.");
        }

        target_revision_ =
            target_revision;

        resident_revision_ =
            invalid_world_cell_revision_id;

        active_request_id_ =
            request_id;

        failure_ =
            foundation::Error{};

        snapshot_.reset();

        state_ =
            WorldCellResidencyState::
                load_queued;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::begin_load(
        const std::uint64_t request_id)
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot begin a load through invalid "
                "world cell residency state.");
        }

        if (state_ !=
            WorldCellResidencyState::
                load_queued)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell load can only begin "
                "from the load-queued state.");
        }

        if (request_id == 0ULL ||
            request_id !=
                active_request_id_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The world cell load request "
                "identifier does not match the "
                "queued request.");
        }

        state_ =
            WorldCellResidencyState::
                loading;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::complete_load(
        const std::uint64_t request_id,
        WorldCellSnapshot snapshot)
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot complete a load through "
                "invalid world cell residency "
                "state.");
        }

        if (state_ !=
            WorldCellResidencyState::loading)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell load can only complete "
                "from the loading state.");
        }

        if (request_id == 0ULL ||
            request_id !=
                active_request_id_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The completed world cell load "
                "request identifier does not match "
                "the active request.");
        }

        if (!snapshot.is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "Cannot complete residency with an "
                "invalid world cell snapshot.");
        }

        const WorldCellRevisionId
            loaded_revision{
                snapshot.key(),
                snapshot.revision()
            };

        if (loaded_revision !=
            target_revision_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The loaded world cell snapshot "
                "does not match the requested "
                "revision identity.");
        }

        snapshot_.emplace(
            std::move(
                snapshot));

        resident_revision_ =
            loaded_revision;

        target_revision_ =
            invalid_world_cell_revision_id;

        active_request_id_ =
            0ULL;

        failure_ =
            foundation::Error{};

        state_ =
            WorldCellResidencyState::
                resident;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::fail_load(
        const std::uint64_t request_id,
        foundation::Error error)
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot fail a load through invalid "
                "world cell residency state.");
        }

        if (state_ !=
                WorldCellResidencyState::
                    load_queued &&
            state_ !=
                WorldCellResidencyState::
                    loading)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell load can only fail "
                "while queued or loading.");
        }

        if (request_id == 0ULL ||
            request_id !=
                active_request_id_)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_argument,
                "The failed world cell load request "
                "identifier does not match the "
                "active request.");
        }

        resident_revision_ =
            invalid_world_cell_revision_id;

        active_request_id_ =
            0ULL;

        snapshot_.reset();

        failure_ =
            normalize_failure(
                std::move(
                    error));

        state_ =
            WorldCellResidencyState::
                failed;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::queue_unload()
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot queue an unload through "
                "invalid world cell residency "
                "state.");
        }

        if (state_ !=
            WorldCellResidencyState::resident)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell unload can only be "
                "queued from the resident state.");
        }

        state_ =
            WorldCellResidencyState::
                unload_queued;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::begin_unload()
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot begin an unload through "
                "invalid world cell residency "
                "state.");
        }

        if (state_ !=
            WorldCellResidencyState::
                unload_queued)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell unload can only begin "
                "from the unload-queued state.");
        }

        state_ =
            WorldCellResidencyState::
                unloading;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::complete_unload()
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot complete an unload through "
                "invalid world cell residency "
                "state.");
        }

        if (state_ !=
            WorldCellResidencyState::
                unloading)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell unload can only "
                "complete from the unloading state.");
        }

        target_revision_ =
            invalid_world_cell_revision_id;

        resident_revision_ =
            invalid_world_cell_revision_id;

        active_request_id_ =
            0ULL;

        failure_ =
            foundation::Error{};

        snapshot_.reset();

        state_ =
            WorldCellResidencyState::
                unloaded;

        return foundation::Status{};
    }

    foundation::Status
    WorldCellResidency::clear_failure()
    {
        if (!is_valid())
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "Cannot clear failure through "
                "invalid world cell residency "
                "state.");
        }

        if (state_ !=
            WorldCellResidencyState::failed)
        {
            return foundation::fail(
                foundation::ErrorCode::
                    invalid_state,
                "A world cell residency failure can "
                "only be cleared from the failed "
                "state.");
        }

        target_revision_ =
            invalid_world_cell_revision_id;

        resident_revision_ =
            invalid_world_cell_revision_id;

        active_request_id_ =
            0ULL;

        failure_ =
            foundation::Error{};

        snapshot_.reset();

        state_ =
            WorldCellResidencyState::
                unloaded;

        return foundation::Status{};
    }
}