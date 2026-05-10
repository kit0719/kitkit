#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_SIZE 9
#define MAX_CAGES 81

typedef struct { int r; int c; } Point;
typedef struct { int target; char op; int num_cells; Point cells[MAX_SIZE * MAX_SIZE]; } Cage;

int N, num_cages;
Cage cages[MAX_CAGES];
int grid[MAX_SIZE][MAX_SIZE];
int cage_map[MAX_SIZE][MAX_SIZE];

bool is_valid_row_col(int r, int c, int val) {
    for (int i = 0; i < N; i++) {
        if (grid[r][i] == val) return false;
        if (grid[i][c] == val) return false;
    }
    return true;
}

bool is_cage_valid(int c_idx) {
    Cage cage = cages[c_idx];
    if (cage.op == '.' || cage.op == '=' || cage.op == ' ') return grid[cage.cells[0].r][cage.cells[0].c] == cage.target;
    if (cage.op == '+') {
        int sum = 0;
        for (int i = 0; i < cage.num_cells; i++) sum += grid[cage.cells[i].r][cage.cells[i].c];
        return sum == cage.target;
    }
    if (cage.op == '*') {
        int prod = 1;
        for (int i = 0; i < cage.num_cells; i++) prod *= grid[cage.cells[i].r][cage.cells[i].c];
        return prod == cage.target;
    }
    if (cage.op == '-') {
        int v1 = grid[cage.cells[0].r][cage.cells[0].c];
        int v2 = grid[cage.cells[1].r][cage.cells[1].c];
        return abs(v1 - v2) == cage.target;
    }
    if (cage.op == '/') {
        int v1 = grid[cage.cells[0].r][cage.cells[0].c];
        int v2 = grid[cage.cells[1].r][cage.cells[1].c];
        if (v1 > v2) return (v1 / v2 == cage.target) && (v1 % v2 == 0);
        else return (v2 / v1 == cage.target) && (v2 % v1 == 0);
    }
    return false;
}

bool solve(int r, int c) {
    if (r == N) return true;
    int next_r = r, next_c = c + 1;
    if (next_c == N) { next_r++; next_c = 0; }
    int c_idx = cage_map[r][c];

    for (int val = 1; val <= N; val++) {
        if (!is_valid_row_col(r, c, val)) continue;
        grid[r][c] = val; 

        bool last_empty = true;
        for (int i = 0; i < cages[c_idx].num_cells; i++) {
            if (grid[cages[c_idx].cells[i].r][cages[c_idx].cells[i].c] == 0) { last_empty = false; break; }
        }
        if (last_empty && !is_cage_valid(c_idx)) { grid[r][c] = 0; continue; }
        if (solve(next_r, next_c)) return true;
        grid[r][c] = 0;
    }
    return false;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Error: No file provided.\n"); return 1; }
    
    // 1. Read the file
    FILE *file = fopen(argv[1], "r");
    if (!file) { printf("Error: Cannot open file.\n"); return 1; }
    if (fscanf(file, "%d %d", &N, &num_cages) != 2) return 1;

    for (int r = 0; r < N; r++) for (int c = 0; c < N; c++) grid[r][c] = 0;

    for (int i = 0; i < num_cages; i++) {
        fscanf(file, "%d %c %d", &cages[i].target, &cages[i].op, &cages[i].num_cells);
        for (int j = 0; j < cages[i].num_cells; j++) {
            fscanf(file, "%d %d", &cages[i].cells[j].r, &cages[i].cells[j].c);
            cage_map[cages[i].cells[j].r][cages[i].cells[j].c] = i; 
        }
    }
    fclose(file);

    // 2. Solve and Append the Solution Cache
    // 2. Solve and Rewrite a Sanitized File
    if (solve(0, 0)) {
        // "w" mode completely overwrites the user's messy file with our clean data
        file = fopen(argv[1], "w"); 
        
        // Write the clean header
        fprintf(file, "%d %d\n", N, num_cages);
        
        // Write the clean cages perfectly formatted
        for (int i = 0; i < num_cages; i++) {
            fprintf(file, "%d %c %d ", cages[i].target, cages[i].op, cages[i].num_cells);
            for (int j = 0; j < cages[i].num_cells; j++) {
                fprintf(file, "%d %d ", cages[i].cells[j].r, cages[i].cells[j].c);
            }
            fprintf(file, "\n");
        }
        
        // Write the Solution Cache at the bottom
        for (int r = 0; r < N; r++) {
            for (int c = 0; c < N; c++) {
                fprintf(file, "%d ", grid[r][c]);
            }
            fprintf(file, "\n");
        }
        
        fclose(file);
        printf("SOLVED\n");
    } else {
        printf("FAILED\n");
    }
    return 0;
}