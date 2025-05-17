/**
 * @file main.cpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief The Main.
 * @version 0.1.0
 * @date 2025-05-17
 *
 * @copyright Copyright (c) 2025
 * @license GPL-3.0-or-later
 *
 */


//==============================================================================
// Includes
//==============================================================================
#include <thread>
#include "tlnt.hpp"
#include "session.hpp"

//==============================================================================
// Global Function Definitions
//==============================================================================
int main() {
    /* Telnet Configurations */
    constexpr in_port_t TELNET_PORT = 2323;
    constexpr int LISTEN_QUEUE = 5;
    /* Variables */
    int srvsocket;   /**< Server socket (listening) */
    int clntsocket;  /**< Client socket (accepted connection) */
    /* Init socket */
    int srvsocket = tlnt_init_srv(TELNET_PORT, LISTEN_QUEUE);
    if (srvsocket < 0) {
        perror("Error: tlnt_init_srv");
        return 1;
    }
    /* Client Threading */
    for (;;) {
        clntsocket = tlnt_accept_clnt(srvsocket);
        if (clntsocket >= 0) {
            std::thread(session_handler, clntsocket).detach();
        }
    }
    /* Close Server Socket */
    return tlnt_close_srv(srvsocket);
}
