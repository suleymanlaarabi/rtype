#include "network.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int net::create_server_socket(uint16_t port) {
    int socket = ::socket(AF_INET, SOCK_STREAM, 0);

    if (socket < 0)
        return -1;

    int reuse = 1;
    ::setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    int flags = ::fcntl(socket, F_GETFL, 0);
    ::fcntl(socket, F_SETFL, flags | O_NONBLOCK);

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (::bind(socket, reinterpret_cast<sockaddr *>(&address), sizeof(address)) < 0) {
        ::close(socket);
        return -1;
    }

    if (::listen(socket, SOMAXCONN) < 0) {
        ::close(socket);
        return -1;
    }

    return socket;
}

std::optional<net::AcceptedConnection> net::accept_connection(int server) {
    sockaddr_in address{};
    socklen_t size = sizeof(address);

    int socket = ::accept(server, reinterpret_cast<sockaddr *>(&address), &size);

    if (socket < 0)
        return std::nullopt;

    int flags = ::fcntl(socket, F_GETFL, 0);
    ::fcntl(socket, F_SETFL, flags | O_NONBLOCK);

    char ip[INET_ADDRSTRLEN]{};

    ::inet_ntop(AF_INET, &address.sin_addr, ip, sizeof(ip));

    return AcceptedConnection{
        .socket = socket,
        .address = ip,
        .port = ntohs(address.sin_port),
    };
}

void net::import(Config config) {
    ecs::component<Connection>();

    ecs::component_hooks<Socket> socket_hooks{
        .on_remove = [](ecs_entity_t, Socket &socket) { ::close(socket.handle); },
    };

    ecs::component<Socket>(socket_hooks);
    ecs::component<Endpoint>();

    ecs::resource_hooks<Server> server_hooks{
        .on_remove = [](const Server &server) { ::close(server.socket); },
    };

    auto server = ecs::resource_handle<Server>(server_hooks);

    int socket = create_server_socket(config.port);

    server.set(
        Server{
            .socket = socket,
        }
    );

    ecs::system("net::AcceptConnections").each([](ecs::res<Server> server) {
        while (auto connection = accept_connection(server->socket)) {
            ecs::entity::create().add<Connection>().set(
                Socket{
                    .handle = connection->socket,
                },
                Endpoint{
                    .address = connection->address,
                    .port = connection->port,
                }
            );
        }
    });
}
