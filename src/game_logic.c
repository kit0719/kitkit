#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/game_logic.h"

// Checks if a value violates row or column uniqueness constraints
bool is_move_valid_basic(Board* board, int row, int col, int value) {
    if (value == 0) return true;

    for (int i = 0; i < board->size; i++) {
        if (i != col && board->grid[row][i].value == value) return false;
        if (i != row && board->grid[i][col].value == value) return false;
    }
    return true;
}

// Verifies that all filled cells in a cage satisfy the arithmetic rule
bool check_cage_math(Cage* cage) {
    for (int i = 0; i < cage->num_cells; i++) {
        if (cage->cells[i]->value == 0) return true;
    }

    int target = cage->target_value;

    switch (cage->op) {
        case OP_ADD: {
            int sum = 0;
            for (int i = 0; i < cage->num_cells; i++) sum += cage->cells[i]->value;
            return sum == target;
        }
        case OP_MULT: {
            int product = 1;
            for (int i = 0; i < cage->num_cells; i++) product *= cage->cells[i]->value;
            return product == target;
        }
        case OP_SUB: {
            if (cage->num_cells != 2) return false;
            int a = cage->cells[0]->value;
            int b = cage->cells[1]->value;
            return abs(a - b) == target;
        }
        case OP_DIV: {
            if (cage->num_cells != 2) return false;
            int a = cage->cells[0]->value;
            int b = cage->cells[1]->value;
            
            int max = (a > b) ? a : b;
            int min = (a < b) ? a : b;
            
            if (min == 0 || max % min != 0) return false;
            return (max / min) == target;
        }
        case OP_NONE: {
            return cage->cells[0]->value == target;
        }
        default:
            return false;
    }
}

// Places a number on the board and validates basic constraints
bool place_number(Board* board, int row, int col, int value) {
    if (row < 0 || row >= board->size || col < 0 || col >= board->size) {
        printf("Error: Coordinates (%d, %d) are out of bounds.\n", row, col);
        return false;
    }
    
    if (value < 0 || value > board->size) {
        printf("Error: Value must be between 1 and %d (or 0 to clear).\n", board->size);
        return false;
    }

    if (value != 0 && !is_move_valid_basic(board, row, col, value)) {
        printf("Error: The number %d already exists in row %d or column %d.\n", value, row, col);
        return false;
    }

    board->grid[row][col].value = value;
    
    if (!check_cage_math(board->grid[row][col].cage)) {
        printf("Warning: The math for this cage no longer adds up!\n");
    }

    return true;
}

// Clears a cell by setting its value to zero
void clear_cell(Board* board, int row, int col) {
    place_number(board, row, col, 0);
}

// Checks if the entire board is correctly solved
bool is_board_solved(Board* board) {
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->grid[i][j].value == 0) return false;
        }
    }

    for (int i = 0; i < board->num_cages; i++) {
        if (!check_cage_math(&board->cages[i])) return false;
    }

    return true;
}