/**
 * @file main.cpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief The Main.
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
#include <thread>
#include <unistd.h>
#include "tlnt.hpp"
#include "parser.hpp"
#include "gc.hpp"

//==============================================================================
// Static Variables
//==============================================================================
static volatile int srvsocket = (-1); /**< Server socket (listening) */
static volatile int signal_exit = (-1); /**< Server socket (listening) */

//==============================================================================
// Static Function Declarations
//==============================================================================
/**
 * @brief Signal handler function.
 * 
 * This function is called when a signal (such as SIGINT, SIGTERM, or SIGHUP)
 * is received.
 * It sets a global flag indicating the program should terminate,
 * and closes the server socket to unblock any blocking calls like accept().
 * 
 * @param signum The signal number received by the program.
 */
static void signal_handler(int signum);

//==============================================================================
// Global Function Definitions
//==============================================================================
int main() {
    /* Telnet Configurations */
    constexpr in_port_t TELNET_PORT = 2323;
    constexpr int LISTEN_QUEUE = 5;
    constexpr int BUFFER_SIZE = 1024;
    /* Variables */
    int clntsocket; /**< Client socket (accepted connection) */
    char *buf; /**< Client Data (Parser) */
    /* Signals Handlers */
    signal(SIGINT, signal_handler); /* Ctrl+C */
    signal(SIGTERM, signal_handler); /* kill <pid> */
    signal(SIGHUP, signal_handler); /* close terminal */
    /* signal(SIGQUIT, signal_handler); mem dump */
    /* Init socket */
    srvsocket = tlnt_init_srv(TELNET_PORT, LISTEN_QUEUE);
    if (srvsocket < 0) {
        std::cout << "Error: tlnt_init_srv" << std::endl;
        return 1;
    }
    /* Client Threading */
    for (;signal_exit == (-1);) {
        clntsocket = tlnt_accept_clnt(srvsocket);
        if (clntsocket >= 0) {
            gc_register_socket(clntsocket);
            buf = (char *)malloc(BUFFER_SIZE);
            if (buf == NULL) {
                std::cout << "Error: session buffer malloc failed" << std::endl;
                gc_unregister_socket(clntsocket);
                close(clntsocket);
            } else {
                gc_register_buffer(buf);
                std::thread(parser_handler, clntsocket,
                            buf, BUFFER_SIZE).detach();
            }
        }
    }
    std::cout << "Finish the Telnet Server " << std::endl;
    /* Cleanup */
    gc_cleanup();
    return 0;
}

//==============================================================================
// Static Function Definitions
//==============================================================================
static void signal_handler(int signum) {
    signal_exit = signum;
    std::cout << " - Get signal_exit: " << signal_exit << std::endl;
    if (srvsocket >= 0) {
        shutdown(srvsocket, SHUT_RDWR);
        close(srvsocket); /* wake up for client accept() */
    }
}
