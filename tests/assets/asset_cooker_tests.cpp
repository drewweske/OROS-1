#include "oros/assets/asset_cooker.hpp"

#include "oros/assets/content_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
#include <type_traits>
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

    [[nodiscard]]
    oros::assets::AssetRecord
    make_record()
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id = AssetId{
            4ULL,
            20ULL
        };

        record.source_path =
            "data/example.echo";

        record.importer_name =
            "oros.echo";

        record.importer_version = 7U;
        record.schema_version = 3U;

        record.source_hash =
            hash_text(
                "source data");

        record.dependencies = {
            AssetId{
                8ULL,
                1ULL
            }
        };

        return record;
    }

    class EchoCooker final
        : public oros::assets::AssetCooker
    {
    public:
        [[nodiscard]]
        std::string_view name()
            const noexcept override
        {
            return "oros.echo_cooker";
        }

        [[nodiscard]]
        std::uint32_t version()
            const noexcept override
        {
            return 5U;
        }

        [[nodiscard]]
        bool supports(
            const oros::assets::AssetRecord&
                record) const noexcept override
        {
            return
                record.importer_name ==
                    "oros.echo" &&
                record.schema_version == 3U;
        }

        [[nodiscard]]
        oros::foundation::Result<
            oros::assets::CookResult>
        cook(
            const oros::assets::CookRequest&
                request) const override
        {
            using namespace oros::assets;
            using namespace oros::foundation;

            if (!request.is_valid())
            {
                return fail(
                    ErrorCode::invalid_argument,
                    "The cook request is invalid.");
            }

            if (!supports(request.record))
            {
                return fail(
                    ErrorCode::
                        unsupported_operation,
                    "The asset record is not supported.");
            }

            CookResult result{};

            result.cooked_bytes.assign(
                request.intermediate_bytes.begin(),
                request.intermediate_bytes.end());

            return result;
        }
    };

    static_assert(
        std::is_abstract_v<
            oros::assets::AssetCooker>);

    static_assert(
        std::has_virtual_destructor_v<
            oros::assets::AssetCooker>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::AssetCooker>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::AssetCooker>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::AssetCooker>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::AssetCooker>);

    static_assert(
        !std::is_copy_constructible_v<
            EchoCooker>);

    static_assert(
        !std::is_move_constructible_v<
            EchoCooker>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const CookRequest default_request{};

    check(
        state,
        !default_request.is_valid(),
        "Default cook request is invalid");

    check(
        state,
        !default_request.record.is_valid(),
        "Default cook request contains invalid metadata");

    check(
        state,
        default_request.intermediate_bytes.empty(),
        "Default cook request has no intermediate bytes");

    const AssetRecord valid_record =
        make_record();

    check(
        state,
        valid_record.is_valid(),
        "Cook test record is valid");

    check(
        state,
        !valid_record.is_cooked(),
        "Cook test record begins uncooked");

    const CookRequest empty_intermediate_request{
        valid_record,
        {}
    };

    check(
        state,
        empty_intermediate_request.is_valid(),
        "Cook request permits empty intermediate data");

    check(
        state,
        empty_intermediate_request.record ==
            valid_record,
        "Cook request preserves its asset record");

    check(
        state,
        empty_intermediate_request.
            intermediate_bytes.empty(),
        "Cook request preserves an empty byte span");

    const std::array<std::byte, 5U>
        intermediate_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50}
        };

    const CookRequest valid_request{
        valid_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    check(
        state,
        valid_request.is_valid(),
        "Complete cook request is valid");

    check(
        state,
        valid_request.record ==
            valid_record,
        "Complete cook request preserves its metadata");

    check(
        state,
        valid_request.intermediate_bytes.size() ==
            intermediate_bytes.size(),
        "Cook request preserves its intermediate byte count");

    check(
        state,
        valid_request.intermediate_bytes.data() ==
            intermediate_bytes.data(),
        "Cook request references the original intermediate bytes");

    bool request_bytes_match{true};

    for (std::size_t index = 0U;
         index < intermediate_bytes.size();
         ++index)
    {
        if (valid_request.
                intermediate_bytes[index] !=
            intermediate_bytes[index])
        {
            request_bytes_match = false;
            break;
        }
    }

    check(
        state,
        request_bytes_match,
        "Cook request exposes every intermediate byte");

    AssetRecord cooked_record =
        valid_record;

    cooked_record.cooked_hash =
        hash_text(
            "existing cooked data");

    const CookRequest already_cooked_request{
        cooked_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    check(
        state,
        cooked_record.is_valid(),
        "Cooked test record remains valid");

    check(
        state,
        cooked_record.is_cooked(),
        "Cooked test record reports cooked state");

    check(
        state,
        !already_cooked_request.is_valid(),
        "Cook request rejects already-cooked metadata");

    AssetRecord invalid_record{};

    const CookRequest invalid_record_request{
        invalid_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    check(
        state,
        !invalid_record_request.is_valid(),
        "Cook request rejects invalid metadata");

    const CookResult default_result{};

    check(
        state,
        !default_result.has_cooked_data(),
        "Default cook result has no cooked data");

    check(
        state,
        default_result.cooked_bytes.empty(),
        "Default cook result has an empty byte payload");

    CookResult populated_result{};

    populated_result.cooked_bytes = {
        std::byte{0xAA},
        std::byte{0xBB},
        std::byte{0xCC}
    };

    check(
        state,
        populated_result.has_cooked_data(),
        "Non-empty cooked payload reports cooked data");

    check(
        state,
        populated_result.cooked_bytes.size() ==
            3U,
        "Cook result preserves its byte count");

    check(
        state,
        populated_result.cooked_bytes[0] ==
            std::byte{0xAA} &&
            populated_result.cooked_bytes[1] ==
                std::byte{0xBB} &&
            populated_result.cooked_bytes[2] ==
                std::byte{0xCC},
        "Cook result preserves every cooked byte");

    populated_result.cooked_bytes.clear();

    check(
        state,
        !populated_result.has_cooked_data(),
        "Clearing cooked bytes removes cooked-data state");

    EchoCooker concrete_cooker{};

    const AssetCooker& cooker =
        concrete_cooker;

    check(
        state,
        cooker.name() ==
            "oros.echo_cooker",
        "Cooker exposes its stable name");

    check(
        state,
        cooker.version() == 5U,
        "Cooker exposes its implementation version");

    check(
        state,
        cooker.supports(
            valid_record),
        "Cooker accepts supported asset metadata");

    AssetRecord unsupported_importer_record =
        valid_record;

    unsupported_importer_record.importer_name =
        "oros.other";

    check(
        state,
        !cooker.supports(
            unsupported_importer_record),
        "Cooker rejects an unsupported importer name");

    AssetRecord unsupported_schema_record =
        valid_record;

    unsupported_schema_record.schema_version =
        99U;

    check(
        state,
        !cooker.supports(
            unsupported_schema_record),
        "Cooker rejects an unsupported schema version");

    const Result<CookResult>
        successful_cook =
            cooker.cook(
                valid_request);

    check(
        state,
        successful_cook.has_value(),
        "Cooker accepts a valid supported request");

    check(
        state,
        successful_cook.has_value() &&
            successful_cook.value().
                has_cooked_data(),
        "Successful cook produces cooked data");

    check(
        state,
        successful_cook.has_value() &&
            successful_cook.value().
                cooked_bytes.size() ==
                intermediate_bytes.size(),
        "Successful cook preserves the intermediate byte count");

    bool cooked_bytes_match{false};

    if (successful_cook.has_value())
    {
        const std::vector<std::byte>&
            cooked_bytes =
                successful_cook.value().
                    cooked_bytes;

        cooked_bytes_match =
            cooked_bytes.size() ==
                intermediate_bytes.size();

        if (cooked_bytes_match)
        {
            for (std::size_t index = 0U;
                 index < intermediate_bytes.size();
                 ++index)
            {
                if (cooked_bytes[index] !=
                    intermediate_bytes[index])
                {
                    cooked_bytes_match = false;
                    break;
                }
            }
        }
    }

    check(
        state,
        cooked_bytes_match,
        "Successful cook preserves every intermediate byte");

    const Result<CookResult>
        empty_cook =
            cooker.cook(
                empty_intermediate_request);

    check(
        state,
        empty_cook.has_value(),
        "Cooker accepts an empty intermediate payload");

    check(
        state,
        empty_cook.has_value() &&
            !empty_cook.value().
                has_cooked_data(),
        "Empty intermediate payload produces empty cooked data");

    const Result<CookResult>
        invalid_cook =
            cooker.cook(
                invalid_record_request);

    check(
        state,
        !invalid_cook.has_value(),
        "Cooker rejects an invalid request");

    check(
        state,
        !invalid_cook.has_value() &&
            invalid_cook.error().code ==
                ErrorCode::invalid_argument,
        "Invalid cook request reports invalid_argument");

    const Result<CookResult>
        recook_result =
            cooker.cook(
                already_cooked_request);

    check(
        state,
        !recook_result.has_value(),
        "Cooker rejects an already-cooked request");

    check(
        state,
        !recook_result.has_value() &&
            recook_result.error().code ==
                ErrorCode::invalid_argument,
        "Already-cooked request reports invalid_argument");

    const CookRequest unsupported_request{
        unsupported_importer_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    check(
        state,
        unsupported_request.is_valid(),
        "Unsupported metadata can still form a valid request");

    const Result<CookResult>
        unsupported_cook =
            cooker.cook(
                unsupported_request);

    check(
        state,
        !unsupported_cook.has_value(),
        "Cooker rejects unsupported metadata");

    check(
        state,
        !unsupported_cook.has_value() &&
            unsupported_cook.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported metadata reports unsupported_operation");

    std::unique_ptr<AssetCooker>
        polymorphic_cooker =
            std::make_unique<EchoCooker>();

    check(
        state,
        polymorphic_cooker != nullptr,
        "Cooker can be owned through its interface");

    check(
        state,
        polymorphic_cooker->name() ==
            "oros.echo_cooker",
        "Owned cooker preserves virtual name dispatch");

    check(
        state,
        polymorphic_cooker->supports(
            valid_record),
        "Owned cooker preserves virtual support dispatch");

    const Result<CookResult>
        polymorphic_cook =
            polymorphic_cooker->cook(
                valid_request);

    check(
        state,
        polymorphic_cook.has_value(),
        "Owned cooker preserves virtual cook dispatch");

    std::cout
        << "\nAsset cooker test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}