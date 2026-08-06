#include "oros/assets/binary_asset_cooker.hpp"

#include "oros/assets/content_hash.hpp"
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
    oros::assets::AssetRecord
    make_binary_record(
        const std::uint64_t asset_sequence)
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id =
            AssetId{
                0x4F524F53ULL,
                asset_sequence
            };

        record.source_path =
            "assets/data/example.orosbin";

        record.importer_name =
            binary_asset_importer_name;

        record.importer_version =
            binary_asset_importer_version;

        record.schema_version =
            binary_asset_schema_version;

        record.source_hash =
            hash_text(
                "binary asset source");

        return record;
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
                BinaryAssetCooker>);

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                BinaryAssetCooker>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                BinaryAssetCooker>);

    static_assert(
        !std::is_move_constructible_v<
            oros::assets::
                BinaryAssetCooker>);

    static_assert(
        !std::is_move_assignable_v<
            oros::assets::
                BinaryAssetCooker>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    BinaryAssetCooker concrete_cooker{};

    const AssetCooker& cooker =
        concrete_cooker;

    check(
        state,
        binary_asset_cooker_name ==
            "oros.binary",
        "Binary cooker name constant is stable");

    check(
        state,
        binary_asset_cooker_version ==
            1U,
        "Binary cooker version constant is stable");

    check(
        state,
        cooker.name() ==
            binary_asset_cooker_name,
        "Cooker exposes its stable name");

    check(
        state,
        cooker.version() ==
            binary_asset_cooker_version,
        "Cooker exposes its implementation version");

    const AssetRecord valid_record =
        make_binary_record(
            600ULL);

    check(
        state,
        valid_record.is_valid(),
        "Binary asset record fixture is valid");

    check(
        state,
        !valid_record.is_cooked(),
        "Binary asset record fixture is uncooked");

    check(
        state,
        cooker.supports(
            valid_record),
        "Cooker supports a valid binary asset record");

    AssetRecord newer_importer_record =
        valid_record;

    newer_importer_record.importer_version =
        binary_asset_importer_version +
        1U;

    check(
        state,
        newer_importer_record.is_valid(),
        "Different importer-version fixture remains valid");

    check(
        state,
        cooker.supports(
            newer_importer_record),
        "Cooker support is governed by importer identity and schema");

    AssetRecord wrong_importer_record =
        valid_record;

    wrong_importer_record.importer_name =
        "oros.other";

    check(
        state,
        wrong_importer_record.is_valid(),
        "Wrong-importer fixture remains valid");

    check(
        state,
        !cooker.supports(
            wrong_importer_record),
        "Cooker rejects a different importer identity");

    AssetRecord wrong_schema_record =
        valid_record;

    wrong_schema_record.schema_version =
        binary_asset_schema_version +
        1U;

    check(
        state,
        wrong_schema_record.is_valid(),
        "Wrong-schema fixture remains valid");

    check(
        state,
        !cooker.supports(
            wrong_schema_record),
        "Cooker rejects an unsupported binary schema");

    AssetRecord cooked_record =
        valid_record;

    cooked_record.cooked_hash =
        hash_text(
            "already cooked binary asset");

    check(
        state,
        cooked_record.is_valid(),
        "Already-cooked fixture remains structurally valid");

    check(
        state,
        cooked_record.is_cooked(),
        "Already-cooked fixture reports cooked state");

    check(
        state,
        !cooker.supports(
            cooked_record),
        "Cooker rejects an already-cooked record");

    const AssetRecord invalid_record{};

    check(
        state,
        !invalid_record.is_valid(),
        "Default asset record is invalid");

    check(
        state,
        !cooker.supports(
            invalid_record),
        "Cooker rejects an invalid asset record");

    std::array<std::byte, 8U>
        intermediate_bytes{
            std::byte{0x00},
            std::byte{0x11},
            std::byte{0x22},
            std::byte{0x7F},
            std::byte{0x80},
            std::byte{0xAA},
            std::byte{0xEE},
            std::byte{0xFF}
        };

    const CookRequest request{
        valid_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    check(
        state,
        request.is_valid(),
        "Binary cook request fixture is valid");

    const Result<CookResult>
        cook_result =
            cooker.cook(
                request);

    check(
        state,
        cook_result.has_value(),
        "Cooker accepts a valid binary request");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                has_cooked_data(),
        "Non-empty intermediate input produces cooked data");

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                cooked_bytes.size() ==
                intermediate_bytes.size(),
        "Cook preserves the intermediate byte count");

    check(
        state,
        cook_result.has_value() &&
            bytes_equal(
                cook_result.value().
                    cooked_bytes,
                std::span<const std::byte>{
                    intermediate_bytes
                }),
        "Cook preserves every intermediate byte");

    const std::byte
        original_first_byte =
            intermediate_bytes.front();

    intermediate_bytes.front() =
        std::byte{0x55};

    check(
        state,
        cook_result.has_value() &&
            cook_result.value().
                cooked_bytes.front() ==
                original_first_byte,
        "Cooked bytes are owned independently of the input span");

    check(
        state,
        request.intermediate_bytes.front() ==
            std::byte{0x55},
        "Cook request continues to reference intermediate storage");

    const CookRequest
        empty_request{
            make_binary_record(
                601ULL),
            {}
        };

    check(
        state,
        empty_request.is_valid(),
        "Empty binary cook request is valid");

    const Result<CookResult>
        empty_cook_result =
            cooker.cook(
                empty_request);

    check(
        state,
        empty_cook_result.has_value(),
        "Cooker accepts empty intermediate input");

    check(
        state,
        empty_cook_result.has_value() &&
            !empty_cook_result.value().
                has_cooked_data(),
        "Empty intermediate input produces no cooked data");

    check(
        state,
        empty_cook_result.has_value() &&
            empty_cook_result.value().
                cooked_bytes.empty(),
        "Empty binary cook owns an empty payload");

    const CookRequest
        invalid_record_request{
            invalid_record,
            std::span<const std::byte>{
                intermediate_bytes
            }
        };

    check(
        state,
        !invalid_record_request.is_valid(),
        "Invalid-record cook request is invalid");

    check_failure(
        state,
        cooker.cook(
            invalid_record_request),
        ErrorCode::invalid_argument,
        "Cooker rejects an invalid asset record");

    const CookRequest
        cooked_record_request{
            cooked_record,
            std::span<const std::byte>{
                intermediate_bytes
            }
        };

    check(
        state,
        !cooked_record_request.is_valid(),
        "Already-cooked request is invalid");

    check_failure(
        state,
        cooker.cook(
            cooked_record_request),
        ErrorCode::invalid_argument,
        "Cooker rejects an already-cooked request");

    const CookRequest
        wrong_importer_request{
            wrong_importer_record,
            std::span<const std::byte>{
                intermediate_bytes
            }
        };

    check(
        state,
        wrong_importer_request.is_valid(),
        "Wrong-importer request remains structurally valid");

    check_failure(
        state,
        cooker.cook(
            wrong_importer_request),
        ErrorCode::unsupported_operation,
        "Cooker rejects a different importer identity");

    const CookRequest
        wrong_schema_request{
            wrong_schema_record,
            std::span<const std::byte>{
                intermediate_bytes
            }
        };

    check(
        state,
        wrong_schema_request.is_valid(),
        "Wrong-schema request remains structurally valid");

    check_failure(
        state,
        cooker.cook(
            wrong_schema_request),
        ErrorCode::unsupported_operation,
        "Cooker rejects an unsupported binary schema");

    const CookRequest
        newer_importer_request{
            newer_importer_record,
            std::span<const std::byte>{
                intermediate_bytes
            }
        };

    const Result<CookResult>
        newer_importer_result =
            cooker.cook(
                newer_importer_request);

    check(
        state,
        newer_importer_result.has_value(),
        "Compatible schema permits a different importer version");

    check(
        state,
        newer_importer_result.has_value() &&
            bytes_equal(
                newer_importer_result.value().
                    cooked_bytes,
                std::span<const std::byte>{
                    intermediate_bytes
                }),
        "Compatible importer-version cook preserves every byte");

    std::cout
        << "\nBinary asset cooker test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return
        state.failures == 0
            ? 0
            : 1;
}