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

## Features

- **Performance**: High-speed C++ simulation engine.
- **Visuals**: Real-time synchronized Candlestick and PNL charts.
- **Analysis**: Detailed Trade Log and PNL Statistics.
