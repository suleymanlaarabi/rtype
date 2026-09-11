#include "physics.hpp"

namespace engine {

physics::physics(flecs::world &world) { world.module<physics>(); }

} // namespace engine
