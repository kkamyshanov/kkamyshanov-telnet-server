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
#include <thread>
#include <chrono>
#include <format>
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
    const int clntsocket; /**< Client socket descriptor */
    std::string *const buf; /**< Pointer to the start of the input buffer */
    const std::string_view *prompt; /**< Prompt string displayed to the user */
    std::vector<std::string> *const history; /**< Command history */
};

/**
 * @brief Parser state data used during Telnet session processing.
 *
 * Holds mutable parsing state, function pointers and buffer positions.
 */
struct parse_data {
    void *func; /**< Generic pointer to the current parser state function */
    char symb;  /**< Last read character (symbol) from input */
    unsigned short history_index; /**< Current index (command history) */
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
 * @return int Returns >=0 on normal termination, or <0 error code.
 */
static int parser_fsm(const int clntsocket, const std::string_view *prompt);

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
void parser_handler(int clntsocket) {
    /* Telnet Session Configurations */
    /* TODO: constexpr unsigned short HISTORY_INDEX_MAX = 10; */
    constexpr std::string_view PROMPT("> ", 2);
    /* constexpr std::string_view ARROW_UP("\x1b[A", 4); */
    /* constexpr std::string_view ARROW_DOWN("\x1b[B", 4); */
    /* Assertion */
    if (clntsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return;
    }
    /* Parser Handler */
    if (parser_fsm(clntsocket, &PROMPT) < 0) {
        std::cout << "Error: parser_fsm" << std::endl;
    }
    /* Free Buffers and Close Client Socket */
    gc_unregister_socket(clntsocket);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "Stop Parser Socket: " << clntsocket << std::endl;
}

//==============================================================================
// Static Function Definitions
//==============================================================================
static int parser_fsm(const int clntsocket, const std::string_view *prompt) {
    /* Assertion */
    if (clntsocket < 0) {
        std::cout << "Error: wrong socket value" << std::endl;
        return (-1);
    }
    if (prompt == NULL) {
        std::cout << "Error: prompt == NULL" << std::endl;
        return (-1);
    }
    /* Variables */
    std::vector<std::string> history_commands;
    std::string buf;
    const struct parse_config prscfg = {
        .clntsocket = clntsocket,
        .buf = &buf,
        .prompt = prompt,
        .history = &history_commands
    };
    struct parse_data prsdata = {
        .func = reinterpret_cast<void *>(parser_fsm_main),
        .history_index = 0
    };
    int result = 0;
    /* Reserve memory */
    prscfg.buf->reserve(256);
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
        result = reinterpret_cast
        <int (*)(const struct parse_config *const, struct parse_data *const)>
        (prsdata.func)(&prscfg, &prsdata);
        if (result != 0) {
            break;
        }
    }
    history_commands.clear();
    buf.clear();
    return result;
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

        if (!(prscfg->buf->empty())) {
            std::string line;
            if (*prscfg->buf == "help") {
                line += "Base Telnet Server \r\n";
                line += "Use ARROW_UP or ARROW_DOWN for restore comand \r\n";
            } else if (*prscfg->buf == "Pinata") {
                line += "Tequila! \r\n";
            } else {
                line += "Received command: " + *prscfg->buf + "\r\n";
            }
            if (send(prscfg->clntsocket, line.c_str(),
                     line.size(), 0) < static_cast<ssize_t>(line.size())) {
                std::cout << "Error: session send failed" << std::endl;
                return (-1);
            }
            if ((prscfg->history->size() > 0)
                && (prsdata->history_index != prscfg->history->size())) {
                prscfg->history->pop_back();
                prsdata->history_index = prscfg->history->size();
            }
            /* TODO: Ring Buffer */
            try {
                prscfg->history->push_back(*prscfg->buf);   
            } catch (const std::bad_alloc& e) {
                return (-1);
            }
            
            prscfg->buf->clear();
            ++prsdata->history_index;
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
        if (prscfg->buf->size() != 0) {
            prscfg->buf->pop_back();
            if (send(prscfg->clntsocket, "\b \b", 3, 0) < 3) {
                return (-1);
            }
        }
        break;

    default:
        if (isprint(prsdata->symb)) {
            try {
                prscfg->buf->push_back(prsdata->symb);
            } catch (const std::bad_alloc& e) {
                return (-1);
            }
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
        std::cout << "Arrow UP" << std::endl;
        if (prsdata->history_index > 0) {
            if (prsdata->history_index == prscfg->history->size()) {
                prscfg->history->push_back(*prscfg->buf);
            }
            --prsdata->history_index;
            std::string cmd = (*prscfg->history)[prsdata->history_index];
            std::string line = "\r\033[K" + std::string(*prscfg->prompt) + cmd;
            if (send(prscfg->clntsocket, line.c_str(),
                     line.size(), 0) < static_cast<ssize_t>(line.size())) {
                std::cout << "Error: session send failed" << std::endl;
                return (-1);
            }
            *prscfg->buf = cmd;
            std::cout << "Arrow UP - " << cmd << std::endl;
        }
        break;

    case 'B':
        std::cout << "Arrow DOWN" << std::endl;
        if (prsdata->history_index < prscfg->history->size()) {
            ++prsdata->history_index;
            std::string cmd = (*prscfg->history)[prsdata->history_index];
            std::string line = "\r\033[K" + std::string(*prscfg->prompt) + cmd;
            if (send(prscfg->clntsocket, line.c_str(),
                     line.size(), 0) < static_cast<ssize_t>(line.size())) {
                std::cout << "Error: session send failed" << std::endl;
                return (-1);
            }
            *prscfg->buf = cmd;
            if (prsdata->history_index == (prscfg->history->size() - 1)) {
                prscfg->history->pop_back();
            }
            std::cout << "Arrow DOWN - " << cmd << std::endl;
        }
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
