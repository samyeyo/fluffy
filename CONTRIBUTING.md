# Contributing to Fluffy

Thanks for your interest in Fluffy! This document covers the practical steps
for setting up the project, building examples, and submitting changes.

## Quick start

```bash
./build.sh                 # configure (Release) + build library only
./build.sh examples        # build with examples enabled
./build.sh wasm            # WebAssembly library + documentation demos
```

After build the examples are at `build/examples/helloworld/helloworld`,
`build/examples/icons/icons`, and `build/examples/yamlui/yamlui`.

The build scripts accept `examples`, `debug`, and `clean`. On macOS/Linux,
`./build.sh wasm` also builds the WebAssembly library and documentation demos;
it requires the Emscripten SDK. See `build.sh` or `build.bat` for the full
command reference.

## Project structure

| Path | Purpose |
|---|---|
| `include/fluffy/` | Public headers — every widget's API surface |
| `src/` | Implementation files |
| `examples/` | Runnable demos |
| `cmake/` | CMake helper scripts and dependency fetchers |
| `doc/` | Documentation and tutorials |
| `LICENSE` | MIT license terms for the project |

## Documentation

The guides in `doc/` are the reference for changing each area:

- [doc/readme.md](doc/readme.md) — documentation index and suggested reading path
- [doc/custom.md](doc/custom.md) — custom widget bases, painting, events and animation
- [doc/widget.md](doc/widget.md) — `Widget` API, sizing, visibility and focus
- [doc/layouts.md](doc/layouts.md) — layouts, spacing, alignment and constraints
- [doc/yaml.md](doc/yaml.md) — YAML UI loader, properties and widget registration
- [doc/theme.md](doc/theme.md) — colors, metrics, fonts and theme loading
- [doc/install.md](doc/install.md) — building, installing and consuming the CMake package

## Coding conventions

Fluffy follows a consistent set of conventions. Key points:

- **Naming:** types are PascalCase, functions/methods are camelCase,
  member variables trail with underscore (`widgets_`), local variables
  are snake_case.
- **Comments:** every declaration (class, struct, function, member
  variable) must be preceded by `// ––––––––[ name ] short description`
  with 20 en-dashes.
- **No trailing inline comments:** use a preceding comment line instead.
- **Section separators:** 80 en-dashes between logical sections.
- **File headers:** every `.cpp` and `.hpp` starts with a file-level
  banner plus the MIT License line.
- **Includes order:** own header, blank line, standard library, blank
  line, third-party, blank line, project headers.
- **Formatting:** 4-space indent, Stroustrup braces, 79-char comments,
  100-char code.

## Making a pull request

1. Create a feature branch from `main`.
2. Make your changes following these conventions.
3. Build all targets and verify they compile with zero errors:
   ```bash
   cmake --build build --parallel
   ```
4. If you added new widgets, register them with
   `WidgetFactory::registerWidget()` and add them to the YAML loader's
   type table as described in [doc/yaml.md](doc/yaml.md).
5. Open a PR with a short description of what changed and why.

## Adding a new widget

1. Create `include/fluffy/my_widget.hpp` and `src/my_widget.cpp`.
2. Choose the simplest base that fits:
   - Derive from `fluffy::SimpleWidget` for controls that can be painted with
     rectangles, rounded rectangles, rings and text. Implement `paint()` and,
     if needed, `onPress()`; the base handles NanoVG setup, hover plumbing and
     size hints.
   - Derive from `fluffy::Widget` when you need raw NanoVG paths, custom
     event handling, animation, or full control over drawing and layout.
   See [doc/custom.md](doc/custom.md) and [doc/widget.md](doc/widget.md) for
   the differences and complete examples.
3. Register it in `WidgetFactory` inside `src/factory.cpp`:
   ```cpp
   WidgetFactory::registerWidget("MyWidget", [] {
       return std::make_unique<MyWidget>();
   });
   ```
4. If it needs custom YAML properties, override `setProperty()`
5. Add it to the project layout in `README.md` and give it a page in `doc/`
   following the existing widget pages.

## Running examples

```bash
./build.sh examples
./build/examples/helloworld/helloworld
./build/examples/icons/icons
./build/examples/yamlui/yamlui
```

Or build + run a single target:

```bash
cmake --build build --target helloworld --parallel
./build/examples/helloworld/helloworld
```

## License

MIT — see [LICENSE](LICENSE).
