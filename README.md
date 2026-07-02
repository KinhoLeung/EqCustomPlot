# EqCustomPlot

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh-CN.md">中文</a>
</p>
Interactive equalizer curve widget and demo application built with Qt Widgets. It uses a self-painted, touch-friendly plot surface with cached frequency response rendering, painter-drawn controls, and a simple demo window.

![Screenshot 1](./1.png)
![Screenshot 2](./2.png)
![Screenshot 3](./3.png)

## Features

- Self-painted interactive equalizer plot optimized for embedded touch screens
- Cached frequency response calculation with a fast precomputed frequency grid
- Touch-friendly controls for frequency, gain, Q, bypass, solo, reset, and filter type
- Painter-drawn EQ handles with no QtSvg runtime dependency
- Qt 6 Widgets app with a minimal `MainWindow` demo

## Tech Stack

- Qt 5.15+ or Qt 6.5+: Core, Widgets
- CMake 3.19+
- C++17

## Build

Prerequisites:

- Install Qt 5.15+ or Qt 6.5+ with modules: Core, Widgets
- Install CMake ≥ 3.19 and a C++17 compiler

Example (Windows, MSVC + Ninja):

```bash
cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2022_64"
cmake --build build --config Release
```

Example (Windows, MSVC + Visual Studio generator):

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2022_64"
cmake --build build --config Release
```

Example (Linux/macOS):

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/Qt/6.5.3/gcc_64"  # or clang_64/macOS kit
cmake --build build --config Release
```

Install (optional):

```bash
cmake --install build
```

The build produces an executable named `EqCustomPlot`.

## Run

Run the built `EqCustomPlot` binary. Qt’s deployment helpers are integrated (`qt_generate_deploy_app_script`) to assist with runtime dependencies on supported platforms.

## Using the Widget

You can embed the custom plot widget into your own Qt application. A minimal example:

```cpp
#include "eqcustomplot.h"

// ...
auto eqPlot = new EqCustomPlot(this);     // QWidget*
setCentralWidget(eqPlot);                 // in your QMainWindow

// Optional: synchronize UI edits to your DSP layer.
connect(eqPlot, &EqCustomPlot::bandChanged, this, [](int index, EqCustomPlot::Band band) {
    // band.frequencyHz, band.gainDb, band.q, band.type, band.bypass
});
```

Key components to explore:

- `eqcustomplot.h/.cpp`: self-painted touch equalizer widget
- `eq.h/.cpp`: equalizer data/model and cached frequency response computation
- `mainwindow.*`, `main.cpp`, `mainwindow.ui`: demo application

Assets:

- `image/light/*.svg`, `image/dark/*.svg`: legacy icon source files, not linked into the current executable
- `resources.qrc`: legacy resource manifest, not used by the current CMake target

## Contributing

Issues and pull requests are welcome. Please keep changes focused and documented.
