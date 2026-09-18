#include "debug.hpp"

#ifndef NDEBUG

#include "rendering.hpp"

#include <siecs_spatial.h>

namespace engine {

namespace {

struct DebugTransformGizmo {
    ecs_entity_t root;
};

struct DebugTransformGizmoRoot {};

struct DebugState {
    bool visible = true;
    bool toggle_down = false;
};

void add_part(ecs::entity root, Position3d position, Cuboid cuboid, Color color) {
    ecs::entity::create().set(position, cuboid, color).child_of(root);
}

void add_arrow(
    ecs::entity root,
    Position3d shaft_position,
    Cuboid shaft,
    Position3d head_position,
    Cuboid head,
    Color color
) {
    add_part(root, shaft_position, shaft, color);
    add_part(root, head_position, head, color);
}

} // namespace

void debug::import() {
    ecs::component<DebugTransform>();
    ecs::component<DebugTransformGizmo>();
    ecs::component<DebugTransformGizmoRoot>();
    ecs::set_resource(DebugState{});

    ecs::system("ToggleDebugTransform")
        .phase(EcsPreUpdate)
        .immediate()
        .each([](ecs::res<DebugState> state, ecs::res<const Keyboard> keyboard) {
            const bool toggle_down = keyboard->down(Key::I);
            if (toggle_down && !state->toggle_down) {
                state->visible = !state->visible;
            }
            state->toggle_down = toggle_down;
        });

    ecs::system("UpdateDebugTransformVisibility")
        .phase(EcsPreUpdate)
        .each(
            [](Scale3d &scale, const DebugTransformGizmoRoot &, ecs::res<const DebugState> state) {
                const float value = state->visible ? 1.0f : 0.0f;
                if (scale.x != value) {
                    scale = Scale3d(value);
                }
            }
        );

    ecs::system("DebugTransformGizmo")
        .phase(EcsPreUpdate)
        .each([](ecs::entity entity,
                 const DebugTransform &,
                 const Position3d &,
                 ecs::optional<const DebugTransformGizmo> gizmo) {
            if (gizmo) {
                return;
            }

            const ecs::entity root = ecs::entity::create()
                                         .add<DebugTransformGizmoRoot>()
                                         .set(Position3d{}, Scale3d(1.0f))
                                         .child_of(entity);

            add_arrow(
                root,
                Position3d(0.4f, 0.0f, 0.0f),
                Cuboid(0.8f, 0.04f, 0.04f),
                Position3d(0.9f, 0.0f, 0.0f),
                Cuboid(0.2f, 0.12f, 0.12f),
                Color::red()
            );
            add_arrow(
                root,
                Position3d(0.0f, 0.4f, 0.0f),
                Cuboid(0.04f, 0.8f, 0.04f),
                Position3d(0.0f, 0.9f, 0.0f),
                Cuboid(0.12f, 0.2f, 0.12f),
                Color::green()
            );
            add_arrow(
                root,
                Position3d(0.0f, 0.0f, -0.4f),
                Cuboid(0.04f, 0.04f, 0.8f),
                Position3d(0.0f, 0.0f, -0.9f),
                Cuboid(0.12f, 0.12f, 0.2f),
                Color::blue()
            );

            entity.set(DebugTransformGizmo{ .root = root.id() });
        });

    ecs::observe<ecs::OnRemove>().require<DebugTransformGizmo>().each(
        [](ecs::entity entity, const DebugTransform &, const DebugTransformGizmo &gizmo) {
            ecs::entity::from(gizmo.root).kill();
            entity.remove<DebugTransformGizmo>();
        }
    );
}

} // namespace engine

#endif
