#include <network.hpp>
#include <siecs.h>

int main() {
    ecs::init();

    ecs::import<net>(net::Config{ .port = 7777 });

    ecs::run();
}
