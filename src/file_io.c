#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>   // Added for mkdir()
#include <sys/types.h>  // Added for system data types
#include "../include/game_logic.h"

// Helper function to allocate memory for the board
Board* create_board(int size, int num_cages) {
    Board* board = (Board*)malloc(sizeof(Board));
    board->size = size;
    board->num_cages = num_cages;
    board->hint_row = 0;
    board->hint_col = 0;
    board->hint_val = 0;
    board->hint_ready = 0;

    // Allocate 2D array for the grid
    board->grid = (Cell**)malloc(size * sizeof(Cell*));
    for (int i = 0; i < size; i++) {
        board->grid[i] = (Cell*)malloc(size * sizeof(Cell));
        for (int j = 0; j < size; j++) {
            board->grid[i][j].row = i;
            board->grid[i][j].col = j;
            board->grid[i][j].value = 0;
            board->grid[i][j].cage = NULL;
        }
    }

    // Allocate array for the cages
    board->cages = (Cage*)malloc(num_cages * sizeof(Cage));
    return board;
}

// Helper function to prevent memory leaks
void free_board(Board* board) {
    if (!board) return;
    
    // Free the dynamically allocated cell pointer arrays inside each cage
    for (int i = 0; i < board->num_cages; i++) {
        free(board->cages[i].cells);
    }
    
    // Free the 2D grid
    for (int i = 0; i < board->size; i++) {
        free(board->grid[i]);
    }
    free(board->grid);
    free(board->cages);
    free(board);
}

// Reads the text file and populates the Board structure
Board* load_puzzle(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Error: Could not open file %s\n", filename);
        return NULL;
    }

    int size, num_cages;
    if (fscanf(file, "%d %d", &size, &num_cages) != 2) {
        printf("Error: Invalid file format (missing size or cage count).\n");
        fclose(file);
        return NULL;
    }

    Board* board = create_board(size, num_cages);

    for (int i = 0; i < num_cages; i++) {
        int target, num_cells;
        char op_char;
        
        fscanf(file, "%d %c %d", &target, &op_char, &num_cells);
        
        board->cages[i].target_value = target;
        board->cages[i].num_cells = num_cells;
        board->cages[i].cells = (Cell**)malloc(num_cells * sizeof(Cell*));
        
        // Map character to our Enum
        switch(op_char) {
            case '+': board->cages[i].op = OP_ADD; break;
            case '-': board->cages[i].op = OP_SUB; break;
            case '*': board->cages[i].op = OP_MULT; break;
            case '/': board->cages[i].op = OP_DIV; break;
            default:  board->cages[i].op = OP_NONE; break;
        }

        // Read the coordinates for each cell in this cage
        for (int j = 0; j < num_cells; j++) {
            int r, c;
            fscanf(file, "%d %d", &r, &c);
            
            // Link the cage to the cell, and the cell back to the cage
            board->cages[i].cells[j] = &(board->grid[r][c]);
            board->grid[r][c].cage = &(board->cages[i]);
        }
    }

    fclose(file);
    return board;
}

// Save the current numbers on the board to a dedicated saves folder
void save_game_state(Board* board) {
    // 1. Ask the operating system to create the "saves" directory.
    // The 0777 sets the folder permissions allowing read/write access.
    // If the folder already exists, mkdir simply ignores this command safely.
    mkdir("saves", 0777);

    // 2. Open the file inside the newly created (or existing) directory
    FILE* file = fopen("saves/savegame.txt", "w");
    if (!file) {
        printf("Error: Could not create save file in the 'saves' directory.\n");
        return;
    }

    // 3. Write the grid values to the file
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            fprintf(file, "%d ", board->grid[i][j].value);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}