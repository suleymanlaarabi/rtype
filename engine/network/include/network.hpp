#pragma once

#include <siecs.h>

#include <cstdint>
#include <optional>
#include <string>

struct net {
    struct Connection {};

    struct Socket {
        int handle = -1;
    };

    struct Endpoint {
        std::string address;
        uint16_t port = 0;
    };

    struct Server {
        int socket = -1;
    };

    struct Config {
        uint16_t port = 7777;
    };

    struct AcceptedConnection {
        int socket;
        std::string address;
        uint16_t port;
    };

    static void import(Config config);

  private:
    static int create_server_socket(uint16_t port);
    static std::optional<AcceptedConnection> accept_connection(int socket);
};
