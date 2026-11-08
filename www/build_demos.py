#!/usr/bin/env python3
# www/build_demos.py
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Compile every complete C++ program found in doc/ and doc/tutorial/ (see
# www/snippets.py) to WebAssembly, staging `<id>.js` and `<id>.wasm` into
# www/demo/ so the "Try it" buttons can run them.  Fonts and YAML files are
# embedded in the wasm, so no separate data package is needed.
#
# Requires the Emscripten SDK on PATH and a configured WebAssembly build of
# the library (run ./build.sh wasm once first).  Usage:
#
#     python3 www/build_demos.py            # build everything
#     python3 www/build_demos.py --only nav-0 checkboxes-0
#     python3 www/build_demos.py --jobs 8
#
# MIT License - Copyright (c) 2026 - Samir Tine
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

import argparse
import concurrent.futures
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import snippets  # noqa: E402

ROOT = snippets.ROOT
BUILD = ROOT / "build-wasm"
BUILD_REL = BUILD.relative_to(ROOT)
SNIPPET_SRC = BUILD / "snippets"
DEMO_DIR = ROOT / "www" / "demo"

EMXX = os.environ.get("EMXX", "em++")

# Same compile/link recipe as the WebAssembly build from ./build.sh wasm,
# with the snippet translation unit added on top of the prebuilt archives.
COMMON_FLAGS = [
    "-std=c++20",
    "-O2",
    "-flto",
    "-DNDEBUG",
    "-Iinclude",
    "-Ithird_party",
    "-Ithird_party/nanosvg",
    "-I{}".format((BUILD_REL / "_deps" / "nanovg-src" / "src").as_posix()),
]

LINK_FLAGS = [
    "-s", "USE_GLFW=3",
    "-s", "USE_WEBGL2=1",
    "-s", "WASM=1",
    "-s", "ALLOW_MEMORY_GROWTH=1",
    "-s", "ASYNCIFY=0",
    "-s", "INVOKE_RUN=1",
    "-s", "EXPORTED_RUNTIME_METHODS=ccall,cwrap",
    "--embed-file", "assets/Roboto-Regular.ttf@/assets/Roboto-Regular.ttf",
    "--embed-file", "assets/Roboto-Bold.ttf@/assets/Roboto-Bold.ttf",
    "--embed-file", "assets/LICENSE-Roboto.txt@/assets/LICENSE-Roboto.txt",
]

ARCHIVES = [(BUILD_REL / "libfluffy.a").as_posix()]


# ––––––––[ _yaml_prelude ] preload a matching ui.yaml for YAML programs
def _yaml_prelude(snippet):
    source = snippet["path"].read_text(encoding="utf-8")
    blocks = []
    language = None
    buffer = []
    for line in source.splitlines():
        if language is None and line.strip().startswith("```yaml"):
            language = "yaml"
            buffer = []
            continue
        if language is not None and line.strip() == "```":
            blocks.append("\n".join(buffer))
            language = None
            continue
        if language is not None:
            buffer.append(line)
    text = ""
    for block in blocks:
        if "Label:" in block or "Container:" in block:
            text = block
            break
    yaml_path = SNIPPET_SRC / (snippet["id"] + ".ui.yaml")
    yaml_path.write_text(text + "\n", encoding="utf-8")
    return ["--embed-file", "{}@/ui.yaml".format(yaml_path)]


# ––––––––[ CUSTOM ] curated hand-written demos shipped alongside the snippets
CUSTOM = {
    "nav": ROOT / "examples" / "nav" / "main.cpp",
    "helloworld": ROOT / "examples" / "helloworld" / "main.cpp",
}

# ––––––––[ CUSTOM_INFO ] logical size and flags for each curated demo
CUSTOM_INFO = {
    "nav": {"width": 920, "height": 640, "title": "Navigation"},
    "helloworld": {"width": 400, "height": 240, "title": "Hello Fluffy"},
}


# ––––––––[ _compile_source ] build one source file, returning (id, ok, log)
def _compile_source(demo_id, source, yaml_source=None):
    output = DEMO_DIR / (demo_id + ".js")
    command = [EMXX, str(source), "-o", str(output)]
    command += COMMON_FLAGS + LINK_FLAGS
    if yaml_source is not None:
        command += _yaml_prelude(yaml_source)
    command += ARCHIVES
    result = subprocess.run(command, cwd=str(ROOT), capture_output=True, text=True)
    if result.returncode == 0:
        stale = DEMO_DIR / (demo_id + ".data")
        if stale.exists():
            stale.unlink()
    return demo_id, result.returncode == 0, result.stderr


# ––––––––[ _compile ] build one snippet, returning (id, ok, log)
def _compile(snippet):
    src = SNIPPET_SRC / (snippet["id"] + ".cpp")
    src.write_text(snippet["code"] + "\n", encoding="utf-8")
    return _compile_source(
        snippet["id"], src,
        yaml_source=snippet if snippet["yaml"] else None)


# ––––––––[ main ] extract, compile in parallel, write the manifest
def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--only", nargs="*", help="build only these ids")
    parser.add_argument("--jobs", type=int, default=4, help="parallel builds")
    args = parser.parse_args()

    if shutil.which(EMXX) is None:
        sys.exit("error: {} not found on PATH".format(EMXX))

    for archive in ARCHIVES:
        if not (ROOT / archive).exists():
            sys.exit("error: {} missing — run ./build.sh wasm first".format(archive))

    SNIPPET_SRC.mkdir(parents=True, exist_ok=True)
    DEMO_DIR.mkdir(parents=True, exist_ok=True)

    targets = [(s["id"], s) for s in snippets.all_snippets()]
    targets += [(name, path) for name, path in CUSTOM.items()]
    if args.only:
        wanted = set(args.only)
        targets = [t for t in targets if t[0] in wanted]
    if not targets:
        sys.exit("error: no matching demos")

    failures = []
    done = 0

    def build(entry):
        demo_id, spec = entry
        if isinstance(spec, dict):
            return _compile(spec)
        return _compile_source(demo_id, spec)

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = {pool.submit(build, t): t for t in targets}
        for future in concurrent.futures.as_completed(futures):
            snippet_id, ok, log = future.result()
            done += 1
            status = "ok" if ok else "FAIL"
            print("[{:>3}/{}] {} {}".format(done, len(targets), status, snippet_id))
            if not ok:
                failures.append((snippet_id, log))

    manifest = {
        s["id"]: {
            "title": s["title"],
            "width": s["width"],
            "height": s["height"],
            "yaml": s["yaml"],
            "multiwindow": s["multiwindow"],
        }
        for s in snippets.all_snippets()
    }
    for name in CUSTOM:
        info = CUSTOM_INFO[name]
        manifest[name] = {
            "title": info["title"],
            "width": info["width"],
            "height": info["height"],
            "yaml": False,
            "multiwindow": False,
        }
    (DEMO_DIR / "demos.json").write_text(json.dumps(manifest, indent=2) + "\n")

    if failures:
        print("\n{} build(s) failed:\n".format(len(failures)))
        for snippet_id, log in failures:
            print("=== {} ===\n{}".format(snippet_id, log[-2000:]))
        sys.exit(1)

    print("\nall {} demos built into {}".format(len(targets), DEMO_DIR))


if __name__ == "__main__":
    main()
