#include "oros/assets/binary_asset_importer.hpp"

#include "oros/foundation/error.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
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

    static_assert(
        std::is_final_v<
            oros::assets::
                BinaryAssetImporter>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                BinaryAssetImporter>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                BinaryAssetImporter>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                BinaryAssetImporter>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                BinaryAssetImporter>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    BinaryAssetImporter concrete_importer{};

    const AssetImporter& importer =
        concrete_importer;

    check(
        state,
        binary_asset_importer_name ==
            "oros.binary",
        "Binary importer name constant is stable");

    check(
        state,
        binary_asset_importer_version ==
            1U,
        "Binary importer version constant is stable");

    check(
        state,
        binary_asset_schema_version ==
            1U,
        "Binary schema version constant is stable");

    check(
        state,
        binary_asset_source_extension ==
            ".orosbin",
        "Binary source extension constant is stable");

    check(
        state,
        importer.name() ==
            binary_asset_importer_name,
        "Importer exposes its stable name");

    check(
        state,
        importer.version() ==
            binary_asset_importer_version,
        "Importer exposes its implementation version");

    check(
        state,
        importer.schema_version() ==
            binary_asset_schema_version,
        "Importer exposes its schema version");

    check(
        state,
        importer.supports(
            "assets/data/example.orosbin"),
        "Importer supports a lowercase binary path");

    check(
        state,
        importer.supports(
            "assets/data/example.OROSBIN"),
        "Importer supports an uppercase binary extension");

    check(
        state,
        importer.supports(
            "assets/data/example.OrOsBiN"),
        "Importer supports a mixed-case binary extension");

    check(
        state,
        importer.supports(
            ".orosbin"),
        "Importer supports a bare binary extension");

    check(
        state,
        importer.supports(
            "example.with.dots.orosbin"),
        "Importer supports paths containing additional dots");

    check(
        state,
        importer.supports(
            "folder\\example.orosbin"),
        "Importer supports Windows-style path separators");

    check(
        state,
        !importer.supports(
            ""),
        "Importer rejects an empty path");

    check(
        state,
        !importer.supports(
            "assets/data/example"),
        "Importer rejects a path without an extension");

    check(
        state,
        !importer.supports(
            "assets/data/example.bin"),
        "Importer rejects an unrelated extension");

    check(
        state,
        !importer.supports(
            "assets/data/example.orosbinary"),
        "Importer rejects a longer extension sharing its prefix");

    check(
        state,
        !importer.supports(
            "assets/data/orosbin"),
        "Importer requires the extension separator");

    check(
        state,
        !importer.supports(
            "assets/data/example.orosbin.backup"),
        "Importer rejects trailing path characters");

    const AssetId asset{
        0x4F524F53ULL,
        500ULL
    };

    std::array<std::byte, 8U>
        source_bytes{
            std::byte{0x00},
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x7F},
            std::byte{0x80},
            std::byte{0xA0},
            std::byte{0xF0},
            std::byte{0xFF}
        };

    const ImportRequest request{
        asset,
        "assets/data/example.orosbin",
        std::span<const std::byte>{
            source_bytes
        }
    };

    check(
        state,
        request.is_valid(),
        "Binary import fixture is valid");

    const Result<ImportResult>
        import_result =
            importer.import(
                request);

    check(
        state,
        import_result.has_value(),
        "Importer accepts a valid binary request");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                has_intermediate_data(),
        "Non-empty binary source produces intermediate data");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                intermediate_bytes.size() ==
                source_bytes.size(),
        "Import preserves the binary byte count");

    check(
        state,
        import_result.has_value() &&
            bytes_equal(
                import_result.value().
                    intermediate_bytes,
                std::span<const std::byte>{
                    source_bytes
                }),
        "Import preserves every binary source byte");

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                dependencies.empty(),
        "Binary import reports no discovered dependencies");

    const std::byte
        original_first_byte =
            source_bytes.front();

    source_bytes.front() =
        std::byte{0x55};

    check(
        state,
        import_result.has_value() &&
            import_result.value().
                intermediate_bytes.front() ==
                original_first_byte,
        "Imported bytes are owned independently of the source span");

    check(
        state,
        request.source_bytes.front() ==
            std::byte{0x55},
        "Import request continues to reference the source storage");

    const ImportRequest
        empty_source_request{
            AssetId{
                0x4F524F53ULL,
                501ULL
            },
            "assets/data/empty.OROSBIN",
            {}
        };

    check(
        state,
        empty_source_request.is_valid(),
        "Empty binary source request is valid");

    const Result<ImportResult>
        empty_import_result =
            importer.import(
                empty_source_request);

    check(
        state,
        empty_import_result.has_value(),
        "Importer accepts an empty binary source");

    check(
        state,
        empty_import_result.has_value() &&
            !empty_import_result.value().
                has_intermediate_data(),
        "Empty binary source produces no intermediate bytes");

    check(
        state,
        empty_import_result.has_value() &&
            empty_import_result.value().
                intermediate_bytes.empty(),
        "Empty binary import owns an empty payload");

    check(
        state,
        empty_import_result.has_value() &&
            empty_import_result.value().
                dependencies.empty(),
        "Empty binary import reports no dependencies");

    const ImportRequest
        invalid_asset_request{
            invalid_asset_id,
            "assets/data/example.orosbin",
            std::span<const std::byte>{
                source_bytes
            }
        };

    check_failure(
        state,
        importer.import(
            invalid_asset_request),
        ErrorCode::invalid_argument,
        "Importer rejects an invalid asset identity");

    const ImportRequest
        empty_path_request{
            asset,
            {},
            std::span<const std::byte>{
                source_bytes
            }
        };

    check_failure(
        state,
        importer.import(
            empty_path_request),
        ErrorCode::invalid_argument,
        "Importer rejects an empty source path");

    const ImportRequest
        unsupported_request{
            asset,
            "assets/data/example.bin",
            std::span<const std::byte>{
                source_bytes
            }
        };

    check(
        state,
        unsupported_request.is_valid(),
        "Unsupported-path fixture remains structurally valid");

    check_failure(
        state,
        importer.import(
            unsupported_request),
        ErrorCode::unsupported_operation,
        "Importer rejects an unsupported extension");

    const ImportRequest
        trailing_extension_request{
            asset,
            "assets/data/example.orosbin.backup",
            std::span<const std::byte>{
                source_bytes
            }
        };

    check_failure(
        state,
        importer.import(
            trailing_extension_request),
        ErrorCode::unsupported_operation,
        "Importer rejects trailing extension characters");

    const ImportRequest
        uppercase_request{
            AssetId{
                0x4F524F53ULL,
                502ULL
            },
            "assets/data/uppercase.OROSBIN",
            std::span<const std::byte>{
                source_bytes
            }
        };

    const Result<ImportResult>
        uppercase_import_result =
            importer.import(
                uppercase_request);

    check(
        state,
        uppercase_import_result.has_value(),
        "Case-insensitive support policy is used during import");

    check(
        state,
        uppercase_import_result.has_value() &&
            bytes_equal(
                uppercase_import_result.value().
                    intermediate_bytes,
                std::span<const std::byte>{
                    source_bytes
                }),
        "Uppercase-extension import preserves every byte");

    std::cout
        << "\nBinary asset importer test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}