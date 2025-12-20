import os
import csv
import subprocess
import json
from flask import Flask, request, jsonify, render_template

import sys

# Determine paths for frozen (exe) vs script mode
if getattr(sys, 'frozen', False):
    # Bundle Dir is the temp folder where PyInstaller extracts files
    BUNDLE_DIR = sys._MEIPASS
    # App.exe is bundled at the root of the temp folder
    EXE_PATH = os.path.join(BUNDLE_DIR, 'App.exe')
else:
    # Script mode: web-ui/server.py
    BUNDLE_DIR = os.path.dirname(os.path.abspath(__file__))
    # App.exe is in ../x64/Release/App.exe
    EXE_PATH = os.path.join(os.path.dirname(BUNDLE_DIR), 'x64', 'Release', 'App.exe')

# Flask setup with explicit folders
app = Flask(__name__, 
            static_folder=os.path.join(BUNDLE_DIR, 'static'), 
            template_folder=os.path.join(BUNDLE_DIR, 'templates'))

# Config and Data remain in the current working directory (where the user runs the exe)
PROJECT_ROOT = os.getcwd() 
DATA_DIR = os.path.join(PROJECT_ROOT, 'data')
CONFIG_PATH = os.path.join(PROJECT_ROOT, 'config.cfg')

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
        # Run C++ App
        result = subprocess.run(
            [EXE_PATH], 
            cwd=PROJECT_ROOT, 
            capture_output=True, 
            text=True
        )
        
        trades, summary, chart_data = parse_trades_and_pnl(result.stdout)
        
        return jsonify({
            'success': True,
            'summary': summary,
            'trades': trades,
            'chart': chart_data
        })
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)}), 500

@app.route('/api/upload', methods=['POST'])
def upload_files():
    try:
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
