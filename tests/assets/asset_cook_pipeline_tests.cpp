#include "oros/assets/asset_cook_pipeline.hpp"

#include "oros/assets/content_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <new>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
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
    oros::assets::ImportedAsset
    make_imported_asset(
        const oros::assets::AssetId id =
            oros::assets::AssetId{
                7ULL,
                20ULL
            })
    {
        using namespace oros::assets;

        ImportedAsset asset{};

        asset.record.id = id;

        asset.record.source_path =
            "textures/terrain/rock.pipeline";

        asset.record.importer_name =
            "oros.pipeline_importer";

        asset.record.importer_version =
            9U;

        asset.record.schema_version =
            4U;

        asset.record.source_hash =
            hash_text(
                "pipeline source data");

        asset.record.dependencies = {
            AssetId{
                8ULL,
                1ULL
            },
            AssetId{
                8ULL,
                2ULL
            }
        };

        asset.intermediate_bytes = {
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

        return asset;
    }

    enum class CookBehavior
    {
        copy_with_marker,
        empty_success,
        explicit_failure,
        throw_bad_alloc,
        throw_unexpected,
        mutate_name,
        mutate_version,
        mutate_support
    };

    class PipelineCooker final
        : public oros::assets::AssetCooker
    {
    public:
        PipelineCooker(
            std::string cooker_name,
            const std::uint32_t cooker_version,
            const CookBehavior behavior)
            : name_{
                  std::move(cooker_name)
              },
              version_{
                  cooker_version
              },
              behavior_{
                  behavior
              }
        {
        }

        [[nodiscard]]
        std::string_view name()
            const noexcept override
        {
            return name_;
        }

        [[nodiscard]]
        std::uint32_t version()
            const noexcept override
        {
            return version_;
        }

        [[nodiscard]]
        bool supports(
            const oros::assets::AssetRecord&
                record) const noexcept override
        {
            return
                support_enabled_ &&
                record.importer_name ==
                    "oros.pipeline_importer" &&
                record.schema_version == 4U;
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
                    "The pipeline cooker received "
                    "an invalid request.");
            }

            switch (behavior_)
            {
            case CookBehavior::copy_with_marker:
            {
                CookResult result{};

                result.cooked_bytes.reserve(
                    request.intermediate_bytes.
                        size() +
                    1U);

                result.cooked_bytes.push_back(
                    std::byte{0xC0});

                result.cooked_bytes.insert(
                    result.cooked_bytes.end(),
                    request.intermediate_bytes.begin(),
                    request.intermediate_bytes.end());

                return result;
            }

            case CookBehavior::empty_success:
                return CookResult{};

            case CookBehavior::explicit_failure:
                return fail(
                    ErrorCode::not_found,
                    "Required cooking data was "
                    "not found.");

            case CookBehavior::throw_bad_alloc:
                throw std::bad_alloc{};

            case CookBehavior::throw_unexpected:
                throw std::runtime_error{
                    "Unexpected cooker failure."
                };

            case CookBehavior::mutate_name:
            {
                name_ =
                    "oros.changed_during_cook";

                CookResult result{};

                result.cooked_bytes.assign(
                    request.intermediate_bytes.begin(),
                    request.intermediate_bytes.end());

                return result;
            }

            case CookBehavior::mutate_version:
            {
                version_ = 99U;

                CookResult result{};

                result.cooked_bytes.assign(
                    request.intermediate_bytes.begin(),
                    request.intermediate_bytes.end());

                return result;
            }

            case CookBehavior::mutate_support:
            {
                support_enabled_ = false;

                CookResult result{};

                result.cooked_bytes.assign(
                    request.intermediate_bytes.begin(),
                    request.intermediate_bytes.end());

                return result;
            }
            }

            return fail(
                ErrorCode::internal_failure,
                "The cooker behavior is invalid.");
        }

        void set_name(
            std::string cooker_name)
        {
            name_ =
                std::move(
                    cooker_name);
        }

        void set_version(
            const std::uint32_t cooker_version)
        {
            version_ =
                cooker_version;
        }

    private:
        mutable std::string name_{};
        mutable std::uint32_t version_{};
        CookBehavior behavior_{};
        mutable bool support_enabled_{true};
    };

    class TransientSupportCooker final
        : public oros::assets::AssetCooker
    {
    public:
        [[nodiscard]]
        std::string_view name()
            const noexcept override
        {
            return
                "oros.transient_support_cooker";
        }

        [[nodiscard]]
        std::uint32_t version()
            const noexcept override
        {
            return 1U;
        }

        [[nodiscard]]
        bool supports(
            const oros::assets::AssetRecord&
                record) const noexcept override
        {
            const bool metadata_matches =
                record.importer_name ==
                    "oros.pipeline_importer" &&
                record.schema_version == 4U;

            const bool supports_this_call =
                support_call_count_ == 0U;

            ++support_call_count_;

            return
                metadata_matches &&
                supports_this_call;
        }

        [[nodiscard]]
        oros::foundation::Result<
            oros::assets::CookResult>
        cook(
            const oros::assets::CookRequest&
                request) const override
        {
            oros::assets::CookResult result{};

            result.cooked_bytes.assign(
                request.intermediate_bytes.begin(),
                request.intermediate_bytes.end());

            return result;
        }

    private:
        mutable std::size_t
            support_call_count_{};
    };

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetCookPipeline>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetCookPipeline>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                AssetCookPipeline>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                AssetCookPipeline>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const CookedAsset default_cooked_asset{};

    check(
        state,
        !default_cooked_asset.is_valid(),
        "Default cooked asset is invalid");

    check(
        state,
        !default_cooked_asset.
            has_cooked_data(),
        "Default cooked asset has no cooked data");

    check(
        state,
        default_cooked_asset.
            cooked_bytes.empty(),
        "Default cooked asset has an empty payload");

    check(
        state,
        default_cooked_asset.
            cooker_name.empty(),
        "Default cooked asset has no cooker name");

    check(
        state,
        default_cooked_asset.
            cooker_version == 0U,
        "Default cooked asset has cooker version zero");

    const ImportedAsset imported_asset =
        make_imported_asset();

    check(
        state,
        imported_asset.is_valid(),
        "Cook test imported asset is valid");

    check(
        state,
        imported_asset.
            has_intermediate_data(),
        "Cook test imported asset has intermediate data");

    check(
        state,
        !imported_asset.record.is_cooked(),
        "Cook test imported record begins uncooked");

    CookedAsset manually_cooked_asset{};

    manually_cooked_asset.record =
        imported_asset.record;

    manually_cooked_asset.record.cooked_hash =
        hash_bytes({});

    manually_cooked_asset.cooker_name =
        "oros.manual_cooker";

    manually_cooked_asset.cooker_version =
        1U;

    check(
        state,
        manually_cooked_asset.is_valid(),
        "Cooked asset permits an empty cooked payload");

    check(
        state,
        !manually_cooked_asset.
            has_cooked_data(),
        "Empty cooked payload reports no cooked data");

    manually_cooked_asset.cooker_name.clear();

    check(
        state,
        !manually_cooked_asset.is_valid(),
        "Cooked asset rejects an empty cooker name");

    manually_cooked_asset.cooker_name =
        "oros.manual_cooker";

    manually_cooked_asset.cooker_version =
        0U;

    check(
        state,
        !manually_cooked_asset.is_valid(),
        "Cooked asset rejects cooker version zero");

    manually_cooked_asset.cooker_version =
        1U;

    manually_cooked_asset.record.
        cooked_hash.reset();

    check(
        state,
        !manually_cooked_asset.is_valid(),
        "Cooked asset requires a cooked content hash");

    AssetCookerRegistry registry{};

    auto successful_cooker =
        std::make_unique<
            PipelineCooker>(
                "oros.pipeline_cooker",
                12U,
                CookBehavior::
                    copy_with_marker);

    PipelineCooker* const
        successful_cooker_pointer =
            successful_cooker.get();

    const Status successful_registration =
        registry.register_cooker(
            std::move(
                successful_cooker));

    check(
        state,
        successful_registration.
            has_value(),
        "Pipeline cooker registers successfully");

    check(
        state,
        registry.find(
            "oros.pipeline_cooker") ==
            successful_cooker_pointer,
        "Registry preserves the pipeline cooker");

    AssetCookPipeline pipeline{
        registry
    };

    const Result<CookedAsset>
        successful_result =
            pipeline.cook(
                imported_asset);

    check(
        state,
        successful_result.has_value(),
        "Cook pipeline accepts a valid imported asset");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                is_valid(),
        "Cook pipeline produces a valid cooked asset");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                has_cooked_data(),
        "Successful cooking produces cooked data");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                cooker_name ==
                "oros.pipeline_cooker",
        "Cook pipeline records the cooker name");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                cooker_version == 12U,
        "Cook pipeline records the cooker version");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.id ==
                imported_asset.record.id,
        "Cook pipeline preserves the asset identity");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.source_path ==
                imported_asset.record.
                    source_path,
        "Cook pipeline preserves the source path");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.importer_name ==
                imported_asset.record.
                    importer_name,
        "Cook pipeline preserves the importer name");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.importer_version ==
                imported_asset.record.
                    importer_version,
        "Cook pipeline preserves the importer version");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.schema_version ==
                imported_asset.record.
                    schema_version,
        "Cook pipeline preserves the schema version");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.source_hash ==
                imported_asset.record.
                    source_hash,
        "Cook pipeline preserves the source hash");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.dependencies ==
                imported_asset.record.
                    dependencies,
        "Cook pipeline preserves asset dependencies");

    const std::vector<std::byte>
        expected_cooked_bytes{
            std::byte{0xC0},
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                cooked_bytes ==
                expected_cooked_bytes,
        "Cook pipeline preserves every cooker output byte");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.cooked_hash.
                    has_value(),
        "Cook pipeline records a cooked content hash");

    check(
        state,
        successful_result.has_value() &&
            successful_result.value().
                record.cooked_hash ==
                hash_bytes(
                    std::span<const std::byte>{
                        expected_cooked_bytes
                    }),
        "Cook pipeline hashes every cooked byte");

    check(
        state,
        !imported_asset.record.is_cooked(),
        "Cooking does not mutate the source record");

    check(
        state,
        imported_asset.intermediate_bytes ==
            std::vector<std::byte>{
                std::byte{0x10},
                std::byte{0x20},
                std::byte{0x30},
                std::byte{0x40}
            },
        "Cooking does not mutate intermediate data");

    const Result<CookedAsset>
        repeated_result =
            pipeline.cook(
                imported_asset);

    check(
        state,
        repeated_result.has_value(),
        "Cook pipeline can repeat a deterministic cook");

    check(
        state,
        repeated_result.has_value() &&
            successful_result.has_value() &&
            repeated_result.value().
                cooked_bytes ==
                successful_result.value().
                    cooked_bytes,
        "Repeated cooking produces identical bytes");

    check(
        state,
        repeated_result.has_value() &&
            successful_result.has_value() &&
            repeated_result.value().
                record.cooked_hash ==
                successful_result.value().
                    record.cooked_hash,
        "Repeated cooking produces an identical hash");

    const Result<CookedAsset>
        invalid_asset_result =
            pipeline.cook(
                ImportedAsset{});

    check(
        state,
        !invalid_asset_result.has_value(),
        "Cook pipeline rejects a default imported asset");

    check(
        state,
        !invalid_asset_result.has_value() &&
            invalid_asset_result.error().code ==
                ErrorCode::invalid_argument,
        "Default imported asset reports invalid_argument");

    ImportedAsset already_cooked_asset =
        imported_asset;

    already_cooked_asset.record.cooked_hash =
        hash_text(
            "existing cooked data");

    const Result<CookedAsset>
        already_cooked_result =
            pipeline.cook(
                already_cooked_asset);

    check(
        state,
        !already_cooked_result.has_value(),
        "Cook pipeline rejects already-cooked metadata");

    check(
        state,
        !already_cooked_result.has_value() &&
            already_cooked_result.error().code ==
                ErrorCode::invalid_argument,
        "Already-cooked metadata reports invalid_argument");

    AssetCookerRegistry
        empty_output_registry{};

    const Status empty_output_registration =
        empty_output_registry.register_cooker(
            std::make_unique<
                PipelineCooker>(
                    "oros.empty_cooker",
                    3U,
                    CookBehavior::
                        empty_success));

    check(
        state,
        empty_output_registration.
            has_value(),
        "Empty-output cooker registers successfully");

    AssetCookPipeline empty_output_pipeline{
        empty_output_registry
    };

    ImportedAsset empty_input_asset =
        make_imported_asset(
            AssetId{
                7ULL,
                21ULL
            });

    empty_input_asset.
        intermediate_bytes.clear();

    check(
        state,
        empty_input_asset.is_valid(),
        "Imported asset permits empty intermediate data");

    const Result<CookedAsset>
        empty_output_result =
            empty_output_pipeline.cook(
                empty_input_asset);

    check(
        state,
        empty_output_result.has_value(),
        "Cook pipeline accepts an empty intermediate payload");

    check(
        state,
        empty_output_result.has_value() &&
            empty_output_result.value().
                is_valid(),
        "Empty cooker output still forms a valid cooked asset");

    check(
        state,
        empty_output_result.has_value() &&
            !empty_output_result.value().
                has_cooked_data(),
        "Empty cooker output reports no cooked data");

    check(
        state,
        empty_output_result.has_value() &&
            empty_output_result.value().
                record.cooked_hash ==
                hash_bytes({}),
        "Cook pipeline hashes an empty cooked payload");

    AssetCookerRegistry
        unsupported_registry{};

    AssetCookPipeline unsupported_pipeline{
        unsupported_registry
    };

    const Result<CookedAsset>
        unsupported_result =
            unsupported_pipeline.cook(
                imported_asset);

    check(
        state,
        !unsupported_result.has_value(),
        "Cook pipeline rejects metadata with no cooker");

    check(
        state,
        !unsupported_result.has_value() &&
            unsupported_result.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Missing cooker preserves unsupported_operation");

    AssetCookerRegistry
        ambiguous_registry{};

    const Status
        first_ambiguous_registration =
            ambiguous_registry.register_cooker(
                std::make_unique<
                    PipelineCooker>(
                        "oros.ambiguous_first",
                        1U,
                        CookBehavior::
                            copy_with_marker));

    const Status
        second_ambiguous_registration =
            ambiguous_registry.register_cooker(
                std::make_unique<
                    PipelineCooker>(
                        "oros.ambiguous_second",
                        1U,
                        CookBehavior::
                            copy_with_marker));

    check(
        state,
        first_ambiguous_registration.
                has_value() &&
            second_ambiguous_registration.
                has_value(),
        "Overlapping cookers register successfully");

    AssetCookPipeline ambiguous_pipeline{
        ambiguous_registry
    };

    const Result<CookedAsset>
        ambiguous_result =
            ambiguous_pipeline.cook(
                imported_asset);

    check(
        state,
        !ambiguous_result.has_value(),
        "Cook pipeline rejects ambiguous cooker resolution");

    check(
        state,
        !ambiguous_result.has_value() &&
            ambiguous_result.error().code ==
                ErrorCode::invalid_state,
        "Ambiguous resolution preserves invalid_state");

    AssetCookerRegistry
        failing_registry{};

    const Status failing_registration =
        failing_registry.register_cooker(
            std::make_unique<
                PipelineCooker>(
                    "oros.failing_cooker",
                    1U,
                    CookBehavior::
                        explicit_failure));

    check(
        state,
        failing_registration.has_value(),
        "Failing cooker registers successfully");

    AssetCookPipeline failing_pipeline{
        failing_registry
    };

    const Result<CookedAsset>
        failing_result =
            failing_pipeline.cook(
                imported_asset);

    check(
        state,
        !failing_result.has_value(),
        "Cook pipeline preserves a cooker failure");

    check(
        state,
        !failing_result.has_value() &&
            failing_result.error().code ==
                ErrorCode::not_found,
        "Cook pipeline preserves the cooker error code");

    check(
        state,
        !failing_result.has_value() &&
            failing_result.error().message ==
                "Required cooking data was "
                "not found.",
        "Cook pipeline preserves the cooker error message");

    AssetCookerRegistry
        allocation_failure_registry{};

    const Status
        allocation_failure_registration =
            allocation_failure_registry.
                register_cooker(
                    std::make_unique<
                        PipelineCooker>(
                            "oros.bad_alloc_cooker",
                            1U,
                            CookBehavior::
                                throw_bad_alloc));

    check(
        state,
        allocation_failure_registration.
            has_value(),
        "Allocation-failure cooker registers successfully");

    AssetCookPipeline
        allocation_failure_pipeline{
            allocation_failure_registry
        };

    const Result<CookedAsset>
        allocation_failure_result =
            allocation_failure_pipeline.cook(
                imported_asset);

    check(
        state,
        !allocation_failure_result.has_value(),
        "Cook pipeline catches cooker allocation failure");

    check(
        state,
        !allocation_failure_result.has_value() &&
            allocation_failure_result.
                error().code ==
                ErrorCode::out_of_memory,
        "Cooker allocation failure reports out_of_memory");

    AssetCookerRegistry
        unexpected_failure_registry{};

    const Status
        unexpected_failure_registration =
            unexpected_failure_registry.
                register_cooker(
                    std::make_unique<
                        PipelineCooker>(
                            "oros.unexpected_cooker",
                            1U,
                            CookBehavior::
                                throw_unexpected));

    check(
        state,
        unexpected_failure_registration.
            has_value(),
        "Throwing cooker registers successfully");

    AssetCookPipeline
        unexpected_failure_pipeline{
            unexpected_failure_registry
        };

    const Result<CookedAsset>
        unexpected_failure_result =
            unexpected_failure_pipeline.cook(
                imported_asset);

    check(
        state,
        !unexpected_failure_result.has_value(),
        "Cook pipeline catches an unexpected cooker exception");

    check(
        state,
        !unexpected_failure_result.has_value() &&
            unexpected_failure_result.
                error().code ==
                ErrorCode::internal_failure,
        "Unexpected cooker exception reports internal_failure");

    AssetCookerRegistry
        empty_name_registry{};

    auto empty_name_cooker =
        std::make_unique<
            PipelineCooker>(
                "oros.mutable_name_cooker",
                1U,
                CookBehavior::
                    copy_with_marker);

    PipelineCooker* const
        empty_name_pointer =
            empty_name_cooker.get();

    const Status empty_name_registration =
        empty_name_registry.register_cooker(
            std::move(
                empty_name_cooker));

    check(
        state,
        empty_name_registration.has_value(),
        "Mutable-name cooker registers successfully");

    empty_name_pointer->set_name(
        "");

    AssetCookPipeline empty_name_pipeline{
        empty_name_registry
    };

    const Result<CookedAsset>
        empty_name_result =
            empty_name_pipeline.cook(
                imported_asset);

    check(
        state,
        !empty_name_result.has_value(),
        "Cook pipeline rejects an empty resolved cooker name");

    check(
        state,
        !empty_name_result.has_value() &&
            empty_name_result.error().code ==
                ErrorCode::invalid_state,
        "Empty resolved cooker name reports invalid_state");

    AssetCookerRegistry
        mismatched_name_registry{};

    auto mismatched_name_cooker =
        std::make_unique<
            PipelineCooker>(
                "oros.indexed_name_cooker",
                1U,
                CookBehavior::
                    copy_with_marker);

    PipelineCooker* const
        mismatched_name_pointer =
            mismatched_name_cooker.get();

    const Status
        mismatched_name_registration =
            mismatched_name_registry.
                register_cooker(
                    std::move(
                        mismatched_name_cooker));

    check(
        state,
        mismatched_name_registration.
            has_value(),
        "Indexed-name cooker registers successfully");

    mismatched_name_pointer->set_name(
        "oros.unindexed_name_cooker");

    AssetCookPipeline
        mismatched_name_pipeline{
            mismatched_name_registry
        };

    const Result<CookedAsset>
        mismatched_name_result =
            mismatched_name_pipeline.cook(
                imported_asset);

    check(
        state,
        !mismatched_name_result.has_value(),
        "Cook pipeline rejects a mismatched registry name");

    check(
        state,
        !mismatched_name_result.has_value() &&
            mismatched_name_result.
                error().code ==
                ErrorCode::invalid_state,
        "Mismatched registry name reports invalid_state");

    AssetCookerRegistry
        zero_version_registry{};

    auto zero_version_cooker =
        std::make_unique<
            PipelineCooker>(
                "oros.mutable_version_cooker",
                1U,
                CookBehavior::
                    copy_with_marker);

    PipelineCooker* const
        zero_version_pointer =
            zero_version_cooker.get();

    const Status zero_version_registration =
        zero_version_registry.register_cooker(
            std::move(
                zero_version_cooker));

    check(
        state,
        zero_version_registration.has_value(),
        "Mutable-version cooker registers successfully");

    zero_version_pointer->set_version(
        0U);

    AssetCookPipeline zero_version_pipeline{
        zero_version_registry
    };

    const Result<CookedAsset>
        zero_version_result =
            zero_version_pipeline.cook(
                imported_asset);

    check(
        state,
        !zero_version_result.has_value(),
        "Cook pipeline rejects resolved cooker version zero");

    check(
        state,
        !zero_version_result.has_value() &&
            zero_version_result.error().code ==
                ErrorCode::invalid_state,
        "Resolved cooker version zero reports invalid_state");

    AssetCookerRegistry
        transient_support_registry{};

    const Status
        transient_support_registration =
            transient_support_registry.
                register_cooker(
                    std::make_unique<
                        TransientSupportCooker>());

    check(
        state,
        transient_support_registration.
            has_value(),
        "Transient-support cooker registers successfully");

    AssetCookPipeline
        transient_support_pipeline{
            transient_support_registry
        };

    const Result<CookedAsset>
        transient_support_result =
            transient_support_pipeline.cook(
                imported_asset);

    check(
        state,
        !transient_support_result.has_value(),
        "Cook pipeline rejects support changes after resolution");

    check(
        state,
        !transient_support_result.has_value() &&
            transient_support_result.
                error().code ==
                ErrorCode::invalid_state,
        "Pre-cook support change reports invalid_state");

    AssetCookerRegistry
        mutate_name_registry{};

    const Status mutate_name_registration =
        mutate_name_registry.register_cooker(
            std::make_unique<
                PipelineCooker>(
                    "oros.mutate_name_cooker",
                    1U,
                    CookBehavior::
                        mutate_name));

    check(
        state,
        mutate_name_registration.has_value(),
        "Name-mutating cooker registers successfully");

    AssetCookPipeline mutate_name_pipeline{
        mutate_name_registry
    };

    const Result<CookedAsset>
        mutate_name_result =
            mutate_name_pipeline.cook(
                imported_asset);

    check(
        state,
        !mutate_name_result.has_value(),
        "Cook pipeline rejects cooker name mutation");

    check(
        state,
        !mutate_name_result.has_value() &&
            mutate_name_result.error().code ==
                ErrorCode::invalid_state,
        "Cooker name mutation reports invalid_state");

    AssetCookerRegistry
        mutate_version_registry{};

    const Status
        mutate_version_registration =
            mutate_version_registry.
                register_cooker(
                    std::make_unique<
                        PipelineCooker>(
                            "oros.mutate_version_cooker",
                            1U,
                            CookBehavior::
                                mutate_version));

    check(
        state,
        mutate_version_registration.
            has_value(),
        "Version-mutating cooker registers successfully");

    AssetCookPipeline
        mutate_version_pipeline{
            mutate_version_registry
        };

    const Result<CookedAsset>
        mutate_version_result =
            mutate_version_pipeline.cook(
                imported_asset);

    check(
        state,
        !mutate_version_result.has_value(),
        "Cook pipeline rejects cooker version mutation");

    check(
        state,
        !mutate_version_result.has_value() &&
            mutate_version_result.
                error().code ==
                ErrorCode::invalid_state,
        "Cooker version mutation reports invalid_state");

    AssetCookerRegistry
        mutate_support_registry{};

    const Status
        mutate_support_registration =
            mutate_support_registry.
                register_cooker(
                    std::make_unique<
                        PipelineCooker>(
                            "oros.mutate_support_cooker",
                            1U,
                            CookBehavior::
                                mutate_support));

    check(
        state,
        mutate_support_registration.
            has_value(),
        "Support-mutating cooker registers successfully");

    AssetCookPipeline
        mutate_support_pipeline{
            mutate_support_registry
        };

    const Result<CookedAsset>
        mutate_support_result =
            mutate_support_pipeline.cook(
                imported_asset);

    check(
        state,
        !mutate_support_result.has_value(),
        "Cook pipeline rejects cooker support mutation");

    check(
        state,
        !mutate_support_result.has_value() &&
            mutate_support_result.
                error().code ==
                ErrorCode::invalid_state,
        "Cooker support mutation reports invalid_state");

    std::cout
        << "\nAsset cook pipeline test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}