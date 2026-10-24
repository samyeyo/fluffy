# Installing Fluffy

This guide takes you from a fresh checkout to a working Fluffy installation. It covers the build scripts, plain CMake, the files that get installed, and how to build your own application against the installed package.

If you only want to try the examples, see the [Hello Fluffy tutorial](tutorial/01-hello-fluffy.md). If you want to use Fluffy in your own project, read on.

## Requirements

| Tool | Minimum | Notes |
|---|---|---|
| CMake | 3.20 | 3.24+ recommended |
| C++ compiler | C++20 | Clang 15+, GCC 13+, Apple Clang 15+, or MSVC 19.32+ |
| Git | optional | Only needed if you use `git clone`; dependencies are downloaded as source archives |
| Internet | first configure | Downloads GLFW, NanoVG, webview, and Lucide icons |

Fluffy fetches its dependencies automatically during the first CMake configure. Git is not used for dependency downloads: GLFW, NanoVG, and webview are unpacked from source archives, while Lucide icons are downloaded individually. Later builds work offline as long as the build directory and its downloaded dependencies are kept.

### Platform notes

- **macOS** — install the Xcode Command Line Tools with `xcode-select --install`. Cocoa, IOKit, CoreFoundation, OpenGL, and WebKit are system frameworks.
- **Windows** — use Visual Studio 2022 with the “Desktop development with C++” workload, or a recent MinGW-w64 toolchain. No external windowing or OpenGL packages are required.
- **Linux** — install a compiler, CMake, pkg-config, OpenGL/X11 development headers, and the optional image/WebView libraries.

On Ubuntu or Debian:

```bash
sudo apt update
sudo apt install build-essential cmake git pkg-config \
    libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev \
    libxcursor-dev libxi-dev libgdk-pixbuf-2.0-dev \
    libwebkit2gtk-4.1-dev
```

`libwebkit2gtk-4.1-dev` is only needed for the `WebView` widget. If it is missing, CMake disables the WebView overlay with a warning. You can also disable it explicitly with `-DFLUFFY_ENABLE_WEBVIEW=OFF`.

Distribution package names differ; on Fedora, for example, the equivalents usually end in `-devel` (`gdk-pixbuf2-devel`, `webkit2gtk4.1-devel`, and so on).

## 1. Download Fluffy

```bash
git clone https://github.com/samyeyo/fluffy.git
cd fluffy
```

If Git is not installed, download the source ZIP instead:

```text
https://github.com/samyeyo/fluffy/archive/refs/heads/main.zip
```

There is no prebuilt binary package to install. Fluffy is built from source and installed into a prefix of your choice.

## 2. Build Fluffy

The helper scripts configure a Release build in `build/`.

macOS and Linux:

```bash
./build.sh              # build the library
./build.sh examples     # library and example apps
./build.sh wasm         # WebAssembly library + documentation demos
./build.sh debug        # Debug build
./build.sh clean        # remove build artifacts
```

`./build.sh wasm` requires the Emscripten SDK (`emcc` on `PATH`) and writes the
library to `build-wasm/` and the demo files to `www/demo/`.

Windows:

```bat
build.bat
build.bat examples
build.bat debug
build.bat clean
```

The examples are written to `build/examples/`. On macOS they are application bundles:

```bash
./build/examples/helloworld/helloworld
open ./build/examples/helloworld/helloworld.app
```

On Windows the executable is under `build\examples\helloworld\Release\` for Visual Studio generators.

### Build with plain CMake

If you prefer to drive CMake yourself:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
```

To include the examples:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DFLUFFY_BUILD_EXAMPLES=ON
cmake --build build --config Release --parallel
```

Useful options:

| Option | Default | Purpose |
|---|---|---|
| `FLUFFY_BUILD_EXAMPLES` | `OFF` | Build the example applications |
| `FLUFFY_ENABLE_WEBVIEW` | `ON` | Build the native WebView overlay |
| `FLUFFY_NO_RTTI` | `ON` | Compile Fluffy without RTTI |
| `FLUFFY_NO_EXCEPTIONS` | `ON` | Compile Fluffy without exceptions |

GLFW, NanoVG, and gl3w are compiled into the single `libfluffy.a` / `fluffy.lib`; there is no separate GLFW library to install or find.

## 3. Install Fluffy

### macOS and Linux

```bash
./build.sh install
```

This installs to `/usr/local` by default and uses `sudo` automatically when that directory is not writable.

To install somewhere you own, set `PREFIX`:

```bash
PREFIX="$HOME/.local" ./build.sh install
```

The prefix is configured into the build and into the exported CMake package files.

### Windows

```bat
build.bat install
```

The default prefix is `%LOCALAPPDATA%\Fluffy`, which does not require administrator rights. Choose another location with `PREFIX`:

```bat
set PREFIX=C:\libs\Fluffy
build.bat install
```

For a machine-wide install, open an elevated Command Prompt:

```bat
set PREFIX=%ProgramFiles%\Fluffy
build.bat install
```

### Install with plain CMake

```bash
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$HOME/.local"
cmake --build build --target fluffy --config Release --parallel
cmake --install build --config Release
```

On multi-config generators such as Visual Studio or Xcode, the `--config Release` arguments select the release binaries. `CMAKE_BUILD_TYPE` is ignored there.

## 4. What gets installed

A typical macOS or Linux prefix contains:

```text
<prefix>/
├── include/
│   ├── GLFW/
│   │   ├── glfw3.h
│   │   └── glfw3native.h
│   ├── fluffy/
│   │   ├── fluffy.hpp
│   │   ├── window.hpp
│   │   ├── button.hpp
│   │   └── ...
│   └── nanovg.h
├── lib/
│   ├── libfluffy.a
│   └── cmake/
│       └── Fluffy/
└── share/
    └── fluffy/
        ├── LICENSE-Roboto.txt
        └── third_party/
            └── glfw/
                └── LICENSE.md
```

On Windows the include and library layout is the same, using `\` and `.lib` extensions with MSVC (`.a` with MinGW).

The important paths are:

- **Fluffy headers:** `include/fluffy/`
- **NanoVG header:** `include/nanovg.h` — Fluffy's public headers include it as `<nanovg.h>`
- **GLFW headers:** `include/GLFW/`
- **Libraries:** `lib/`
- **CMake package:** `lib/cmake/Fluffy/`
- **Licenses and third-party notices:** `share/fluffy/`

Fluffy is installed as one library: `libfluffy.a` on Unix-like systems and `fluffy.lib` with MSVC. GLFW, NanoVG, and gl3w are compiled directly into it. Only the GLFW headers are installed separately, for applications that interact with a `GLFWwindow`. There is no separate GLFW library or runtime DLL.

## 5. Use Fluffy from your own CMake project

The recommended approach is to use the installed CMake package. Create a `CMakeLists.txt` like this:

```cmake
cmake_minimum_required(VERSION 3.20)
project(myapp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Fluffy CONFIG REQUIRED)

add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE fluffy::fluffy)
```

Then configure with the prefix you installed into:

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH="$HOME/.local"
cmake --build build --config Release
```

On Windows:

```bat
cmake -S . -B build -DCMAKE_PREFIX_PATH="%LOCALAPPDATA%\Fluffy"
cmake --build build --config Release
```

`fluffy::fluffy` carries everything your app needs: the include directories, C++20 requirement, NanoVG, gl3w, GLFW, OpenGL, and the required platform frameworks. Do not add Fluffy's headers or libraries to your global compiler search paths.

If CMake cannot find the package, point it directly at the package directory:

```bash
cmake -S . -B build -DFluffy_DIR="$HOME/.local/lib/cmake/Fluffy"
```

or on Windows:

```bat
cmake -S . -B build -DFluffy_DIR="%LOCALAPPDATA%\Fluffy\lib\cmake\Fluffy"
```

You can then write normal Fluffy code, for example the program from [Hello Fluffy](tutorial/01-hello-fluffy.md).

### CMake with an existing project

If your project already uses CMake, add the `find_package` call and link `fluffy::fluffy`. No manual `include_directories()` or `link_directories()` are required.

If you vendor Fluffy with `add_subdirectory(fluffy)`, link the in-tree target `fluffy` instead of `fluffy::fluffy`. The install rules and exported namespace are only used after installation.

## 6. Compile the helloworld example manually

CMake is strongly recommended because it knows the platform libraries and link order. The commands below show what it does under the hood and are useful for learning or for small standalone builds.

These commands assume:

- You are in the Fluffy source checkout.
- `PREFIX` points at the install prefix.
- The example source is `examples/helloworld/main.cpp`.

### macOS

```bash
PREFIX="$HOME/.local"

c++ -std=c++20 -I"$PREFIX/include" examples/helloworld/main.cpp \
    "$PREFIX/lib/libfluffy.a" \
    -framework Cocoa -framework IOKit -framework CoreFoundation \
    -framework OpenGL \
    -o helloworld

./helloworld
```

Static libraries must come after the object file that uses them. If your application uses `fluffy::WebView`, add `-framework WebKit`.

### Linux

```bash
PREFIX="$HOME/.local"

c++ -std=c++20 -I"$PREFIX/include" examples/helloworld/main.cpp \
    "$PREFIX/lib/libfluffy.a" \
    $(pkg-config --libs gdk-pixbuf-2.0) \
    -lGL -ldl -lpthread -lm -lrt \
    -o helloworld

./helloworld
```

GLFW loads the X11 or Wayland client libraries at runtime, so `-ldl` is required. If you use `WebView`, also add the libraries from `pkg-config --libs webkit2gtk-4.1`.

### Windows with MSVC

Open an “x64 Native Tools Command Prompt for VS 2022”:

```bat
set PREFIX=%LOCALAPPDATA%\Fluffy

cl /std:c++20 /EHsc /I"%PREFIX%\include" examples\helloworld\main.cpp ^
    "%PREFIX%\lib\fluffy.lib" ^
    opengl32.lib gdi32.lib user32.lib shell32.lib ^
    /Fe:helloworld.exe

helloworld.exe
```

If you use `WebView`, add `advapi32.lib ole32.lib shlwapi.lib version.lib`.

### Windows with MinGW

```bat
set PREFIX=%LOCALAPPDATA%\Fluffy

g++ -std=c++20 -I"%PREFIX%\include" examples\helloworld\main.cpp ^
    -L"%PREFIX%\lib" ^
    -lfluffy ^
    -lopengl32 -lgdi32 -luser32 -lshell32 ^
    -o helloworld.exe
```

## Troubleshooting

**CMake cannot find Fluffy**

Set `CMAKE_PREFIX_PATH` to the install prefix, or set `Fluffy_DIR` to `<prefix>/lib/cmake/Fluffy`. Make sure you use the same prefix that `./build.sh install` or `build.bat install` printed.

**`nanovg.h` is not found**

The prefix is from an older Fluffy build. Reinstall with the current source tree; the NanoVG header is installed at `<prefix>/include/nanovg.h`.

**The WebView widget is disabled on Linux**

Install `libwebkit2gtk-4.1-dev` (`webkit2gtk4.1-devel` on Fedora), then reconfigure. You can also build without WebView using `-DFLUFFY_ENABLE_WEBVIEW=OFF`.

**The first configure fails while downloading dependencies**

Fluffy needs network access during the first configure to fetch GLFW, NanoVG, webview, and Lucide SVG files. Check your proxy or firewall settings, or configure once on a machine with network access and reuse the build directory.
