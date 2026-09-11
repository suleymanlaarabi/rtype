#include "spatial.hpp"
#include <cstddef>

namespace engine {
spatial::spatial(flecs::world &world) {
    world.module<spatial>();
    world.component<Position>().member<float>("x").member<float>("y");
    world.component<Velocity>().member<float>("x").member<float>("y");

    world.system<Position, const Velocity>().each(
        [](flecs::iter &it, size_t, Position &pos, const Velocity &vel) {
            pos.x += vel.x * it.delta_time();
            pos.y += vel.y * it.delta_time();
        }
    );
}
} // namespace engine
