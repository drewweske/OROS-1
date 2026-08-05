#include "oros/assets/asset_cooker_registry.hpp"

#include "oros/assets/content_hash.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <span>
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
    oros::assets::AssetRecord
    make_record(
        std::string importer_name,
        const std::uint32_t schema_version,
        const oros::assets::AssetId id)
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id = id;

        record.source_path =
            "data/example.asset";

        record.importer_name =
            std::move(importer_name);

        record.importer_version = 1U;

        record.schema_version =
            schema_version;

        record.source_hash =
            hash_text(
                "source data");

        return record;
    }

    class TestCooker final
        : public oros::assets::AssetCooker
    {
    public:
        TestCooker(
            std::string cooker_name,
            const std::uint32_t cooker_version,
            std::string supported_importer_name,
            const std::uint32_t supported_schema_version,
            int* destruction_count = nullptr)
            : name_{
                  std::move(cooker_name)
              },
              version_{
                  cooker_version
              },
              supported_importer_name_{
                  std::move(
                      supported_importer_name)
              },
              supported_schema_version_{
                  supported_schema_version
              },
              destruction_count_{
                  destruction_count
              }
        {
        }

        ~TestCooker() override
        {
            if (destruction_count_ != nullptr)
            {
                ++(*destruction_count_);
            }
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
                record.importer_name ==
                    supported_importer_name_ &&
                record.schema_version ==
                    supported_schema_version_;
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
                    "The cooker does not support "
                    "the asset record.");
            }

            CookResult result{};

            result.cooked_bytes.assign(
                request.intermediate_bytes.begin(),
                request.intermediate_bytes.end());

            return result;
        }

    private:
        std::string name_{};
        std::uint32_t version_{};

        std::string
            supported_importer_name_{};

        std::uint32_t
            supported_schema_version_{};

        int* destruction_count_{};
    };

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetCookerRegistry>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetCookerRegistry>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            oros::assets::
                AssetCookerRegistry>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            oros::assets::
                AssetCookerRegistry>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    AssetCookerRegistry registry{};

    const AssetRecord echo_record =
        make_record(
            "oros.echo",
            3U,
            AssetId{
                1ULL,
                1ULL
            });

    const AssetRecord material_record =
        make_record(
            "oros.material",
            5U,
            AssetId{
                1ULL,
                2ULL
            });

    const AssetRecord unsupported_record =
        make_record(
            "oros.sound",
            2U,
            AssetId{
                1ULL,
                3ULL
            });

    check(
        state,
        registry.empty(),
        "New cooker registry is empty");

    check(
        state,
        registry.size() == 0U,
        "New cooker registry has zero cookers");

    check(
        state,
        registry.find(
            "oros.echo_cooker") ==
            nullptr,
        "Empty registry name lookup returns null");

    const Result<const AssetCooker*>
        invalid_record_resolution =
            registry.resolve(
                AssetRecord{});

    check(
        state,
        !invalid_record_resolution.has_value(),
        "Registry rejects resolution of invalid metadata");

    check(
        state,
        !invalid_record_resolution.has_value() &&
            invalid_record_resolution.
                error().code ==
                ErrorCode::invalid_argument,
        "Invalid metadata resolution reports invalid_argument");

    AssetRecord already_cooked_record =
        echo_record;

    already_cooked_record.cooked_hash =
        hash_text(
            "existing cooked data");

    const Result<const AssetCooker*>
        cooked_record_resolution =
            registry.resolve(
                already_cooked_record);

    check(
        state,
        !cooked_record_resolution.has_value(),
        "Registry rejects resolution of cooked metadata");

    check(
        state,
        !cooked_record_resolution.has_value() &&
            cooked_record_resolution.
                error().code ==
                ErrorCode::invalid_argument,
        "Cooked metadata resolution reports invalid_argument");

    const Result<const AssetCooker*>
        empty_registry_resolution =
            registry.resolve(
                echo_record);

    check(
        state,
        !empty_registry_resolution.has_value(),
        "Empty registry cannot resolve a cooker");

    check(
        state,
        !empty_registry_resolution.has_value() &&
            empty_registry_resolution.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Missing cooker reports unsupported_operation");

    const Status null_registration =
        registry.register_cooker(
            nullptr);

    check(
        state,
        !null_registration.has_value(),
        "Registry rejects a null cooker");

    check(
        state,
        !null_registration.has_value() &&
            null_registration.error().code ==
                ErrorCode::invalid_argument,
        "Null cooker reports invalid_argument");

    check(
        state,
        registry.empty(),
        "Rejected null cooker does not mutate the registry");

    int invalid_destructions{};

    const Status empty_name_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "",
                1U,
                "oros.empty",
                1U,
                &invalid_destructions));

    check(
        state,
        !empty_name_registration.has_value(),
        "Registry rejects a cooker with an empty name");

    check(
        state,
        !empty_name_registration.has_value() &&
            empty_name_registration.error().code ==
                ErrorCode::invalid_argument,
        "Empty cooker name reports invalid_argument");

    check(
        state,
        invalid_destructions == 1,
        "Rejected empty-name cooker is destroyed");

    const Status zero_version_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.zero_version",
                0U,
                "oros.zero",
                1U,
                &invalid_destructions));

    check(
        state,
        !zero_version_registration.has_value(),
        "Registry rejects cooker version zero");

    check(
        state,
        !zero_version_registration.has_value() &&
            zero_version_registration.error().code ==
                ErrorCode::invalid_argument,
        "Zero cooker version reports invalid_argument");

    check(
        state,
        invalid_destructions == 2,
        "Rejected zero-version cooker is destroyed");

    check(
        state,
        registry.empty(),
        "Rejected invalid cookers leave the registry empty");

    int echo_destructions{};

    const Status echo_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.echo_cooker",
                4U,
                "oros.echo",
                3U,
                &echo_destructions));

    check(
        state,
        echo_registration.has_value(),
        "Registry accepts a valid cooker");

    check(
        state,
        !registry.empty(),
        "Registry is non-empty after registration");

    check(
        state,
        registry.size() == 1U,
        "First registration increases registry size");

    check(
        state,
        echo_destructions == 0,
        "Registered cooker remains alive");

    const AssetCooker* echo_cooker =
        registry.find(
            "oros.echo_cooker");

    check(
        state,
        echo_cooker != nullptr,
        "Name lookup finds the registered cooker");

    check(
        state,
        echo_cooker != nullptr &&
            echo_cooker->name() ==
                "oros.echo_cooker",
        "Name lookup preserves cooker identity");

    check(
        state,
        echo_cooker != nullptr &&
            echo_cooker->version() == 4U,
        "Name lookup preserves cooker version");

    check(
        state,
        registry.find(
            "OROS.ECHO_COOKER") ==
            nullptr,
        "Cooker name lookup is case-sensitive");

    check(
        state,
        registry.find(
            "") ==
            nullptr,
        "Empty cooker name lookup returns null");

    const Result<const AssetCooker*>
        echo_resolution =
            registry.resolve(
                echo_record);

    check(
        state,
        echo_resolution.has_value(),
        "Registry resolves supported metadata");

    check(
        state,
        echo_resolution.has_value() &&
            echo_resolution.value() ==
                echo_cooker,
        "Resolution returns the registered cooker");

    check(
        state,
        echo_resolution.has_value() &&
            echo_resolution.value()->
                name() ==
                "oros.echo_cooker",
        "Resolved cooker preserves virtual metadata");

    const std::array<std::byte, 4U>
        intermediate_bytes{
            std::byte{0x10},
            std::byte{0x20},
            std::byte{0x30},
            std::byte{0x40}
        };

    const CookRequest echo_request{
        echo_record,
        std::span<const std::byte>{
            intermediate_bytes
        }
    };

    Result<CookResult> resolved_cook =
        fail(
            ErrorCode::internal_failure,
            "The echo cooker was not resolved.");

    if (echo_resolution.has_value())
    {
        resolved_cook =
            echo_resolution.value()->
                cook(
                    echo_request);
    }

    check(
        state,
        resolved_cook.has_value(),
        "Resolved cooker remains operational");

    check(
        state,
        resolved_cook.has_value() &&
            resolved_cook.value().
                cooked_bytes.size() ==
                intermediate_bytes.size(),
        "Resolved cooker preserves the byte count");

    bool cooked_bytes_match{false};

    if (resolved_cook.has_value())
    {
        const std::vector<std::byte>&
            cooked_bytes =
                resolved_cook.value().
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
        "Resolved cooker preserves every byte");

    int duplicate_destructions{};

    const Status duplicate_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.echo_cooker",
                99U,
                "oros.other",
                99U,
                &duplicate_destructions));

    check(
        state,
        !duplicate_registration.has_value(),
        "Registry rejects a duplicate cooker name");

    check(
        state,
        !duplicate_registration.has_value() &&
            duplicate_registration.error().code ==
                ErrorCode::invalid_state,
        "Duplicate cooker name reports invalid_state");

    check(
        state,
        duplicate_destructions == 1,
        "Rejected duplicate cooker is destroyed");

    check(
        state,
        registry.size() == 1U,
        "Duplicate registration does not change registry size");

    check(
        state,
        registry.find(
            "oros.echo_cooker") ==
            echo_cooker,
        "Duplicate registration preserves the original cooker");

    int material_destructions{};

    const Status material_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.material_cooker",
                6U,
                "oros.material",
                5U,
                &material_destructions));

    check(
        state,
        material_registration.has_value(),
        "Registry accepts a second cooker");

    check(
        state,
        registry.size() == 2U,
        "Second registration increases registry size");

    const AssetCooker* material_cooker =
        registry.find(
            "oros.material_cooker");

    check(
        state,
        material_cooker != nullptr,
        "Name lookup finds the second cooker");

    check(
        state,
        material_cooker != nullptr &&
            material_cooker->version() ==
                6U,
        "Second cooker preserves version metadata");

    const Result<const AssetCooker*>
        material_resolution =
            registry.resolve(
                material_record);

    check(
        state,
        material_resolution.has_value(),
        "Registry resolves the second cooker");

    check(
        state,
        material_resolution.has_value() &&
            material_resolution.value() ==
                material_cooker,
        "Second record resolves to the second cooker");

    check(
        state,
        registry.resolve(
            echo_record).
            has_value(),
        "Second registration preserves first resolution");

    const Result<const AssetCooker*>
        unsupported_resolution =
            registry.resolve(
                unsupported_record);

    check(
        state,
        !unsupported_resolution.has_value(),
        "Registry rejects unsupported metadata");

    check(
        state,
        !unsupported_resolution.has_value() &&
            unsupported_resolution.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported metadata reports unsupported_operation");

    int broad_destructions{};

    const Status broad_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.broad_material_cooker",
                1U,
                "oros.material",
                5U,
                &broad_destructions));

    check(
        state,
        broad_registration.has_value(),
        "Registry accepts an overlapping cooker");

    check(
        state,
        registry.size() == 3U,
        "Overlapping registration increases registry size");

    const Result<const AssetCooker*>
        ambiguous_resolution =
            registry.resolve(
                material_record);

    check(
        state,
        !ambiguous_resolution.has_value(),
        "Registry rejects ambiguous cooker resolution");

    check(
        state,
        !ambiguous_resolution.has_value() &&
            ambiguous_resolution.error().code ==
                ErrorCode::invalid_state,
        "Ambiguous resolution reports invalid_state");

    check(
        state,
        registry.find(
            "oros.material_cooker") ==
            material_cooker,
        "Ambiguous resolution preserves existing cookers");

    const AssetCooker* broad_cooker =
        registry.find(
            "oros.broad_material_cooker");

    check(
        state,
        broad_cooker != nullptr,
        "Overlapping cooker remains registered");

    const Status empty_unregister =
        registry.unregister_cooker(
            "");

    check(
        state,
        !empty_unregister.has_value(),
        "Registry rejects unregistering an empty name");

    check(
        state,
        !empty_unregister.has_value() &&
            empty_unregister.error().code ==
                ErrorCode::invalid_argument,
        "Empty unregister name reports invalid_argument");

    const Status missing_unregister =
        registry.unregister_cooker(
            "oros.missing");

    check(
        state,
        !missing_unregister.has_value(),
        "Registry rejects unregistering a missing cooker");

    check(
        state,
        !missing_unregister.has_value() &&
            missing_unregister.error().code ==
                ErrorCode::not_found,
        "Missing unregister reports not_found");

    check(
        state,
        registry.size() == 3U,
        "Rejected unregister operations preserve registry size");

    const Status material_unregister =
        registry.unregister_cooker(
            "oros.material_cooker");

    check(
        state,
        material_unregister.has_value(),
        "Registry unregisters a cooker");

    check(
        state,
        material_destructions == 1,
        "Unregistered cooker is destroyed");

    check(
        state,
        registry.size() == 2U,
        "Unregistering decreases registry size");

    check(
        state,
        registry.find(
            "oros.material_cooker") ==
            nullptr,
        "Unregistered cooker name lookup returns null");

    check(
        state,
        registry.find(
            "oros.echo_cooker") ==
            echo_cooker,
        "Unregistering preserves an earlier cooker");

    broad_cooker =
        registry.find(
            "oros.broad_material_cooker");

    check(
        state,
        broad_cooker != nullptr,
        "Unregistering repairs the shifted name index");

    const Result<const AssetCooker*>
        formerly_ambiguous_resolution =
            registry.resolve(
                material_record);

    check(
        state,
        formerly_ambiguous_resolution.
            has_value(),
        "Removing one match resolves prior ambiguity");

    check(
        state,
        formerly_ambiguous_resolution.
            has_value() &&
            formerly_ambiguous_resolution.
                value() ==
                broad_cooker,
        "Prior ambiguity selects the remaining cooker");

    const Status repeated_unregister =
        registry.unregister_cooker(
            "oros.material_cooker");

    check(
        state,
        !repeated_unregister.has_value(),
        "Registry rejects repeated unregister");

    check(
        state,
        !repeated_unregister.has_value() &&
            repeated_unregister.error().code ==
                ErrorCode::not_found,
        "Repeated unregister reports not_found");

    const Status echo_unregister =
        registry.unregister_cooker(
            "oros.echo_cooker");

    check(
        state,
        echo_unregister.has_value(),
        "Registry unregisters its first stored cooker");

    check(
        state,
        echo_destructions == 1,
        "First stored cooker is destroyed after unregister");

    check(
        state,
        registry.size() == 1U,
        "First-element unregister decreases registry size");

    check(
        state,
        registry.find(
            "oros.broad_material_cooker") ==
            broad_cooker,
        "First-element unregister repairs remaining indexes");

    const Status echo_reregistration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.echo_cooker",
                8U,
                "oros.echo",
                3U,
                &echo_destructions));

    check(
        state,
        echo_reregistration.has_value(),
        "Registry permits name reuse after unregister");

    check(
        state,
        registry.size() == 2U,
        "Name reuse restores registry size");

    const AssetCooker* replacement_echo =
        registry.find(
            "oros.echo_cooker");

    check(
        state,
        replacement_echo != nullptr &&
            replacement_echo->version() ==
                8U,
        "Re-registered cooker exposes replacement metadata");

    AssetCookerRegistry move_constructed{
        std::move(registry)
    };

    check(
        state,
        move_constructed.size() == 2U,
        "Move construction transfers registry size");

    check(
        state,
        move_constructed.find(
            "oros.broad_material_cooker") !=
            nullptr &&
            move_constructed.find(
                "oros.echo_cooker") !=
            nullptr,
        "Move construction transfers name lookup");

    check(
        state,
        move_constructed.resolve(
            echo_record).
            has_value(),
        "Move construction transfers cooker resolution");

    check(
        state,
        registry.empty(),
        "Move construction empties the source registry");

    check(
        state,
        registry.size() == 0U,
        "Moved-from registry reports zero size");

    check(
        state,
        registry.find(
            "oros.echo_cooker") ==
            nullptr,
        "Moved-from registry has no name entries");

    const Status moved_from_registration =
        registry.register_cooker(
            std::make_unique<TestCooker>(
                "oros.reused_cooker",
                1U,
                "oros.reused",
                1U));

    check(
        state,
        moved_from_registration.has_value(),
        "Moved-from registry can be reused");

    check(
        state,
        registry.size() == 1U,
        "Reused moved-from registry stores new cookers");

    int discarded_destructions{};

    AssetCookerRegistry
        move_assignment_destination{};

    const Status discarded_registration =
        move_assignment_destination.
            register_cooker(
                std::make_unique<TestCooker>(
                    "oros.discarded_cooker",
                    1U,
                    "oros.discarded",
                    1U,
                    &discarded_destructions));

    check(
        state,
        discarded_registration.has_value(),
        "Move-assignment destination begins with a cooker");

    move_assignment_destination =
        std::move(
            move_constructed);

    check(
        state,
        discarded_destructions == 1,
        "Move assignment destroys previous destination cookers");

    check(
        state,
        move_assignment_destination.size() ==
            2U,
        "Move assignment transfers registry size");

    check(
        state,
        move_assignment_destination.find(
            "oros.discarded_cooker") ==
            nullptr,
        "Move assignment removes previous destination indexes");

    check(
        state,
        move_assignment_destination.find(
            "oros.broad_material_cooker") !=
            nullptr &&
            move_assignment_destination.find(
                "oros.echo_cooker") !=
            nullptr,
        "Move assignment transfers name lookup");

    check(
        state,
        move_assignment_destination.resolve(
            echo_record).
            has_value(),
        "Move assignment transfers cooker resolution");

    check(
        state,
        move_constructed.empty(),
        "Move assignment empties the source registry");

    const std::size_t
        size_before_self_move =
            move_assignment_destination.size();

    const AssetCooker* const
        echo_before_self_move =
            move_assignment_destination.find(
                "oros.echo_cooker");

    move_assignment_destination =
        std::move(
            move_assignment_destination);

    check(
        state,
        move_assignment_destination.size() ==
            size_before_self_move,
        "Self move assignment preserves registry size");

    check(
        state,
        move_assignment_destination.find(
            "oros.echo_cooker") ==
            echo_before_self_move,
        "Self move assignment preserves cooker ownership");

    check(
        state,
        move_assignment_destination.resolve(
            echo_record).
            has_value(),
        "Self move assignment preserves cooker resolution");

    const Status broad_unregister =
        move_assignment_destination.
            unregister_cooker(
                "oros.broad_material_cooker");

    check(
        state,
        broad_unregister.has_value(),
        "Moved registry can unregister a cooker");

    check(
        state,
        broad_destructions == 1,
        "Moved cooker is destroyed exactly once");

    const Status replacement_echo_unregister =
        move_assignment_destination.
            unregister_cooker(
                "oros.echo_cooker");

    check(
        state,
        replacement_echo_unregister.
            has_value(),
        "Moved registry unregisters its final cooker");

    check(
        state,
        echo_destructions == 2,
        "Re-registered cooker is destroyed exactly once");

    check(
        state,
        move_assignment_destination.empty(),
        "Registry becomes empty after final unregister");

    std::cout
        << "\nAsset cooker registry test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}