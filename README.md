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

## Running Tests

```bash
cd build
cmake --build . --config Release
ctest -C Release
```

Or run directly:
```bash
./build/Release/ArbSimTests
```

## Build Options

### Enable Per-Event Timing (Profiling)
```bash
cmake -DENABLE_PER_EVENT_TIMING=ON ..
cmake --build . --config Release
```
This adds per-event timing instrumentation (disabled by default to avoid overhead).

## Code Quality

### Security
- **Path validation**: Config file paths are validated to prevent directory traversal attacks
- **File upload validation**: CSV uploads are validated for extension, size (100MB limit), and content
- **Subprocess timeout**: Simulation runs have a 5-minute timeout to prevent hanging

### Robustness
- **CSV validation**: Input files are validated for correct field count (7 fields per line)
- **Strategy parameter validation**: Parameters are validated on construction (edge >= 0, exposure >= 1, stop-loss <= 0)
- **Float epsilon comparison**: Edge comparisons use epsilon tolerance (1e-9) to avoid precision errors

### Observability
- **Dropped trade tracking**: Engine tracks and reports dropped buy/sell attempts due to insufficient liquidity
- **Debug overflow checks**: Integer overflow assertions in debug builds for P&L calculations

### Code Organization
- **Constants.h**: Centralized constants (buffer sizes, time intervals, precision multipliers)
- **RAII test cleanup**: Temporary test files use RAII pattern for automatic cleanup
- **Non-template SimulationEngine**: Simplified from template to concrete class for better maintainability

### Build Quality
- **Compiler warnings**: Enabled `/W4` (MSVC) and `-Wall -Wextra -pedantic` (GCC/Clang)
- **CMake test target**: Integrated test executable with `ctest` support
- **requirements.txt**: Python dependencies documented for easy installation
