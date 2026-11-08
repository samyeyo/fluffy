// src/font.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// Font — platform-specific system font discovery.

#include <fluffy/font.hpp>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "stb_truetype.h"

#if defined(__APPLE__)
    // macOS — no extra includes needed, just directory scanning
#elif defined(_WIN32)
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <windows.h>
    #include <shlobj.h>
#endif

namespace fluffy {
namespace fs = std::filesystem;

// ––––––––[ toLower ] lowercase a string for case-insensitive matching.
static std::string toLower(const std::string& s) {
    std::string r = s;
    std::transform(r.begin(), r.end(), r.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return r;
}

// ––––––––[ baseName ] returns the filename without path or extension.
static std::string baseName(const std::string& path) {
    auto name = fs::path(path).stem().string();
    return toLower(name);
}

// ––––––––[ platformFontDirectories ] returns the directories to search.
static std::vector<fs::path> platformFontDirectories() {
    std::vector<fs::path> dirs;
#if defined(__APPLE__)
    dirs.emplace_back("/System/Library/Fonts");
    dirs.emplace_back("/Library/Fonts");
    if (const char* home = std::getenv("HOME"))
        dirs.emplace_back(std::string(home) + "/Library/Fonts");
#elif defined(_WIN32)
    // Windows Fonts directory
    char winDir[MAX_PATH];
    if (GetWindowsDirectoryA(winDir, MAX_PATH)) {
        dirs.emplace_back(std::string(winDir) + "\\Fonts");
    }
    // User fonts directory
    char appData[MAX_PATH];
    if (SHGetFolderPathA(nullptr, CSIDL_LOCAL_APPDATA, nullptr, 0, appData)) {
        dirs.emplace_back(std::string(appData) + "\\Microsoft\\Windows\\Fonts");
    }
#else
    // Linux / BSD — common font directories
    dirs.emplace_back("/usr/share/fonts");
    dirs.emplace_back("/usr/local/share/fonts");
    if (home) {
        dirs.emplace_back(std::string(home) + "/.fonts");
        dirs.emplace_back(std::string(home) + "/.local/share/fonts");
    }
    // XDG_DATA_HOME override
    const char* xdg = std::getenv("XDG_DATA_HOME");
    if (xdg) dirs.emplace_back(std::string(xdg) + "/fonts");
#endif
    return dirs;
}

// ––––––––[ collectFontFiles ] recursively finds all TTF/TTC files in dirs.
static std::vector<fs::path> collectFontFiles(const std::vector<fs::path>& dirs) {
    std::vector<fs::path> files;
    for (const auto& dir : dirs) {
        std::error_code ec;
        if (!fs::exists(dir, ec)) continue;
        for (const auto& entry : fs::recursive_directory_iterator(
                 dir, fs::directory_options::skip_permission_denied, ec)) {
            if (!entry.is_regular_file(ec)) continue;
            const auto ext = toLower(entry.path().extension().string());
            if (ext == ".ttf" || ext == ".ttc") {
                files.push_back(entry.path());
            }
        }
    }
    return files;
}

// ––––––––[ findSystemFont ] locates a font file by family name.
std::string findSystemFont(const std::string& family_name) {
    if (family_name.empty()) return {};
    const std::string needle = toLower(family_name);

    // 1. Try direct path — if the user passes a full path, use it.
    if (fs::exists(family_name)) return family_name;

    // 2. Scan font directories.
    const auto dirs = platformFontDirectories();
    const auto files = collectFontFiles(dirs);

    // Exact match on basename (e.g. "arial" matches "Arial.ttf")
    for (const auto& f : files) {
        if (baseName(f.string()) == needle) return f.string();
    }

    // Partial match — font name contains the needle (e.g. "HelveticaNeue" matches "Helvetica")
    for (const auto& f : files) {
        if (baseName(f.string()).find(needle) != std::string::npos) return f.string();
    }

    return {};
}

// ––––––––[ decodeUtf16BE ] converts a UTF-16BE buffer to UTF-8.
static std::string decodeUtf16BE(const char* data, int len) {
    std::string out;
    for (int i = 0; i + 1 < len; i += 2) {
        const unsigned int cp =
            (static_cast<unsigned char>(data[i]) << 8) |
             static_cast<unsigned char>(data[i + 1]);
        if (cp == 0) break;
        if (cp < 0x80) {
            out.push_back(static_cast<char>(cp));
        } else if (cp < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
            out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
        }
    }
    return out;
}

// ––––––––[ fontDisplayName ] returns the human-readable name of a font file,
// read from its OpenType `name` table, falling back to the file stem.
std::string fontDisplayName(const std::string& path) {
    const std::string fallback = fs::path(path).stem().string();

    FILE* f = std::fopen(path.c_str(), "rb");
    if (!f) return fallback;
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return fallback; }
    auto* buf = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(sz)));
    if (!buf) { std::fclose(f); return fallback; }
    const bool ok = std::fread(buf, 1, static_cast<size_t>(sz), f) == static_cast<size_t>(sz);
    std::fclose(f);
    if (!ok) { std::free(buf); return fallback; }

    stbtt_fontinfo font;
    if (!stbtt_InitFont(&font, buf, stbtt_GetFontOffsetForIndex(buf, 0))) {
        std::free(buf);
        return fallback;
    }

    // Prefer the full name (4), then family (1), then typographic family (16).
    const int ids[] = {4, 1, 16};
    std::string name;
    for (int id : ids) {
        int len = 0;
        const char* s = stbtt_GetFontNameString(&font, &len, 3, 1, 0x0409, id);
        if (s && len > 0) {
            name = decodeUtf16BE(s, len);
        } else {
            s = stbtt_GetFontNameString(&font, &len, 3, 1, 0x0000, id);
            if (s && len > 0) {
                name = decodeUtf16BE(s, len);
            } else {
                s = stbtt_GetFontNameString(&font, &len, 1, 0, 0, id);
                if (s && len > 0) name.assign(s, s + len);
            }
        }
        if (!name.empty()) break;
    }
    std::free(buf);
    return name.empty() ? fallback : name;
}

// ––––––––[ listSystemFonts ] returns all discoverable system font file paths.
std::vector<std::string> listSystemFonts() {
    const auto dirs = platformFontDirectories();
    const auto files = collectFontFiles(dirs);
    std::vector<std::string> result;
    result.reserve(files.size());
    for (const auto& f : files) {
        result.push_back(f.string());
    }
    return result;
}

// ––––––––[ hasLatinGlyphs ] checks if a font file contains Latin characters
// and has TrueType outlines — rejects CFF/OTTO fonts that
// stb_truetype can parse but not rasterize.
static bool hasLatinGlyphs(const fs::path& path) {
    FILE* f = std::fopen(path.string().c_str(), "rb");
    if (!f) return false;
    std::fseek(f, 0, SEEK_END);
    const long sz = std::ftell(f);
    std::fseek(f, 0, SEEK_SET);
    if (sz <= 0) { std::fclose(f); return false; }
    auto* buf = static_cast<unsigned char*>(std::malloc(static_cast<size_t>(sz)));
    if (!buf) { std::fclose(f); return false; }
    const size_t read_bytes = std::fread(buf, 1, static_cast<size_t>(sz), f);
    std::fclose(f);
    if (read_bytes != static_cast<size_t>(sz)) {
        std::free(buf);
        return false;
    }

    // Reject CFF/OTTO fonts — check the sfVersion at the first sub-font offset.
    // Standalone OTTO: bytes 0-3 == "OTTO".
    // TTC (ttcf): read offset of sub-font 0 from the TTC header, then check
    // the 4-byte sfVersion at that offset.
    if (sz >= 12) {
        const int offset = stbtt_GetFontOffsetForIndex(buf, 0);
        if (offset >= 0 && offset + 4 <= sz) {
            const unsigned char* sf = buf + offset;
            if (sf[0] == 'O' && sf[1] == 'T' && sf[2] == 'T' && sf[3] == 'O') {
                std::free(buf);
                return false;
            }
        }
    }

    stbtt_fontinfo font;
    int ok = stbtt_InitFont(&font, buf, stbtt_GetFontOffsetForIndex(buf, 0));
    bool result = false;
    if (ok) {
        result = stbtt_FindGlyphIndex(&font, 0x0041) != 0;
    }
    std::free(buf);
    return result;
}

// ––––––––[ listLatinFonts ] returns only fonts with Latin cmap entries.
std::vector<std::string> listLatinFonts() {
    const auto all = listSystemFonts();
    std::vector<std::string> result;
    result.reserve(all.size());
    for (const auto& path : all) {
        if (hasLatinGlyphs(path)) {
            result.push_back(path);
        }
    }
    return result;
}

}  // namespace fluffy
