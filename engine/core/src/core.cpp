#include "core.hpp"
#include "siecs.h"

namespace engine {

void core::import() {
    ecs::component<Timer>();
    ecs::component<DespawnIn>();
    ecs::component<DisableFor>().with<Disabled>();

    ecs::system().each([](ecs::entity entity, DespawnIn &despawn, ecs::res<const DeltaTime> delta) {
        if (despawn.timer.tick(delta->value)) {
            entity.kill();
        }
    });

    ecs::system().require<Disabled>().each(
        [](ecs::entity entity, DisableFor &despawn, ecs::res<const DeltaTime> delta) {
            if (despawn.timer.tick(delta->value)) {
                entity.enable().remove<DisableFor>();
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

Timer Timer::seconds(float seconds) { return Timer{ .elapsed = 0, .duration = seconds }; }

DisableFor DisableFor::seconds(float seconds) {
    return DisableFor{ .timer = { .elapsed = 0, .duration = seconds } };
}

DespawnIn DespawnIn::seconds(float seconds) {
    return DespawnIn{ .timer = { .elapsed = 0, .duration = seconds } };
}

} // namespace engine
