# IERG2080-final-project
# KitKit: Smart KenKen Engine
A full-stack KenKen puzzle game featuring a high-performance C backend engine, an SQLite database, and an asynchronous Flask/JavaScript frontend. 

## Features
* **C-Powered Math Engine:** Handles board generation, validation, and Latin Square math operations via POSIX threads.
* **Auto-Solve & Hint Systems:** $O(1)$ lookup via a hidden Solution Cache generated entirely in C.
* **Game Modes:** Play "Normal Mode" for the best time, or "Rush Mode" to solve as many boards as possible in a time limit.
* **Custom Board Input:** Upload your own `.txt` KenKen files. A standalone C backtracking algorithm will mathematically solve it and deploy it to the UI.
* **Global Leaderboard:** Live SQLite database tracking user profiles, secure password hashing, and match history.

## Prerequisites
Before running, ensure you have the following installed on your Mac/Linux environment:
1. **GCC (C Compiler)**
2. **Python 3**
3. **Flask & Werkzeug** (Python libraries)
   ```bash
   pip3 install flask werkzeug

### How to test the deployment:
1. Open your terminal in the project folder.
2. Type `make clean` and hit Enter (this tests if it deletes the binaries properly).
3. Type `make run` and hit Enter. 

If everything is structured correctly, GCC will compile both `kenken` and `custom_solver`, create the `test_cases` folder if it's missing, and seamlessly boot up `app.py` on Port 8888. 
