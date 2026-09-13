#include "core.hpp"
namespace engine {

void core::import() {
    ecs::component<Timer>();
    ecs::component<DespawnIn>();
    ecs::system("Despawn").each(
        [](ecs::entity entity, DespawnIn &despawn, ecs::res<const DeltaTime> delta) {
            if (despawn.timer.tick(delta->value)) {
                entity.kill();
            }
        }
    );
}

bool Timer::tick(float delta) {
    this->elapsed += delta;
    if (this->duration <= this->elapsed) {
        this->elapsed = 0;
        return true;
    }
    return false;
}

Timer Timer::from_seconds(float seconds) { return Timer{ .elapsed = 0, .duration = seconds }; }

DespawnIn DespawnIn::from_seconds(float seconds) {
    return DespawnIn{ .timer = { .elapsed = 0, .duration = seconds } };
}

} // namespace engine
