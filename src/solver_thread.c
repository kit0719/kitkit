#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/game_logic.h"

// Background thread that finds the next empty cell and provides the correct value
void* calculate_hint(void* arg) {
    Board* board = (Board*)arg;

    for (int r = 0; r < board->size; r++) {
        for (int c = 0; c < board->size; c++) {
            if (board->grid[r][c].value == 0 || board->grid[r][c].value != board->solution[r][c]) {
                
                board->hint_row = r;
                board->hint_col = c;
                board->hint_val = board->solution[r][c];
                board->hint_ready = 1; 
                
                pthread_exit(NULL);
            }
        }
    }
    
    board->hint_val = -1;
    board->hint_ready = 1; 
    pthread_exit(NULL);
}

// Launches a detached thread to compute a hint without blocking the game
void get_hint_concurrent(Board* board) {
    pthread_t hint_thread;
    if (pthread_create(&hint_thread, NULL, calculate_hint, (void*)board) != 0) {
        return;
    }
    pthread_detach(hint_thread);
}