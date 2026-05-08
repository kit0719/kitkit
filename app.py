from flask import Flask, request, jsonify, render_template
import subprocess

# Point Flask to your new 'web' folder for HTML and static files
app = Flask(__name__, template_folder='web', static_folder='web')

# A global variable to hold our running C process
game_process = None

@app.route('/')
def index():
    # This will load your HTML game interface (we will build this next)
    return render_template('index.html')

@app.route('/api/start', methods=['POST'])
def start_game():
    global game_process
    
    # If a game is already running, kill it so we can start fresh
    if game_process:
        game_process.terminate()
        
    # Spawn the C program with the --web flag!
    game_process = subprocess.Popen(
        ['./kenken', 'test_cases/puzzle_3x3_1.txt', '--web'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True, # Allows us to send/receive normal strings instead of raw bytes
        bufsize=1  # Line buffered so it doesn't get stuck
    )
    
    # The C program instantly outputs the initial JSON board state. 
    # We read it line by line until we hit the closing bracket '}'
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
    # Run the server on port 5000
    print("🚀 KenKen Web Server starting on http://127.0.0.1:5000")
    app.run(debug=True, port=5000)