#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
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

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Usage: %s <puzzle_file.txt>\n", argv[0]);
        return 1;
    }

    Board* board = load_puzzle(argv[1]);
    if (!board) return 1;

    // 1. Signal Timers
    signal(SIGALRM, handle_timeout);
    
    // Tell the OS to automatically reap child processes (prevents zombies)
    signal(SIGCHLD, SIG_IGN); 

    int time_limit_seconds = 600; 
    printf("\nStarting game with a %d second time limit...\n", time_limit_seconds);
    alarm(time_limit_seconds); 

    bool playing = true;
    int r, c, v;

    while (playing && !timeout_flag) {
        print_board(board);
        
        if (is_board_solved(board)) {
            alarm(0); 
            printf("\n🎉 CONGRATULATIONS! You solved the puzzle! 🎉\n");
            break;
        }

        if (board->hint_ready) {
            if (board->hint_val != -1) {
                printf("\n[Hint System] 💡 Try placing a %d at Row %d, Col %d.\n", 
                       board->hint_val, board->hint_row, board->hint_col);
            } else {
                printf("\n[Hint System] 💡 No obvious hints available right now.\n");
            }
            board->hint_ready = 0; // Reset the flag so it only prints once
        }
        
        printf("\nCommands:\n");
        printf("- Place a number: <row> <col> <value>\n");
        printf("- Clear a cell:   <row> <col> 0\n");
        printf("- Save game:      99 99 99\n");
        printf("- Get a Hint:     88 88 88\n"); // New command UI
        printf("- Quit game:      -1 -1 -1\n");
        printf("Enter command: ");

        if (scanf("%d %d %d", &r, &c, &v) != 3) {
            if (timeout_flag) break; 
            printf("\nInvalid input. Please enter three numbers.\n");
            while(getchar() != '\n'); 
            continue;
        }

        if (r == -1 && c == -1 && v == -1) {
            printf("\nExiting game...\n");
            playing = false;
            break;
        }

        // 2. The Process Forking Logic
        if (r == 99 && c == 99 && v == 99) {
            pid_t pid = fork();
            
            if (pid < 0) {
                // Fork failed
                printf("\n[Error] Could not create background process to save.\n");
            } 
            else if (pid == 0) {
                // THIS IS THE CHILD PROCESS
                save_game_state(board);
                exit(0); // The child must exit after saving, otherwise you get two games running!
            } 
            else {
                // THIS IS THE PARENT PROCESS
                printf("\n[Auto-Save] Saving game state in the background...\n");
            }
            continue; // Skip the rest of the loop and draw the board again
        }

        // 3. The Thread Launching Logic
        if (r == 88 && c == 88 && v == 88) {
            get_hint_concurrent(board);
            continue; // Skip drawing the board again so it doesn't interrupt the user
        }

        // Standard move placement
        if (place_number(board, r, c, v)) {
            printf("\nMove accepted!\n");
        }
    }

    free_board(board);
    return 0;
}