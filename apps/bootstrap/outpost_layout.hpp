#pragma once

#include <array>
#include <cstdint>

namespace oros::bootstrap
{
    // Bootstrap-authored building layout relative to the player's
    // initial WorldPosition. Rendering and world-cell collision must
    // read from this same description, not duplicate coordinates.
    //
    // The floor World entity owns these static cell collider slots;
    // distinct persistent building entities are future work.
    struct OutpostBuildingDescription final
    {
        double center_x{};
        double center_z{};
        double width{};
        double depth{};
        double height{};
        float red{};
        float green{};
        float blue{};
    };

    inline constexpr std::array<
        OutpostBuildingDescription, 3U>
        outpost_buildings{{
            {-12.0, 25.0, 8.0, 8.0, 8.0,
             1.0F, 0.66F, 0.23F},
            {12.0, 29.0, 10.0, 7.0, 6.0,
             0.69F, 0.44F, 1.0F},
            {0.0, 47.0, 7.0, 7.0, 12.0,
             0.34F, 0.94F, 0.72F}
        }};

    // Slot 1 belongs to the floor; these three static building
    // colliders occupy the next canonical, nonoverlapping slots.
    inline constexpr std::uint32_t
        first_outpost_collider_slot{2U};
}
