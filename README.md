# High-Frequency Arbitrage Simulator

A C++ based HFT simulation engine with a modern Web-based Dashboard (Python/Flask + HTML/JS).

## Project Structure

- **src/**: Source code.
  - **core/**: Trading engine logic.
  - **app/**: Main application entry point.
  - **config/**: Configuration logic.
- **config/**: Default configuration files.
- **tools/**: Helper tools (e.g., Python server).
- **web/**: Web UI templates and assets.
- **data/**: Market data.

## Prerequisites

- **CMake** (3.10+).
- **C++ Compiler** (MSVC 2019+, GCC, or Clang).
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

> **Note**: If `cmake` is not in your PATH, you can often find it in:
> `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\`


### Web Dashboard
1.  Install dependencies:
    ```bash
    pip install -r requirements.txt
    ```
2.  Start the Server:
    ```bash
    python tools/server.py
    ```
    *Note: The server will automatically look for the built C++ executable.*

## Configuration

The simulation parameters and data sources are defined in `config.cfg`.
You can modify them directly or use the UI inputs to override them for a single run.

## Features

- **Performance**: High-speed C++ simulation engine.
- **Visuals**: Real-time synchronized Candlestick and PNL charts.
- **Analysis**: Detailed Trade Log and PNL Statistics (Best, Worst, Max Exposure).
