#include "oros/assets/asset_record.hpp"

#include <iostream>
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
    make_valid_record()
    {
        using namespace oros::assets;

        AssetRecord record{};

        record.id =
            AssetId{
                1ULL,
                10ULL
            };

        record.source_path =
            "textures/terrain/rock.png";

        record.importer_name =
            "oros.texture";

        record.importer_version = 3U;
        record.schema_version = 2U;

        record.source_hash =
            hash_text(
                "source texture data");

        record.dependencies = {
            AssetId{
                1ULL,
                20ULL
            },
            AssetId{
                1ULL,
                21ULL
            }
        };

        return record;
    }
}

int main()
{
    using namespace oros::assets;

    TestState state{};

    const AssetRecord default_record{};

    check(
        state,
        !default_record.is_valid(),
        "Default asset record is invalid");

    check(
        state,
        !default_record.is_cooked(),
        "Default asset record is not cooked");

    AssetRecord valid_record =
        make_valid_record();

    check(
        state,
        valid_record.is_valid(),
        "Complete asset record is valid");

    check(
        state,
        valid_record.id ==
            AssetId{
                1ULL,
                10ULL
            },
        "Asset record preserves its identity");

    check(
        state,
        valid_record.source_path ==
            "textures/terrain/rock.png",
        "Asset record preserves its source path");

    check(
        state,
        valid_record.importer_name ==
            "oros.texture",
        "Asset record preserves its importer name");

    check(
        state,
        valid_record.importer_version == 3U,
        "Asset record preserves its importer version");

    check(
        state,
        valid_record.schema_version == 2U,
        "Asset record preserves its schema version");

    check(
        state,
        valid_record.source_hash ==
            hash_text(
                "source texture data"),
        "Asset record preserves its source hash");

    check(
        state,
        valid_record.dependencies.size() == 2U,
        "Asset record preserves its dependency count");

    check(
        state,
        valid_record.dependencies[0] ==
            AssetId{
                1ULL,
                20ULL
            },
        "Asset record preserves its first dependency");

    check(
        state,
        valid_record.dependencies[1] ==
            AssetId{
                1ULL,
                21ULL
            },
        "Asset record preserves its second dependency");

    check(
        state,
        !valid_record.is_cooked(),
        "Valid uncooked record reports uncooked state");

    valid_record.cooked_hash =
        hash_text(
            "cooked texture data");

    check(
        state,
        valid_record.is_cooked(),
        "Cooked hash marks the record as cooked");

    check(
        state,
        valid_record.is_valid(),
        "Cooked record remains valid");

    check(
        state,
        valid_record.cooked_hash.has_value() &&
            valid_record.cooked_hash.value() ==
                hash_text(
                    "cooked texture data"),
        "Asset record preserves its cooked hash");

    valid_record.cooked_hash.reset();

    check(
        state,
        !valid_record.is_cooked(),
        "Removing the cooked hash restores uncooked state");

    AssetRecord no_dependencies =
        make_valid_record();

    no_dependencies.dependencies.clear();

    check(
        state,
        no_dependencies.is_valid(),
        "Asset record permits no dependencies");

    AssetRecord invalid_identity =
        make_valid_record();

    invalid_identity.id =
        invalid_asset_id;

    check(
        state,
        !invalid_identity.is_valid(),
        "Asset record rejects an invalid identity");

    AssetRecord empty_source_path =
        make_valid_record();

    empty_source_path.source_path.clear();

    check(
        state,
        !empty_source_path.is_valid(),
        "Asset record rejects an empty source path");

    AssetRecord empty_importer =
        make_valid_record();

    empty_importer.importer_name.clear();

    check(
        state,
        !empty_importer.is_valid(),
        "Asset record rejects an empty importer name");

    AssetRecord zero_importer_version =
        make_valid_record();

    zero_importer_version.importer_version = 0U;

    check(
        state,
        !zero_importer_version.is_valid(),
        "Asset record rejects importer version zero");

    AssetRecord zero_schema_version =
        make_valid_record();

    zero_schema_version.schema_version = 0U;

    check(
        state,
        !zero_schema_version.is_valid(),
        "Asset record rejects schema version zero");

    AssetRecord invalid_dependency =
        make_valid_record();

    invalid_dependency.dependencies.push_back(
        invalid_asset_id);

    check(
        state,
        !invalid_dependency.is_valid(),
        "Asset record rejects an invalid dependency");

    AssetRecord self_dependency =
        make_valid_record();

    self_dependency.dependencies.push_back(
        self_dependency.id);

    check(
        state,
        !self_dependency.is_valid(),
        "Asset record rejects a self dependency");

    AssetRecord adjacent_duplicate =
        make_valid_record();

    adjacent_duplicate.dependencies.push_back(
        adjacent_duplicate.dependencies.back());

    check(
        state,
        !adjacent_duplicate.is_valid(),
        "Asset record rejects adjacent duplicate dependencies");

    AssetRecord separated_duplicate =
        make_valid_record();

    separated_duplicate.dependencies.insert(
        separated_duplicate.dependencies.begin() +
            1,
        AssetId{
            1ULL,
            30ULL
        });

    separated_duplicate.dependencies.push_back(
        separated_duplicate.dependencies.front());

    check(
        state,
        !separated_duplicate.is_valid(),
        "Asset record rejects separated duplicate dependencies");

    AssetRecord cross_namespace_dependencies =
        make_valid_record();

    cross_namespace_dependencies.dependencies = {
        AssetId{
            2ULL,
            1ULL
        },
        AssetId{
            3ULL,
            1ULL
        }
    };

    check(
        state,
        cross_namespace_dependencies.is_valid(),
        "Asset record permits dependencies from other catalogs");

    AssetRecord same_sequence_dependencies =
        make_valid_record();

    same_sequence_dependencies.dependencies = {
        AssetId{
            2ULL,
            50ULL
        },
        AssetId{
            3ULL,
            50ULL
        }
    };

    check(
        state,
        same_sequence_dependencies.is_valid(),
        "Different catalogs may share a dependency sequence");

    const AssetRecord equal_record =
        make_valid_record();

    check(
        state,
        equal_record ==
            make_valid_record(),
        "Equivalent asset records compare equal");

    AssetRecord different_identity =
        make_valid_record();

    different_identity.id =
        AssetId{
            1ULL,
            11ULL
        };

    check(
        state,
        different_identity !=
            make_valid_record(),
        "Different asset identities compare unequal");

    AssetRecord different_source_path =
        make_valid_record();

    different_source_path.source_path =
        "textures/terrain/sand.png";

    check(
        state,
        different_source_path !=
            make_valid_record(),
        "Different source paths compare unequal");

    AssetRecord different_importer =
        make_valid_record();

    different_importer.importer_name =
        "oros.image";

    check(
        state,
        different_importer !=
            make_valid_record(),
        "Different importer names compare unequal");

    AssetRecord different_importer_version =
        make_valid_record();

    different_importer_version.importer_version =
        4U;

    check(
        state,
        different_importer_version !=
            make_valid_record(),
        "Different importer versions compare unequal");

    AssetRecord different_schema =
        make_valid_record();

    different_schema.schema_version =
        3U;

    check(
        state,
        different_schema !=
            make_valid_record(),
        "Different schema versions compare unequal");

    AssetRecord different_source_hash =
        make_valid_record();

    different_source_hash.source_hash =
        hash_text(
            "changed source texture data");

    check(
        state,
        different_source_hash !=
            make_valid_record(),
        "Different source hashes compare unequal");

    AssetRecord different_cooked_hash =
        make_valid_record();

    different_cooked_hash.cooked_hash =
        hash_text(
            "cooked texture data");

    check(
        state,
        different_cooked_hash !=
            make_valid_record(),
        "Different cooked hashes compare unequal");

    AssetRecord different_dependencies =
        make_valid_record();

    different_dependencies.dependencies.push_back(
        AssetId{
            1ULL,
            22ULL
        });

    check(
        state,
        different_dependencies !=
            make_valid_record(),
        "Different dependency lists compare unequal");

    AssetRecord copied_record =
        make_valid_record();

    const AssetRecord copy{
        copied_record
    };

    check(
        state,
        copy == copied_record,
        "Asset records are copy constructible");

    AssetRecord assigned_record{};

    assigned_record =
        copied_record;

    check(
        state,
        assigned_record ==
            copied_record,
        "Asset records are copy assignable");

    AssetRecord move_source =
        make_valid_record();

    const AssetId move_identity =
        move_source.id;

    AssetRecord moved_record{
        std::move(move_source)
    };

    check(
        state,
        moved_record.id ==
            move_identity &&
            moved_record.is_valid(),
        "Asset records are move constructible");

    AssetRecord move_assignment_source =
        make_valid_record();

    move_assignment_source.id =
        AssetId{
            5ULL,
            5ULL
        };

    AssetRecord move_assignment_destination{};

    move_assignment_destination =
        std::move(
            move_assignment_source);

    check(
        state,
        move_assignment_destination.id ==
            AssetId{
                5ULL,
                5ULL
            } &&
            move_assignment_destination.is_valid(),
        "Asset records are move assignable");

    std::cout
        << "\nAsset record test summary: "
        << state.checks - state.failures
        << '/'
        << state.checks
        << " passed.\n";

    return state.failures == 0 ? 0 : 1;
}