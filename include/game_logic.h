#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include <stdbool.h>

// -----------------------------------------------------------------------------
// Data Structures
// -----------------------------------------------------------------------------

// Enumeration for the math operators found in cages.
typedef enum {
    OP_ADD,      // '+'
    OP_SUB,      // '-'
    OP_MULT,     // '*'
    OP_DIV,      // '/'
    OP_NONE      // Used for single-cell cages that just have a target number
} Operator;

// Forward declarations so our structs can reference each other
typedef struct Cell Cell;
typedef struct Cage Cage;

// Represents a single square on the KenKen board
struct Cell {
    int row;
    int col;
    int value;       // 0 means the cell is currently empty
    Cage* cage;      // Pointer back to the cage this cell belongs to
};

// Represents a grouping of cells that share a target number and math operator
struct Cage {
    int target_value;
    Operator op;
    int num_cells;   // How many cells are in this specific cage
    Cell** cells;    // An array of pointers to the Cells inside this cage
};

// The master structure that holds the entire game state
typedef struct {
    int size;
    int num_cages;
    Cell** grid;
    Cage* cages;
    
    // --- NEW: Solution Cache ---
    int** solution; 
    int** errors;   // 1 if the cell is currently wrong, 0 if correct/empty

    // Hint System Variables
    int hint_row;
    int hint_col;
    int hint_val;
    volatile int hint_ready; 
} Board;

// -----------------------------------------------------------------------------
// Function Prototypes
// -----------------------------------------------------------------------------

/* * Memory Management
 * These will be implemented in your file_io.c or a board_init.c 
 */
Board* create_board(int size, int num_cages);
void free_board(Board* board);

/* * Core Gameplay Actions
 * These will be implemented in game_logic.c
 */
// Attempts to place a number. Returns true if successful, false if out of bounds.
bool place_number(Board* board, int row, int col, int value);

void save_game_state(Board* board);
// Clears a cell (sets value to 0)
void clear_cell(Board* board, int row, int col);

// Threaded Hint System
void get_hint_concurrent(Board* board);
/* * Validation Rules
 * The heavy lifting of the KenKen rules engine.
 */
// Checks if placing 'value' at (row, col) violates basic row/col uniqueness
bool is_move_valid_basic(Board* board, int row, int col, int value);

// Checks if a specific cage's math is correct (only returns true if cage is full AND math is right)
bool check_cage_math(Cage* cage);

// Scans the entire board to see if all cells are filled and all cages are valid
bool is_board_solved(Board* board);

// Procedural Generation
void generate_puzzle(int size);

#endif // GAME_LOGIC_H