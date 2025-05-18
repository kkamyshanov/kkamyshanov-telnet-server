/**
 * @file parser.cpp
 * @author Konstantin Kamyshanov (kkamyshanov)
 * @brief Raw Telnet Data Parser.
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
#include <iostream>
#include <unistd.h>
#include "tlnt.hpp"
#include "parser.hpp"
#include "gc.hpp"

//==============================================================================
// Structures
//==============================================================================
/**
 * @brief Configuration structure for the Telnet parser.
 *
 * Contains socket and buffer-related settings used during input parsing.
 */
struct parse_config {
    const int clntsocket;           /**< Client socket descriptor */
    char *const buf_start;          /**< Pointer to the start of the input buffer */
    char *const buf_end;            /**< Pointer to the end of the buffer (size - 1) */
    const int buf_size;             /**< Size of the input buffer in bytes */
    const std::string_view *prompt; /**< Prompt string displayed to the user */
};

/**
 * @brief Parser state data used during Telnet session processing.
 *
 * Holds mutable parsing state, function pointers and buffer positions.
 */
struct parse_data {
    void *func; /**< Generic pointer to the current parser state function */
    char symb;  /**< Last read character (symbol) from input */
    char *buf;  /**< Pointer to the current position in the buffer */
};

//==============================================================================
// Static Function Declarations
//==============================================================================
/**
 * @brief Starts the parser finite state machine (FSM) for a Telnet session.
 *
 * This function initializes and runs the command parsing logic using a
 * finite state machine (FSM) approach. It is responsible for managing
 * the session’s input loop: receiving data from the client socket,
 * displaying the prompt, and feeding data into the FSM core logic.
 *
 * @param clntsocket The client socket file descriptor to read input from.
 * @param prompt A pointer to the prompt string to display to the client.
 * @param buf The buffer used to store and build the client input string.
 * @param buf_size The size of the input buffer.
 * @return int Returns >=0 on normal termination, or <0 error code.
 */
static int parser_fsm(const int clntsocket, const std::string_view *prompt,
                      char *const buf, const int buf_size);

/**
 * @brief Core state machine logic for parsing client input.
 *
 * This function contains the main logic of the parser FSM, processing
 * a single input character at a time. It handles character classification,
 * buffer management, control character handling (e.g., Enter, Backspace),
 * and transition between input states.
 *
 * @param prscfg Parsing configuration structure (e.g., socket, buffer limits).
 * @param prsdata Parsing state and data (e.g., buffer pointer, current char).
 * @return int Returns >=0 on normal termination, or <0 error code.
 */
static int parser_fsm_main(const struct parse_config *const prscfg,
                           struct parse_data *const prsdata);

/**
 * @brief Detects if the current input character is part of an arrow key escape sequence.
 *
 * This function checks whether the current input byte is part of an
 * ANSI escape sequence that indicates an arrow key (Up, Down, Left, Right).
 * It's usually triggered after receiving the escape character (`\x1b`)
 * and processes subsequent characters to determine
 * the full sequence.
 *
 * @param prscfg Parsing configuration structure (e.g., socket, buffer limits).
 * @param prsdata Parsing state and data (e.g., buffer pointer, current char).
 * @return int Returns >=0 on normal termination, or <0 error code.
 */
static int parser_fsm_arrow_check(const struct parse_config *const prscfg,
                                  struct parse_data *const prsdata);

/**
 * @brief Handles an identified arrow key input (Up, Down, Left, Right).
 *
 * This function performs appropriate action based on which arrow key was
 * pressed. For example, it may load a previous command from history (Up),
 * or move the cursor in the input buffer (Left/Right).
 *
 * Must be called only after confirming the presence of a full escape sequence
 * using `parser_fsm_arrow_check`.
 *
 * @param prscfg Parsing configuration structure (e.g., socket, buffer limits).
 * @param prsdata Parsing state and data (e.g., buffer pointer, current char).
 * @return int Returns >=0 on normal termination, or <0 error code.
 */
static int parser_fsm_arrow(const struct parse_config *const prscfg,
                            struct parse_data *const prsdata);

//==============================================================================
// Global Function Definitions
//==============================================================================
void parser_handler(int clntsocket, char *buf, const int buf_size) {
    /* Telnet Session Configurations */
    constexpr std::string_view PROMPT("> ", 2);
    /* constexpr std::string_view ARROW_UP("\x1b[A", 4); */
    /* constexpr std::string_view ARROW_DOWN("\x1b[B", 4); */
    /* Assertion */
    if (clntsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return;
    }
    /* Parser Handler */
    if (parser_fsm(clntsocket, &PROMPT, buf, buf_size) < 0) {
        std::cout << "Error: parser_fsm" << std::endl;
    }
    /* Free Buffers and Close Client Socket */
    gc_unregister_socket(clntsocket);
    shutdown(clntsocket, SHUT_RDWR);
    close(clntsocket);
    gc_unregister_buffer(buf);
    free(buf);
    std::cout << "Client disconnected" << std::endl;
}

//==============================================================================
// Static Function Definitions
//==============================================================================
static int parser_fsm(const int clntsocket, const std::string_view *prompt,
                      char *const buf, const int buf_size) {
    /* Assertion */
    if (clntsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return (-1);
    }
    if (prompt == NULL) {
        std::cout << "Error: prompt == NULL" << std::endl;
        return (-1);
    }
    if (buf == NULL) {
        std::cout << "Error: buf == NULL" << std::endl;
        return (-1);
    }
    if (buf_size < 2) {
        std::cout << "Error: buf_size < 2" << std::endl;
        return (-1);
    }
    /* Variables */
    const struct parse_config prscfg = {
        .clntsocket = clntsocket,
        .buf_start = buf,
        .buf_end = buf + (buf_size - 1),
        .buf_size = buf_size,
        .prompt = prompt};
    struct parse_data prsdata = {
        .func = reinterpret_cast<void *>(parser_fsm_main),
        /* .symb */
        .buf = prscfg.buf_start,
    };
    int result;
    /* Welcome Message */
    if (send(prscfg.clntsocket, prscfg.prompt->data(),
             prscfg.prompt->size(), 0) <
        static_cast<ssize_t>(prscfg.prompt->size())) {
        std::cout << "Error: session send failed" << std::endl;
        return (-1);
    }
    /* Parser */
    for (;;) {
        if (recv(clntsocket, &prsdata.symb, 1, 0) < 1) {
            break;
        }
        if (isprint(prsdata.symb)) {
            std::cout << "SMB: " << prsdata.symb;
        }
        std::cout << " CODE: " << static_cast<short>(prsdata.symb) << std::endl;
        result = reinterpret_cast<int (*)(const struct parse_config *const, struct parse_data *const)>(prsdata.func)(&prscfg, &prsdata);
        if (result != 0) {
            return result;
        }
    }
    return 0;
}

static int parser_fsm_main(const struct parse_config *const prscfg,
                           struct parse_data *const prsdata)
{
    switch (prsdata->symb) {
    /* CNTRL + C */
    case '\x03':
    /* EOT (End of Transmission) CNTRL + D */
    case '\x04':
        return 1; /* Close the Client */
        break;
    /* TODO: Windows Case */
    case '\r':
    case '\n':
        if (send(prscfg->clntsocket, "\r\n", 2, 0) < 2) {
            std::cout << "Error: session send failed" << std::endl;
            return (-1);
        }

        /* TODO: Execute the command */
        if ((prsdata->buf - prscfg->buf_start) > 0) {
            if (send(prscfg->clntsocket, "Get the CMD\r\n", 13, 0) < 13) {
                std::cout << "Error: session send failed" << std::endl;
                return (-1);
            }
            prsdata->buf = prscfg->buf_start;
            *prsdata->buf = '\0';
            /* TODO: Save The Command (ARROW) */
        }

        if (send(prscfg->clntsocket, prscfg->prompt->data(),
                 prscfg->prompt->size(), 0) <
            static_cast<ssize_t>(prscfg->prompt->size())) {
            std::cout << "Error: session send failed" << std::endl;
            return (-1);
        }
        break;

    case '\x1b':
        prsdata->func = reinterpret_cast<void *>(parser_fsm_arrow_check);
        break;

    case '\b':
    case '\x7F': /* Delete */
        if (prsdata->buf > prscfg->buf_start) {
            --prsdata->buf;
            *prsdata->buf = '\0';
            if (send(prscfg->clntsocket, "\b \b", 3, 0) < 3) {
                return (-1);
            }
        }
        break;

    default:
        if (isprint(prsdata->symb)) {
            if (prsdata->buf >= prscfg->buf_end) {
                return (-1);
            }
            *prsdata->buf = prsdata->symb;
            ++prsdata->buf;
            *prsdata->buf = '\0';
            if (send(prscfg->clntsocket, &prsdata->symb, 1, 0) < 1) {
                return (-1);
            }
        }
        break;
    }
    return 0;
}

static int parser_fsm_arrow_check(const struct parse_config *const prscfg,
                                  struct parse_data *const prsdata) {
    switch (prsdata->symb) {
    case '[':
        prsdata->func = reinterpret_cast<void *>(parser_fsm_arrow);
        break;

    default:
        prsdata->func = reinterpret_cast<void *>(parser_fsm_main);
        return parser_fsm_main(prscfg, prsdata);
        break;
    }
    return 0;
}

static int parser_fsm_arrow(const struct parse_config *const prscfg,
                            struct parse_data *const prsdata) {
    switch (prsdata->symb) {
    case 'A':
        /* TODO: Arrow Up */
        std::cout << "Arrow UP" << std::endl;
        break;

    case 'B':
        /* TODO: Arrow Down */
        std::cout << "Arrow DOWN" << std::endl;
        break;

    case 'C':
        /* TODO: Arrow Right */
        std::cout << "Arrow RIGHT" << std::endl;
        break;
    
    case 'D':
        /* TODO: Arrow Left */
        std::cout << "Arrow LEFT" << std::endl;
        break;

    default:
        prsdata->func = reinterpret_cast<void *>(parser_fsm_main);
        return parser_fsm_main(prscfg, prsdata);
        break;
    }
    return 0;
}
