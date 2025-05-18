/**
 * @file tlntsrv.cpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Initialization, configuration, manage of the Telnet server/client.
 * @version 0.1.1
 * @date 2025-05-17
 *
 * @copyright Copyright (c) 2025
 * @license GPL-3.0-or-later
 *
 */

//==============================================================================
// Includes
//==============================================================================
#include <iostream>
#include <sys/socket.h>
#include <unistd.h>
#include "tlnt.hpp"

//==============================================================================
// Static Function Declarations
//==============================================================================
/**
 * @brief Binds a server socket to the specified address family and port.
 *
 * This function sets up the socket address structure (sockaddr_in)
 * with the given address family and port, then calls the system's
 * bind() to associate the socket with this address.
 *
 * @param srvsocket The socket file descriptor to bind.
 * @param family The address family (e.g., AF_INET for IPv4).
 * @param port The port number (not network byte order (use htons)).
 *
 * @return int Returns >0 on successful bind, -1 on failure.
 */
static int tlnt_bind_srv(const int srvsocket,
                         const sa_family_t family,
                         const in_port_t port);

//==============================================================================
// Global Function Definitions
//==============================================================================
int tlnt_init_srv(const in_port_t port, int lqueue) {
    /* Assertion */
    if (port < 1) {
        std::cout << "Error: port 0 is invalid for server socket" << std::endl;
        return (-1);
    }
    if (lqueue < 1) {
        std::cout << "Error: lqueue must be more than 1" << std::endl;
        return (-1);
    }
    /* Variables */
    int srvsocket; /**< Server socket (listening) */
    /* Init Server Socket */
    srvsocket = socket(AF_INET, SOCK_STREAM, 0);
    if (srvsocket < 0) {
        std::cout << "Error: get server socket" << std::endl;
        return (-1);
    }
    if (tlnt_bind_srv(srvsocket, AF_INET, port) < 0) {
        std::cout << "Error: bind addr to srvsocket" << std::endl;
        goto tlnt_init_srv_close_srv;
    }
    if (listen(srvsocket, lqueue) < 0) {
        std::cout << "Error: init listen srvsocket" << std::endl;
        goto tlnt_init_srv_close_srv;
    }
    /* Success */
    std::cout << "Telnet Server started on port " << port << std::endl;
    return srvsocket;

tlnt_init_srv_close_srv:
    shutdown(srvsocket, SHUT_RDWR);
    close(srvsocket);
    return (-1);
}

int tlnt_accept_clnt(int srvsocket) {
    /* Assertion */
    if (srvsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return (-1);
    }
    /* Variables */
    sockaddr_in client_addr{}; /**< Socket address, internet style */
    socklen_t client_size; /**< Size of sockaddr_in */
    /* Accept Client */
    client_size = sizeof(client_addr);
    return (accept(srvsocket, (sockaddr *)&client_addr, &client_size));
}

//==============================================================================
// Static Function Definitions
//==============================================================================
static int tlnt_bind_srv(const int srvsocket,
                         const sa_family_t family,
                         const in_port_t port) {
    /* Assertion */
    if (srvsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return (-1);
    }
    if (port < 1) {
        std::cout << "Error: port 0 is invalid for server socket" << std::endl;
        return (-1);
    }
    /* Variables */
    sockaddr_in addr{}; /**< Socket address, internet style */
    int opt = 1;
    /* Init Sockaddr */
    addr.sin_family = family;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    /* Bind */
    setsockopt(srvsocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return (bind(srvsocket, (sockaddr *)&addr, sizeof(addr)));
}
