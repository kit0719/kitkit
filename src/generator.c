#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "../include/game_logic.h"

// Shuffles an array to ensure every puzzle is completely unique
void shuffle_array(int* array, int size) {
    for (int i = size - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = array[i];
        array[i] = array[j];
        array[j] = temp;
    }
}

// 1. The Latin Square Generator
bool fill_latin_square(Board* board, int row, int col) {
    if (row == board->size) return true; 

    int next_row = (col == board->size - 1) ? row + 1 : row;
    int next_col = (col == board->size - 1) ? 0 : col + 1;

    int options[board->size];
    for (int i = 0; i < board->size; i++) options[i] = i + 1;
    shuffle_array(options, board->size);

    for (int i = 0; i < board->size; i++) {
        int candidate = options[i];
        if (is_move_valid_basic(board, row, col, candidate)) {
            board->grid[row][col].value = candidate;
            if (fill_latin_square(board, next_row, next_col)) return true;
            board->grid[row][col].value = 0;
        }
    }
    return false;
}

// 2. The Cage Partitioner & File Writer
int dRow[] = {-1, 1, 0, 0};
int dCol[] = {0, 0, -1, 1};

void partition_and_save(Board* board, int size) {
    int cage_map[9][9];
    // Mark all cells as unassigned (-1)
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) cage_map[r][c] = -1;
    }

    int current_cage = 0;
    
    // Part 1: Carve out the cage shapes
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            if (cage_map[r][c] == -1) {
                cage_map[r][c] = current_cage;
                int target_size = (rand() % 4) + 1; // Cages can be 1 to 4 cells
                int current_size = 1;
                int f_r = r, f_c = c;
                
                // Grow the cage randomly
                while (current_size < target_size) {
                    bool absorbed = false;
                    int dirs[] = {0, 1, 2, 3};
                    shuffle_array(dirs, 4);

                    for (int i = 0; i < 4; i++) {
                        int nr = f_r + dRow[dirs[i]];
                        int nc = f_c + dCol[dirs[i]];

                        if (nr >= 0 && nr < size && nc >= 0 && nc < size && cage_map[nr][nc] == -1) {
                            cage_map[nr][nc] = current_cage;
                            current_size++;
                            f_r = nr; f_c = nc;
                            absorbed = true;
                            break;
                        }
                    }
                    if (!absorbed) break; // Trapped cells just become 1-cell cages 
                }
                current_cage++;
            }
        }
    }

    // Part 2: Calculate Math & Write to File
    char filename[100];
    sprintf(filename, "test_cases/generated_%dx%d.txt", size, size);
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        printf("Error: Could not create %s\n", filename);
        return;
    }

    // Print the header (Size and Total Cages)
    fprintf(file, "%d %d\n", size, current_cage);

    for (int i = 0; i < current_cage; i++) {
        int cells_r[9], cells_c[9], vals[9];
        int count = 0;
        
        // Find all cells belonging to this specific cage
        for (int r = 0; r < size; r++) {
            for (int c = 0; c < size; c++) {
                if (cage_map[r][c] == i) {
                    cells_r[count] = r;
                    cells_c[count] = c;
                    vals[count] = board->grid[r][c].value;
                    count++;
                }
            }
        }

        char op = '.';
        int target = vals[0];

        // If it's a 2-cell cage, it can use +, -, *, /
        if (count == 2) {
            int a = vals[0], b = vals[1];
            int max = (a > b) ? a : b;
            int min = (a < b) ? a : b;

            int ops[] = {0, 1, 2, 3}; // 0:+, 1:-, 2:*, 3:/
            shuffle_array(ops, 4);
            
            for(int j = 0; j < 4; j++) {
                if (ops[j] == 0) { op = '+'; target = a + b; break; }
                if (ops[j] == 1) { op = '-'; target = max - min; break; }
                if (ops[j] == 2) { op = '*'; target = a * b; break; }
                if (ops[j] == 3 && min != 0 && max % min == 0) { op = '/'; target = max / min; break; } // Only divide if it's perfectly clean
            }
        } 
        // If it's 3 or 4 cells, stick to + and * to keep the math friendly
        else if (count > 2) {
            if (rand() % 2 == 0) {
                op = '+'; target = 0;
                for (int j = 0; j < count; j++) target += vals[j];
            } else {
                op = '*'; target = 1;
                for (int j = 0; j < count; j++) target *= vals[j];
            }
        }

        // Print the cage rules
        fprintf(file, "%d %c %d", target, op, count);
        for (int j = 0; j < count; j++) {
            fprintf(file, " %d %d", cells_r[j], cells_c[j]);
        }
        fprintf(file, "\n");
    }
    
    for (int r = 0; r < size; r++) {
        for (int c = 0; c < size; c++) {
            fprintf(file, "%d ", board->grid[r][c].value);
        }
        fprintf(file, "\n");
    }

    fclose(file);
    printf("✅ Successfully generated mathematically valid puzzle: %s\n", filename);
}

// 3. The Master Generator Wrapper
void generate_puzzle(int size) {
    srand(time(NULL));
    Board* template_board = create_board(size, 0);

    if (fill_latin_square(template_board, 0, 0)) {
        partition_and_save(template_board, size);
    } else {
        printf("Failed to generate Latin Square.\n");
    }

    free_board(template_board);
}