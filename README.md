# High-Frequency Arbitrage Simulator

A C++ based HFT simulation engine with a modern Web-based Dashboard (Python/Flask + HTML/JS).

## Project Structure

- **src/**: Source code (Core logic, App entry, Config).
- **config/**: Configuration files (`config.cfg`).
- **tools/**: Python server and helper scripts.
- **web/**: Web UI templates and assets.
- **data/**: Market data (CSVs).

## Prerequisites

- **CMake** (3.10+).
- **C++ Compiler** (MSVC, GCC, Clang).
- **Python 3.x**.

## How to Build

### C++ Engine
```bash
mkdir build
cd build
cmake ..
cmake --build . --config Release
```
This generates `ArbSim.exe` in `build/Release/`.

### Web Dashboard
1.  Install dependencies:
    ```bash
    pip install -r requirements.txt
    ```
2.  Start the Server:
    ```bash
    python tools/server.py
    ```
    *The server automatically looks for `build/Release/ArbSim.exe`.*
    
    **Custom Executable Path**:
    You can override the executable location:
    ```bash
    python tools/server.py --exe /path/to/ArbSim.exe
    ```
    Or set `ARBSIM_EXE` environment variable.

## Configuration

The simulation parameters are defined in `config/config.cfg`.
You can modify them directly or use the UI inputs to override them for a single run.

## Dashboard Interface

The web interface is divided into two main sections:

### 1. Control Panel (Left Sidebar)
-   **Data Source**:
    -   Displays currently loaded market data files.
    -   Allows uploading new CSV files for `Future A` and `Future B`.
-   **Strategy Parameters**:
    -   **Arbitrage Threshold (X)**: Minimum price difference to trigger a trade.
    -   **Max Exposure (Y)**: Maximum net position (lots) allowed.
    -   **Stop Loss (Z)**: PNL threshold to stop the simulation.
-   **Statistics Panel**:
    -   Shows real-time `Total PNL`, `Best/Worst PNL`, `Max Exposure`, and `Traded Volume` after a simulation run.

### 2. Visualization Area (Right Panel)
-   **Main Chart**:
    -   **Green Line**: Cumulative PNL over time.
    -   **Blue Line**: Price of Future B (secondary axis) for context.
-   **Execution Log**:
    -   A scrollable table showing every trade executed by the engine (Time, Side, Qty, Price).

## Features

-   **Performance**: High-speed C++ simulation engine.
-   **Visuals**: Real-time synchronized Candlestick and PNL charts.
-   **Analysis**: Detailed Trade Log and PNL Statistics.
