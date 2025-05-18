/**
 * @file parser.hpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Raw Telnet Data Parser.
 * @version 0.1.0
 * @date 2025-05-17
 *
 * @copyright Copyright (c) 2025
 * @license GPL-3.0-or-later
 *
 */

#ifndef PARSER_HPP
#define PARSER_HPP

//=============================================================================
// Global Function Declarations
//=============================================================================
/**
 * @brief Handles a Telnet client session (parses raw Telnet data).
 *
 * This function runs in a separate thread and manages the lifecycle of
 * a single Telnet client session. It continuously receives input
 * from the client over the provided socket, interprets or parses
 * the raw Telnet data (including control characters),
 * and responds accordingly.
 * 
 * Responsibilities include:
 * - Receiving user input byte-by-byte.
 * - Handling control sequences (e.g., Enter, Backspace, Ctrl+D).
 * - Interpreting and executing recognized commands (like "help").
 * - Sending command results or echoing input back to the client.
 * - Ensuring buffer limits are respected to prevent overflows.
 * 
 * When the client disconnects or sends an exit signal (e.g., Ctrl+D),
 * the function performs cleanup: it closes the client socket and releases any
 * allocated resources.
 * 
 * @param clntsocket The file descriptor of the accepted client socket.
 * @param buf Buffer used for collecting and parsing client input.
 * @param buf_size The size of the input buffer.
 */
void parser_handler(int clntsocket, char *buf, const int buf_size);

#endif /* PARSER_HPP */
