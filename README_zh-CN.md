# EqCustomPlot

<p align="center">
  <a href="README.md">English</a> | <a href="README_zh-CN.md">中文</a>
</p>
一个基于 Qt 6 和 QCustomPlot 的交互式均衡器曲线控件与演示程序。它展示了可编辑的频响曲线、自定义频率坐标刻度（Hz/kHz）、明暗两套 SVG 图标，以及简洁的 Qt Widgets 界面。

![截图 1](./1.png)
![截图 2](./2.png)
![截图 3](./3.png)

## 特性

- 基于 QCustomPlot 的交互式均衡器曲线
- 自定义频率轴刻度（Hz/kHz 显示）
- 明/暗两套 SVG 图标（0–9 频段与 LP/HP，实心/描边）
- 基于 Qt Widgets 的示例窗口
- 通过 Qt PrintSupport 支持打印/导出（可按需扩展）

## 技术栈

- Qt 6.5+：Core、Widgets、Svg、PrintSupport
- CMake 3.19+
- C++17
- [QCustomPlot](https://www.qcustomplot.com/)（源码内置）

## 构建

前置条件：

- 安装 Qt 6.5+，勾选 Core、Widgets、Svg、PrintSupport 模块
- 安装 CMake ≥ 3.19 与支持 C++17 的编译器

示例（Windows，MSVC + Ninja）：

```bash
cmake -S . -B build -G "Ninja" -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2022_64"
cmake --build build --config Release
```

示例（Windows，MSVC + Visual Studio 生成器）：

```bash
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_PREFIX_PATH="C:/Qt/6.5.3/msvc2022_64"
cmake --build build --config Release
```

示例（Linux/macOS）：

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="/path/to/Qt/6.5.3/gcc_64"  # 或 clang_64/macOS kit
cmake --build build --config Release
```

可选安装：

```bash
cmake --install build
```

构建产物为 `EqCustomPlot` 可执行文件。

## 运行

直接运行构建生成的 `EqCustomPlot`。工程已使用 `qt_generate_deploy_app_script` 辅助部署（在受支持的平台上简化运行时依赖）。

## 作为控件集成

在你的 Qt 应用中嵌入自定义控件的最简示例：

```cpp
#include "eqcustomplot.h"

// ...
auto eqPlot = new EqCustomPlot(this);   // QWidget*
setCentralWidget(eqPlot);               // 在你的 QMainWindow 中
```

建议阅读以下核心文件：

- `eqcustomplot.h/.cpp`：自定义均衡器绘图控件（基于 QCustomPlot）
- `qcpaxistickerfreq.h/.cpp`：频率坐标刻度（Hz/kHz）
- `eq.h/.cpp`：均衡器数据/模型与曲线计算
- `qcustomplot.h/.cpp`：嵌入的绘图库
- `mainwindow.*`、`main.cpp`、`mainwindow.ui`：演示应用

资源文件：

- `image/light/*.svg`、`image/dark/*.svg`：0–9 与 LP/HP 图标（实心/描边）
- `resources.qrc`：资源清单

## 参与贡献

欢迎提交 Issue 与 PR。请尽量保持修改聚焦并补充必要说明。

