# High-Frequency Arbitrage Simulator

A C++ based HFT simulation engine with a modern Web-based Dashboard (Python/Flask + HTML/JS).

## Project Structure

- **App/**: C++ Application Entry point (`Main.cpp`).
- **ArbSimCore/**: Core trading logic (`Strategy`, `Matching`, `PNL`).
- **web-ui/**: Python backend and HTML frontend for visualization.
- **data/**: CSV Market Data files.

## Prerequisites

- **Visual Studio 2022** (or compatible MSBuild environment) with C++ Desktop Development workload.
- **Python 3.x**.

## How to Build (C++)

1.  Open `ArbSim.slnx` in Visual Studio.
2.  Select **Release** / **x64** configuration.
3.  Build Solution (`Ctrl+Shift+B`).

*Alternatively, via command line:*
```powershell
MSBuild ArbSim.slnx /p:Configuration=Release /p:Platform=x64
```

## How to Run (Web Dashboard)

1.  Install Python dependencies:
    ```bash
    pip install -r requirements.txt
    ```
2.  Start the Web Server:
    ```bash
    python web-ui/server.py
    ```
3.  Open browser to `http://localhost:5000`.

## Configuration

The simulation parameters and data sources are defined in `config.cfg`.
You can modify them directly or use the UI inputs to override them for a single run.

## Features

- **Performance**: High-speed C++ simulation engine.
- **Visuals**: Real-time synchronized Candlestick and PNL charts.
- **Analysis**: Detailed Trade Log and PNL Statistics (Best, Worst, Max Exposure).
