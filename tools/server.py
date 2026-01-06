import os
import csv
import subprocess
import json
from flask import Flask, request, jsonify, render_template

import sys

# Constants for security limits
MAX_FILE_SIZE_MB = 100
MAX_FILE_SIZE_BYTES = MAX_FILE_SIZE_MB * 1024 * 1024
ALLOWED_EXTENSIONS = {'.csv'}
SUBPROCESS_TIMEOUT_SECONDS = 300  # 5 minutes

# Determine paths for frozen (exe) vs script mode
if getattr(sys, 'frozen', False):
    # Bundle Dir is the temp folder where PyInstaller extracts files
    BUNDLE_DIR = sys._MEIPASS
    TEMPLATE_DIR = os.path.join(BUNDLE_DIR, 'templates')
    STATIC_DIR = os.path.join(BUNDLE_DIR, 'static')
    EXE_PATH = os.path.join(BUNDLE_DIR, 'ArbSim.exe')
else:
    # Script mode: tools/server.py -> ../web/
    TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
    PROJECT_ROOT = os.path.dirname(TOOLS_DIR)
    
    TEMPLATE_DIR = os.path.join(PROJECT_ROOT, 'web', 'templates')
    STATIC_DIR = os.path.join(PROJECT_ROOT, 'web', 'static')
    
    # App.exe is usually in build/Release
    # (Exe path resolution happens later, but we init vars here)
    pass

# Flask setup with explicit folders
app = Flask(__name__, 
            static_folder=STATIC_DIR, 
            template_folder=TEMPLATE_DIR)


import argparse

# Config and Data paths
PROJECT_ROOT = os.getcwd() 
DATA_DIR = os.path.join(PROJECT_ROOT, 'data')
CONFIG_PATH = os.path.join(PROJECT_ROOT, 'config', 'config.cfg')

# Argument Parsing
parser = argparse.ArgumentParser(description='ArbSim Dashboard Server')
parser.add_argument('--exe', type=str, help='Path to ArbSim executable')
args, unknown = parser.parse_known_args()

# EXE Path Resolution Priority:
# 1. CLI Argument (--exe)
# 2. Environment Variable (ARBSIM_EXE)
# 3. Frozen (PyInstaller)
# 4. Standard Build Locations

POSSIBLE_EXES = [
    # CMake (Default)
    os.path.join(PROJECT_ROOT, 'build', 'Release', 'ArbSim.exe'),
    # VS
    os.path.join(PROJECT_ROOT, 'x64', 'Release', 'App.exe'),
    os.path.join(PROJECT_ROOT, 'build', 'Debug', 'ArbSim.exe')
]

EXE_PATH = None

if args.exe:
    EXE_PATH = args.exe
elif os.environ.get('ARBSIM_EXE'):
    EXE_PATH = os.environ['ARBSIM_EXE']
elif getattr(sys, 'frozen', False):
    EXE_PATH = os.path.join(sys._MEIPASS, 'ArbSim.exe')
else:
    for p in POSSIBLE_EXES:
        if os.path.exists(p):
            EXE_PATH = p
            break
    
    if not EXE_PATH:
        EXE_PATH = 'ArbSim.exe' # Fallback to PATH or current dir

print(f"Using Executable: {EXE_PATH}")

@app.route('/')
def index():
    return render_template('index.html')

def load_config():
    cfg = {}
    if os.path.exists(CONFIG_PATH):
        with open(CONFIG_PATH, 'r') as f:
            for line in f:
                if '=' in line:
                    key, val = line.strip().split('=', 1)
                    cfg[key] = val
    return cfg

def update_config(x, y, z):
    # Load existing to preserve paths
    cfg = load_config()
    
    # Default paths if missing
    future_a = cfg.get('Data.FutureA', os.path.join(DATA_DIR, 'futureA.csv').replace('\\', '/'))
    future_b = cfg.get('Data.FutureB', os.path.join(DATA_DIR, 'futureB.csv').replace('\\', '/'))
    
    config_content = f"""Data.FutureA={future_a}
Data.FutureB={future_b}
Strategy.MinArbitrageEdge={x}
Strategy.MaxAbsExposureLots={y}
Strategy.StopLossPnl={z}
"""
    with open(CONFIG_PATH, 'w') as f:
        f.write(config_content)

@app.route('/api/config', methods=['GET'])
def get_config():
    cfg = load_config()
    return jsonify({
        'x': float(cfg.get('Strategy.MinArbitrageEdge', 1.5)),
        'y': int(cfg.get('Strategy.MaxAbsExposureLots', 10)),
        'z': float(cfg.get('Strategy.StopLossPnl', -5000)),
        'fileA': cfg.get('Data.FutureA', ''),
        'fileB': cfg.get('Data.FutureB', '')
    })

# ... existing parsing logic ...



def parse_trades_and_pnl(stdout_data):
    trades = []
    chart_data = []
    lines = stdout_data.splitlines()
    summary = {}
    
    for line in lines:
        line = line.strip()
        if not line: continue
        
        # Check for Summary lines
        if line.startswith("Total PnL:"):
            summary['total_pnl'] = float(line.split(':')[1])
            continue
        if line.startswith("Best PnL:"):
            summary['best_pnl'] = float(line.split(':')[1])
            continue
        if line.startswith("Worst PnL:"):
            summary['worst_pnl'] = float(line.split(':')[1])
            continue
        if line.startswith("Max exposure:"):
            summary['max_exposure'] = int(line.split(':')[1])
            continue
        if line.startswith("Traded lots:"):
            summary['traded_lots'] = int(line.split(':')[1])
            continue
            
        # PNL Snapshot: TIME,PNL,TotalPnl,MidPriceB
        if ",PNL," in line:
            parts = line.split(',')
            if len(parts) >= 4:
                try:
                    chart_data.append({
                        'time': int(parts[0]),
                        'pnl': float(parts[2]),
                        'priceB': float(parts[3])
                    })
                except ValueError:
                    pass
            continue

        # Parse Trade CSV line: time,Side,Inst,Qty,Price
        # Example: 1544166006144506979,BUY,FutureB,1,10928.5
        if ",FutureB," in line:
            parts = line.split(',')
            if len(parts) >= 5:
                try:
                    trade = {
                        'time': int(parts[0]),
                        'side': parts[1],
                        'price': float(parts[4]),
                        'qty': int(parts[3])
                    }
                    trades.append(trade)
                except ValueError:
                    pass
                
    return trades, summary, chart_data

@app.route('/api/run', methods=['POST'])
def run_simulation():
    data = request.json
    x = data.get('x', 0.0)
    y = data.get('y', 0)
    z = data.get('z', -100000)
    
    update_config(x, y, z)
    
    try:
        # Run C++ App with timeout protection
        result = subprocess.run(
            [EXE_PATH],
            cwd=PROJECT_ROOT,
            capture_output=True,
            text=True,
            timeout=SUBPROCESS_TIMEOUT_SECONDS
        )

        trades, summary, chart_data = parse_trades_and_pnl(result.stdout)

        return jsonify({
            'success': True,
            'summary': summary,
            'trades': trades,
            'chart': chart_data
        })
    except subprocess.TimeoutExpired:
        return jsonify({'success': False, 'error': f'Simulation timed out after {SUBPROCESS_TIMEOUT_SECONDS} seconds'}), 504
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

def validate_uploaded_file(file_storage, field_name):
    """Validate an uploaded file for security and correctness.

    Returns (is_valid, error_message) tuple.
    """
    if not file_storage or file_storage.filename == '':
        return True, None  # Empty upload is OK (optional)

    filename = file_storage.filename

    # Check extension
    _, ext = os.path.splitext(filename.lower())
    if ext not in ALLOWED_EXTENSIONS:
        return False, f'{field_name}: Only CSV files are allowed (got {ext})'

    # Check file size
    file_storage.seek(0, 2)  # Seek to end
    file_size = file_storage.tell()
    file_storage.seek(0)  # Reset to beginning

    if file_size > MAX_FILE_SIZE_BYTES:
        return False, f'{field_name}: File too large ({file_size // (1024*1024)}MB > {MAX_FILE_SIZE_MB}MB limit)'

    if file_size == 0:
        return False, f'{field_name}: File is empty'

    return True, None


@app.route('/api/upload', methods=['POST'])
def upload_files():
    try:
        errors = []

        # Validate both files first
        if 'futureA' in request.files:
            fa = request.files['futureA']
            valid, error = validate_uploaded_file(fa, 'FutureA')
            if not valid:
                errors.append(error)

        if 'futureB' in request.files:
            fb = request.files['futureB']
            valid, error = validate_uploaded_file(fb, 'FutureB')
            if not valid:
                errors.append(error)

        if errors:
            return jsonify({'success': False, 'error': '; '.join(errors)}), 400

        # Ensure data directory exists
        os.makedirs(DATA_DIR, exist_ok=True)

        # Save files with fixed safe filenames (prevents path traversal)
        if 'futureA' in request.files:
            fa = request.files['futureA']
            if fa.filename != '':
                fa.save(os.path.join(DATA_DIR, 'futureA.csv'))

        if 'futureB' in request.files:
            fb = request.files['futureB']
            if fb.filename != '':
                fb.save(os.path.join(DATA_DIR, 'futureB.csv'))

        return jsonify({'success': True})
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

if __name__ == '__main__':
    app.run(port=5000)
