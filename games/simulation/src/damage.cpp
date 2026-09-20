#include "components.hpp"
#include "resources.hpp"
#include "simulation.hpp"

namespace simulation {

ecs_system_id_t register_damage(ecs_system_id_t collision_system) {
    const ecs_system_id_t resolve =
        ecs::system("Simulation.ResolveDamage")
            .phase(EcsPostUpdate)
            .after(collision_system)
            .immediate()
            .each([](ecs::res<DamageQueueResource> damage_queue) {
                for (const DamageEvent &event : damage_queue->value->events) {
                    ecs::entity target = ecs::entity::from(event.target);
                    target.get_mut<Health>().current -= event.damage;
                }
                damage_queue->value->events.clear();
            });

    return ecs::system("Simulation.DestroyDeadShips")
        .phase(EcsPostUpdate)
        .after(resolve)
        .immediate()
        .each([](ecs::entity entity,
                 const Ship &,
                 const Team &team,
                 const Health &health,
                 ecs::res<SimulationStats> stats) {
            if (health.current > 0.0f) {
                return;
            }

            --stats->team_alive[team.id];
            ++stats->ships_destroyed;
            entity.kill();
        });
}

} // namespace simulation
