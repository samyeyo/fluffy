<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="www/fluffy-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="www/fluffy-light.png">
    <img src="www/fluffy-light.png" alt="Fluffy" width="520">
  </picture>
</p>

<p align="center">
  <img src="https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux&logoColor=black" alt="Linux">
  <img src="https://img.shields.io/badge/Windows-0078D6?style=flat-square&logo=windows&logoColor=white" alt="Windows">
  <img src="https://img.shields.io/badge/macOS-000000?style=flat-square&logo=apple&logoColor=white" alt="macOS">
  <img src="https://img.shields.io/badge/C%2B%2B-20-0FA79C?style=flat-square" alt="C++20">
  <img src="https://img.shields.io/badge/license-MIT-6A46E5?style=flat-square" alt="MIT license">
</p>

<p align="center" style="font-size: 1.05em; color: #4C5A78; max-width: 560px; margin: 0 auto;">
  Lightweight multiplatform C++20 GUI toolkit that brings modern interfaces to native desktop apps
</p>

<br>

---

## What is Fluffy?

Fluffy is a **tiny, cross-platform GUI toolkit** that wraps NanoVG with a modern C++ widget layer and a GLFW + OpenGL backend.

**What you get:**

| What | How it feels |
|---|---|
| **Label, Button, TextInput** | Like UI, but cuddly. Icons inline, text styled, word wrap built-in. |
| **Nav, Menu, Slider** | Full controls with smooth animations and hover states. |
| **Layouts** | Vertical & Horizontal, stretch & auto-size, all composable. |
| **Scrollable panels** | Clip overflow, scroll indicators, mouse wheel + trackpad. |
| **Modal dialogs** | Info, confirm, custom — one function call. |
| **SVG icons** | 654 Lucide icons baked in, zero external deps. |
| **Dropdown menus** | Buttons that open vertical menus with icons and submenus. |
| **Color picker** | HSV bars + editable RGBA/hex fields in a compact card. |
| **Image widget** | PNG/JPG/GIF/BMP/TIFF via native decoders, no stb_image. |
| **Webview widget** | Use the system Webview component to browse html pages and websites. |
| **YAML themes** | Swap colors, shadows, corners, fonts — at runtime. No recompile. |
| **YAML UI** | Declare your UI from a file or string — at runtime. No recompile. |

**The best part?** Your app compiles to **~400 KB**.  Just clean, fast, native pixels.

---

## Quick start

### What you'll need

- **CMake ≥ 3.20**
- **C++20 compiler** — Clang ≥ 15, GCC ≥ 13, MSVC ≥ 19.32 (Fluffy uses `std::format`)
- Internet on first build (deps auto-fetch)

For the full Linux package list, Wayland/X11 notes, and optional WebView
dependencies, see [Installing Fluffy](doc/install.md). macOS and Windows need
no extra system packages.

### Build it

```bash
./build.sh examples   # library + example apps
./build.sh install    # build and install the library
```

On Windows, use `build.bat examples` and `build.bat install`.

See [Installing Fluffy](doc/install.md) for the full walkthrough: requirements,
all build-script commands, plain CMake usage, installation prefixes, installed
file layout, and manual compilation.

### Run it

```bash
./build/examples/helloworld/helloworld            # Linux and Windows
open ./build/examples/helloworld/helloworld.app   # MacOS

```

Or explore the **widget showcase** :

```bash
./build/examples/nav/nav              # Linux and Windows
open ./build/examples/nav/nav.app     # MacOS
```

---

## Learn more

| Folder | What's inside |
|---|---|
| [`doc/`](doc/readme.md) | Documentation index and reading path |
| [`doc/tutorial/`](doc/tutorial/readme.md) | Tutorials |


---

## Demonstration

A live demo compiled with WebAssembly is available at
[samyeyo.github.io/fluffy](https://samyeyo.github.io/fluffy).

---

## License

MIT License — because software should be free and fluffy !

Third-party assets bundled into binaries:

- **Roboto** (default text font in WebAssembly builds) — Font data copyright Google 2012, [Apache License 2.0](assets/LICENSE-Roboto.txt).
- **Lucide icons** (compiled-in SVG icon set) — ISC, see [third_party/fluffy_lucide/LICENSE.txt](third_party/fluffy_lucide/LICENSE.txt).
