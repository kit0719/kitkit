#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/game_logic.h"

void* calculate_hint(void* arg) {
    Board* board = (Board*)arg;

    // Find the first empty cell
    for (int r = 0; r < board->size; r++) {
        for (int c = 0; c < board->size; c++) {
            if (board->grid[r][c].value == 0) {
                
                // Try to find a valid number
                for (int v = 1; v <= board->size; v++) {
                    if (is_move_valid_basic(board, r, c, v)) {
                        // Save the answer to the board struct instead of printing
                        board->hint_row = r;
                        board->hint_col = c;
                        board->hint_val = v;
                        board->hint_ready = 1; // Flag that a hint is ready
                        
                        pthread_exit(NULL);
                    }
                }
            }
        }
    }
    
    // If no hint is found, we use -1 to signal failure
    board->hint_val = -1;
    board->hint_ready = 1; 
    pthread_exit(NULL);
}

void get_hint_concurrent(Board* board) {
    pthread_t hint_thread;
    if (pthread_create(&hint_thread, NULL, calculate_hint, (void*)board) != 0) {
        return;
    }
    pthread_detach(hint_thread);
}