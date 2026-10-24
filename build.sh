#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="build"
DO_INSTALL=false
DO_WASM=false

case "${1:-}" in
    clean)
        echo "Cleaning build directories..."
        _had_svg=false
        if [ -d "$BUILD_DIR/third_party/lucide_svg" ]; then
            mv "$BUILD_DIR/third_party/lucide_svg" /tmp/fluffy_lucide_svg_backup
            _had_svg=true
        fi
        rm -rf "$BUILD_DIR" "${ROOT}/build-wasm"
        if [ "$_had_svg" = true ]; then
            mkdir -p "$BUILD_DIR/third_party"
            mv /tmp/fluffy_lucide_svg_backup "$BUILD_DIR/third_party/lucide_svg"
        fi
        echo "Done."
        exit 0
        ;;
    examples)
        echo "Configuring with examples..."
        cmake -S . -B "$BUILD_DIR" -DFLUFFY_BUILD_EXAMPLES=ON
        ;;
    debug)
        echo "Configuring Debug..."
        cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug
        ;;
    install)
        echo "Configuring release for install..."
        INSTALL_PREFIX="${PREFIX:-/usr/local}"
        cmake -S . -B "$BUILD_DIR" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        DO_INSTALL=true
        ;;
    wasm)
        DO_WASM=true
        BUILD_DIR="${ROOT}/build-wasm"

        if ! command -v emcc >/dev/null 2>&1; then
            if [ -f "/tmp/emsdk/emsdk_env.sh" ]; then
                echo "→ sourcing /tmp/emsdk/emsdk_env.sh"
                # shellcheck disable=SC1091
                source /tmp/emsdk/emsdk_env.sh
            else
                echo "error: emcc not found. Install emsdk first:"
                echo "  git clone https://github.com/emscripten-core/emsdk /tmp/emsdk"
                echo "  /tmp/emsdk/emsdk install latest && /tmp/emsdk/emsdk activate latest && source /tmp/emsdk/emsdk_env.sh"
                exit 1
            fi
        fi

        echo "→ emcc $(emcc --version | head -n1)"

        # Reuse the native build's fetched dependencies when available.
        NANOVG_SRC="${ROOT}/build/_deps/nanovg-src"
        GLFW_SRC="${ROOT}/build/_deps/glfw-src"
        EXTRA_FETCH=""
        if [ -d "$NANOVG_SRC" ]; then
            EXTRA_FETCH="-DFETCHCONTENT_SOURCE_DIR_NANOVG=$NANOVG_SRC"
        fi
        if [ -d "$GLFW_SRC" ]; then
            EXTRA_FETCH="$EXTRA_FETCH -DFETCHCONTENT_SOURCE_DIR_GLFW=$GLFW_SRC"
        fi

        rm -rf "$BUILD_DIR"
        mkdir -p "$BUILD_DIR"

        echo "→ Configuring WebAssembly..."
        emcmake cmake -S "$ROOT" -B "$BUILD_DIR" \
            -DCMAKE_BUILD_TYPE=Release \
            -DFLUFFY_BUILD_EXAMPLES=ON \
            -DFLUFFY_ENABLE_WEBVIEW=OFF \
            -DCMAKE_EXECUTABLE_SUFFIX=".js" \
            ${EXTRA_FETCH} \
            -DCMAKE_CXX_FLAGS="-s USE_GLFW=3 -s USE_WEBGL2=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY=0" \
            -DCMAKE_EXE_LINKER_FLAGS="-s USE_GLFW=3 -s USE_WEBGL2=1 -s WASM=1 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY=0 -s INVOKE_RUN=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap']"
        ;;
    *)
        echo "Configuring..."
        cmake -S . -B "$BUILD_DIR"
        ;;
esac

if [ "$DO_WASM" = true ]; then
    echo ""
    echo "Building WebAssembly library..."
    cmake --build "$BUILD_DIR" --target fluffy --parallel

    echo ""
    echo "Compiling documentation demos to ${ROOT}/www/demo..."
    python3 "${ROOT}/www/build_demos.py" \
        --jobs "$(sysctl -n hw.ncpu 2>/dev/null || nproc 2>/dev/null || echo 4)"

    echo ""
    echo "WebAssembly build complete."
    echo "Serve with:"
    echo "  python3 -m http.server --directory ${ROOT}/www 8000"
    echo "  open http://localhost:8000/"
    exit 0
fi

echo ""
echo "Building..."
if [ "$DO_INSTALL" = true ]; then
    cmake --build "$BUILD_DIR" --target fluffy --config Release --parallel
else
    cmake --build "$BUILD_DIR" --parallel
fi

echo ""
if [ "$DO_INSTALL" = true ]; then
    echo "Installing Fluffy..."
    INSTALL_PARENT="$(dirname "$INSTALL_PREFIX")"
    if [ -w "$INSTALL_PREFIX" ] || [ "$(id -u)" -eq 0 ] || \
       { [ ! -e "$INSTALL_PREFIX" ] && [ -w "$INSTALL_PARENT" ]; }; then
        cmake --install "$BUILD_DIR" --config Release
    else
        echo "Administrator permission is required to install into $INSTALL_PREFIX."
        sudo "$(command -v cmake)" --install "$BUILD_DIR" --config Release
    fi
    echo "Installed."
else
    echo "Build complete."
fi
