/**
 * @file tlntsrv.hpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Initialization, configuration, manage of the Telnet server/client.
 * @version 0.1.1
 * @date 2025-05-17
 *
 * @copyright Copyright (c) 2025
 * @license GPL-3.0-or-later
 *
 */

#ifndef TLNT_HPP
#define TLNT_HPP

//=============================================================================
// Includes
//=============================================================================
#include <netinet/in.h>

//=============================================================================
// Global Function Declarations
//=============================================================================
/**
 * @brief Initializes a TCP socket for the Telnet server
 *
 * This function creates a socket using the IPv4 address family (AF_INET),
 * binds it to the specified port on all available interfaces (INADDR_ANY),
 * and puts it into a listening state with a specified listen queue size.
 *
 * @param port The port number on which the Telnet server will listen.
 * @param lqueue The maximum number of pending connections (backlog).
 * @return int Socket on success, or -1 on failure.
 */
int tlnt_init_srv(const in_port_t port, int lqueue);

/**
 * @brief Accepts a new client connection from the listening Telnet server socket.
 *
 * This function wraps the `accept()` system call. It waits (blocks) until a client
 * attempts to connect to the server socket specified by srvsocket.
 * Upon successful connection, it returns a new socket file descriptor
 * for communication with the client.
 *
 * @param srvsocket A server socket
 *
 * @return int New client socket on success, or -1 on failure.
 */
int tlnt_accept_clnt(int srvsocket);

#endif /* TLNT_HPP */
