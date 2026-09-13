#include "spatial.hpp"
namespace engine {

void spatial::import() {
    ecs::component<Position>();
    ecs::component<Velocity>();

    ecs::system("IntegrateVelocity")
        .each([](Position &position, const Velocity &velocity, ecs::res<const DeltaTime> delta) {
            position.x += velocity.x * delta->value;
            position.y += velocity.y * delta->value;
        });
}

} // namespace engine
