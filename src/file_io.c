#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../include/game_logic.h"

// Allocates memory and initializes a new game board structure
Board* create_board(int size, int num_cages) {
    Board* board = (Board*)malloc(sizeof(Board));
    board->size = size;
    board->num_cages = num_cages;
    board->hint_row = 0;
    board->hint_col = 0;
    board->hint_val = 0;
    board->hint_ready = 0;

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
    board->solution = (int**)malloc(size * sizeof(int*));
    board->errors = (int**)malloc(size * sizeof(int*));
    for (int i = 0; i < size; i++) {
        board->solution[i] = (int*)calloc(size, sizeof(int));
        board->errors[i] = (int*)calloc(size, sizeof(int));
    }

    board->cages = (Cage*)malloc(num_cages * sizeof(Cage));
    return board;
}

// Frees all dynamically allocated memory for a board
void free_board(Board* board) {
    if (!board) return;
    
    for (int i = 0; i < board->num_cages; i++) {
        free(board->cages[i].cells);
    }
    
    for (int i = 0; i < board->size; i++) {
        free(board->grid[i]);
    }
    for (int i = 0; i < board->size; i++) {
        free(board->solution[i]);
        free(board->errors[i]);
    }
    free(board->solution);
    free(board->errors);
    free(board->grid);
    free(board->cages);
    free(board);
}

// Reads puzzle file and populates board structure with cages and solution cache
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
        
        switch(op_char) {
            case '+': board->cages[i].op = OP_ADD; break;
            case '-': board->cages[i].op = OP_SUB; break;
            case '*': board->cages[i].op = OP_MULT; break;
            case '/': board->cages[i].op = OP_DIV; break;
            default:  board->cages[i].op = OP_NONE; break;
        }

        for (int j = 0; j < num_cells; j++) {
            int r, c;
            fscanf(file, "%d %d", &r, &c);
            
            board->cages[i].cells[j] = &(board->grid[r][c]);
            board->grid[r][c].cage = &(board->cages[i]);
        }
    }

    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (fscanf(file, "%d", &board->solution[r][c]) != 1) {
                board->solution[r][c] = 0; 
            }
        }
    }

    fclose(file);
    return board;
}

// Saves current board state to a file in the saves directory
void save_game_state(Board* board) {
    mkdir("saves", 0777);

    FILE* file = fopen("saves/savegame.txt", "w");
    if (!file) {
        printf("Error: Could not create save file in the 'saves' directory.\n");
        return;
    }

    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            fprintf(file, "%d ", board->grid[i][j].value);
        }
        fprintf(file, "\n");
    }

    fclose(file);
}