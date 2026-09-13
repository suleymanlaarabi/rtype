#include "core.hpp"
#include "gameplay.hpp"
#include "raylib.h"
#include "raylib.hpp"
#include "rendering.hpp"
#include "spatial.hpp"
#include <cstdio>
#include <siecs.h>
#include <siecs_rest.h>
#include <string>

enum class ValueType : int8_t {
    Pixel,
    Percent,
};

enum class Direction : int8_t {
    Column,
    Row,
};

struct Value {
    ValueType type;
    float value;
};

Value px(float value) { return { ValueType::Pixel, value }; }
Value percent(float value) { return { ValueType::Percent, value }; }

struct Node {
    Value width = px(0);
    Value height = px(0);
    Direction direction = Direction::Row;
};

ecs::entity hstack() {
    return ecs::entity::create().set(Node{ .width = percent(100), .direction = Direction::Row });
}

ecs::entity vstack() {
    return ecs::entity::create().set(
        Node{ .height = percent(100), .direction = Direction::Column }
    );
}

ecs::entity button(const std::string &title) {
    return ecs::entity::create().set(Node{}).children(ecs::entity::create().set(Node{}));
}

struct OnClick {};
struct PointerEnter {};
struct PointerLeave {};

struct Hovered {};

#define add(cname) [](ecs::entity e) { e.add<cname>(); }
#define remove(cname) [](ecs::entity e) { e.remove<cname>(); }

int main() {
    ecs::init({ .target_fps = 120, .worker_threads = 4 });

    ecs::import<engine::core>();
    ecs::import<engine::spatial>();
    ecs::import<engine::rendering>();
    ecs::import<rtype::gameplay>();
    ecs::import<engine::raylib>();
    ecs::import<sirest>();

    // ecs::entity::create("rtype::player")
    //     .set(
    //         engine::Position(0, 0),
    //         engine::Velocity(0, 0),
    //         engine::Rectangle(100, 100),
    //         engine::Color(255, 0, 0, 255),
    //         rtype::MoveInput{
    //             .left = KEY_A,
    //             .right = KEY_D,
    //             .up = KEY_W,
    //             .down = KEY_S,
    //             .speed = 200,
    //         },
    //         rtype::Speed(200)
    //     )
    //     .add<rtype::Player, rtype::Gun>();

    ecs::run();

    engine::raylib::fini();
}
