#include "oros/assets/asset_catalog.hpp"

#include <iostream>
#include <string>
#include <string_view>
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

    [[nodiscard]]
    oros::assets::AssetRecord
    make_record(
        const oros::assets::AssetId id,
        std::string source_path)
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id = id;

        record.source_path =
            std::move(source_path);

        record.importer_name =
            "oros.texture";

        record.importer_version = 1U;
        record.schema_version = 1U;

        record.source_hash =
            hash_text(
                "source data");

        return record;
    }
}

int main()
{
    using namespace oros::assets;
    using namespace oros::foundation;

    TestState state{};
    AssetCatalog catalog{};

    const AssetId first_asset{
        1ULL,
        1ULL
    };

    const AssetId second_asset{
        1ULL,
        2ULL
    };

    const AssetId third_asset{
        1ULL,
        3ULL
    };

    check(
        state,
        catalog.empty(),
        "New asset catalog is empty");

    check(
        state,
        catalog.size() == 0U,
        "New asset catalog has zero records");

    check(
        state,
        catalog.find(
            invalid_asset_id) ==
            nullptr,
        "Invalid asset lookup returns null");

    check(
        state,
        catalog.find_by_source_path(
            "") ==
            nullptr,
        "Empty source-path lookup returns null");

    check(
        state,
        catalog.id_for_source_path(
            "") ==
            invalid_asset_id,
        "Empty path resolves to an invalid asset");

    check(
        state,
        !catalog.contains(
            invalid_asset_id),
        "Catalog does not contain the invalid asset");

    check(
        state,
        !catalog.contains_source_path(
            ""),
        "Catalog does not contain an empty path");

    const Status invalid_record_result =
        catalog.insert(
            AssetRecord{});

    check(
        state,
        !invalid_record_result.has_value(),
        "Catalog rejects an invalid record");

    check(
        state,
        !invalid_record_result.has_value() &&
            invalid_record_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid record insertion reports invalid_argument");

    check(
        state,
        catalog.empty(),
        "Rejected invalid record does not mutate the catalog");

    AssetRecord first_record =
        make_record(
            first_asset,
            "Textures\\Terrain//./Rock.PNG");

    first_record.dependencies = {
        AssetId{
            2ULL,
            1ULL
        }
    };

    const Status first_insert_result =
        catalog.insert(
            first_record);

    check(
        state,
        first_insert_result.has_value(),
        "Catalog inserts its first record");

    check(
        state,
        !catalog.empty(),
        "Catalog is non-empty after insertion");

    check(
        state,
        catalog.size() == 1U,
        "First insertion increases catalog size");

    check(
        state,
        catalog.contains(
            first_asset),
        "Catalog contains the first asset identity");

    check(
        state,
        catalog.contains_source_path(
            "textures/terrain/rock.png"),
        "Catalog contains the normalized source path");

    check(
        state,
        catalog.contains_source_path(
            "TEXTURES\\TERRAIN\\ROCK.PNG"),
        "Source-path lookup ignores ASCII case and separators");

    check(
        state,
        catalog.contains_source_path(
            "textures//terrain/./rock.png"),
        "Source-path lookup removes redundant components");

    const AssetRecord* stored_first =
        catalog.find(
            first_asset);

    check(
        state,
        stored_first != nullptr,
        "Identity lookup finds the first record");

    check(
        state,
        stored_first != nullptr &&
            stored_first->source_path ==
                "textures/terrain/rock.png",
        "Inserted record stores its normalized source path");

    check(
        state,
        stored_first != nullptr &&
            stored_first->id ==
                first_asset,
        "Inserted record preserves its identity");

    check(
        state,
        stored_first != nullptr &&
            stored_first->dependencies ==
                first_record.dependencies,
        "Inserted record preserves its dependencies");

    const AssetRecord* path_first =
        catalog.find_by_source_path(
            "Textures/Terrain/Rock.PNG");

    check(
        state,
        path_first != nullptr &&
            path_first->id ==
                first_asset,
        "Normalized path lookup finds the first record");

    check(
        state,
        catalog.id_for_source_path(
            "Textures\\Terrain\\Rock.PNG") ==
            first_asset,
        "Normalized path resolves to the first identity");

    check(
        state,
        catalog.find(
            second_asset) ==
            nullptr,
        "Missing asset identity lookup returns null");

    check(
        state,
        catalog.find_by_source_path(
            "textures/terrain/missing.png") ==
            nullptr,
        "Missing source-path lookup returns null");

    check(
        state,
        catalog.id_for_source_path(
            "textures/terrain/missing.png") ==
            invalid_asset_id,
        "Missing source path resolves to an invalid identity");

    AssetRecord duplicate_identity =
        make_record(
            first_asset,
            "textures/terrain/sand.png");

    const Status duplicate_identity_result =
        catalog.insert(
            duplicate_identity);

    check(
        state,
        !duplicate_identity_result.has_value(),
        "Catalog rejects a duplicate asset identity");

    check(
        state,
        !duplicate_identity_result.has_value() &&
            duplicate_identity_result.error().code ==
                ErrorCode::invalid_state,
        "Duplicate identity reports invalid_state");

    check(
        state,
        catalog.size() == 1U,
        "Duplicate identity does not change catalog size");

    check(
        state,
        !catalog.contains_source_path(
            "textures/terrain/sand.png"),
        "Duplicate identity does not create a path entry");

    AssetRecord duplicate_path =
        make_record(
            second_asset,
            "TEXTURES\\TERRAIN\\.\\ROCK.PNG");

    const Status duplicate_path_result =
        catalog.insert(
            duplicate_path);

    check(
        state,
        !duplicate_path_result.has_value(),
        "Catalog rejects a duplicate normalized path");

    check(
        state,
        !duplicate_path_result.has_value() &&
            duplicate_path_result.error().code ==
                ErrorCode::invalid_state,
        "Duplicate normalized path reports invalid_state");

    check(
        state,
        catalog.size() == 1U,
        "Duplicate path does not change catalog size");

    check(
        state,
        !catalog.contains(
            second_asset),
        "Duplicate path does not create an identity entry");

    AssetRecord second_record =
        make_record(
            second_asset,
            "materials/terrain/rock.material");

    second_record.importer_name =
        "oros.material";

    second_record.importer_version = 2U;
    second_record.schema_version = 3U;

    const Status second_insert_result =
        catalog.insert(
            second_record);

    check(
        state,
        second_insert_result.has_value(),
        "Catalog inserts a second record");

    check(
        state,
        catalog.size() == 2U,
        "Second insertion increases catalog size");

    check(
        state,
        catalog.contains(
            first_asset) &&
            catalog.contains(
                second_asset),
        "Catalog contains both asset identities");

    check(
        state,
        catalog.contains_source_path(
            "materials/terrain/rock.material"),
        "Catalog contains the second source path");

    AssetRecord invalid_replacement{};

    const Status invalid_replace_result =
        catalog.replace(
            invalid_replacement);

    check(
        state,
        !invalid_replace_result.has_value(),
        "Catalog rejects an invalid replacement record");

    check(
        state,
        !invalid_replace_result.has_value() &&
            invalid_replace_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid replacement reports invalid_argument");

    AssetRecord missing_replacement =
        make_record(
            third_asset,
            "textures/terrain/mud.png");

    const Status missing_replace_result =
        catalog.replace(
            missing_replacement);

    check(
        state,
        !missing_replace_result.has_value(),
        "Catalog rejects replacement of a missing identity");

    check(
        state,
        !missing_replace_result.has_value() &&
            missing_replace_result.error().code ==
                ErrorCode::not_found,
        "Missing replacement reports not_found");

    AssetRecord same_path_replacement =
        make_record(
            first_asset,
            "TEXTURES\\TERRAIN\\.\\ROCK.PNG");

    same_path_replacement.importer_version = 5U;
    same_path_replacement.schema_version = 4U;

    same_path_replacement.source_hash =
        hash_text(
            "updated source data");

    same_path_replacement.cooked_hash =
        hash_text(
            "updated cooked data");

    const Status same_path_replace_result =
        catalog.replace(
            same_path_replacement);

    check(
        state,
        same_path_replace_result.has_value(),
        "Catalog replaces metadata while retaining a path");

    stored_first =
        catalog.find(
            first_asset);

    check(
        state,
        stored_first != nullptr &&
            stored_first->source_path ==
                "textures/terrain/rock.png",
        "Same-path replacement preserves normalized path");

    check(
        state,
        stored_first != nullptr &&
            stored_first->importer_version ==
                5U &&
            stored_first->schema_version ==
                4U,
        "Same-path replacement updates version metadata");

    check(
        state,
        stored_first != nullptr &&
            stored_first->source_hash ==
                hash_text(
                    "updated source data"),
        "Same-path replacement updates the source hash");

    check(
        state,
        stored_first != nullptr &&
            stored_first->cooked_hash.has_value() &&
            stored_first->cooked_hash.value() ==
                hash_text(
                    "updated cooked data"),
        "Same-path replacement updates the cooked hash");

    check(
        state,
        catalog.id_for_source_path(
            "textures/terrain/rock.png") ==
            first_asset,
        "Same-path replacement preserves path indexing");

    AssetRecord changed_path_replacement =
        same_path_replacement;

    changed_path_replacement.source_path =
        "Textures\\Terrain\\Cliff.PNG";

    changed_path_replacement.importer_version =
        6U;

    const Status changed_path_replace_result =
        catalog.replace(
            changed_path_replacement);

    check(
        state,
        changed_path_replace_result.has_value(),
        "Catalog replaces a record with a new path");

    check(
        state,
        !catalog.contains_source_path(
            "textures/terrain/rock.png"),
        "Path replacement removes the previous path index");

    check(
        state,
        catalog.contains_source_path(
            "textures/terrain/cliff.png"),
        "Path replacement creates the new path index");

    check(
        state,
        catalog.id_for_source_path(
            "TEXTURES\\TERRAIN\\CLIFF.PNG") ==
            first_asset,
        "New normalized path resolves to the replaced asset");

    stored_first =
        catalog.find(
            first_asset);

    check(
        state,
        stored_first != nullptr &&
            stored_first->source_path ==
                "textures/terrain/cliff.png",
        "Path replacement stores the new normalized path");

    check(
        state,
        stored_first != nullptr &&
            stored_first->importer_version ==
                6U,
        "Path replacement preserves replacement metadata");

    const AssetRecord first_before_conflict =
        stored_first != nullptr
            ? *stored_first
            : AssetRecord{};

    AssetRecord conflicting_replacement =
        changed_path_replacement;

    conflicting_replacement.source_path =
        "MATERIALS\\TERRAIN\\ROCK.MATERIAL";

    conflicting_replacement.importer_version =
        99U;

    const Status conflict_replace_result =
        catalog.replace(
            conflicting_replacement);

    check(
        state,
        !conflict_replace_result.has_value(),
        "Catalog rejects replacement with an owned path");

    check(
        state,
        !conflict_replace_result.has_value() &&
            conflict_replace_result.error().code ==
                ErrorCode::invalid_state,
        "Replacement path conflict reports invalid_state");

    stored_first =
        catalog.find(
            first_asset);

    check(
        state,
        stored_first != nullptr &&
            *stored_first ==
                first_before_conflict,
        "Rejected path replacement preserves the original record");

    check(
        state,
        catalog.id_for_source_path(
            "textures/terrain/cliff.png") ==
            first_asset,
        "Rejected replacement preserves the original path index");

    check(
        state,
        catalog.id_for_source_path(
            "materials/terrain/rock.material") ==
            second_asset,
        "Rejected replacement preserves the conflicting owner");

    const Status invalid_erase_result =
        catalog.erase(
            invalid_asset_id);

    check(
        state,
        !invalid_erase_result.has_value(),
        "Catalog rejects erasing an invalid identity");

    check(
        state,
        !invalid_erase_result.has_value() &&
            invalid_erase_result.error().code ==
                ErrorCode::invalid_argument,
        "Invalid erase reports invalid_argument");

    const Status missing_erase_result =
        catalog.erase(
            third_asset);

    check(
        state,
        !missing_erase_result.has_value(),
        "Catalog rejects erasing a missing identity");

    check(
        state,
        !missing_erase_result.has_value() &&
            missing_erase_result.error().code ==
                ErrorCode::not_found,
        "Missing erase reports not_found");

    const Status first_erase_result =
        catalog.erase(
            first_asset);

    check(
        state,
        first_erase_result.has_value(),
        "Catalog erases the first asset");

    check(
        state,
        catalog.size() == 1U,
        "Erasing an asset decreases catalog size");

    check(
        state,
        !catalog.contains(
            first_asset),
        "Erased identity is no longer contained");

    check(
        state,
        catalog.find(
            first_asset) ==
            nullptr,
        "Erased identity lookup returns null");

    check(
        state,
        !catalog.contains_source_path(
            "textures/terrain/cliff.png"),
        "Erasing an asset removes its path index");

    check(
        state,
        catalog.id_for_source_path(
            "textures/terrain/cliff.png") ==
            invalid_asset_id,
        "Erased path resolves to an invalid identity");

    check(
        state,
        catalog.contains(
            second_asset),
        "Erasing one asset preserves another identity");

    check(
        state,
        catalog.contains_source_path(
            "materials/terrain/rock.material"),
        "Erasing one asset preserves another path");

    const Status repeated_erase_result =
        catalog.erase(
            first_asset);

    check(
        state,
        !repeated_erase_result.has_value(),
        "Catalog rejects repeated erase");

    check(
        state,
        !repeated_erase_result.has_value() &&
            repeated_erase_result.error().code ==
                ErrorCode::not_found,
        "Repeated erase reports not_found");

    AssetRecord reinserted_first =
        make_record(
            first_asset,
            "textures/terrain/rock.png");

    const Status reinsert_result =
        catalog.insert(
            reinserted_first);

    check(
        state,
        reinsert_result.has_value(),
        "Catalog permits reinsertion after erase");

    check(
        state,
        catalog.size() == 2U,
        "Reinsertion restores catalog size");

    check(
        state,
        catalog.contains(
            first_asset) &&
            catalog.contains_source_path(
                "textures/terrain/rock.png"),
        "Reinserted asset restores both indexes");

    AssetCatalog copy_constructed{
        catalog
    };

    check(
        state,
        copy_constructed.size() ==
            catalog.size(),
        "Copy construction preserves catalog size");

    check(
        state,
        copy_constructed.find(
            first_asset) != nullptr &&
            copy_constructed.find(
                first_asset)->
                source_path ==
                    "textures/terrain/rock.png",
        "Copy construction preserves the first record");

    check(
        state,
        copy_constructed.find(
            second_asset) != nullptr &&
            copy_constructed.find(
                second_asset)->
                source_path ==
                    "materials/terrain/rock.material",
        "Copy construction preserves the second record");

    check(
        state,
        copy_constructed.id_for_source_path(
            "MATERIALS\\TERRAIN\\ROCK.MATERIAL") ==
            second_asset,
        "Copy construction preserves path lookup");

    const Status copy_erase_result =
        copy_constructed.erase(
            first_asset);

    check(
        state,
        copy_erase_result.has_value(),
        "Copied catalog can be modified independently");

    check(
        state,
        !copy_constructed.contains(
            first_asset) &&
            catalog.contains(
                first_asset),
        "Modifying a copy does not change the original catalog");

    AssetCatalog copy_assigned{};

    const AssetId discarded_copy_asset{
        9ULL,
        1ULL
    };

    const Status discarded_copy_insert =
        copy_assigned.insert(
            make_record(
                discarded_copy_asset,
                "temporary/discarded.asset"));

    check(
        state,
        discarded_copy_insert.has_value(),
        "Copy-assignment destination begins with a record");

    copy_assigned =
        catalog;

    check(
        state,
        copy_assigned.size() ==
            catalog.size(),
        "Copy assignment replaces catalog size");

    check(
        state,
        !copy_assigned.contains(
            discarded_copy_asset),
        "Copy assignment discards previous destination records");

    check(
        state,
        copy_assigned.contains(
            first_asset) &&
            copy_assigned.contains(
                second_asset),
        "Copy assignment preserves source identities");

    check(
        state,
        copy_assigned.contains_source_path(
            "textures/terrain/rock.png") &&
            copy_assigned.contains_source_path(
                "materials/terrain/rock.material"),
        "Copy assignment preserves source paths");

    AssetCatalog move_constructed{
        std::move(copy_assigned)
    };

    check(
        state,
        move_constructed.size() == 2U,
        "Move construction transfers catalog size");

    check(
        state,
        move_constructed.contains(
            first_asset) &&
            move_constructed.contains(
                second_asset),
        "Move construction transfers identity lookup");

    check(
        state,
        move_constructed.id_for_source_path(
            "textures/terrain/rock.png") ==
            first_asset &&
            move_constructed.id_for_source_path(
                "materials/terrain/rock.material") ==
            second_asset,
        "Move construction transfers path lookup");

    AssetCatalog move_assignment_source{
        catalog
    };

    AssetCatalog move_assignment_destination{};

    const AssetId discarded_move_asset{
        9ULL,
        2ULL
    };

    const Status discarded_move_insert =
        move_assignment_destination.insert(
            make_record(
                discarded_move_asset,
                "temporary/move-discarded.asset"));

    check(
        state,
        discarded_move_insert.has_value(),
        "Move-assignment destination begins with a record");

    move_assignment_destination =
        std::move(
            move_assignment_source);

    check(
        state,
        move_assignment_destination.size() ==
            2U,
        "Move assignment transfers catalog size");

    check(
        state,
        !move_assignment_destination.contains(
            discarded_move_asset),
        "Move assignment discards previous destination records");

    check(
        state,
        move_assignment_destination.contains(
            first_asset) &&
            move_assignment_destination.contains(
                second_asset),
        "Move assignment transfers identity lookup");

    check(
        state,
        move_assignment_destination.
            id_for_source_path(
                "textures/terrain/rock.png") ==
            first_asset &&
            move_assignment_destination.
                id_for_source_path(
                    "materials/terrain/rock.material") ==
            second_asset,
        "Move assignment transfers path lookup");

    AssetCatalog path_catalog{};

    bool invalid_paths_rejected{true};

    const auto invalid_path_was_rejected =
        [&path_catalog](
            const std::string& path,
            const std::uint64_t sequence)
        {
            const Status result =
                path_catalog.insert(
                    make_record(
                        AssetId{
                            20ULL,
                            sequence
                        },
                        path));

            return
                !result.has_value() &&
                result.error().code ==
                    ErrorCode::invalid_argument;
        };

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "",
            1ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "/textures/rock.png",
            2ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "\\textures\\rock.png",
            3ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "C:\\textures\\rock.png",
            4ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "c:/textures/rock.png",
            5ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "../textures/rock.png",
            6ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "textures/../rock.png",
            7ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            ".",
            8ULL);

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            "././",
            9ULL);

    std::string control_path{
        "textures/rock"
    };

    control_path.push_back(
        static_cast<char>(0x1F));

    control_path += ".png";

    invalid_paths_rejected =
        invalid_paths_rejected &&
        invalid_path_was_rejected(
            control_path,
            10ULL);

    std::uint64_t invalid_character_sequence{
        11ULL
    };

    for (const char invalid_character :
         std::string_view{
             "\"<>|?*:"
         })
    {
        std::string invalid_character_path{
            "textures/rock"
        };

        invalid_character_path.push_back(
            invalid_character);

        invalid_character_path += ".png";

        invalid_paths_rejected =
            invalid_paths_rejected &&
            invalid_path_was_rejected(
                invalid_character_path,
                invalid_character_sequence);

        ++invalid_character_sequence;
    }

    check(
        state,
        invalid_paths_rejected,
        "Catalog rejects every invalid source-path form");

    check(
        state,
        path_catalog.empty(),
        "Rejected paths do not mutate the catalog");

    const AssetId normalized_asset{
        21ULL,
        1ULL
    };

    const Status normalized_insert_result =
        path_catalog.insert(
            make_record(
                normalized_asset,
                "Assets\\\\Textures///./Terrain\\ROCK.PNG/"));

    check(
        state,
        normalized_insert_result.has_value(),
        "Catalog accepts a normalizable relative path");

    const AssetRecord* normalized_record =
        path_catalog.find(
            normalized_asset);

    check(
        state,
        normalized_record != nullptr &&
            normalized_record->source_path ==
                "assets/textures/terrain/rock.png",
        "Catalog removes redundant separators and dot components");

    check(
        state,
        path_catalog.contains_source_path(
            "ASSETS/TEXTURES/TERRAIN/ROCK.PNG"),
        "Canonical lookup accepts an equivalent path");

    check(
        state,
        path_catalog.find_by_source_path(
            "assets/textures/../rock.png") ==
            nullptr,
        "Lookup rejects parent traversal");

    check(
        state,
        path_catalog.id_for_source_path(
            "C:\\assets\\textures\\terrain\\rock.png") ==
            invalid_asset_id,
        "Lookup rejects a drive-prefixed path");

    check(
        state,
        path_catalog.find_by_source_path(
            "/assets/textures/terrain/rock.png") ==
            nullptr,
        "Lookup rejects an absolute path");

    std::cout
        << "\nAsset catalog test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}