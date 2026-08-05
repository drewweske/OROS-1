#include "oros/assets/asset_importer_registry.hpp"

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

    class TestImporter final
        : public oros::assets::AssetImporter
    {
    public:
        TestImporter(
            std::string importer_name,
            std::uint32_t importer_version,
            std::uint32_t output_schema_version,
            std::string supported_suffix,
            const oros::assets::AssetId dependency,
            int* destruction_count = nullptr)
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
              dependency_{
                  dependency
              },
              destruction_count_{
                  destruction_count
              }
        {
        }

        ~TestImporter() override
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
                    "The importer does not support "
                    "the source path.");
            }

            ImportResult result{};

            result.intermediate_bytes.assign(
                request.source_bytes.begin(),
                request.source_bytes.end());

            if (dependency_.is_valid())
            {
                result.dependencies.push_back(
                    dependency_);
            }

            return result;
        }

    private:
        std::string name_{};
        std::uint32_t version_{};
        std::uint32_t schema_version_{};
        std::string suffix_{};
        oros::assets::AssetId dependency_{};
        int* destruction_count_{};
    };

    static_assert(
        !std::is_copy_constructible_v<
            oros::assets::
                AssetImporterRegistry>);

    static_assert(
        !std::is_copy_assignable_v<
            oros::assets::
                AssetImporterRegistry>);

    static_assert(
        std::is_nothrow_move_constructible_v<
            oros::assets::
                AssetImporterRegistry>);

    static_assert(
        std::is_nothrow_move_assignable_v<
            oros::assets::
                AssetImporterRegistry>);
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    AssetImporterRegistry registry{};

    check(
        state,
        registry.empty(),
        "New importer registry is empty");

    check(
        state,
        registry.size() == 0U,
        "New importer registry has zero importers");

    check(
        state,
        registry.find(
            "oros.texture") ==
            nullptr,
        "Empty registry name lookup returns null");

    const Result<const AssetImporter*>
        empty_path_resolution =
            registry.resolve(
                "");

    check(
        state,
        !empty_path_resolution.has_value(),
        "Registry rejects resolution of an empty path");

    check(
        state,
        !empty_path_resolution.has_value() &&
            empty_path_resolution.error().code ==
                ErrorCode::invalid_argument,
        "Empty path resolution reports invalid_argument");

    const Result<const AssetImporter*>
        empty_registry_resolution =
            registry.resolve(
                "textures/rock.texture");

    check(
        state,
        !empty_registry_resolution.has_value(),
        "Empty registry cannot resolve an importer");

    check(
        state,
        !empty_registry_resolution.has_value() &&
            empty_registry_resolution.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Missing importer reports unsupported_operation");

    const Status null_registration =
        registry.register_importer(
            nullptr);

    check(
        state,
        !null_registration.has_value(),
        "Registry rejects a null importer");

    check(
        state,
        !null_registration.has_value() &&
            null_registration.error().code ==
                ErrorCode::invalid_argument,
        "Null importer reports invalid_argument");

    check(
        state,
        registry.empty(),
        "Rejected null importer does not mutate the registry");

    int invalid_destructions{};

    const Status empty_name_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "",
                1U,
                1U,
                ".empty",
                invalid_asset_id,
                &invalid_destructions));

    check(
        state,
        !empty_name_registration.has_value(),
        "Registry rejects an importer with an empty name");

    check(
        state,
        !empty_name_registration.has_value() &&
            empty_name_registration.error().code ==
                ErrorCode::invalid_argument,
        "Empty importer name reports invalid_argument");

    check(
        state,
        invalid_destructions == 1,
        "Rejected empty-name importer is destroyed");

    const Status zero_version_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.zero_version",
                0U,
                1U,
                ".zero-version",
                invalid_asset_id,
                &invalid_destructions));

    check(
        state,
        !zero_version_registration.has_value(),
        "Registry rejects importer version zero");

    check(
        state,
        !zero_version_registration.has_value() &&
            zero_version_registration.error().code ==
                ErrorCode::invalid_argument,
        "Zero importer version reports invalid_argument");

    check(
        state,
        invalid_destructions == 2,
        "Rejected zero-version importer is destroyed");

    const Status zero_schema_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.zero_schema",
                1U,
                0U,
                ".zero-schema",
                invalid_asset_id,
                &invalid_destructions));

    check(
        state,
        !zero_schema_registration.has_value(),
        "Registry rejects schema version zero");

    check(
        state,
        !zero_schema_registration.has_value() &&
            zero_schema_registration.error().code ==
                ErrorCode::invalid_argument,
        "Zero schema version reports invalid_argument");

    check(
        state,
        invalid_destructions == 3,
        "Rejected zero-schema importer is destroyed");

    check(
        state,
        registry.empty(),
        "Rejected invalid importers leave the registry empty");

    int texture_destructions{};

    const AssetId texture_dependency{
        10ULL,
        1ULL
    };

    const Status texture_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.texture",
                4U,
                2U,
                ".texture",
                texture_dependency,
                &texture_destructions));

    check(
        state,
        texture_registration.has_value(),
        "Registry accepts a valid importer");

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
        texture_destructions == 0,
        "Registered importer remains alive");

    const AssetImporter* texture_importer =
        registry.find(
            "oros.texture");

    check(
        state,
        texture_importer != nullptr,
        "Name lookup finds the registered importer");

    check(
        state,
        texture_importer != nullptr &&
            texture_importer->name() ==
                "oros.texture",
        "Name lookup preserves importer identity");

    check(
        state,
        texture_importer != nullptr &&
            texture_importer->version() ==
                4U,
        "Name lookup preserves importer version");

    check(
        state,
        texture_importer != nullptr &&
            texture_importer->
                schema_version() == 2U,
        "Name lookup preserves schema version");

    check(
        state,
        registry.find(
            "OROS.TEXTURE") ==
            nullptr,
        "Importer name lookup is case-sensitive");

    check(
        state,
        registry.find(
            "") ==
            nullptr,
        "Empty importer name lookup returns null");

    const Result<const AssetImporter*>
        texture_resolution =
            registry.resolve(
                "textures/terrain/rock.texture");

    check(
        state,
        texture_resolution.has_value(),
        "Registry resolves a supported source path");

    check(
        state,
        texture_resolution.has_value() &&
            texture_resolution.value() ==
                texture_importer,
        "Resolution returns the registered importer");

    check(
        state,
        texture_resolution.has_value() &&
            texture_resolution.value()->
                name() ==
                "oros.texture",
        "Resolved importer preserves virtual metadata");

    const Result<const AssetImporter*>
        case_mismatched_resolution =
            registry.resolve(
                "textures/terrain/rock.TEXTURE");

    check(
        state,
        !case_mismatched_resolution.has_value(),
        "Resolution follows importer case policy");

    check(
        state,
        !case_mismatched_resolution.has_value() &&
            case_mismatched_resolution.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported case reports unsupported_operation");

    const std::array<std::byte, 3U>
        texture_source{
            std::byte{0x11},
            std::byte{0x22},
            std::byte{0x33}
        };

    const ImportRequest texture_request{
        AssetId{
            20ULL,
            1ULL
        },
        "textures/terrain/rock.texture",
        std::span<const std::byte>{
            texture_source
        }
    };

    Result<ImportResult>
        resolved_import_result =
            texture_resolution.has_value()
                ? texture_resolution.value()->
                    import(
                        texture_request)
                : fail(
                    ErrorCode::
                        internal_failure,
                    "The texture importer was not "
                    "resolved.");

    check(
        state,
        resolved_import_result.has_value(),
        "Resolved importer remains operational");

    check(
        state,
        resolved_import_result.has_value() &&
            resolved_import_result.value().
                intermediate_bytes.size() ==
                texture_source.size(),
        "Resolved importer preserves source byte count");

    check(
        state,
        resolved_import_result.has_value() &&
            resolved_import_result.value().
                dependencies.size() == 1U,
        "Resolved importer reports its dependency");

    check(
        state,
        resolved_import_result.has_value() &&
            resolved_import_result.value().
                dependencies.front() ==
                texture_dependency,
        "Resolved importer preserves dependency identity");

    int duplicate_destructions{};

    const Status duplicate_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.texture",
                99U,
                99U,
                ".duplicate",
                invalid_asset_id,
                &duplicate_destructions));

    check(
        state,
        !duplicate_registration.has_value(),
        "Registry rejects a duplicate importer name");

    check(
        state,
        !duplicate_registration.has_value() &&
            duplicate_registration.error().code ==
                ErrorCode::invalid_state,
        "Duplicate importer name reports invalid_state");

    check(
        state,
        duplicate_destructions == 1,
        "Rejected duplicate importer is destroyed");

    check(
        state,
        registry.size() == 1U,
        "Duplicate registration does not change registry size");

    check(
        state,
        registry.find(
            "oros.texture") ==
            texture_importer,
        "Duplicate registration preserves the original importer");

    int material_destructions{};

    const AssetId material_dependency{
        10ULL,
        2ULL
    };

    const Status material_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.material",
                6U,
                5U,
                ".material",
                material_dependency,
                &material_destructions));

    check(
        state,
        material_registration.has_value(),
        "Registry accepts a second importer");

    check(
        state,
        registry.size() == 2U,
        "Second registration increases registry size");

    const AssetImporter* material_importer =
        registry.find(
            "oros.material");

    check(
        state,
        material_importer != nullptr,
        "Name lookup finds the second importer");

    check(
        state,
        material_importer != nullptr &&
            material_importer->version() ==
                6U &&
            material_importer->
                schema_version() == 5U,
        "Second importer preserves version metadata");

    const Result<const AssetImporter*>
        material_resolution =
            registry.resolve(
                "materials/terrain/rock.material");

    check(
        state,
        material_resolution.has_value(),
        "Registry resolves the second importer");

    check(
        state,
        material_resolution.has_value() &&
            material_resolution.value() ==
                material_importer,
        "Second path resolves to the second importer");

    check(
        state,
        registry.resolve(
            "textures/terrain/rock.texture").
            has_value(),
        "Second registration preserves first resolution");

    const Result<const AssetImporter*>
        unsupported_resolution =
            registry.resolve(
                "audio/ambient/wind.sound");

    check(
        state,
        !unsupported_resolution.has_value(),
        "Registry rejects an unsupported source path");

    check(
        state,
        !unsupported_resolution.has_value() &&
            unsupported_resolution.error().code ==
                ErrorCode::
                    unsupported_operation,
        "Unsupported source path reports unsupported_operation");

    int broad_destructions{};

    const Status broad_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.broad",
                1U,
                1U,
                "rock.material",
                invalid_asset_id,
                &broad_destructions));

    check(
        state,
        broad_registration.has_value(),
        "Registry accepts an overlapping importer");

    check(
        state,
        registry.size() == 3U,
        "Overlapping registration increases registry size");

    const Result<const AssetImporter*>
        ambiguous_resolution =
            registry.resolve(
                "materials/terrain/rock.material");

    check(
        state,
        !ambiguous_resolution.has_value(),
        "Registry rejects ambiguous path resolution");

    check(
        state,
        !ambiguous_resolution.has_value() &&
            ambiguous_resolution.error().code ==
                ErrorCode::invalid_state,
        "Ambiguous resolution reports invalid_state");

    check(
        state,
        registry.find(
            "oros.material") ==
            material_importer,
        "Ambiguous resolution does not alter existing importers");

    check(
        state,
        registry.find(
            "oros.broad") !=
            nullptr,
        "Overlapping importer remains registered");

    const Status empty_unregister =
        registry.unregister_importer(
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
        registry.unregister_importer(
            "oros.missing");

    check(
        state,
        !missing_unregister.has_value(),
        "Registry rejects unregistering a missing importer");

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
        registry.unregister_importer(
            "oros.material");

    check(
        state,
        material_unregister.has_value(),
        "Registry unregisters an importer");

    check(
        state,
        material_destructions == 1,
        "Unregistered importer is destroyed");

    check(
        state,
        registry.size() == 2U,
        "Unregistering decreases registry size");

    check(
        state,
        registry.find(
            "oros.material") ==
            nullptr,
        "Unregistered importer name lookup returns null");

    check(
        state,
        registry.find(
            "oros.texture") ==
            texture_importer,
        "Unregistering preserves an earlier importer");

    const AssetImporter* broad_importer =
        registry.find(
            "oros.broad");

    check(
        state,
        broad_importer != nullptr,
        "Unregistering repairs the shifted name index");

    const Result<const AssetImporter*>
        formerly_ambiguous_resolution =
            registry.resolve(
                "materials/terrain/rock.material");

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
                broad_importer,
        "Resolved prior ambiguity selects the remaining importer");

    const Status repeated_unregister =
        registry.unregister_importer(
            "oros.material");

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

    const Status texture_unregister =
        registry.unregister_importer(
            "oros.texture");

    check(
        state,
        texture_unregister.has_value(),
        "Registry unregisters its first stored importer");

    check(
        state,
        texture_destructions == 1,
        "First stored importer is destroyed after unregister");

    check(
        state,
        registry.size() == 1U,
        "First-element unregister decreases registry size");

    check(
        state,
        registry.find(
            "oros.broad") ==
            broad_importer,
        "First-element unregister repairs remaining indexes");

    const Status texture_reregistration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.texture",
                8U,
                7U,
                ".texture",
                texture_dependency,
                &texture_destructions));

    check(
        state,
        texture_reregistration.has_value(),
        "Registry permits name reuse after unregister");

    check(
        state,
        registry.size() == 2U,
        "Name reuse restores registry size");

    const AssetImporter* replacement_texture =
        registry.find(
            "oros.texture");

    check(
        state,
        replacement_texture != nullptr &&
            replacement_texture->version() ==
                8U &&
            replacement_texture->
                schema_version() == 7U,
        "Re-registered importer exposes replacement metadata");

    AssetImporterRegistry move_constructed{
        std::move(registry)
    };

    check(
        state,
        move_constructed.size() == 2U,
        "Move construction transfers registry size");

    check(
        state,
        move_constructed.find(
            "oros.broad") !=
            nullptr &&
            move_constructed.find(
                "oros.texture") !=
            nullptr,
        "Move construction transfers name lookup");

    check(
        state,
        move_constructed.resolve(
            "textures/rock.texture").
            has_value(),
        "Move construction transfers path resolution");

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
            "oros.texture") ==
            nullptr,
        "Moved-from registry has no name entries");

    const Status moved_from_registration =
        registry.register_importer(
            std::make_unique<TestImporter>(
                "oros.reused",
                1U,
                1U,
                ".reused",
                invalid_asset_id));

    check(
        state,
        moved_from_registration.has_value(),
        "Moved-from registry can be reused");

    check(
        state,
        registry.size() == 1U,
        "Reused moved-from registry stores new importers");

    int discarded_destructions{};

    AssetImporterRegistry
        move_assignment_destination{};

    const Status discarded_registration =
        move_assignment_destination.
            register_importer(
                std::make_unique<TestImporter>(
                    "oros.discarded",
                    1U,
                    1U,
                    ".discarded",
                    invalid_asset_id,
                    &discarded_destructions));

    check(
        state,
        discarded_registration.has_value(),
        "Move-assignment destination begins with an importer");

    move_assignment_destination =
        std::move(
            move_constructed);

    check(
        state,
        discarded_destructions == 1,
        "Move assignment destroys previous destination importers");

    check(
        state,
        move_assignment_destination.size() ==
            2U,
        "Move assignment transfers registry size");

    check(
        state,
        move_assignment_destination.find(
            "oros.discarded") ==
            nullptr,
        "Move assignment removes previous destination indexes");

    check(
        state,
        move_assignment_destination.find(
            "oros.broad") !=
            nullptr &&
            move_assignment_destination.find(
                "oros.texture") !=
            nullptr,
        "Move assignment transfers name lookup");

    check(
        state,
        move_assignment_destination.resolve(
            "textures/rock.texture").
            has_value(),
        "Move assignment transfers path resolution");

    check(
        state,
        move_constructed.empty(),
        "Move assignment empties the source registry");

    const std::size_t
        size_before_self_move =
            move_assignment_destination.size();

    const AssetImporter* const
        texture_before_self_move =
            move_assignment_destination.find(
                "oros.texture");

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
            "oros.texture") ==
            texture_before_self_move,
        "Self move assignment preserves importer ownership");

    check(
        state,
        move_assignment_destination.resolve(
            "textures/rock.texture").
            has_value(),
        "Self move assignment preserves path resolution");

    const Status broad_unregister =
        move_assignment_destination.
            unregister_importer(
                "oros.broad");

    check(
        state,
        broad_unregister.has_value(),
        "Moved registry can unregister an importer");

    check(
        state,
        broad_destructions == 1,
        "Moved importer is destroyed exactly once");

    const Status replacement_texture_unregister =
        move_assignment_destination.
            unregister_importer(
                "oros.texture");

    check(
        state,
        replacement_texture_unregister.
            has_value(),
        "Moved registry unregisters its final importer");

    check(
        state,
        texture_destructions == 2,
        "Re-registered importer is destroyed exactly once");

    check(
        state,
        move_assignment_destination.empty(),
        "Registry becomes empty after final unregister");

    std::cout
        << "\nAsset importer registry test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}