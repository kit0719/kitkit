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

# Virtual Environment settings
VENV = venv
PYTHON = $(VENV)/bin/python3
PIP = $(VENV)/bin/pip

# Default target: build C programs and initialize folders
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

# Setup the virtual environment and install Python dependencies
venv: $(VENV)/bin/activate
$(VENV)/bin/activate:
	python3 -m venv $(VENV)
	$(PIP) install Flask werkzeug

# Boot the server (Builds C files -> Sets up Venv -> Runs Flask)
run: all venv
	$(PYTHON) app.py

# Standard cleanup: removes binaries, puzzles, and the database
clean:
	rm -f $(TARGET) $(SOLVER)
	rm -f test_cases/*.txt
	rm -f kitkit.db

# Deep cleanup: removes everything including the virtual environment
clean-all: clean
	rm -rf $(VENV)