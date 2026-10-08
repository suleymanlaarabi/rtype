#include "session.hpp"

#include "network.hpp"

#include <optional>

namespace rtype {

namespace {

constexpr float spawn_x = 32.0f;
constexpr float spawn_y = 40.0f;
constexpr float spawn_gap = 48.0f;

void release_session(ecs_entity_t, Session &session) {
    if (Lobby *lobby = ecs::try_resource<Lobby>()) {
        const uint8_t bit = static_cast<uint8_t>(1u << session.slot);
        lobby->occupied = static_cast<uint8_t>(lobby->occupied & static_cast<uint8_t>(~bit));
    }

    ecs::entity player = ecs::entity::from(session.player);
    if (player.is_alive()) {
        player.kill();
    }
}

std::optional<uint8_t> take_slot(Lobby &lobby) {
    for (uint8_t slot = 0; slot < Lobby::capacity; ++slot) {
        const uint8_t bit = static_cast<uint8_t>(1u << slot);
        if ((lobby.occupied & bit) != 0) {
            continue;
        }

        lobby.occupied = static_cast<uint8_t>(lobby.occupied | bit);
        return slot;
    }

    return std::nullopt;
}

} // namespace

void session::import() {
    ecs::component<Player>();
    ecs::component<Position>();
    ecs::component<Health>();
    ecs::component<Session>(
        ecs::component_hooks<Session>{
            .on_remove = release_session,
        }
    );
    ecs::set_resource(Lobby{});

    ecs::system("session::SpawnPlayer")
        .phase(EcsPostUpdate)
        .require<net::Connection>()
        .exclude<Session>()
        .immediate()
        .each([](ecs::entity connection) {
        Lobby &lobby = ecs::resource<Lobby>();
        const std::optional<uint8_t> slot = take_slot(lobby);
        if (!slot) {
            connection.kill();
            return;
        }

        const ecs::entity player = ecs::entity::create().set(
            Player{
                .slot = *slot,
            },
            Position{
                .x = spawn_x,
                .y = spawn_y + static_cast<float>(*slot) * spawn_gap,
            },
            Health{
                .current = 100.0f,
                .maximum = 100.0f,
            }
        );

        connection.set(
            Session{
                .player = player.id(),
                .slot = *slot,
            }
        );
    });
}

} // namespace rtype
