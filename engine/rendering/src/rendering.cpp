#include "rendering.hpp"

namespace engine {

void rendering::import() {
    ecs::component<Color>();
    ecs::component<Circle>();
    ecs::component<Rectangle>();
}

} // namespace engine
