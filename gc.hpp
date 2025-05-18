/**
 * @file gc.hpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Garbage Collector.
 * @version 0.1.0
 * @date 2025-05-18
 *
 * @copyright Copyright (c) 2025
 * @license GPL-3.0-or-later
 *
 */

#ifndef GC_HPP
#define GC_HPP

//=============================================================================
// Global Function Declarations
//=============================================================================
/**
 * @brief Registers a socket for cleanup.
 *
 * Adds the specified socket descriptor to the global list of sockets.
 * This ensures the socket will be properly closed during cleanup, e.g.,
 * when the program receives a termination signal or exits gracefully.
 *
 * @param socket The socket file descriptor to register.
 */
void gc_register_socket(const int socket);

/**
 * @brief Registers a dynamically allocated buffer for cleanup.
 *
 * Adds the given buffer pointer (malloc or new) to the global list of buffers.
 * The buffer will be automatically freed during cleanup using free().
 *
 * @param buf Pointer to the dynamically allocated buffer.
 */
void gc_register_buffer(char *const buf);

/**
 * @brief Unregisters a socket from the cleanup list.
 *
 * Removes the specified socket descriptor from the global list of sockets,
 * preventing it from being closed during a future call to gc_cleanup().
 *
 * @param socket The socket file descriptor to unregister.
 */
void gc_unregister_socket(const int socket);

/**
 * @brief Unregisters a buffer from the cleanup list.
 *
 * Removes the given buffer pointer from the global list of buffers,
 * preventing it from being freed during a future call to gc_cleanup().
 *
 * @param buf Pointer to the buffer to unregister.
 */
void gc_unregister_buffer(char *const buf);

/**
 * @brief Cleans up all registered resources.
 *
 * Closes all registered sockets and frees all registered buffers.
 * This function is typically called on program termination
 * or in a signal handler to ensure graceful resource deallocation.
 */
void gc_cleanup();

#endif /* GC_HPP */
