#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "../include/game_logic.h"

// 1. Basic Sudoku-style Row and Column Check
bool is_move_valid_basic(Board* board, int row, int col, int value) {
    if (value == 0) return true; // Clearing a cell is always valid

    for (int i = 0; i < board->size; i++) {
        // Check row (ignoring the cell we are currently placing in)
        if (i != col && board->grid[row][i].value == value) return false;
        // Check column (ignoring the cell we are currently placing in)
        if (i != row && board->grid[i][col].value == value) return false;
    }
    return true;
}

// 2. The Core KenKen Math Validation
bool check_cage_math(Cage* cage) {
    // First, verify if the cage is completely filled. 
    // We only check the math once all cells in the cage have a number.
    for (int i = 0; i < cage->num_cells; i++) {
        if (cage->cells[i]->value == 0) return true; // Still playing, don't flag as wrong yet
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
            // Subtraction is only valid for exactly 2 cells
            if (cage->num_cells != 2) return false;
            int a = cage->cells[0]->value;
            int b = cage->cells[1]->value;
            return abs(a - b) == target;
        }
        case OP_DIV: {
            // Division is only valid for exactly 2 cells
            if (cage->num_cells != 2) return false;
            int a = cage->cells[0]->value;
            int b = cage->cells[1]->value;
            
            // Find max and min to divide properly
            int max = (a > b) ? a : b;
            int min = (a < b) ? a : b;
            
            // Avoid division by zero, ensure perfect division, and check target
            if (min == 0 || max % min != 0) return false;
            return (max / min) == target;
        }
        case OP_NONE: {
            // Single cell cage
            return cage->cells[0]->value == target;
        }
        default:
            return false;
    }
}

// 3. The Action Function
bool place_number(Board* board, int row, int col, int value) {
    if (row < 0 || row >= board->size || col < 0 || col >= board->size) {
        printf("Error: Coordinates (%d, %d) are out of bounds.\n", row, col);
        return false;
    }
    
    if (value < 0 || value > board->size) {
        printf("Error: Value must be between 1 and %d (or 0 to clear).\n", board->size);
        return false;
    }

    // Prevent placing a number if it violates row/col rules immediately
    if (value != 0 && !is_move_valid_basic(board, row, col, value)) {
        printf("Error: The number %d already exists in row %d or column %d.\n", value, row, col);
        return false;
    }

    // Update the cell
    board->grid[row][col].value = value;
    
    // Check if that move broke the math for its specific cage
    if (!check_cage_math(board->grid[row][col].cage)) {
        printf("Warning: The math for this cage no longer adds up!\n");
        // We don't return false here, because pencil-and-paper KenKen lets you make 
        // math mistakes. We just warn them.
    }

    return true;
}

void clear_cell(Board* board, int row, int col) {
    place_number(board, row, col, 0);
}

// 4. The Win Condition
bool is_board_solved(Board* board) {
    // 1. Check if all cells are filled
    for (int i = 0; i < board->size; i++) {
        for (int j = 0; j < board->size; j++) {
            if (board->grid[i][j].value == 0) return false;
        }
    }

    // 2. Check if all cages have correct math
    for (int i = 0; i < board->num_cages; i++) {
        if (!check_cage_math(&board->cages[i])) return false;
    }

    return true; // If we made it here, the player wins!
}