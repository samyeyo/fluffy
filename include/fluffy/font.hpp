// include/fluffy/font.hpp

// ––––––––––––––––––––[ font.hpp ] system font discovery and loading.

#pragma once
#include <string>
#include <vector>

namespace fluffy {

// ––––––––[ findSystemFont ] locates a font file by family name.
// Returns the first matching file path, or empty string if not found.
// The match is case-insensitive against the font file basename.
std::string findSystemFont(const std::string& family_name);

// ––––––––[ listSystemFonts ] returns all discoverable system font file paths.
std::vector<std::string> listSystemFonts();

// ––––––––[ listLatinFonts ] returns only fonts that contain a Latin cmap.
// Fonts without Latin glyphs (e.g. Noto Sans for non-Latin scripts) are excluded.
std::vector<std::string> listLatinFonts();

// ––––––––[ fontDisplayName ] returns the human-readable name of a font file,
// read from its OpenType `name` table (e.g. "Verdana Italic" for verdanai.ttf).
// Falls back to the file stem when the table cannot be read.
std::string fontDisplayName(const std::string& path);

}  // namespace fluffy
