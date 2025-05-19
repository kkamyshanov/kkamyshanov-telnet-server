/**
 * @file gc.cpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Garbage Collector.
 * @version 0.1.0
 * @date 2025-05-18
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
#include <chrono>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>
#include <sys/socket.h>

//==============================================================================
// Static Variables
//==============================================================================
static std::vector<int> sockets;
static std::vector<char *> buffers;
static std::mutex gc_mutex;

//==============================================================================
// Static Function Declarations
//==============================================================================
/**
 * @brief Cleans up all active client connections.
 *
 * Expected behavior:
 * - Closes all client sockets.
 */
static void gc_cleanup_clients();

//==============================================================================
// Global Function Definitions
//==============================================================================
void gc_register_socket(const int socket) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Register Socket */
    std::cout << "Register: socket " << socket << std::endl;
    sockets.push_back(socket);
}

void gc_unregister_socket(const int socket) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Unregister Socket */
    auto fsocket = std::find(sockets.begin(), sockets.end(), socket);
    if (fsocket != sockets.end()) {
        std::cout << "Unregister: socket " << socket << std::endl;
        sockets.erase(fsocket);
        shutdown(socket, SHUT_RDWR);
        close(socket);
        std::cout << "Client disconnected" << std::endl;
    }
}

void gc_cleanup() {
    gc_cleanup_clients();
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "Cleanup Success" << std::endl;
}

//==============================================================================
// Static Function Definitions
//==============================================================================
static void gc_cleanup_clients() {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Cleanup */
    if (sockets.size() == 0) {
        return;
    }
    std::cout << "Cleanup All Clients" << std::endl;
    for (int socket : sockets) {
        std::cout << "Close: socket " << socket << std::endl;
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }
    sockets.clear();
}
