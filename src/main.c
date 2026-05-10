#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h> // Added for process management
#include "../include/game_logic.h"

// External declarations
Board* load_puzzle(const char* filename);
void free_board(Board* board);
void save_game_state(Board* board); // New save function

volatile sig_atomic_t timeout_flag = 0;

void handle_timeout(int sig) {
    timeout_flag = 1;
    printf("\n\n⏰ TIME'S UP! You took too long to solve the puzzle.\n");
    printf("Please press Enter to acknowledge and exit...\n");
}

char get_op_char(Operator op) {
    switch(op) {
        case OP_ADD: return '+';
        case OP_SUB: return '-';
        case OP_MULT: return '*';
        case OP_DIV: return '/';
        default: return ' '; 
    }
}

void print_board(Board* board) {
    // ... (Keep your existing print_board code exactly the same) ...
    printf("\n=================================\n");
    printf("        KENKEN (%dx%d)           \n", board->size, board->size);
    printf("=================================\n\n");

    printf("    ");
    for (int j = 0; j < board->size; j++) {
        printf("Col%d ", j);
    }
    printf("\n");

    for (int i = 0; i < board->size; i++) {
        printf("Row%d ", i);
        for (int j = 0; j < board->size; j++) {
            if (board->grid[i][j].value == 0) {
                printf("[ ]  ");
            } else {
                printf("[%d]  ", board->grid[i][j].value);
            }
        }
        printf("\n\n");
    }

    printf("--- Cage Rules ---\n");
    for (int i = 0; i < board->num_cages; i++) {
        Cage* c = &board->cages[i];
        printf("Target: %d%c | Covers %d cells at: ", c->target_value, get_op_char(c->op), c->num_cells);
        for (int j = 0; j < c->num_cells; j++) {
            printf("(%d,%d) ", c->cells[j]->row, c->cells[j]->col);
        }
        printf("\n");
    }
    printf("=================================\n");
}

void print_board_json(Board* board) {
    printf("{\n");
    printf("  \"size\": %d,\n", board->size);
    printf("  \"solved\": %s,\n", is_board_solved(board) ? "true" : "false");
    
    // 1. Print the Grid state
    printf("  \"grid\": [\n");
    for (int i = 0; i < board->size; i++) {
        printf("    [");
        for (int j = 0; j < board->size; j++) {
            printf("%d%s", board->grid[i][j].value, (j < board->size - 1) ? ", " : "");
        }
        printf("]%s\n", (i < board->size - 1) ? "," : "");
    }
    printf("  ],\n");

    // Print the Errors Array
    printf("  \"errors\": [\n");
    for (int i = 0; i < board->size; i++) {
        printf("    [");
        for (int j = 0; j < board->size; j++) {
            printf("%d%s", board->errors[i][j], (j < board->size - 1) ? ", " : "");
        }
        printf("]%s\n", (i < board->size - 1) ? "," : "");
    }
    printf("  ],\n");

    // 2. Print the Cages and Rules
    printf("  \"cages\": [\n");
    for (int i = 0; i < board->num_cages; i++) {
        Cage* c = &board->cages[i];
        
        // Handle the 'no operator' case for JSON clarity
        char op_str[2] = {get_op_char(c->op), '\0'};
        if (op_str[0] == ' ') op_str[0] = '.'; 
        
        printf("    { \"target\": %d, \"op\": \"%s\", \"cells\": [", c->target_value, op_str);
        for (int j = 0; j < c->num_cells; j++) {
            printf("[%d, %d]%s", c->cells[j]->row, c->cells[j]->col, (j < c->num_cells - 1) ? ", " : "");
        }
        printf("] }%s\n", (i < board->num_cages - 1) ? "," : "");
    }
    printf("  ],\n");

    // 3. Print the Hint State
    printf("  \"hint\": { \"ready\": %s", board->hint_ready ? "true" : "false");
    if (board->hint_ready && board->hint_val != -1) {
        printf(", \"row\": %d, \"col\": %d, \"val\": %d", board->hint_row, board->hint_col, board->hint_val);
    }
    printf(" }\n");

    printf("}\n");
    fflush(stdout); // Force the JSON to print immediately for the Python server
}

int main(int argc, char* argv[]) {
    // NEW: Check for the generator flag
    if (argc == 3 && strcmp(argv[1], "--generate") == 0) {
        int size = atoi(argv[2]);
        if (size < 3 || size > 9) {
            printf("Error: Generator size must be between 3 and 9.\n");
            return 1;
        }
        
        // Actually call the generator engine!
        generate_puzzle(size);
        
        return 0; // Exit the program immediately after generating
    }

    // Check for standard mode OR web mode
    if (argc < 2 || argc > 3) {
        printf("Usage: %s <puzzle_file.txt> [--web]\n", argv[0]);
        return 1;
    }

    // Check if the --web flag was passed
    bool web_mode = false;
    if (argc == 3 && strcmp(argv[2], "--web") == 0) {
        web_mode = true;
    }

    Board* board = load_puzzle(argv[1]);
    if (!board) return 1;

    // We only want the signal timer going off in human mode
    if (!web_mode) {
        signal(SIGALRM, handle_timeout);
        signal(SIGCHLD, SIG_IGN); 
        int time_limit_seconds = 600; 
        printf("\nStarting game with a %d second time limit...\n", time_limit_seconds);
        alarm(time_limit_seconds); 
    }

    bool playing = true;
    int r, c, v;

    while (playing && !timeout_flag) {
        
        // Draw the board based on the mode
        if (web_mode) {
            print_board_json(board);
        } else {
            print_board(board);
            
            // Print Human Hints and Menus
            if (board->hint_ready) {
                if (board->hint_val != -1) {
                    printf("\n[Hint System] 💡 Try placing a %d at Row %d, Col %d.\n", 
                           board->hint_val, board->hint_row, board->hint_col);
                } else {
                    printf("\n[Hint System] 💡 No obvious hints available right now.\n");
                }
                board->hint_ready = 0; 
            }
            
            printf("\nCommands:\n");
            printf("- Place a number: <row> <col> <value>\n");
            printf("- Clear a cell:   <row> <col> 0\n");
            printf("- Get a Hint:     88 88 88\n");
            printf("- Save game:      99 99 99\n");
            printf("- Quit game:      -1 -1 -1\n");
            printf("Enter command: ");
        }

        // Wait for input (from the human OR the python server)
        if (scanf("%d %d %d", &r, &c, &v) != 3) {
            if (timeout_flag) break; 
            if (!web_mode) printf("\nInvalid input. Please enter three numbers.\n");
            while(getchar() != '\n'); 
            continue;
        }

        // Handle commands silently if in web mode
        if (r == -1 && c == -1 && v == -1) {
            if (!web_mode) printf("\nExiting game...\n");
            playing = false;
            break;
        }

        if (r == 99 && c == 99 && v == 99) {
            pid_t pid = fork();
            if (pid == 0) {
                save_game_state(board);
                exit(0); 
            } else if (!web_mode && pid > 0) {
                printf("\n[Auto-Save] Saving game state in the background...\n");
            }
            continue; 
        }

        if (r == 88 && c == 88 && v == 88) {
            get_hint_concurrent(board);
            // In web mode, we need a slight delay to ensure the thread finishes
            // before it loops back around to print the JSON with the hint included.
            if (web_mode) usleep(10000); // Wait 10 milliseconds
            continue; 
        }

        // 4. Check Command (77 77 77)
        if (r == 77 && c == 77 && v == 77) {
            for (int i = 0; i < board->size; i++) {
                for (int j = 0; j < board->size; j++) {
                    // Mark cell as error IF it's filled and doesn't match the solution
                    if (board->grid[i][j].value != 0 && board->grid[i][j].value != board->solution[i][j]) {
                        board->errors[i][j] = 1;
                    } else {
                        board->errors[i][j] = 0;
                    }
                }
            }
            if (!web_mode) printf("\n[Check System] Board verified. Errors marked.\n");
            continue;
        }

        // 5. Auto-Solve Command (66 66 66)
        if (r == 66 && c == 66 && v == 66) {
            for (int i = 0; i < board->size; i++) {
                for (int j = 0; j < board->size; j++) {
                    board->grid[i][j].value = board->solution[i][j];
                    board->errors[i][j] = 0; // Clear any errors
                }
            }
            if (!web_mode) printf("\n[Auto-Solve] Puzzle solved!\n");
            continue;
        }

        // Standard move placement
        if (place_number(board, r, c, v)) {
            board->errors[r][c] = 0; // Clear the red error flag when a valid move is made
            if (!web_mode) printf("\nMove accepted!\n");
        }
    }

    if (!web_mode && is_board_solved(board)) {
        alarm(0);
        printf("\n🎉 CONGRATULATIONS! You solved the puzzle! 🎉\n");
    }

    free_board(board);
    return 0;
}