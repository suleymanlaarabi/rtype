#include "session.hpp"

#include "network.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char *expression) {
    if (condition) {
        return;
    }

    std::cerr << "failed: " << expression << '\n';
    ++failures;
}

ecs::entity connect() { return ecs::entity::create().add<net::Connection>(); }

void tick() { ecs::run_phase(EcsPostUpdate); }

void close_connection(ecs::entity connection) {
    if (connection.is_alive()) {
        connection.kill();
    }
}

float spawn_y(uint8_t slot) { return 40.0f + static_cast<float>(slot) * 48.0f; }

void expect_ship(ecs::entity connection, uint8_t slot) {
    expect(connection.is_alive(), "connection stays alive");
    expect(connection.has<rtype::Session>(), "connection gains a session");
    if (!connection.is_alive() || !connection.has<rtype::Session>()) {
        return;
    }

    const rtype::Session &session = connection.get<rtype::Session>();
    expect(session.slot == slot, "session stores the slot");

    ecs::entity player = ecs::entity::from(session.player);
    expect(player.is_alive(), "player entity is alive");
    if (!player.is_alive()) {
        return;
    }

    expect(player.get<rtype::Player>().slot == slot, "player slot matches the session");
    expect(player.get<rtype::Position>().x == 32.0f, "player spawns at x = 32");
    expect(player.get<rtype::Position>().y == spawn_y(slot), "player spawn height follows the slot");
    expect(player.get<rtype::Health>().current == 100.0f, "player starts at 100 health");
    expect(player.get<rtype::Health>().maximum == 100.0f, "player maximum health is 100");
}

void test_one_player() {
    ecs::entity connection = connect();
    expect(!connection.has<rtype::Session>(), "a new connection has no session yet");

    tick();

    expect_ship(connection, 0);
    expect(ecs::resource<rtype::Lobby>().occupied == 0x1, "slot 0 is occupied");

    close_connection(connection);
    expect(ecs::resource<rtype::Lobby>().occupied == 0, "closing the connection frees the slot");
}

void test_four_players_are_distinct() {
    std::array<ecs::entity, rtype::Lobby::capacity> connections{};
    for (uint8_t slot = 0; slot < connections.size(); ++slot) {
        connections[slot] = connect();
    }

    tick();

    for (uint8_t slot = 0; slot < connections.size(); ++slot) {
        expect_ship(connections[slot], slot);
    }
    expect(ecs::resource<rtype::Lobby>().occupied == 0x0f, "four slots are occupied");

    for (ecs::entity connection : connections) {
        close_connection(connection);
    }
    expect(ecs::resource<rtype::Lobby>().occupied == 0, "all slots are free");
}

void test_fifth_connection_is_refused() {
    std::array<ecs::entity, rtype::Lobby::capacity> connections{};
    for (ecs::entity &connection : connections) {
        connection = connect();
    }
    ecs::entity extra = connect();

    tick();

    for (uint8_t slot = 0; slot < connections.size(); ++slot) {
        expect_ship(connections[slot], slot);
    }
    expect(!extra.is_alive(), "the fifth connection is destroyed");
    expect(ecs::resource<rtype::Lobby>().occupied == 0x0f, "the fifth client takes no slot");

    for (ecs::entity connection : connections) {
        close_connection(connection);
    }
}

void test_spawn_runs_once() {
    ecs::entity connection = connect();
    tick();

    const ecs_entity_t player = connection.get<rtype::Session>().player;
    tick();

    expect(connection.get<rtype::Session>().player == player, "a second tick keeps the same player");
    expect(ecs::resource<rtype::Lobby>().occupied == 0x1, "a second tick does not take another slot");

    close_connection(connection);
}

void test_disconnect_reuses_slot() {
    std::array<ecs::entity, rtype::Lobby::capacity> connections{};
    for (ecs::entity &connection : connections) {
        connection = connect();
    }
    tick();

    const ecs_entity_t removed_player = connections[1].get<rtype::Session>().player;
    connections[1].kill();

    expect(!ecs::entity::from(removed_player).is_alive(), "the ship is destroyed with the connection");
    expect(ecs::resource<rtype::Lobby>().occupied == 0x0d, "slot 1 is free again");

    ecs::entity replacement = connect();
    tick();

    expect_ship(replacement, 1);

    close_connection(connections[0]);
    close_connection(connections[2]);
    close_connection(connections[3]);
    close_connection(replacement);
    expect(ecs::resource<rtype::Lobby>().occupied == 0, "every slot is free after the last disconnect");
}

} // namespace

int main() {
    ecs::init();
    ecs::component<net::Connection>();
    ecs::import<rtype::session>();

    test_one_player();
    test_four_players_are_distinct();
    test_fifth_connection_is_refused();
    test_spawn_runs_once();
    test_disconnect_reuses_slot();

    ecs::fini();
    return failures == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
}
