#include "oros/assets/asset_import_pipeline.hpp"

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

    enum class ImportBehavior
    {
        copy_source,
        empty_success,
        importer_failure,
        invalid_dependency,
        self_dependency,
        duplicate_dependency,
        throw_bad_alloc,
        throw_unexpected
    };

    class PipelineImporter final
        : public oros::assets::AssetImporter
    {
    public:
        PipelineImporter(
            std::string importer_name,
            const std::uint32_t importer_version,
            const std::uint32_t output_schema_version,
            std::string supported_suffix,
            const ImportBehavior behavior,
            std::vector<oros::assets::AssetId>
                dependencies = {})
            : name_{
                  std::move(importer_name)
              },
              version_{
                  importer_version
              },
              schema_version_{
                  output_schema_version
              },
              suffix_{
                  std::move(supported_suffix)
              },
              behavior_{
                  behavior
              },
              dependencies_{
                  std::move(dependencies)
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
        std::uint32_t schema_version()
            const noexcept override
        {
            return schema_version_;
        }

        [[nodiscard]]
        bool supports(
            const std::string_view source_path)
            const noexcept override
        {
            return
                !source_path.empty() &&
                !suffix_.empty() &&
                source_path.ends_with(
                    suffix_);
        }

        [[nodiscard]]
        oros::foundation::Result<
            oros::assets::ImportResult>
        import(
            const oros::assets::ImportRequest&
                request) const override
        {
            using namespace oros::assets;
            using namespace oros::foundation;

            switch (behavior_)
            {
            case ImportBehavior::copy_source:
            {
                ImportResult result{};

                result.intermediate_bytes.assign(
                    request.source_bytes.begin(),
                    request.source_bytes.end());

                result.dependencies =
                    dependencies_;

                return result;
            }

            case ImportBehavior::empty_success:
                return ImportResult{};

            case ImportBehavior::importer_failure:
                return fail(
                    ErrorCode::not_found,
                    "The importer could not locate "
                    "required source data.");

            case ImportBehavior::invalid_dependency:
            {
                ImportResult result{};

                result.dependencies.push_back(
                    invalid_asset_id);

                return result;
            }

            case ImportBehavior::self_dependency:
            {
                ImportResult result{};

                result.dependencies.push_back(
                    request.asset);

                return result;
            }

            case ImportBehavior::duplicate_dependency:
            {
                ImportResult result{};

                const AssetId dependency{
                    90ULL,
                    1ULL
                };

                result.dependencies = {
                    dependency,
                    dependency
                };

                return result;
            }

            case ImportBehavior::throw_bad_alloc:
                throw std::bad_alloc{};

            case ImportBehavior::throw_unexpected:
                throw std::runtime_error{
                    "Unexpected importer failure."
                };
            }

            return fail(
                ErrorCode::internal_failure,
                "The importer behavior is invalid.");
        }

        void clear_name()
        {
            name_.clear();
        }

    private:
        std::string name_{};
        std::uint32_t version_{};
        std::uint32_t schema_version_{};
        std::string suffix_{};
        ImportBehavior behavior_{};
        std::vector<oros::assets::AssetId>
            dependencies_{};
    };

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetImportPipeline>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetImportPipeline>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                AssetImportPipeline>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                AssetImportPipeline>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const ImportedAsset default_asset{};

    check(
        state,
        !default_asset.is_valid(),
        "Default imported asset is invalid");

    check(
        state,
        !default_asset.has_intermediate_data(),
        "Default imported asset has no intermediate data");

    check(
        state,
        default_asset.intermediate_bytes.empty(),
        "Default imported asset has an empty byte payload");

    AssetImporterRegistry registry{};

    const AssetId first_dependency{
        8ULL,
        1ULL
    };

    const AssetId second_dependency{
        8ULL,
        2ULL
    };

    const Status importer_registration =
        registry.register_importer(
            std::make_unique<
                PipelineImporter>(
                "oros.pipeline_test",
                7U,
                4U,
                ".pipe",
                ImportBehavior::copy_source,
                std::vector<AssetId>{
                    first_dependency,
                    second_dependency
                }));

    check(
        state,
        importer_registration.has_value(),
        "Pipeline test importer registers successfully");

    AssetImportPipeline pipeline{
        registry
    };

    const Result<ImportedAsset>
        default_request_result =
            pipeline.import(
                ImportRequest{});

    check(
        state,
        !default_request_result.has_value(),
        "Pipeline rejects a default request");

    check(
        state,
        !default_request_result.has_value() &&
            default_request_result.error().code ==
                ErrorCode::invalid_argument,
        "Default request reports invalid_argument");

    const AssetId source_asset{
        5ULL,
        20ULL
    };

    const ImportRequest missing_path{
        source_asset,
        {},
        {}
    };

    const Result<ImportedAsset>
        missing_path_result =
            pipeline.import(
                missing_path);

    check(
        state,
        !missing_path_result.has_value(),
        "Pipeline rejects a request with no source path");

    check(
        state,
        !missing_path_result.has_value() &&
            missing_path_result.error().code ==
                ErrorCode::invalid_argument,
        "Missing source path reports invalid_argument");

    const std::array<std::byte, 6U>
        source_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50},
            std::byte{0x60}
        };

    const ImportRequest valid_request{
        source_asset,
        "textures/terrain/rock.pipe",
        std::span<const std::byte>{
            source_bytes
        }
    };

    const Result<ImportedAsset>
        successful_import =
            pipeline.import(
                valid_request);

    check(
        state,
        successful_import.has_value(),
        "Pipeline imports a supported request");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                is_valid(),
        "Successful pipeline output is valid");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                has_intermediate_data(),
        "Successful pipeline output has intermediate data");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.id ==
                source_asset,
        "Pipeline preserves the asset identity");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.source_path ==
                "textures/terrain/rock.pipe",
        "Pipeline preserves the source path");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.importer_name ==
                "oros.pipeline_test",
        "Pipeline records the importer name");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.importer_version ==
                7U,
        "Pipeline records the importer version");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.schema_version ==
                4U,
        "Pipeline records the output schema version");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.source_hash ==
                hash_bytes(
                    std::span<const std::byte>{
                        source_bytes
                    }),
        "Pipeline hashes every source byte");

    check(
        state,
        successful_import.has_value() &&
            !successful_import.value().
                record.is_cooked(),
        "Imported record begins uncooked");

    check(
        state,
        successful_import.has_value() &&
            !successful_import.value().
                record.cooked_hash.has_value(),
        "Pipeline does not invent a cooked hash");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.dependencies.size() ==
                2U,
        "Pipeline preserves the dependency count");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.dependencies[0] ==
                first_dependency,
        "Pipeline preserves the first dependency");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                record.dependencies[1] ==
                second_dependency,
        "Pipeline preserves the second dependency");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                intermediate_bytes.size() ==
                source_bytes.size(),
        "Pipeline preserves the intermediate byte count");

    bool intermediate_bytes_match{false};

    if (successful_import.has_value())
    {
        const std::vector<std::byte>&
            intermediate_bytes =
                successful_import.value().
                    intermediate_bytes;

        intermediate_bytes_match =
            intermediate_bytes.size() ==
                source_bytes.size();

        if (intermediate_bytes_match)
        {
            for (std::size_t index = 0U;
                 index < source_bytes.size();
                 ++index)
            {
                if (intermediate_bytes[index] !=
                    source_bytes[index])
                {
                    intermediate_bytes_match =
                        false;

                    break;
                }
            }
        }
    }

    check(
        state,
        intermediate_bytes_match,
        "Pipeline preserves every intermediate byte");

    const Result<ImportedAsset>
        unsupported_import =
            pipeline.import(
                ImportRequest{
                    source_asset,
                    "textures/terrain/rock.unknown",
                    std::span<const std::byte>{
                        source_bytes
                    }
                });

    check(
        state,
        !unsupported_import.has_value(),
        "Pipeline rejects an unsupported source path");

    check(
        state,
        !unsupported_import.has_value() &&
            unsupported_import.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported path preserves the registry error");

    AssetImporterRegistry
        empty_output_registry{};

    const Status empty_output_registration =
        empty_output_registry.register_importer(
            std::make_unique<
                PipelineImporter>(
                "oros.empty",
                1U,
                1U,
                ".empty",
                ImportBehavior::
                    empty_success));

    check(
        state,
        empty_output_registration.has_value(),
        "Empty-output importer registers successfully");

    AssetImportPipeline empty_output_pipeline{
        empty_output_registry
    };

    const ImportRequest empty_source_request{
        AssetId{
            6ULL,
            1ULL
        },
        "data/empty.empty",
        {}
    };

    const Result<ImportedAsset>
        empty_output_import =
            empty_output_pipeline.import(
                empty_source_request);

    check(
        state,
        empty_output_import.has_value(),
        "Pipeline accepts an empty source file");

    check(
        state,
        empty_output_import.has_value() &&
            empty_output_import.value().
                is_valid(),
        "Empty source still produces valid metadata");

    check(
        state,
        empty_output_import.has_value() &&
            !empty_output_import.value().
                has_intermediate_data(),
        "Empty importer output has no intermediate data");

    check(
        state,
        empty_output_import.has_value() &&
            empty_output_import.value().
                record.source_hash ==
                hash_bytes({}),
        "Pipeline hashes an empty source file");

    AssetImporterRegistry
        failing_registry{};

    const Status failing_registration =
        failing_registry.register_importer(
            std::make_unique<
                PipelineImporter>(
                "oros.failure",
                1U,
                1U,
                ".failure",
                ImportBehavior::
                    importer_failure));

    check(
        state,
        failing_registration.has_value(),
        "Failing importer registers successfully");

    AssetImportPipeline failing_pipeline{
        failing_registry
    };

    const Result<ImportedAsset>
        importer_failure =
            failing_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        2ULL
                    },
                    "data/test.failure",
                    {}
                });

    check(
        state,
        !importer_failure.has_value(),
        "Pipeline preserves an importer failure");

    check(
        state,
        !importer_failure.has_value() &&
            importer_failure.error().code ==
                ErrorCode::not_found,
        "Pipeline preserves the importer error code");

    AssetImporterRegistry
        invalid_dependency_registry{};

    const Status
        invalid_dependency_registration =
            invalid_dependency_registry.
                register_importer(
                    std::make_unique<
                        PipelineImporter>(
                        "oros.invalid_dependency",
                        1U,
                        1U,
                        ".invalid-dependency",
                        ImportBehavior::
                            invalid_dependency));

    check(
        state,
        invalid_dependency_registration.
            has_value(),
        "Invalid-dependency importer registers successfully");

    AssetImportPipeline
        invalid_dependency_pipeline{
            invalid_dependency_registry
        };

    const Result<ImportedAsset>
        invalid_dependency_import =
            invalid_dependency_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        3ULL
                    },
                    "data/test.invalid-dependency",
                    {}
                });

    check(
        state,
        !invalid_dependency_import.has_value(),
        "Pipeline rejects an invalid dependency");

    check(
        state,
        !invalid_dependency_import.has_value() &&
            invalid_dependency_import.
                error().code ==
                ErrorCode::invalid_state,
        "Invalid dependency reports invalid_state");

    AssetImporterRegistry
        self_dependency_registry{};

    const Status
        self_dependency_registration =
            self_dependency_registry.
                register_importer(
                    std::make_unique<
                        PipelineImporter>(
                        "oros.self_dependency",
                        1U,
                        1U,
                        ".self-dependency",
                        ImportBehavior::
                            self_dependency));

    check(
        state,
        self_dependency_registration.
            has_value(),
        "Self-dependency importer registers successfully");

    AssetImportPipeline
        self_dependency_pipeline{
            self_dependency_registry
        };

    const Result<ImportedAsset>
        self_dependency_import =
            self_dependency_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        4ULL
                    },
                    "data/test.self-dependency",
                    {}
                });

    check(
        state,
        !self_dependency_import.has_value(),
        "Pipeline rejects a self dependency");

    check(
        state,
        !self_dependency_import.has_value() &&
            self_dependency_import.
                error().code ==
                ErrorCode::invalid_state,
        "Self dependency reports invalid_state");

    AssetImporterRegistry
        duplicate_dependency_registry{};

    const Status
        duplicate_dependency_registration =
            duplicate_dependency_registry.
                register_importer(
                    std::make_unique<
                        PipelineImporter>(
                        "oros.duplicate_dependency",
                        1U,
                        1U,
                        ".duplicate-dependency",
                        ImportBehavior::
                            duplicate_dependency));

    check(
        state,
        duplicate_dependency_registration.
            has_value(),
        "Duplicate-dependency importer registers successfully");

    AssetImportPipeline
        duplicate_dependency_pipeline{
            duplicate_dependency_registry
        };

    const Result<ImportedAsset>
        duplicate_dependency_import =
            duplicate_dependency_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        5ULL
                    },
                    "data/test.duplicate-dependency",
                    {}
                });

    check(
        state,
        !duplicate_dependency_import.has_value(),
        "Pipeline rejects duplicate dependencies");

    check(
        state,
        !duplicate_dependency_import.has_value() &&
            duplicate_dependency_import.
                error().code ==
                ErrorCode::invalid_state,
        "Duplicate dependencies report invalid_state");

    AssetImporterRegistry
        allocation_failure_registry{};

    const Status
        allocation_failure_registration =
            allocation_failure_registry.
                register_importer(
                    std::make_unique<
                        PipelineImporter>(
                        "oros.bad_alloc",
                        1U,
                        1U,
                        ".bad-alloc",
                        ImportBehavior::
                            throw_bad_alloc));

    check(
        state,
        allocation_failure_registration.
            has_value(),
        "Allocation-failure importer registers successfully");

    AssetImportPipeline
        allocation_failure_pipeline{
            allocation_failure_registry
        };

    const Result<ImportedAsset>
        allocation_failure_import =
            allocation_failure_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        6ULL
                    },
                    "data/test.bad-alloc",
                    {}
                });

    check(
        state,
        !allocation_failure_import.has_value(),
        "Pipeline catches importer allocation failure");

    check(
        state,
        !allocation_failure_import.has_value() &&
            allocation_failure_import.
                error().code ==
                ErrorCode::out_of_memory,
        "Importer allocation failure reports out_of_memory");

    AssetImporterRegistry
        unexpected_failure_registry{};

    const Status
        unexpected_failure_registration =
            unexpected_failure_registry.
                register_importer(
                    std::make_unique<
                        PipelineImporter>(
                        "oros.unexpected",
                        1U,
                        1U,
                        ".unexpected",
                        ImportBehavior::
                            throw_unexpected));

    check(
        state,
        unexpected_failure_registration.
            has_value(),
        "Throwing importer registers successfully");

    AssetImportPipeline
        unexpected_failure_pipeline{
            unexpected_failure_registry
        };

    const Result<ImportedAsset>
        unexpected_failure_import =
            unexpected_failure_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        7ULL
                    },
                    "data/test.unexpected",
                    {}
                });

    check(
        state,
        !unexpected_failure_import.has_value(),
        "Pipeline catches an unexpected importer exception");

    check(
        state,
        !unexpected_failure_import.has_value() &&
            unexpected_failure_import.
                error().code ==
                ErrorCode::internal_failure,
        "Unexpected importer exception reports internal_failure");

    AssetImporterRegistry
        ambiguous_registry{};

    const Status first_ambiguous_registration =
        ambiguous_registry.register_importer(
            std::make_unique<
                PipelineImporter>(
                "oros.ambiguous_first",
                1U,
                1U,
                ".ambiguous",
                ImportBehavior::
                    empty_success));

    const Status second_ambiguous_registration =
        ambiguous_registry.register_importer(
            std::make_unique<
                PipelineImporter>(
                "oros.ambiguous_second",
                1U,
                1U,
                ".ambiguous",
                ImportBehavior::
                    empty_success));

    check(
        state,
        first_ambiguous_registration.
                has_value() &&
            second_ambiguous_registration.
                has_value(),
        "Overlapping importers register successfully");

    AssetImportPipeline ambiguous_pipeline{
        ambiguous_registry
    };

    const Result<ImportedAsset>
        ambiguous_import =
            ambiguous_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        8ULL
                    },
                    "data/test.ambiguous",
                    {}
                });

    check(
        state,
        !ambiguous_import.has_value(),
        "Pipeline rejects ambiguous importer resolution");

    check(
        state,
        !ambiguous_import.has_value() &&
            ambiguous_import.error().code ==
                ErrorCode::invalid_state,
        "Ambiguous resolution preserves invalid_state");

    AssetImporterRegistry
        mutable_metadata_registry{};

    auto mutable_metadata_importer =
        std::make_unique<
            PipelineImporter>(
            "oros.mutable",
            1U,
            1U,
            ".mutable",
            ImportBehavior::
                empty_success);

    PipelineImporter* const
        mutable_metadata_pointer =
            mutable_metadata_importer.get();

    const Status
        mutable_metadata_registration =
            mutable_metadata_registry.
                register_importer(
                    std::move(
                        mutable_metadata_importer));

    check(
        state,
        mutable_metadata_registration.
            has_value(),
        "Mutable-metadata importer registers successfully");

    mutable_metadata_pointer->clear_name();

    AssetImportPipeline
        mutable_metadata_pipeline{
            mutable_metadata_registry
        };

    const Result<ImportedAsset>
        invalid_metadata_import =
            mutable_metadata_pipeline.import(
                ImportRequest{
                    AssetId{
                        6ULL,
                        9ULL
                    },
                    "data/test.mutable",
                    {}
                });

    check(
        state,
        !invalid_metadata_import.has_value(),
        "Pipeline rejects invalid importer metadata");

    check(
        state,
        !invalid_metadata_import.has_value() &&
            invalid_metadata_import.
                error().code ==
                ErrorCode::invalid_state,
        "Invalid importer metadata reports invalid_state");

    std::cout
        << "\nAsset import pipeline test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}