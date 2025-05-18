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
// Global Function Definitions
//==============================================================================
void gc_register_socket(const int socket) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Register Socket */
    std::cout << "Register: socket " << socket << std::endl;
    sockets.push_back(socket);
}

void gc_register_buffer(char *const buf) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Register Buffer */
    std::cout << "Register: buffer " << static_cast<const void*>(buf) \
    << std::endl;
    buffers.push_back(buf);
}

void gc_unregister_socket(const int socket) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Unregister Socket */
    auto fsocket = std::find(sockets.begin(), sockets.end(), socket);
    if (fsocket != sockets.end()) {
        std::cout << "Unregister: socket " << socket << std::endl;
        sockets.erase(fsocket);
    }
}

void gc_unregister_buffer(char *const buf) {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Unregister Buffer */
    auto fbuf = std::find(buffers.begin(), buffers.end(), buf);
    if (fbuf != buffers.end()) {
        std::cout << "Unregister: buffer " << static_cast<const void*>(buf) \
        << std::endl;
        buffers.erase(fbuf);
    }
}

void gc_cleanup() {
    /* Mutex */
    std::lock_guard<std::mutex> lock(gc_mutex);
    /* Cleanup */
    for (int socket : sockets) {
        std::cout << "Close: socket " << socket << std::endl;
        shutdown(socket, SHUT_RDWR);
        close(socket);
    }
    for (char *buf : buffers) {
        std::cout << "Free: buffer " << static_cast<const void*>(buf) \
        << std::endl;
        free(buf);
    }
    sockets.clear();
    buffers.clear();
}
