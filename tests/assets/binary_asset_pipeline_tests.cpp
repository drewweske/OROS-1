#include "oros/assets/asset_cook_pipeline.hpp"
#include "oros/assets/asset_cooker_registry.hpp"
#include "oros/assets/asset_import_pipeline.hpp"
#include "oros/assets/asset_importer_registry.hpp"
#include "oros/assets/binary_asset_cooker.hpp"
#include "oros/assets/binary_asset_importer.hpp"
#include "oros/assets/content_hash.hpp"
#include "oros/foundation/error.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
#include <string_view>
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

    template <typename T>
    void check_failure(
        TestState& state,
        const oros::foundation::Result<T>& result,
        const oros::foundation::ErrorCode
            expected_code,
        const std::string_view name)
    {
        check(
            state,
            !result.has_value() &&
                result.error().code ==
                    expected_code,
            name);
    }

    [[nodiscard]]
    bool bytes_equal(
        const std::vector<std::byte>& left,
        const std::span<const std::byte>
            right) noexcept
    {
        if (left.size() !=
            right.size())
        {
            return false;
        }

        for (std::size_t index = 0U;
             index < left.size();
             ++index)
        {
            if (left[index] !=
                right[index])
            {
                return false;
            }
        }

        return true;
    }
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    AssetImporterRegistry importer_registry{};
    AssetCookerRegistry cooker_registry{};

    check(
        state,
        importer_registry.empty(),
        "Importer registry begins empty");

    check(
        state,
        cooker_registry.empty(),
        "Cooker registry begins empty");

    const Status importer_registration =
        importer_registry.register_importer(
            std::make_unique<
                BinaryAssetImporter>());

    check(
        state,
        importer_registration.has_value(),
        "Binary importer registers successfully");

    const Status cooker_registration =
        cooker_registry.register_cooker(
            std::make_unique<
                BinaryAssetCooker>());

    check(
        state,
        cooker_registration.has_value(),
        "Binary cooker registers successfully");

    check(
        state,
        importer_registry.size() == 1U,
        "Importer registry owns one importer");

    check(
        state,
        cooker_registry.size() == 1U,
        "Cooker registry owns one cooker");

    const AssetImporter*
        registered_importer =
            importer_registry.find(
                binary_asset_importer_name);

    const AssetCooker*
        registered_cooker =
            cooker_registry.find(
                binary_asset_cooker_name);

    check(
        state,
        registered_importer != nullptr,
        "Binary importer is indexed by stable name");

    check(
        state,
        registered_cooker != nullptr,
        "Binary cooker is indexed by stable name");

    check(
        state,
        registered_importer != nullptr &&
            registered_importer->version() ==
                binary_asset_importer_version,
        "Registered importer preserves its version");

    check(
        state,
        registered_importer != nullptr &&
            registered_importer->
                schema_version() ==
                binary_asset_schema_version,
        "Registered importer preserves its schema version");

    check(
        state,
        registered_cooker != nullptr &&
            registered_cooker->version() ==
                binary_asset_cooker_version,
        "Registered cooker preserves its version");

    AssetImportPipeline import_pipeline{
        importer_registry
    };

    AssetCookPipeline cook_pipeline{
        cooker_registry
    };

    const std::array<std::byte, 12U>
        expected_source_bytes{
            std::byte{0x00},
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x7F},
            std::byte{0x80},
            std::byte{0x90},
            std::byte{0xA0},
            std::byte{0xC0},
            std::byte{0xE0},
            std::byte{0xFF}
        };

    std::array<std::byte, 12U>
        source_bytes =
            expected_source_bytes;

    const AssetId asset{
        0x4F524F53ULL,
        700ULL
    };

    const ImportRequest request{
        asset,
        "assets/data/integration.OROSBIN",
        std::span<const std::byte>{
            source_bytes
        }
    };

    check(
        state,
        request.is_valid(),
        "End-to-end binary import request is valid");

    Result<ImportedAsset> import_result =
        import_pipeline.import(
            request);

    check(
        state,
        import_result.has_value(),
        "Import pipeline resolves the binary importer");

    check(
        state,
        import_result.has_value() &&
            import_result.value().is_valid(),
        "Import pipeline produces a valid imported asset");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                has_intermediate_data(),
        "Imported binary asset contains intermediate data");

    check(
        state,
        import_result.has_value() &&
            import_result.value().record.id ==
                asset,
        "Imported record preserves the asset identity");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.source_path ==
                request.source_path,
        "Imported record preserves the source path");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.importer_name ==
                binary_asset_importer_name,
        "Imported record identifies the binary importer");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.importer_version ==
                binary_asset_importer_version,
        "Imported record preserves the importer version");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.schema_version ==
                binary_asset_schema_version,
        "Imported record preserves the binary schema version");

    const ContentHash expected_source_hash =
        hash_bytes(
            std::span<const std::byte>{
                expected_source_bytes
            });

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.source_hash ==
                expected_source_hash,
        "Import pipeline hashes the original source bytes");

    check(
        state,
        import_result.has_value() &&
            !import_result.value().
                record.is_cooked(),
        "Imported record remains uncooked");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                record.dependencies.empty(),
        "Binary imported record has no dependencies");

    check(
        state,
        import_result.has_value() &&
            bytes_equal(
                import_result.value().
                    intermediate_bytes,
                std::span<const std::byte>{
                    expected_source_bytes
                }),
        "Import pipeline preserves every binary source byte");

    source_bytes.front() =
        std::byte{0x55};

    check(
        state,
        request.source_bytes.front() ==
            std::byte{0x55},
        "Import request continues to reference source storage");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                intermediate_bytes.front() ==
                expected_source_bytes.front(),
        "Imported asset owns bytes independently of source storage");

    Result<CookedAsset> cook_result =
        import_result.has_value()
            ? cook_pipeline.cook(
                  import_result.value())
            : Result<CookedAsset>{
                  oros::foundation::fail(
                      ErrorCode::internal_failure,
                      "The integration import failed.")
              };

    check(
        state,
        cook_result.has_value(),
        "Cook pipeline resolves the binary cooker");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().is_valid(),
        "Cook pipeline produces a valid cooked asset");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                has_cooked_data(),
        "Cooked binary asset contains cooked data");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().record.id ==
                asset,
        "Cooked record preserves the asset identity");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.source_path ==
                request.source_path,
        "Cooked record preserves the source path");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.importer_name ==
                binary_asset_importer_name,
        "Cooked record preserves importer identity");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.importer_version ==
                binary_asset_importer_version,
        "Cooked record preserves importer version");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.schema_version ==
                binary_asset_schema_version,
        "Cooked record preserves schema version");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.source_hash ==
                expected_source_hash,
        "Cooked record preserves the source hash");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.dependencies.empty(),
        "Cooked record preserves the dependency set");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                cooker_name ==
                binary_asset_cooker_name,
        "Cooked asset identifies the binary cooker");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                cooker_version ==
                binary_asset_cooker_version,
        "Cooked asset preserves the cooker version");

    check(
        state,
        cook_result.has_value() &&
            bytes_equal(
                cook_result.value().
                    cooked_bytes,
                std::span<const std::byte>{
                    expected_source_bytes
                }),
        "Cook pipeline preserves every binary byte");

    const ContentHash expected_cooked_hash =
        hash_bytes(
            std::span<const std::byte>{
                expected_source_bytes
            });

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.cooked_hash.has_value(),
        "Cook pipeline records a cooked-content hash");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.cooked_hash ==
                expected_cooked_hash,
        "Cooked-content hash matches the cooked bytes");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                record.source_hash ==
                cook_result.value().
                    record.cooked_hash,
        "Pass-through binary cooking produces matching source and cooked hashes");

    if (import_result.has_value() &&
        !import_result.value().
            intermediate_bytes.empty())
    {
        import_result.value().
            intermediate_bytes[1U] =
                std::byte{0x66};
    }

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                cooked_bytes[1U] ==
                expected_source_bytes[1U],
        "Cooked asset owns bytes independently of intermediate storage");

    const ImportRequest empty_request{
        AssetId{
            0x4F524F53ULL,
            701ULL
        },
        "assets/data/empty.orosbin",
        {}
    };

    const Result<ImportedAsset>
        empty_import_result =
            import_pipeline.import(
                empty_request);

    check(
        state,
        empty_import_result.has_value(),
        "Import pipeline accepts an empty binary source");

    check(
        state,
        empty_import_result.has_value() &&
            empty_import_result.value().
                is_valid(),
        "Empty binary import produces valid metadata");

    check(
        state,
        empty_import_result.has_value() &&
            !empty_import_result.value().
                has_intermediate_data(),
        "Empty binary import has no intermediate data");

    const ContentHash empty_hash =
        hash_bytes(
            std::span<const std::byte>{});

    check(
        state,
        empty_import_result.has_value() &&
            empty_import_result.value().
                record.source_hash ==
                empty_hash,
        "Empty binary source receives the empty-content hash");

    const Result<CookedAsset>
        empty_cook_result =
            empty_import_result.has_value()
                ? cook_pipeline.cook(
                      empty_import_result.value())
                : Result<CookedAsset>{
                      oros::foundation::fail(
                          ErrorCode::internal_failure,
                          "The empty integration import failed.")
                  };

    check(
        state,
        empty_cook_result.has_value(),
        "Cook pipeline accepts empty binary intermediate data");

    check(
        state,
        empty_cook_result.has_value() &&
            empty_cook_result.value().
                is_valid(),
        "Empty binary cook produces valid metadata");

    check(
        state,
        empty_cook_result.has_value() &&
            !empty_cook_result.value().
                has_cooked_data(),
        "Empty binary cook has no cooked payload");

    check(
        state,
        empty_cook_result.has_value() &&
            empty_cook_result.value().
                record.cooked_hash ==
                empty_hash,
        "Empty binary cook records the empty-content hash");

    const ImportRequest unsupported_request{
        AssetId{
            0x4F524F53ULL,
            702ULL
        },
        "assets/data/unsupported.bin",
        std::span<const std::byte>{
            expected_source_bytes
        }
    };

    check_failure(
        state,
        import_pipeline.import(
            unsupported_request),
        ErrorCode::unsupported_operation,
        "Import pipeline rejects an unsupported source extension");

    if (import_result.has_value())
    {
        ImportedAsset wrong_schema_asset =
            import_result.value();

        wrong_schema_asset.record.schema_version =
            binary_asset_schema_version +
            1U;

        check(
            state,
            wrong_schema_asset.is_valid(),
            "Wrong-schema imported fixture remains valid");

        check_failure(
            state,
            cook_pipeline.cook(
                wrong_schema_asset),
            ErrorCode::unsupported_operation,
            "Cook pipeline rejects an unsupported binary schema");
    }
    else
    {
        check(
            state,
            false,
            "Wrong-schema imported fixture remains valid");

        check(
            state,
            false,
            "Cook pipeline rejects an unsupported binary schema");
    }

    std::cout
        << "\nBinary asset pipeline test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}