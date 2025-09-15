# EqCustomPlot

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh-CN.md">中文</a>
</p>
Interactive equalizer curve widget and demo application built with Qt 6 and QCustomPlot. It showcases a frequency response editor with a custom frequency axis ticker, light/dark SVG icons, and a simple Qt Widgets UI.

![Screenshot 1](./1.png)
![Screenshot 2](./2.png)
![Screenshot 3](./3.png)

## Features

- Interactive equalizer plot (QCustomPlot-based)
- Custom frequency axis ticker (Hz/kHz labeling)
- Light and dark SVG icon sets for EQ bands and filters
- Qt 6 Widgets app with a minimal `MainWindow` demo
- Print/export support via Qt PrintSupport (extend as needed)

## Tech Stack

- Qt 6.5+: Core, Widgets, Svg, PrintSupport
- CMake 3.19+
- C++17
- [QCustomPlot](https://www.qcustomplot.com/) (embedded source)

## Build

Prerequisites:

- Install Qt 6.5+ with modules: Core, Widgets, Svg, PrintSupport
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
```

Key components to explore:

- `eqcustomplot.h/.cpp`: custom equalizer plot widget (QCustomPlot-based)
- `qcpaxistickerfreq.h/.cpp`: frequency axis ticker (Hz/kHz)
- `eq.h/.cpp`: equalizer data/model and curve computation
- `qcustomplot.h/.cpp`: embedded plotting library
- `mainwindow.*`, `main.cpp`, `mainwindow.ui`: demo application

Assets:

- `image/light/*.svg`, `image/dark/*.svg`: icons for bands 0–9 and LP/HP, both filled/regular
- `resources.qrc`: resource manifest

## Contributing

Issues and pull requests are welcome. Please keep changes focused and documented.
