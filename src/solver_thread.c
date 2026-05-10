#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "../include/game_logic.h"

void* calculate_hint(void* arg) {
    Board* board = (Board*)arg;

    // Scan the board for an empty cell OR a wrong cell
    for (int r = 0; r < board->size; r++) {
        for (int c = 0; c < board->size; c++) {
            // If it's empty, or if it's filled with the wrong number
            if (board->grid[r][c].value == 0 || board->grid[r][c].value != board->solution[r][c]) {
                
                // Grab the guaranteed correct answer from the Solution Cache
                board->hint_row = r;
                board->hint_col = c;
                board->hint_val = board->solution[r][c];
                board->hint_ready = 1; 
                
                pthread_exit(NULL);
            }
        }
    }
    
    // If the board is full and completely correct, no hints needed
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