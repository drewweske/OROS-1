#include "oros/assets/asset_importer.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
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

    class EchoImporter final
        : public oros::assets::AssetImporter
    {
    public:
        [[nodiscard]]
        std::string_view name()
            const noexcept override
        {
            return "oros.echo";
        }

        [[nodiscard]]
        std::uint32_t version()
            const noexcept override
        {
            return 7U;
        }

        [[nodiscard]]
        std::uint32_t schema_version()
            const noexcept override
        {
            return 3U;
        }

        [[nodiscard]]
        bool supports(
            const std::string_view source_path)
            const noexcept override
        {
            return source_path.ends_with(
                ".echo");
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

            if (!request.is_valid())
            {
                return fail(
                    ErrorCode::invalid_argument,
                    "The import request is invalid.");
            }

            if (!supports(request.source_path))
            {
                return fail(
                    ErrorCode::
                        unsupported_operation,
                    "The source path is not supported.");
            }

            ImportResult result{};

            result.intermediate_bytes.assign(
                request.source_bytes.begin(),
                request.source_bytes.end());

            result.dependencies = {
                AssetId{
                    9ULL,
                    1ULL
                },
                AssetId{
                    9ULL,
                    2ULL
                }
            };

            return result;
        }
    };

    static_assert(
        std::is_abstract_v<
            oros::assets::AssetImporter>);

    static_assert(
        std::has_virtual_destructor_v<
            oros::assets::AssetImporter>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::AssetImporter>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::AssetImporter>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::AssetImporter>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::AssetImporter>);

    static_assert(
        !std::is_copy_constructible_v<
            EchoImporter>);

    static_assert(
        !std::is_move_constructible_v<
            EchoImporter>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};

    const ImportRequest default_request{};

    check(
        state,
        !default_request.is_valid(),
        "Default import request is invalid");

    const AssetId valid_asset{
        4ULL,
        10ULL
    };

    const ImportRequest missing_path{
        valid_asset,
        {},
        {}
    };

    check(
        state,
        !missing_path.is_valid(),
        "Import request rejects an empty source path");

    const ImportRequest missing_asset{
        invalid_asset_id,
        "data/example.echo",
        {}
    };

    check(
        state,
        !missing_asset.is_valid(),
        "Import request rejects an invalid asset identity");

    const ImportRequest empty_source{
        valid_asset,
        "data/empty.echo",
        {}
    };

    check(
        state,
        empty_source.is_valid(),
        "Import request permits an empty source file");

    const std::array<std::byte, 5U>
        source_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40},
            std::byte{0x50}
        };

    const ImportRequest valid_request{
        valid_asset,
        "data/example.echo",
        std::span<const std::byte>{
            source_bytes
        }
    };

    check(
        state,
        valid_request.is_valid(),
        "Complete import request is valid");

    check(
        state,
        valid_request.asset ==
            valid_asset,
        "Import request preserves its asset identity");

    check(
        state,
        valid_request.source_path ==
            "data/example.echo",
        "Import request preserves its source path");

    check(
        state,
        valid_request.source_bytes.size() ==
            source_bytes.size(),
        "Import request preserves its source byte count");

    check(
        state,
        valid_request.source_bytes.data() ==
            source_bytes.data(),
        "Import request references the original source bytes");

    bool request_bytes_match{true};

    for (std::size_t index = 0U;
         index < source_bytes.size();
         ++index)
    {
        if (valid_request.source_bytes[index] !=
            source_bytes[index])
        {
            request_bytes_match = false;
            break;
        }
    }

    check(
        state,
        request_bytes_match,
        "Import request exposes every source byte");

    const ImportResult default_result{};

    check(
        state,
        !default_result.has_intermediate_data(),
        "Default import result has no intermediate data");

    check(
        state,
        default_result.intermediate_bytes.empty(),
        "Default import result has no byte payload");

    check(
        state,
        default_result.dependencies.empty(),
        "Default import result has no dependencies");

    ImportResult populated_result{};

    populated_result.intermediate_bytes = {
        std::byte{0xAA},
        std::byte{0xBB}
    };

    populated_result.dependencies = {
        AssetId{
            5ULL,
            1ULL
        }
    };

    check(
        state,
        populated_result.has_intermediate_data(),
        "Non-empty byte payload reports intermediate data");

    check(
        state,
        populated_result.intermediate_bytes.size() ==
            2U,
        "Import result preserves its byte count");

    check(
        state,
        populated_result.intermediate_bytes[0] ==
            std::byte{0xAA} &&
            populated_result.intermediate_bytes[1] ==
                std::byte{0xBB},
        "Import result preserves its intermediate bytes");

    check(
        state,
        populated_result.dependencies.size() ==
            1U,
        "Import result preserves its dependency count");

    check(
        state,
        populated_result.dependencies.front() ==
            AssetId{
                5ULL,
                1ULL
            },
        "Import result preserves its dependency identity");

    populated_result.intermediate_bytes.clear();

    check(
        state,
        !populated_result.has_intermediate_data(),
        "Clearing bytes removes intermediate-data state");

    check(
        state,
        populated_result.dependencies.size() ==
            1U,
        "Intermediate-data state is independent of dependencies");

    EchoImporter concrete_importer{};

    const AssetImporter& importer =
        concrete_importer;

    check(
        state,
        importer.name() ==
            "oros.echo",
        "Importer exposes its stable name");

    check(
        state,
        importer.version() == 7U,
        "Importer exposes its implementation version");

    check(
        state,
        importer.schema_version() == 3U,
        "Importer exposes its output schema version");

    check(
        state,
        importer.supports(
            "data/example.echo"),
        "Importer accepts its supported extension");

    check(
        state,
        importer.supports(
            ".echo"),
        "Importer accepts a bare supported file name");

    check(
        state,
        !importer.supports(
            "data/example.bin"),
        "Importer rejects an unsupported extension");

    check(
        state,
        !importer.supports(
            "data/example.ECHO"),
        "Importer support matching follows its declared case policy");

    check(
        state,
        !importer.supports(
            ""),
        "Importer rejects an empty source path");

    const Result<ImportResult>
        successful_import =
            importer.import(
                valid_request);

    check(
        state,
        successful_import.has_value(),
        "Importer accepts a valid supported request");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                has_intermediate_data(),
        "Successful import produces intermediate data");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                intermediate_bytes.size() ==
                source_bytes.size(),
        "Successful import preserves the source byte count");

    bool imported_bytes_match{false};

    if (successful_import.has_value())
    {
        const std::vector<std::byte>&
            imported_bytes =
                successful_import.value().
                    intermediate_bytes;

        imported_bytes_match =
            imported_bytes.size() ==
                source_bytes.size();

        if (imported_bytes_match)
        {
            for (std::size_t index = 0U;
                 index < source_bytes.size();
                 ++index)
            {
                if (imported_bytes[index] !=
                    source_bytes[index])
                {
                    imported_bytes_match = false;
                    break;
                }
            }
        }
    }

    check(
        state,
        imported_bytes_match,
        "Successful import preserves every source byte");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                dependencies.size() == 2U,
        "Successful import reports discovered dependencies");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                dependencies[0] ==
                AssetId{
                    9ULL,
                    1ULL
                },
        "Successful import preserves its first dependency");

    check(
        state,
        successful_import.has_value() &&
            successful_import.value().
                dependencies[1] ==
                AssetId{
                    9ULL,
                    2ULL
                },
        "Successful import preserves its second dependency");

    const Result<ImportResult>
        empty_import =
            importer.import(
                empty_source);

    check(
        state,
        empty_import.has_value(),
        "Importer accepts an empty supported source file");

    check(
        state,
        empty_import.has_value() &&
            !empty_import.value().
                has_intermediate_data(),
        "Empty source produces an empty intermediate payload");

    check(
        state,
        empty_import.has_value() &&
            empty_import.value().
                dependencies.size() == 2U,
        "Empty source can still report dependencies");

    const ImportRequest invalid_request{
        invalid_asset_id,
        "data/example.echo",
        std::span<const std::byte>{
            source_bytes
        }
    };

    const Result<ImportResult>
        invalid_import =
            importer.import(
                invalid_request);

    check(
        state,
        !invalid_import.has_value(),
        "Importer rejects an invalid request");

    check(
        state,
        !invalid_import.has_value() &&
            invalid_import.error().code ==
                ErrorCode::invalid_argument,
        "Invalid request reports invalid_argument");

    const ImportRequest unsupported_request{
        valid_asset,
        "data/example.bin",
        std::span<const std::byte>{
            source_bytes
        }
    };

    const Result<ImportResult>
        unsupported_import =
            importer.import(
                unsupported_request);

    check(
        state,
        !unsupported_import.has_value(),
        "Importer rejects an unsupported path");

    check(
        state,
        !unsupported_import.has_value() &&
            unsupported_import.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported path reports unsupported_operation");

    std::unique_ptr<AssetImporter>
        polymorphic_importer =
            std::make_unique<EchoImporter>();

    check(
        state,
        polymorphic_importer != nullptr,
        "Importer can be owned through its interface");

    check(
        state,
        polymorphic_importer->name() ==
            "oros.echo",
        "Owned importer preserves virtual name dispatch");

    check(
        state,
        polymorphic_importer->supports(
            "owned.echo"),
        "Owned importer preserves virtual support dispatch");

    const Result<ImportResult>
        polymorphic_import =
            polymorphic_importer->import(
                valid_request);

    check(
        state,
        polymorphic_import.has_value(),
        "Owned importer preserves virtual import dispatch");

    std::cout
        << "\nAsset importer test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}