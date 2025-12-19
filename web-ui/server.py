import os
import csv
import subprocess
import json
from flask import Flask, request, jsonify, render_template

app = Flask(__name__, static_folder='static', template_folder='templates')

PROJECT_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
DATA_DIR = os.path.join(PROJECT_ROOT, 'data')
EXE_PATH = os.path.join(PROJECT_ROOT, 'x64', 'Release', 'App.exe')
CONFIG_PATH = os.path.join(PROJECT_ROOT, 'config.cfg')

@app.route('/')
def index():
    return render_template('index.html')

def update_config(x, y, z):
    # Determine absolute paths for data files to ensure App.exe finds them
    future_a = os.path.join(DATA_DIR, 'futureA.csv').replace('\\', '/')
    future_b = os.path.join(DATA_DIR, 'futureB.csv').replace('\\', '/')
    
    config_content = f"""Data.FutureA={future_a}
Data.FutureB={future_b}
Strategy.MinArbitrageEdge={x}
Strategy.MaxAbsExposureLots={y}
Strategy.StopLossPnl={z}
"""
    with open(CONFIG_PATH, 'w') as f:
        f.write(config_content)

def parse_trades(stdout_data):
    trades = []
    lines = stdout_data.splitlines()
    summary = {}
    
    # Simple parse
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
            
        # Parse Trade CSV line: time,Side,Inst,Qty,Price
        # Example: 1544166006144506979,BUY,FutureB,1,10928.5
        parts = line.split(',')
        if len(parts) >= 5 and parts[2] == 'FutureB':
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
                
    return trades, summary

def replay_data(trades):
    # Generator to read CSVs
    def csv_reader(filepath, inst_id):
        with open(filepath, 'r') as f:
            reader = csv.reader(f)
            # Skip header if present? Assuming no header or handled.
            # Sample: 1544166006144506979,FutureA,2,1,10928,10928.5,1
            for row in reader:
                if len(row) < 6: continue
                # event: time, inst, bid, ask
                try:
                    yield {
                        't': int(row[0]),
                        'inst': inst_id,
                        'bid': float(row[4]),
                        'ask': float(row[5])
                    }
                except ValueError:
                    continue

    # Load all trades into a dict key=time for fast lookup or just iterate?
    # Trades are sorted by time.
    trade_idx = 0
    num_trades = len(trades)
    
    # State
    position = 0
    cash = 0.0
    
    # We want to sample output every 1 minute (60 * 1e9 ns? No, timestamp seems to be ns)
    # 1544166006144506979 -> 1.54e18. 
    # 1 sec = 1,000,000,000 ns.
    SAMPLE_INTERVAL = 60 * 1_000_000_000 # 1 minute
    last_sample_time = 0
    
    chart_data = []
    
    # Stream Merger
    gen_a = csv_reader(os.path.join(DATA_DIR, 'futureA.csv'), 'A')
    gen_b = csv_reader(os.path.join(DATA_DIR, 'futureB.csv'), 'B')
    
    curr_a = next(gen_a, None)
    curr_b = next(gen_b, None)
    
    last_price_a = 0
    last_price_b = 0
    
    while curr_a or curr_b:
        # Determine next event
        if curr_a and curr_b:
            if curr_a['t'] < curr_b['t']:
                ev = curr_a
                curr_a = next(gen_a, None)
            else:
                ev = curr_b
                curr_b = next(gen_b, None)
        elif curr_a:
            ev = curr_a
            curr_a = next(gen_a, None)
        else:
            ev = curr_b
            curr_b = next(gen_b, None)
            
        t = ev['t']
        
        # Update Prices
        if ev['inst'] == 'A':
            last_price_a = (ev['bid'] + ev['ask']) / 2
        else:
            last_price_b = (ev['bid'] + ev['ask']) / 2
            
            # Process Trades at this exact timestamp (approx)
            # Since trade timestamp matches market data timestamp exactly in simulation
            while trade_idx < num_trades and trades[trade_idx]['time'] <= t:
                tr = trades[trade_idx]
                qty = tr['qty']
                price = tr['price']
                if tr['side'] == 'BUY':
                    position += qty
                    cash -= (qty * price)
                else:
                    position -= qty
                    cash += (qty * price)
                trade_idx += 1
        
        # Sample
        if last_sample_time == 0:
            last_sample_time = t
            
        if t - last_sample_time >= SAMPLE_INTERVAL:
            # Calculate PNL
            # Unrealized = Position * MidB
            # Total = Cash + Unrealized
            if last_price_b > 0:
                unrealized = position * last_price_b
                total_pnl = cash + unrealized
                
                chart_data.append({
                    'time': t,
                    'priceA': last_price_a,
                    'priceB': last_price_b,
                    'pnl': total_pnl,
                    'pos': position
                })
            last_sample_time = t
            
            # Limit points to avoid huge JSON? 
            # If day is long, 1 min might be too much?
            # 1 day = 1440 mins. That is small. 
            
    return chart_data

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
        
        trades, summary = parse_trades(result.stdout)
        
        # Replay to get charts
        chart_data = replay_data(trades)
        
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
