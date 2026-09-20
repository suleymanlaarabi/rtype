#pragma once

#include <siecs.h>

namespace simulation {

struct FighterPrefabs {
    ecs::entity blue;
    ecs::entity red;
};

FighterPrefabs create_fighter_prefabs();
void create_projectile_prefabs();

} // namespace simulation
