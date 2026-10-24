<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-0FA79C?style=flat-square">
  <img src="https://img.shields.io/badge/license-MIT-6A46E5?style=flat-square">
</p>

<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="www/fluffy-dark.png">
    <source media="(prefers-color-scheme: light)" srcset="www/fluffy-light.png">
    <img src="www/fluffy-light.png" alt="Fluffy" width="520">
  </picture>
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

### Build it

```bash
./build.sh                # build the library
./build.sh examples       # library and example apps
./build.sh install        # build + install library and headers (uses sudo when needed)
./build.sh wasm           # WebAssembly library + documentation demos
# other commands :
./build.sh debug          # Debug build
./build.sh clean          # clean build artifacts
```

`./build.sh wasm` requires the Emscripten SDK (`emcc` on `PATH`). It writes the
library to `build-wasm/` and the runnable demo files to `www/demo/`.

Use `PREFIX=/custom/path ./build.sh install` to install somewhere other than `/usr/local`.
The install produces a single library with GLFW, NanoVG, and gl3w compiled in, so
`find_package(Fluffy CONFIG REQUIRED)` works from that prefix with no separate dependencies.

On Windows, use `build.bat install`. It installs to `%LOCALAPPDATA%\Fluffy` by default;
set `PREFIX` first to choose another location:

```bat
set PREFIX=C:\libs\Fluffy
build.bat install
```

Point CMake at the chosen prefix with `-DCMAKE_PREFIX_PATH=...` and link `fluffy::fluffy`
rather than editing global `INCLUDE` / `LIB` paths.

For the complete walkthrough — requirements, installed file layout, CMake setup,
and manual `helloworld` compilation — see [`doc/install.md`](doc/install.md).

Or manually:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

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

## Examples gallery

| Command | What you'll see |
|---|---|
| `./build/examples/helloworld/helloworld` | Minimal label + button |
| `./build/examples/nav/nav` | **Full widget showcase** with Nav browser |
| `./build/examples/showcase/showcase` | All widgets in a grid |
| `./build/examples/menus/menus` | Menu bar, dropdowns, submenus |
| `./build/examples/icons/icons` | All 654 Lucide icons |
| `./build/examples/yamlui/yamlui` | UI described entirely in YAML |
| `./build/examples/multiwindow/multiwindow` | Multiwindow example |
| `./build/examples/webview/webview` | Webview usage example |

---

## License

MIT License — because software should be free and fluffy !

Third-party assets bundled into binaries:

- **Roboto** (default text font in WebAssembly builds) — Font data copyright Google 2012, [Apache License 2.0](assets/LICENSE-Roboto.txt).
- **Lucide icons** (compiled-in SVG icon set) — ISC, see [third_party/fluffy_lucide/LICENSE.txt](third_party/fluffy_lucide/LICENSE.txt).
