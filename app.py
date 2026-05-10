from flask import Flask, request, jsonify, render_template
import subprocess
import sqlite3
import os
import uuid
import json
from flask import Flask, render_template, request, jsonify, session
from werkzeug.security import generate_password_hash, check_password_hash

# Point Flask to your new 'web' folder for HTML and static files
# This tells Flask to use 'web' for both templates AND static assets (like images)
app = Flask(__name__, template_folder='web', static_folder='web', static_url_path='')

# --- DATABASE CONFIGURATION ---
DB_PATH = 'kitkit.db'
app.secret_key = 'super_secret_kitkit_key_change_in_production' # Required for session cookies

def init_db():
    """Creates the SQLite database and relational tables if they don't exist."""
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()

    # 1. Users Table: Stores login info and the player's total money
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS users (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            password_hash TEXT NOT NULL,
            money INTEGER DEFAULT 200
        )
    ''')

    # 2. Matches Table: Stores a history of every completed game
    cursor.execute('''
        CREATE TABLE IF NOT EXISTS matches (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user_id INTEGER NOT NULL,
            mode TEXT NOT NULL,        -- 'normal' or 'rush'
            grid_size INTEGER NOT NULL,
            score INTEGER NOT NULL,    -- seconds (Normal) OR boards solved (Rush)
            timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
            FOREIGN KEY(user_id) REFERENCES users(id)
        )
    ''')

    conn.commit()
    conn.close()
    print("✅ KitKit Database initialized successfully.")

# Run the initialization right away
init_db()
# A global variable to hold our running C process
active_games = {}
def cleanup_game(token):
    """Safely terminates a C process and explicitly closes its OS pipelines."""
    if token and token in active_games:
        proc = active_games[token]
        try:
            # Explicitly release the file descriptors back to the OS
            if proc.stdin: proc.stdin.close()
            if proc.stdout: proc.stdout.close()
            if proc.stderr: proc.stderr.close()
            proc.terminate()
            proc.wait(timeout=1) # Ensure the zombie process is reaped
        except Exception:
            pass
            
        del active_games[token]
        
        # Clean up the hard drive
        file_path = f'test_cases/board_{token}.txt'
        if os.path.exists(file_path):
            try:
                os.remove(file_path)
            except OSError:
                pass
@app.route('/')
def index():
    # This will load your HTML game interface (we will build this next)
    return render_template('index.html')

@app.route('/game.html')
def game_page():
    return render_template('game.html')

@app.route('/rush.html')
def rush_page():
    return render_template('rush.html')

@app.route('/api/start', methods=['POST'])
def start_game():
    data = request.json or {}
    
    # --- 1. PLUG THE LEAK: KILL THE OLD GAME ---
    old_token = data.get('old_session_id')
    if old_token:
        cleanup_game(old_token)
        
    match_token = str(uuid.uuid4())
    size = data.get('size', 3)
    mode = data.get('mode', 'normal') 
    
    os.makedirs('test_cases', exist_ok=True)
    unique_file_path = f'test_cases/board_{match_token}.txt'
    
    if mode == 'custom':
        import shutil
        shutil.copyfile('test_cases/custom.txt', unique_file_path)
    else:
        gen_file = f'test_cases/generated_{size}x{size}.txt'
        subprocess.run(['./kenken', '--generate', str(size)])
        os.rename(gen_file, unique_file_path)
        
    active_games[match_token] = subprocess.Popen(
        ['./kenken', unique_file_path, '--web'],
        stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, bufsize=1  
    )
    
    initial_state = ""
    json_started = False
    
    for line in iter(active_games[match_token].stdout.readline, ''):
        if line.strip() == "{":
            json_started = True
            
        if json_started:
            initial_state += line
            if line.strip() == "}": 
                break
            
    board_data = json.loads(initial_state)
    board_data['session_id'] = match_token
    return jsonify(board_data)

@app.route('/api/command', methods=['POST'])
def send_command():
    data = request.json
    match_token = data.get('session_id')
    
    if not match_token or match_token not in active_games:
        return jsonify({'error': 'Game session lost. Please refresh the page.'})

    # Route the move to THIS specific user's C process
    user_process = active_games[match_token]
    r, c, v = data.get('row', -1), data.get('col', -1), data.get('val', -1)
    
    user_process.stdin.write(f"{r} {c} {v}\n")
    user_process.stdin.flush()
    
    new_state = ""
    json_started = False
    
    for line in iter(user_process.stdout.readline, ''):
        if line.strip() == "{":
            json_started = True  # Ignore C warnings until we see the JSON bracket
            
        if json_started:
            new_state += line
            if line.strip() == "}": 
                break
            
    board_data = json.loads(new_state)
    board_data['session_id'] = match_token # Give the token back!
    return jsonify(board_data)

@app.route('/api/stop', methods=['POST'])
def stop_game():
    """Cleans up the C process when a player leaves."""
    data = request.json or {}
    match_token = data.get('session_id')
    if match_token and match_token in active_games:
        active_games[match_token].terminate()
        del active_games[match_token]
        file_path = f'test_cases/board_{match_token}.txt'
        if os.path.exists(file_path):
            os.remove(file_path)
    return jsonify({'success': True})
# ==========================================
# AUTHENTICATION & USER API
# ==========================================

@app.route('/api/register', methods=['POST'])
def register():
    data = request.json
    username = data.get('username')
    password = data.get('password')

    if not username or not password:
        return jsonify({'error': 'Username and password are required'}), 400

    # Never store raw passwords! Hash it for security.
    hashed_pw = generate_password_hash(password)

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        cursor.execute('INSERT INTO users (username, password_hash) VALUES (?, ?)', (username, hashed_pw))
        conn.commit()
        
        # Auto-login the user after successful registration
        user_id = cursor.lastrowid
        session['user_id'] = user_id
        session['username'] = username
        
        # Fetch the default 200 money to send back to the UI
        cursor.execute('SELECT money FROM users WHERE id = ?', (user_id,))
        money = cursor.fetchone()[0]
        
        return jsonify({'success': True, 'username': username, 'money': money})
    except sqlite3.IntegrityError:
        # The UNIQUE constraint in our database will trigger this if the name exists
        return jsonify({'error': 'Username already taken'}), 409
    finally:
        conn.close()

@app.route('/api/login', methods=['POST'])
def login():
    data = request.json
    username = data.get('username')
    password = data.get('password')

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute('SELECT id, username, password_hash, money FROM users WHERE username = ?', (username,))
    user = cursor.fetchone()
    conn.close()

    # user[2] is the hashed password from the database
    if user and check_password_hash(user[2], password):
        # Create the secure session cookie
        session['user_id'] = user[0]
        session['username'] = user[1]
        return jsonify({'success': True, 'username': user[1], 'money': user[3]})
    else:
        return jsonify({'error': 'Invalid username or password'}), 401

@app.route('/api/me', methods=['GET'])
def get_profile():
    """Returns the current logged-in user's data, or False if a guest."""
    if 'user_id' not in session:
        return jsonify({'logged_in': False})

    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    cursor.execute('SELECT username, money FROM users WHERE id = ?', (session['user_id'],))
    user = cursor.fetchone()
    conn.close()

    if user:
        return jsonify({'logged_in': True, 'username': user[0], 'money': user[1]})
    
    # Fallback if the session exists but user was deleted
    session.clear() 
    return jsonify({'logged_in': False})

@app.route('/api/logout', methods=['POST'])
def logout():
    session.clear()
    return jsonify({'success': True})

@app.route('/api/save_match', methods=['POST'])
def save_match():
    # Only process payouts if the user is securely logged in
    if 'user_id' not in session:
        return jsonify({'success': False, 'error': 'Guest mode. Log in to save scores.'})
    
    data = request.json
    mode = data.get('mode')
    size = int(data.get('size', 3))
    score = int(data.get('score', 0)) # Seconds for Normal, Boards Solved for Rush
    
    # The Payout Formula: n^3
    payout = 0
    if mode == 'normal':
        payout = size ** 3
    elif mode == 'rush':
        payout = (size ** 3) * score # Multiplier for every board solved
        
    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()
        
        # 1. Log the Match History
        cursor.execute('''
            INSERT INTO matches (user_id, mode, grid_size, score) 
            VALUES (?, ?, ?, ?)
        ''', (session['user_id'], mode, size, score))
        
        # 2. Deposit the Money
        cursor.execute('''
            UPDATE users SET money = money + ? WHERE id = ?
        ''', (payout, session['user_id']))
        
        # 3. Retrieve the Updated Balance
        cursor.execute('SELECT money FROM users WHERE id = ?', (session['user_id'],))
        new_balance = cursor.fetchone()[0]
        
        conn.commit()
        return jsonify({'success': True, 'payout': payout, 'new_balance': new_balance})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500
    finally:
        conn.close()
    
@app.route('/api/leaderboard', methods=['GET'])
def get_leaderboard():
    mode = request.args.get('mode', 'normal')
    size = request.args.get('size', 4)

    try:
        conn = sqlite3.connect(DB_PATH)
        cursor = conn.cursor()

        # We use a JOIN to connect the match scores with the actual usernames
        if mode == 'normal':
            # NORMAL MODE: Lowest time is best (ORDER BY best_score ASC)
            cursor.execute('''
                SELECT users.username, MIN(matches.score) as best_score
                FROM matches
                JOIN users ON matches.user_id = users.id
                WHERE matches.mode = 'normal' AND matches.grid_size = ?
                GROUP BY users.id
                ORDER BY best_score ASC
                LIMIT 10
            ''', (size,))
        else:
            # RUSH MODE: Highest boards solved is best (ORDER BY best_score DESC)
            cursor.execute('''
                SELECT users.username, MAX(matches.score) as best_score
                FROM matches
                JOIN users ON matches.user_id = users.id
                WHERE matches.mode = 'rush' AND matches.grid_size = ?
                GROUP BY users.id
                ORDER BY best_score DESC
                LIMIT 10
            ''', (size,))

        results = cursor.fetchall()
        
        # Format the data into a clean JSON array for Javascript
        leaderboard_data = [{'name': row[0], 'score': row[1]} for row in results]
        return jsonify({'success': True, 'leaderboard': leaderboard_data})

    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500
    finally:
        conn.close()

@app.route('/api/upload', methods=['POST'])
def upload_custom():
    data = request.json
    content = data.get('content')
    size = data.get('size')
    
    file_path = 'test_cases/custom.txt'
    
    # 1. Save the user's uploaded text to a custom file
    with open(file_path, 'w') as f:
        f.write(content)
        
    # 2. Run the standalone C solver to generate the Solution Cache
    print(f"Solving custom {size}x{size} puzzle...")
    result = subprocess.run(['./custom_solver', file_path], capture_output=True, text=True)
    
    if "SOLVED" in result.stdout:
        return jsonify({'success': True})
    else:
        return jsonify({'success': False, 'error': 'The C engine determined this puzzle is mathematically unsolvable.'})

if __name__ == '__main__':
    print("🚀 KenKen Web Server starting on http://127.0.0.1:8888")
    app.run(debug=True, port=8888)