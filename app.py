from flask import Flask, request, jsonify, render_template
import subprocess

# Point Flask to your new 'web' folder for HTML and static files
# This tells Flask to use 'web' for both templates AND static assets (like images)
app = Flask(__name__, template_folder='web', static_folder='web', static_url_path='')

# A global variable to hold our running C process
game_process = None

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
    global game_process
    
    # If a game is already running, kill it so we can start fresh
    if game_process:
        game_process.terminate()
        
    # 1. Get the requested grid size from the frontend (default to 3)
    data = request.json or {}
    size = data.get('size', 3)
    
    # 2. RUN THE GENERATOR FIRST!
    # We use subprocess.run to make Python pause and wait until the C program finishes generating
    print(f"Generating new {size}x{size} puzzle...")
    subprocess.run(['./kenken', '--generate', str(size)])
    
    # 3. Spawn the C game engine using the newly generated file
    file_path = f'test_cases/generated_{size}x{size}.txt'
    
    game_process = subprocess.Popen(
        ['./kenken', file_path, '--web'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True, 
        bufsize=1  
    )
    
    # Read the initial JSON board state
    initial_state = ""
    for line in iter(game_process.stdout.readline, ''):
        initial_state += line
        if line.strip() == "}": 
            break
            
    return app.response_class(
        response=initial_state,
        status=200,
        mimetype='application/json'
    )

@app.route('/api/command', methods=['POST'])
def send_command():
    global game_process
    if not game_process:
        return jsonify({"error": "Game not started"}), 400
        
    # Get the row, col, and value sent by the web browser
    data = request.json
    r = data.get('row', -1)
    c = data.get('col', -1)
    v = data.get('val', -1)
    
    # Format it exactly how our C program's scanf expects it
    command = f"{r} {c} {v}\n"
    
    # Pipe the command directly into the running C program
    game_process.stdin.write(command)
    game_process.stdin.flush()
    
    # Read the updated JSON board state back from the C program
    new_state = ""
    for line in iter(game_process.stdout.readline, ''):
        new_state += line
        if line.strip() == "}":
            break
            
    return app.response_class(
        response=new_state,
        status=200,
        mimetype='application/json'
    )

if __name__ == '__main__':
    print("🚀 KenKen Web Server starting on http://127.0.0.1:8888")
    app.run(debug=True, port=8888)