# Compiler settings
CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lpthread

# Executables
TARGET = kenken
SOLVER = custom_solver

# Source files
SRC_DIR = src
SRCS = $(SRC_DIR)/main.c $(SRC_DIR)/file_io.c $(SRC_DIR)/game_logic.c $(SRC_DIR)/solver_thread.c $(SRC_DIR)/generator.c
SOLVER_SRC = $(SRC_DIR)/custom_solver.c

# Default target: build both C programs
all: $(TARGET) $(SOLVER) init_folders

# Compile the main KenKen engine
$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

# Compile the Custom Board Solver
$(SOLVER): $(SOLVER_SRC)
	$(CC) $(CFLAGS) $(SOLVER_SRC) -o $(SOLVER)

# Create necessary directories if they don't exist
init_folders:
	mkdir -p test_cases
	mkdir -p web/img

# Run the Python server
run: all
	python3 app.py

# Clean up compiled files and generated puzzles
clean:
	rm -f $(TARGET) $(SOLVER)
	rm -f test_cases/*.txt
	rm -f kitkit.db