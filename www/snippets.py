#!/usr/bin/env python3
# www/snippets.py
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
# Discover every complete C++ program (a fenced block containing `int main(`)
# in doc/ and doc/tutorial/, and give it a stable id, a window size, a flag
# when the program loads a YAML file and a flag when it opens more than one
# window (which cannot run in a single browser canvas).  Shared by
# www/gendocs.py (to tag the code blocks) and www/build_demos.py (to compile
# them to WebAssembly).
#
# The id is "<file-stem>-<index>", where index counts the complete programs
# inside that single Markdown file, starting at zero.  Both consumers derive
# ids the same way, so they always agree.
#
# MIT License - Copyright (c) 2026 - Samir Tine
# ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DOC_DIR = ROOT / "doc"
TUT_DIR = DOC_DIR / "tutorial"

SKIP = {"book.md"}

CPP_LANGS = {"cpp", "c++", "cxx", "cc"}

FENCE_RE = re.compile(r"^(`{3,}|~{3,})\s*([^\s`~]*)\s*$")
OPEN_RE = re.compile(r'Window::open\s*\(\s*"[^"]*"\s*,\s*(\d+)\s*,\s*(\d+)')
WH_RE = re.compile(r"\.width\s*=\s*(\d+)\s*,\s*\.height\s*=\s*(\d+)")
WIDTH_RE = re.compile(r"\.width\s*=\s*(\d+)")
HEIGHT_RE = re.compile(r"\.height\s*=\s*(\d+)")


# ––––––––[ _markdown_files ] every source Markdown file, in a stable order
def _markdown_files():
    files = []
    for directory in (DOC_DIR, TUT_DIR):
        for path in sorted(directory.glob("*.md")):
            if path.name in SKIP:
                continue
            files.append(path)
    return files


# ––––––––[ _fences ] yield (language, code) for every fenced block in text
def _fences(text):
    lines = text.splitlines()
    language = None
    buffer = []
    for line in lines:
        match = FENCE_RE.match(line)
        if match and language is None:
            language = (match.group(2) or "").lower()
            buffer = []
            continue
        if match and language is not None:
            yield language, "\n".join(buffer)
            language = None
            continue
        if language is not None:
            buffer.append(line)


# ––––––––[ _size ] best-effort window size parsed from the program source
def _size(code):
    match = OPEN_RE.search(code)
    if match:
        return int(match.group(1)), int(match.group(2))
    match = WH_RE.search(code)
    if match:
        return int(match.group(1)), int(match.group(2))
    width = WIDTH_RE.search(code)
    height = HEIGHT_RE.search(code)
    if width and height:
        return int(width.group(1)), int(height.group(1))
    return 920, 640


# ––––––––[ _title ] first level-1 heading of a Markdown file
def _title(path):
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return re.sub(r"[*_`]", "", line[2:].strip())
    return path.stem.replace("-", " ").title()


# ––––––––[ _needs_yaml ] true when the program calls win.load("*.yaml")
def _needs_yaml(code):
    return bool(re.search(r'\.load\s*\(\s*"[^"]*\.ya?ml"', code))


# ––––––––[ _windows ] number of top-level windows the program opens
def _windows(code):
    return len(re.findall(r"Window::open\s*\(", code))


# ––––––––[ snippets ] ordered list of complete programs for one Markdown file
def snippets(path):
    path = Path(path)
    title = _title(path)
    found = []
    index = 0
    for language, code in _fences(path.read_text(encoding="utf-8")):
        if language not in CPP_LANGS or "int main(" not in code:
            continue
        width, height = _size(code)
        windows = _windows(code)
        found.append({
            "id": "{}-{}".format(path.stem, index),
            "file": path.name,
            "path": path,
            "index": index,
            "title": title,
            "code": code,
            "width": width,
            "height": height,
            "yaml": _needs_yaml(code),
            "windows": windows,
            "multiwindow": windows > 1,
        })
        index += 1
    if len(found) > 1:
        for number, snippet in enumerate(found, 1):
            snippet["title"] = "{} ({})".format(title, number)
    return found


# ––––––––[ all_snippets ] every complete program across doc/ and tutorial/
def all_snippets():
    collected = []
    for path in _markdown_files():
        collected.extend(snippets(path))
    return collected


# ––––––––[ by_file ] map "<relative-path-without-.md>" -> ordered snippets
def by_file():
    mapping = {}
    for path in _markdown_files():
        key = path.relative_to(DOC_DIR).with_suffix("").as_posix()
        mapping[key] = snippets(path)
    return mapping


if __name__ == "__main__":
    for snippet in all_snippets():
        print("{:24} {:>4}x{:<4} yaml={} {}".format(
            snippet["id"], snippet["width"], snippet["height"],
            int(snippet["yaml"]), snippet["file"]))
