// src/textinput.cpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// TextInput — draw, handleEvent, tick with cursor, selection, undo/redo, clipboard.

#include <fluffy/textinput.hpp>
#include <fluffy/theme.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>
#include <nanovg.h>
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
namespace fluffy {

// ––––––––[ TextInput ] constructs with bounds and optional placeholder.
TextInput::TextInput(Rect bounds, std::string placeholder)
    : Widget(bounds),
      placeholder_(std::move(placeholder))
{
    rebuildLineIndex();
}

// ––––––––[ pushState ] saves current state to undo stack, clears redo.
void TextInput::pushState() {
    redo_stack_.clear();
    undo_stack_.push_back({text_, cursor_pos_, sel_anchor_});
    if (undo_stack_.size() > kMaxUndo)
        undo_stack_.erase(undo_stack_.begin());
}

// ––––––––[ undo ] restores previous state, pushes current to redo.
void TextInput::undo() {
    if (undo_stack_.empty()) return;
    redo_stack_.push_back({text_, cursor_pos_, sel_anchor_});
    auto s = std::move(undo_stack_.back());
    undo_stack_.pop_back();
    text_       = std::move(s.text);
    cursor_pos_ = s.cursor_pos;
    sel_anchor_ = s.sel_anchor;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ redo ] restores next state, pushes current to undo.
void TextInput::redo() {
    if (redo_stack_.empty()) return;
    undo_stack_.push_back({text_, cursor_pos_, sel_anchor_});
    auto s = std::move(redo_stack_.back());
    redo_stack_.pop_back();
    text_       = std::move(s.text);
    cursor_pos_ = s.cursor_pos;
    sel_anchor_ = s.sel_anchor;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ selectAll ] selects entire text.
void TextInput::selectAll() {
    sel_anchor_ = 0;
    cursor_pos_ = static_cast<int>(text_.size());
    selecting_  = false;
    resetCursorBlink();
}

// ––––––––[ copySelection ] copies selected text to system clipboard.
void TextInput::copySelection(const Window& w) const {
    if (!hasSelection() || text_.empty()) return;
    const int s = selStart();
    const int e = selEnd();
    const auto sel = text_.substr(static_cast<std::size_t>(s),
                                   static_cast<std::size_t>(e - s));
    const_cast<Window&>(w).setClipboardString(sel.c_str());
}

// ––––––––[ pasteText ] inserts clipboard text at cursor, replaces selection.
// When multiline with a prefix, pasted content goes on line 2 — a leading \n
// is added so the prefix keeps its own line.
void TextInput::pasteText(const char* str) {
    if (!str || !*str) return;
    pushState();
    if (hasSelection()) deleteSelection();
    if (multiline_ && text_.empty() && !prefix_.empty()) {
        text_.push_back('\n');
        cursor_pos_ = 1;
        sel_anchor_ = 1;
    }
    const std::string_view sv(str);
    text_.insert(static_cast<std::size_t>(cursor_pos_), sv);
    cursor_pos_ += static_cast<int>(sv.size());
    sel_anchor_ = cursor_pos_;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ setText ] replaces text, clears selection and undo stacks.
void TextInput::setText(std::string t) {
    pushState();
    text_ = std::move(t);
    cursor_pos_ = std::min(cursor_pos_, static_cast<int>(text_.size()));
    sel_anchor_ = cursor_pos_;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ setFocused ] enables/disables focus, clears selection on blur.
void TextInput::setFocused(bool f) noexcept {
    focused_ = f;
    if (!focused_) {
        cursor_visible_ = false;
        selecting_ = false;
    }
    resetCursorBlink();
}

// ––––––––[ resetCursorBlink ] shows cursor and resets timer.
void TextInput::resetCursorBlink() {
    cursor_blink_timer_ = 0.0f;
    cursor_visible_ = true;
}

// ––––––––[ insertChar ] inserts a Unicode codepoint at cursor.
// When multiline with a prefix, the first insertion prepends \n so the prefix
// keeps its own line and entered text starts on the line below.
void TextInput::insertChar(char32_t cp) {
    pushState();
    if (hasSelection()) deleteSelection();
    if (multiline_ && text_.empty() && !prefix_.empty() && cp != U'\n') {
        text_.push_back('\n');
        cursor_pos_ = 1;
        sel_anchor_ = 1;
    }
    char buf[5]{};
    int n = 0;
    if      (cp < 0x80)       { buf[0] = static_cast<char>(cp);         n = 1; }
    else if (cp < 0x800)      { buf[0] = static_cast<char>(0xC0 | (cp >> 6));
                                 buf[1] = static_cast<char>(0x80 | (cp & 0x3F));
                                 n = 2; }
    else if (cp < 0x10000)    { buf[0] = static_cast<char>(0xE0 | (cp >> 12));
                                 buf[1] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                                 buf[2] = static_cast<char>(0x80 | (cp & 0x3F));
                                 n = 3; }
    else                      { buf[0] = static_cast<char>(0xF0 | (cp >> 18));
                                 buf[1] = static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                                 buf[2] = static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                                 buf[3] = static_cast<char>(0x80 | (cp & 0x3F));
                                 n = 4; }
    const int pos = std::clamp(cursor_pos_, 0, static_cast<int>(text_.size()));
    text_.insert(static_cast<std::size_t>(pos), buf, static_cast<std::size_t>(n));
    cursor_pos_ = pos + n;
    sel_anchor_ = cursor_pos_;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ backspace ] deletes character before cursor.
void TextInput::backspace() {
    if (hasSelection()) { pushState(); deleteSelection(); resetCursorBlink(); if (on_change_) on_change_(text_); return; }
    if (cursor_pos_ <= 0) return;
    pushState();
    int begin = cursor_pos_;
    while (begin > 0 && (static_cast<unsigned char>(text_[static_cast<std::size_t>(begin - 1)]) & 0xC0) == 0x80)
        --begin;
    --begin;
    text_.erase(static_cast<std::size_t>(begin),
                static_cast<std::size_t>(cursor_pos_ - begin));
    cursor_pos_ = begin;
    sel_anchor_ = cursor_pos_;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ deleteForward ] deletes character after cursor.
void TextInput::deleteForward() {
    if (hasSelection()) { pushState(); deleteSelection(); resetCursorBlink(); if (on_change_) on_change_(text_); return; }
    if (cursor_pos_ >= static_cast<int>(text_.size())) return;
    pushState();
    int end = cursor_pos_ + 1;
    while (end < static_cast<int>(text_.size()) &&
           (static_cast<unsigned char>(text_[static_cast<std::size_t>(end)]) & 0xC0) == 0x80)
        ++end;
    text_.erase(static_cast<std::size_t>(cursor_pos_),
                static_cast<std::size_t>(end - cursor_pos_));
    sel_anchor_ = cursor_pos_;
    selecting_  = false;
    rebuildLineIndex();
    resetCursorBlink();
    if (on_change_) on_change_(text_);
}

// ––––––––[ deleteSelection ] erases the selected range.
void TextInput::deleteSelection() {
    if (!hasSelection()) return;
    const int s = selStart();
    const int e = selEnd();
    text_.erase(static_cast<std::size_t>(s), static_cast<std::size_t>(e - s));
    cursor_pos_ = s;
    sel_anchor_ = s;
    selecting_  = false;
    rebuildLineIndex();
}

// ––––––––[ moveCursorLeft ] moves cursor one codepoint left.
void TextInput::moveCursorLeft() {
    if (cursor_pos_ <= 0) return;
    --cursor_pos_;
    while (cursor_pos_ > 0 &&
           (static_cast<unsigned char>(text_[static_cast<std::size_t>(cursor_pos_)]) & 0xC0) == 0x80)
        --cursor_pos_;
    resetCursorBlink();
}

// ––––––––[ moveCursorRight ] moves cursor one codepoint right.
void TextInput::moveCursorRight() {
    if (cursor_pos_ >= static_cast<int>(text_.size())) return;
    ++cursor_pos_;
    while (cursor_pos_ < static_cast<int>(text_.size()) &&
           (static_cast<unsigned char>(text_[static_cast<std::size_t>(cursor_pos_)]) & 0xC0) == 0x80)
        ++cursor_pos_;
    resetCursorBlink();
}

// ––––––––[ moveCursorHome / moveCursorEnd ] jump to start/end of text.
void TextInput::moveCursorHome() { cursor_pos_ = 0; resetCursorBlink(); }
void TextInput::moveCursorEnd()  { cursor_pos_ = static_cast<int>(text_.size()); resetCursorBlink(); }

// ––––––––[ prefixWidth ] measures the rendered width of the prefix Inline content.
float TextInput::prefixWidth(const Theme& theme, const Window& w) const {
    if (prefix_.empty()) return 0.0f;
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 0.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    const_cast<Window&>(w).prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    const float iconGain = theme.metrics.animation.icon_size_gain;
    const float iconPad  = theme.metrics.animation.icon_padding;
    float total = 0.0f;
    for (const auto& r : prefix_) {
        if (r.type == Inline::Run::Type::SvgIcon) {
            const float rs = r.size > 0.0f ? r.size : size;
            total += rs * iconGain * (1.0f + 2.0f * iconPad);
        } else {
            total += nvgTextBounds(vg, 0, 0, r.utf8.c_str(), nullptr, nullptr);
        }
    }
    return total;
}

// ––––––––[ textX ] returns the pixel x of the editable text start inside the widget.
// In multiline mode with a prefix, the prefix acts as a fixed header row above
// the text, so the text starts at the normal padded position (no prefix offset).
float TextInput::textX(const Theme& theme, const Window& w) const {
    float x = bounds_.x + theme.metrics.input_padding_x;
    if (!prefix_.empty() && !multiline_) x += prefixWidth(theme, w) + 4.0f;
    return x;
}

// ––––––––[ rebuildLineIndex ] rebuilds the cached line_starts_ vector after any text mutation.
// O(N) walk; called once per typing event, so per-frame renderers can do
// O(log N) line lookups and O(col_width) measurements instead of an
// O(line_length) backward scan on every cursorXFromIndex call.
void TextInput::rebuildLineIndex() noexcept {
    line_starts_.clear();
    line_starts_.push_back(0);
    for (std::size_t i = 0; i < text_.size(); ++i) {
        if (text_[i] == '\n') {
            line_starts_.push_back(static_cast<int>(i) + 1);
        }
    }
}

namespace {
bool isWordChar(unsigned char c) noexcept {
    if (c >= 0x80) return true;
    if (c >= '0' && c <= '9') return true;
    if (c >= 'A' && c <= 'Z') return true;
    if (c >= 'a' && c <= 'z') return true;
    if (c == '_') return true;
    return false;
}
}  // namespace

// ––––––––[ selectWordAt ] selects the word under byte index idx.
void TextInput::selectWordAt(int idx) {
    if (text_.empty()) return;
    const int n = static_cast<int>(text_.size());
    if (idx < 0) idx = 0;
    if (idx > n) idx = n;
    int probe = idx;
    bool atWord = false;
    if (probe < n && isWordChar(static_cast<unsigned char>(text_[static_cast<std::size_t>(probe)]))) {
        atWord = true;
    } else if (probe > 0 && isWordChar(static_cast<unsigned char>(text_[static_cast<std::size_t>(probe - 1)]))) {
        probe -= 1;
        while (probe > 0 && (static_cast<unsigned char>(text_[static_cast<std::size_t>(probe)]) & 0xC0) == 0x80) {
            --probe;
        }
        atWord = true;
        idx = probe;
    }
    if (!atWord) return;
    int start = idx;
    int end = idx;
    while (start > 0) {
        unsigned char c = static_cast<unsigned char>(text_[static_cast<std::size_t>(start - 1)]);
        if (c == '\n') break;
        if (!isWordChar(c)) break;
        --start;
    }
    while (start > 0 && (static_cast<unsigned char>(text_[static_cast<std::size_t>(start)]) & 0xC0) == 0x80) {
        --start;
    }
    while (end < n) {
        unsigned char c = static_cast<unsigned char>(text_[static_cast<std::size_t>(end)]);
        if (c == '\n') break;
        if (!isWordChar(c)) break;
        ++end;
    }
    sel_anchor_ = start;
    cursor_pos_ = end;
    selecting_ = false;
    resetCursorBlink();
}

// ––––––––[ lineCount ] returns number of text lines (1 for empty, mirrors rebuildLineIndex).
int TextInput::lineCount() const noexcept {
    return static_cast<int>(line_starts_.size());
}

// ––––––––[ lineStart ] returns byte index of the start of line `line` (0-based).
int TextInput::lineStart(int line) const noexcept {
    if (line <= 0) return 0;
    if (static_cast<std::size_t>(line) < line_starts_.size()) {
        return line_starts_[static_cast<std::size_t>(line)];
    }
    return static_cast<int>(text_.size());
}

// ––––––––[ lineEnd ] returns byte index of the end of line `line` (excludes \n).
int TextInput::lineEnd(int line) const noexcept {
    const auto next = static_cast<std::size_t>(line) + 1;
    if (next < line_starts_.size()) return line_starts_[next] - 1;
    return static_cast<int>(text_.size());
}

// ––––––––[ cursorLine ] returns the 0-based line number of cursor_pos_.
// O(log N) lookup via std::upper_bound on the cached line_starts_.
int TextInput::cursorLine() const noexcept {
    if (cursor_pos_ <= 0) return 0;
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), cursor_pos_);
    const std::size_t idx = static_cast<std::size_t>(it - line_starts_.begin());
    return idx == 0 ? 0 : static_cast<int>(idx) - 1;
}

// ––––––––[ cursorCol ] returns the column offset on the current line.
int TextInput::cursorCol() const noexcept {
    return cursor_pos_ - lineStart(cursorLine());
}

// ––––––––[ moveCursorUp ] moves cursor to previous line, same visual column.
void TextInput::moveCursorUp(const Theme& theme, const Window& w) {
    const int cl = cursorLine();
    if (cl <= 0) { moveCursorHome(); return; }
    const float target_x = cursorXFromIndex(cursor_pos_, theme, w);
    const int prev_line_end = lineEnd(cl - 1);
    const int prev_line_start = lineStart(cl - 1);
    int best = prev_line_end;
    float best_dx = std::numeric_limits<float>::max();
    for (int i = prev_line_start; i <= prev_line_end; ++i) {
        float cx = cursorXFromIndex(i, theme, w);
        float dx = std::abs(cx - target_x);
        if (dx < best_dx) { best_dx = dx; best = i; }
    }
    cursor_pos_ = std::min(best, prev_line_end);
    sel_anchor_ = cursor_pos_;
    selecting_ = false;
    resetCursorBlink();
}

// ––––––––[ moveCursorDown ] moves cursor to next line, same visual column.
void TextInput::moveCursorDown(const Theme& theme, const Window& w) {
    const int cl = cursorLine();
    if (cl >= lineCount() - 1) { moveCursorEnd(); return; }
    const float target_x = cursorXFromIndex(cursor_pos_, theme, w);
    const int next_line_start = lineStart(cl + 1);
    const int next_line_end = lineEnd(cl + 1);
    int best = next_line_start;
    float best_dx = std::numeric_limits<float>::max();
    for (int i = next_line_start; i <= next_line_end; ++i) {
        float cx = cursorXFromIndex(i, theme, w);
        float dx = std::abs(cx - target_x);
        if (dx < best_dx) { best_dx = dx; best = i; }
    }
    cursor_pos_ = std::min(best, next_line_end);
    sel_anchor_ = cursor_pos_;
    selecting_ = false;
    resetCursorBlink();
}

// ––––––––[ cursorXFromIndex ] measures pixel x within the line containing idx.
// In multiline mode we only consider bytes from the line's start, not the
// cumulative prefix from byte 0.  This makes the value directly comparable
// across Line N and Line M when navigating with up/down arrows or when
// drawing the per-line selection highlight rectangle.  With the cached
// line_starts_, the line membership is O(log N) instead of an O(line_length)
// backward scan.
float TextInput::cursorXFromIndex(int idx, const Theme& theme, const Window& w) const {
    if (idx <= 0) return 0.0f;
    NVGcontext* vg = w.nvgContext();
    if (!vg) return 0.0f;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    // Binary search in line_starts_ for the largest offset <= idx.
    auto it = std::upper_bound(line_starts_.begin(), line_starts_.end(), idx);
    const std::size_t li = static_cast<std::size_t>(it - line_starts_.begin());
    const int line_start = (li == 0) ? 0 : line_starts_[li - 1];
    const std::string sub = text_.substr(static_cast<std::size_t>(line_start),
                                         static_cast<std::size_t>(idx - line_start));
    return nvgTextBounds(vg, 0.0f, 0.0f, sub.c_str(), nullptr, nullptr);
}

// ––––––––[ cursorIndexFromXY ] picks the row from click_y, then the col from click_x.
// click_x: pixel x relative to text origin (textX() returns that origin).
// click_y: pixel y relative to widget top (bounds_.y).
//            For multiline we map click_y to a line index via line_h grid.
int TextInput::cursorIndexFromXY(float click_x, float click_y,
                                  const Theme& theme, const Window& w) const {
    NVGcontext* vg = w.nvgContext();
    if (!vg || text_.empty()) return 0;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());

    // 1. Pick the line from click_y using the same vertical origin
    //    as draw(): input_padding_y + size (+ prefix header when present)
    //    with NVG_ALIGN_MIDDLE. click_y is widget-relative (my - bounds_.y).
    //    In multiline mode the rendered text is offset by scrollY_, so add
    //    it back to map screen y to content y. The line middle is at
    //    text_origin_y + li*line_h, so the hit-grid must be offset by
    //    line_h/2 (top of first line), otherwise clicks near the boundary
    //    map one line above.
    const float line_h = size * 1.3f;
    const int total_lines = lineCount();
    float text_origin_y = theme.metrics.input_padding_y + size;
    if (multiline_ && !prefix_.empty()) {
        text_origin_y += line_h + theme.metrics.input_padding_y;
    }
    if (!multiline_) {
        text_origin_y = bounds_.h * 0.5f;
    }
    int li = 0;
    if (multiline_ && total_lines > 0) {
        const float adj_y = click_y + scrollY_;
        const float top = text_origin_y - line_h * 0.5f;
        int est = static_cast<int>(std::floor((adj_y - top) / line_h));
        if (est < 0) est = 0;
        if (est >= total_lines) est = total_lines - 1;
        li = est;
    }
    if (li < 0) li = 0;
    if (li >= total_lines) li = total_lines - 1;

    // 2. Walk only the bytes of line `li`, left-to-right, finding the col
    //    whose midpoint is past click_x.  click_x = 0 means "before
    //    first char" -- returns lineStart(li).
    const int start = lineStart(li);
    const int end_  = lineEnd(li);
    if (click_x <= 0.0f) return start;
    if (start >= end_) return start;
    float cumulative = 0.0f;
    const char* ptr  = text_.c_str() + start;
    const char* stop = text_.c_str() + end_;
    int idx = start;
    while (ptr < stop) {
        const char* next = ptr + 1;
        while (next < stop && (static_cast<unsigned char>(*next) & 0xC0) == 0x80)
            ++next;
        const float char_width = nvgTextBounds(vg, 0, 0, ptr, next, nullptr);
        if (cumulative + char_width * 0.5f > click_x) break;
        cumulative += char_width;
        ptr = next;
        idx = static_cast<int>(ptr - text_.c_str());
    }
    return idx;
}

// ––––––––[ naturalWidth ] measures prefix + text width + padding.
float TextInput::naturalWidth(const Theme& theme, const Window& w) const {
    if (!growable_) return bounds_.w;
    const float pw = prefixWidth(theme, w);
    if (text_.empty()) {
        return pw + (multiline_ ? 120.0f : 80.0f)
               + theme.metrics.input_padding_x * 2.0f;
    }
    if (multiline_) {
        float max_w = 0.0f;
        const int n = lineCount();
        for (int i = 0; i < n; ++i) {
            const int start = lineStart(i);
            const int end   = lineEnd(i);
            if (end > start) {
                float lw = cursorXFromIndex(end, theme, w)
                           - cursorXFromIndex(start, theme, w);
                if (lw > max_w) max_w = lw;
            }
        }
        return pw + max_w + theme.metrics.input_padding_x * 2.0f;
    }
    return pw + cursorXFromIndex(static_cast<int>(text_.size()), theme, w)
           + theme.metrics.input_padding_x * 2.0f;
}

// ––––––––[ naturalHeight ] returns preferred height.
// When growable is false and bounds have already been assigned (> 0), report the
// current fixed height so parent auto-size calculations don't grow us with content.
// When bounds are still zero (initial layout), compute the natural text height.
float TextInput::naturalHeight(const Theme& theme, const Window&) const {
    if (!growable_ && bounds_.h > 0.0f) return bounds_.h;
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float line_h = size * 1.3f;
    if (multiline_) {
        const int n = (std::max)(lineCount(), 3);
        return line_h * static_cast<float>(n)
               + theme.metrics.input_padding_y * 2.0f;
    }
    return (std::max)(size + theme.metrics.input_padding_y * 2.0f,
                        theme.metrics.input_min_height);
}

// ––––––––[ contentWidth ] returns total pixel width of all text content (prefix + lines).
float TextInput::contentWidth(const Theme& theme, const Window& w) const {
    const float pw = prefixWidth(theme, w);
    if (text_.empty()) return pw + (multiline_ ? 120.0f : 80.0f);
    if (multiline_) {
        float max_w = 0.0f;
        for (int i = 0, n = lineCount(); i < n; ++i) {
            const int end = lineEnd(i);
            if (end > lineStart(i)) {
                float lw = cursorXFromIndex(end, theme, w)
                           - cursorXFromIndex(lineStart(i), theme, w);
                if (lw > max_w) max_w = lw;
            }
        }
        return pw + max_w;
    }
    return pw + cursorXFromIndex(static_cast<int>(text_.size()), theme, w);
}

// ––––––––[ contentHeight ] returns total pixel height of all text lines.
float TextInput::contentHeight(const Theme& theme, const Window&) const {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const float line_h = size * 1.3f;
    return static_cast<float>(lineCount()) * line_h;
}

// ––––––––[ ensureCursorVisible ] scrolls so the cursor is within the visible area.
void TextInput::ensureCursorVisible(const Theme& theme, const Window& w) {
    const float cursorOffset = cursorXFromIndex(cursor_pos_, theme, w);
    const float pw = prefixWidth(theme, w);
    const float visibleW = bounds_.w - 2.0f * theme.metrics.input_padding_x - 8.0f;
    const float totalContentW = pw + cursorXFromIndex(static_cast<int>(text_.size()), theme, w);
    const float cursorScreenPos = pw + cursorOffset - scrollX_;
    if (cursorScreenPos < 0.0f)
        scrollX_ = (std::max)(0.0f, pw + cursorOffset - 4.0f);
    if (cursorScreenPos + 4.0f > visibleW)
        scrollX_ = (std::max)(0.0f, pw + cursorOffset + 4.0f - visibleW);
    scrollX_ = (std::min)(scrollX_, (std::max)(0.0f, totalContentW - visibleW));
    if (multiline_) {
        const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
        const float line_h = size * 1.3f;
        const float cursorY = static_cast<float>(cursorLine()) * line_h;
        const float prefixLineH = (!prefix_.empty()) ? line_h : 0.0f;
        const float visibleH = bounds_.h - 2.0f * theme.metrics.input_padding_y - 8.0f - prefixLineH;
        const float totalContentH = static_cast<float>(lineCount()) * line_h;
        if (cursorY - scrollY_ < 0.0f)
            scrollY_ = (std::max)(0.0f, cursorY - 2.0f);
        if (cursorY - scrollY_ + line_h > visibleH)
            scrollY_ = (std::max)(0.0f, cursorY + line_h - visibleH);
        scrollY_ = (std::min)(scrollY_, (std::max)(0.0f, totalContentH - visibleH));
    }
}

// ––––––––[ setProperty ] supports "text", "placeholder", "multiline", "growable", plus base props.
void TextInput::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "text")        { setText(value.asString()); return; }
    if (name == "placeholder") { setPlaceholder(value.asString()); return; }
    if (name == "multiline")  { multiline_ = value.asBool(); return; }
    if (name == "growable")   { growable_ = value.asBool(); return; }
    Widget::setProperty(name, value);
}

// ––––––––[ updateHover ] sets IBeam cursor when hovering over the input.
void TextInput::updateHover(float mx, float my) noexcept {
    const bool inside = containsPoint(mx, my);
    hovered_ = inside;
    if (window_ && inside && enabled_) {
        window_->setCursorShape(CursorShape::IBeam);
    }
}

// ––––––––[ draw ] renders background, border, text/placeholder, cursor and selection.
void TextInput::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    const float rx = bounds_.x, ry = bounds_.y;
    const float rw = bounds_.w, rh = bounds_.h;
    const float corner = theme.metrics.corner_radius;
    const bool  has_focus = focused_;
    nvgBeginPath(vg);
    nvgRoundedRect(vg, rx, ry, rw, rh, corner);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g,
                              theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    const Color c_accent = accentColor(theme);
    Color border_col = has_focus ? c_accent : theme.colors.border;
    nvgStrokeWidth(vg, has_focus ? theme.metrics.input_focus_width
                                 : theme.metrics.border_width);
    nvgStrokeColor(vg, nvgRGBA(border_col.r, border_col.g, border_col.b, border_col.a));
    nvgStroke(vg);
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    const std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const float tx = textX(theme, w);
    const float renderTx = tx - scrollX_;
    const float ty = ry + rh * 0.5f;
    // Compute scrollbar visibility so we can narrow the scissor clip.
    const float sbSize = 5.0f;
    const float sbGap  = 2.0f;  // extra gap between edge and scrollbar to avoid focus ring
    const float padX = theme.metrics.input_padding_x;
    const float padY = theme.metrics.input_padding_y;
    const float visW = bounds_.w - 2.0f * padX;
    const float visH = bounds_.h - 2.0f * padY;
    const float cw = contentWidth(theme, w);
    const float ch = multiline_ ? contentHeight(theme, w) : 0.0f;
    const float line_h = size * 1.3f;
    // Prefix header in multiline: the prefix occupies one full line (the header
    // row).  The text visible height is reduced by one line_h so the vertical
    // scrollbar, its track, and the Y-scroll bound stay in sync with the actual
    // text viewport.
    const float prefixLineH = (multiline_ && !prefix_.empty()) ? line_h : 0.0f;
    const float effVisH = visH - prefixLineH;
    const bool sbH = cw > visW + 1.0f;
    const bool sbV = multiline_ && ch > effVisH + 1.0f;
    // clip text away from scrollbar tracks — both vertical (inset by padX+sbSize)
    // and horizontal (inset by sbSize+sbGap).
    const float clipW = rw - (sbV ? (padX + sbSize) : 0.0f);
    const float clipH = rh - (sbH ? (sbSize + sbGap) : 0.0f);
    // Prefix header in multiline mode must be rendered in its own scissor so it
    // stays visible in the header area while text is clipped to below the header.
    if (!prefix_.empty()) {
        nvgSave(vg);
        nvgIntersectScissor(vg, rx, ry, clipW, clipH);
        const float px = bounds_.x + theme.metrics.input_padding_x;
        const float renderPx = px;
        renderInline(vg, prefix_,
                      renderPx, multiline_ ? (ry + theme.metrics.input_padding_y + size)
                                     : ty,
                      NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE,
                      size,
                      theme.metrics.animation.icon_size_gain,
                      theme.metrics.animation.icon_padding,
                      face.c_str(),
                      theme.colors.on_surface_mute,
                      c_accent);
        nvgRestore(vg);
    }
    // Text + cursor are rendered in a scissor that clips below the prefix header
    // line (multiline with prefix) or uses the full widget area (otherwise).
    // This prevents scrolled text from leaking into the fixed header row.
    {
        const float textClipTop = (multiline_ && !prefix_.empty())
            ? (ry + padY + line_h)
            : ry;
        const float textClipHgt = clipH - (textClipTop - ry);
        nvgSave(vg);
        nvgIntersectScissor(vg, rx, textClipTop, clipW, textClipHgt);
        const float base_y = multiline_ ? (ry + theme.metrics.input_padding_y + size)
                                         : ty;
        const float text_base_y = (multiline_ && !prefix_.empty()) ? (base_y + line_h + padY) : base_y;
        const bool empty = text_.empty();
        if (multiline_ && !empty) {
        const int n = lineCount();
        for (int li = 0; li < n; ++li) {
            const int start = lineStart(li);
            const int end   = lineEnd(li);
            const float ly = text_base_y + static_cast<float>(li) * line_h;
            const float renderLy = ly - scrollY_;
            if (has_focus && hasSelection()) {
                const int sel_s = selStart();
                const int sel_e = selEnd();
                if (sel_e > start && sel_s < end) {
                    const float hl = renderTx + cursorXFromIndex(std::max(sel_s, start), theme, w);
                    const float hr = renderTx + cursorXFromIndex(std::min(sel_e, end),   theme, w);
                    nvgBeginPath(vg);
                    const float sel_h = size * 0.85f;
                    nvgRect(vg, hl, renderLy - sel_h * 0.5f, hr - hl, sel_h);
                    nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b, 80));
                    nvgFill(vg);
                }
            }
            if (end > start) {
                nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                          theme.colors.on_surface.g,
                                          theme.colors.on_surface.b,
                                          theme.colors.on_surface.a));
                const auto line_text = text_.substr(static_cast<std::size_t>(start),
                                                     static_cast<std::size_t>(end - start));
                nvgText(vg, renderTx, renderLy, line_text.c_str(), nullptr);
            }
        }
    } else if (empty && !placeholder_.empty() && !has_focus) {
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface_mute.r,
                                  theme.colors.on_surface_mute.g,
                                  theme.colors.on_surface_mute.b,
                                  theme.colors.on_surface_mute.a));
        nvgText(vg, tx, text_base_y, placeholder_.c_str(), nullptr);
    } else if (!empty) {
        if (has_focus && hasSelection()) {
            const float sel_l = renderTx + cursorXFromIndex(selStart(), theme, w);
            const float sel_r = renderTx + cursorXFromIndex(selEnd(),   theme, w);
            const float sel_h = size * 0.85f;
            const float sel_y = ry + (rh - sel_h) * 0.5f;
            nvgBeginPath(vg);
            nvgRect(vg, sel_l, sel_y, sel_r - sel_l, sel_h);
            nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b, 80));
            nvgFill(vg);
        }
        nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r,
                                  theme.colors.on_surface.g,
                                  theme.colors.on_surface.b,
                                  theme.colors.on_surface.a));
        nvgText(vg, renderTx, text_base_y, text_.c_str(), nullptr);
    }
    if (has_focus && cursor_visible_) {
        const float cx = renderTx + cursorXFromIndex(cursor_pos_, theme, w);
        const float cursor_h = size * 0.85f;
        float cursor_y;
        if (multiline_) {
            // Center the cursor on the text baseline `ly` to match
            // NVG_ALIGN_MIDDLE placement of the rendered text glyphs.
            const float ly = text_base_y + static_cast<float>(cursorLine()) * line_h;
            cursor_y = (ly - scrollY_) - cursor_h * 0.5f;
        } else {
            cursor_y = ry + (rh - cursor_h) * 0.5f;
        }
        nvgBeginPath(vg);
        nvgRect(vg, cx, cursor_y, 1.5f, cursor_h);
        nvgFillColor(vg, nvgRGBA(c_accent.r, c_accent.g, c_accent.b, c_accent.a));
        nvgFill(vg);
    }
        nvgRestore(vg);
    }
    // Draw scrollbars when content overflows visible area.
    // The horizontal bar is lowered by sbGap (2px) from the bottom edge so it
    // does not overlap the focus ring border stroke.
    {
        const bool needH = sbH;
        const bool needV = sbV;
        const float trackW = needV ? (visW - sbSize) : visW;
        // Vertical track also gets sbGap clearance at the bottom, same as horizontal.
        const float trackH = (needH ? (effVisH - sbSize) : effVisH) - sbGap;
        const float scMaxH = (std::max)(1.0f, cw - trackW);
        const float scMaxV = (std::max)(1.0f, ch - trackH);
        const NVGcolor trackCol = nvgRGBA(
            theme.colors.surface.r, theme.colors.surface.g,
            theme.colors.surface.b, theme.colors.surface.a);
        const NVGcolor thumbCol = nvgRGBA(
            theme.colors.on_surface.r, theme.colors.on_surface.g,
            theme.colors.on_surface.b, 40);
        // Horizontal scrollbar
        if (needH) {
            const float trackX = bounds_.x + padX;
            const float trackY = bounds_.y + bounds_.h - sbSize - sbGap;
            const float tw    = trackW;
            const float thumbW = (std::max)(20.0f, tw * tw / cw);
            const float thumbX = trackX + (scrollX_ / scMaxH) * (tw - thumbW);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, trackX, trackY, tw, sbSize, 3.0f);
            nvgFillColor(vg, trackCol);
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, thumbX, trackY, thumbW, sbSize, 3.0f);
            nvgFillColor(vg, thumbCol);
            nvgFill(vg);
        }
        // Vertical scrollbar
        if (needV) {
            const float trackX = bounds_.x + bounds_.w - padX - sbSize;
            const float trackY = bounds_.y + padY + prefixLineH;
            const float th    = trackH;
            const float thumbH = (std::max)(20.0f, th * th / ch);
            const float thumbY = trackY + (scrollY_ / scMaxV) * (th - thumbH);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, trackX, trackY, sbSize, th, 3.0f);
            nvgFillColor(vg, trackCol);
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRoundedRect(vg, trackX, thumbY, sbSize, thumbH, 3.0f);
            nvgFillColor(vg, thumbCol);
            nvgFill(vg);
        }
    }
}

// ––––––––[ tick ] blinks cursor every ~0.53s when focused.
void TextInput::tick(float dt, const Theme& /*theme*/) noexcept {
    if (!focused_) { cursor_visible_ = false; return; }
    cursor_blink_timer_ += dt;
    if (cursor_blink_timer_ >= 0.53f) {
        cursor_blink_timer_ -= 0.53f;
        cursor_visible_ = !cursor_visible_;
    }
}

// ––––––––[ handleEvent ] mouse/keyboard input with selection, clipboard, undo/redo.
EventResult TextInput::handleEvent(const Event& e, const Window& w) {
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    const bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseMove:
        if (selecting_ && focused_) {
            const float tx = textX(w.theme(), w);
            cursor_pos_ = cursorIndexFromXY(mx - tx + scrollX_, my - bounds_.y, w.theme(), w);
            ensureCursorVisible(w.theme(), w);
            resetCursorBlink();
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    case EventKind::MouseDown: {
        if (inside) {
            focused_ = true;
            const_cast<Window&>(w).requestFocus(
                const_cast<TextInput*>(this));
            const float tx = textX(w.theme(), w);
            const int click_idx = cursorIndexFromXY(mx - tx + scrollX_, my - bounds_.y, w.theme(), w);
            const double now = glfwGetTime();
            const bool is_double = (now - last_click_time_ < 0.35);
            if (is_double) {
                selectWordAt(click_idx);
                last_click_time_ = -1e9;
                last_click_idx_ = -1;
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            last_click_time_ = now;
            last_click_idx_ = click_idx;
            cursor_pos_ = click_idx;
            sel_anchor_ = click_idx;
            selecting_ = true;
            resetCursorBlink();
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        }
        return EventResult::Ignored;
    }
    case EventKind::MouseScroll: {
        if (!inside) return EventResult::Ignored;
        const auto& se = std::get<MouseScrollEvent>(e);
        if (multiline_) {
            scrollY_ = (std::max)(0.0f, scrollY_ - se.dy * 40.0f);
            const float size = textSize_ > 0.0f ? textSize_ : w.theme().metrics.text_size;
            const float line_h = size * 1.3f;
            const float prefixLineH = (multiline_ && !prefix_.empty()) ? line_h : 0.0f;
            const float effVisH = bounds_.h - 2.0f * w.theme().metrics.input_padding_y - prefixLineH;
            const float maxY = (std::max)(0.0f, contentHeight(w.theme(), w) - effVisH);
            scrollY_ = (std::min)(scrollY_, maxY);
        }
        scrollX_ = (std::max)(0.0f, scrollX_ + se.dx * 20.0f);
        const float maxX = (std::max)(0.0f, contentWidth(w.theme(), w) - (bounds_.w - 2.0f * w.theme().metrics.input_padding_x));
        scrollX_ = (std::min)(scrollX_, maxX);
        return EventResult::Consumed;
    }
    case EventKind::MouseUp:
        if (selecting_) { selecting_ = false; return EventResult::Consumed; }
        if (inside) return EventResult::Consumed;
        return EventResult::Ignored;
    case EventKind::KeyDown: {
        if (!focused_) return EventResult::Ignored;
        const auto& ke = std::get<KeyDownEvent>(e);
        const int key = ke.key;
        const int mods = ke.mods;
        const bool shift = (mods & GLFW_MOD_SHIFT) != 0;
        const bool ctrl  = (mods & GLFW_MOD_CONTROL) != 0;
        const bool cmd   = (mods & GLFW_MOD_SUPER) != 0;
        const bool is_ctrl_cmd = ctrl || cmd;
        const bool meta = ctrl || cmd;

        // Clipboard & undo/redo shortcuts
        if (meta && key == GLFW_KEY_A) { selectAll(); return EventResult::Consumed; }
        if (meta && key == GLFW_KEY_C) { copySelection(w); return EventResult::Consumed; }
        if (meta && key == GLFW_KEY_X) {
            if (hasSelection()) { pushState(); copySelection(w); deleteSelection(); resetCursorBlink(); if (on_change_) on_change_(text_); }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        }
        if (meta && key == GLFW_KEY_V) { pasteText(w.getClipboardString()); ensureCursorVisible(w.theme(), w); return EventResult::Consumed; }
        if (meta && !shift && key == GLFW_KEY_Z) { undo(); ensureCursorVisible(w.theme(), w); return EventResult::Consumed; }
        if (meta &&  shift && key == GLFW_KEY_Z) { redo(); ensureCursorVisible(w.theme(), w); return EventResult::Consumed; }
        if (ctrl && key == GLFW_KEY_Y) { redo(); ensureCursorVisible(w.theme(), w); return EventResult::Consumed; }

        switch (key) {
        case GLFW_KEY_TAB: {
            if (!multiline_) {
                // Unconsumed Tab lets the window run global focus traversal.
                return EventResult::Ignored;
            }
            // Insert 4 spaces instead of a literal tab (NanoVG doesn't render \t)
            pushState();
            if (hasSelection()) deleteSelection();
            static constexpr const char* kTabSpaces = "    ";
            text_.insert(static_cast<std::size_t>(cursor_pos_), kTabSpaces);
            cursor_pos_ += 4;
            sel_anchor_ = cursor_pos_;
            selecting_  = false;
            rebuildLineIndex();
            resetCursorBlink();
            if (on_change_) on_change_(text_);
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        }
        case GLFW_KEY_BACKSPACE: backspace();      ensureCursorVisible(w.theme(), w); return EventResult::Consumed;
        case GLFW_KEY_DELETE:    deleteForward();   ensureCursorVisible(w.theme(), w); return EventResult::Consumed;
        case GLFW_KEY_LEFT:
            if (!shift && hasSelection()) {
                cursor_pos_ = selStart();
                sel_anchor_ = cursor_pos_;
                selecting_  = false;
                resetCursorBlink();
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            if (is_ctrl_cmd) moveCursorHome();
            else             moveCursorLeft();
            if (!shift) {
                selecting_  = false;
                sel_anchor_ = cursor_pos_;
            }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        case GLFW_KEY_RIGHT:
            if (!shift && hasSelection()) {
                cursor_pos_ = selEnd();
                sel_anchor_ = cursor_pos_;
                selecting_  = false;
                resetCursorBlink();
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            if (is_ctrl_cmd) moveCursorEnd();
            else             moveCursorRight();
            if (!shift) {
                selecting_  = false;
                sel_anchor_ = cursor_pos_;
            }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        case GLFW_KEY_HOME:
            if (!shift && hasSelection()) {
                cursor_pos_ = selStart();
                sel_anchor_ = cursor_pos_;
                selecting_  = false;
                resetCursorBlink();
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            moveCursorHome();
            if (!shift) {
                selecting_  = false;
                sel_anchor_ = cursor_pos_;
            }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        case GLFW_KEY_END:
            if (!shift && hasSelection()) {
                cursor_pos_ = selEnd();
                sel_anchor_ = cursor_pos_;
                selecting_  = false;
                resetCursorBlink();
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            moveCursorEnd();
            if (!shift) {
                selecting_  = false;
                sel_anchor_ = cursor_pos_;
            }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        case GLFW_KEY_ENTER:
        case GLFW_KEY_KP_ENTER:
            if (multiline_) {
                insertChar(U'\n');
            } else {
                if (on_submit_) on_submit_(text_);
                triggerEvent("onSubmit", {std::string(text_)});
            }
            ensureCursorVisible(w.theme(), w);
            return EventResult::Consumed;
        case GLFW_KEY_UP:
            if (multiline_) {
                if (!focused_) return EventResult::Ignored;
                if (!shift && hasSelection()) {
                    cursor_pos_ = selStart();
                    sel_anchor_ = cursor_pos_;
                    selecting_  = false;
                    resetCursorBlink();
                    ensureCursorVisible(w.theme(), w);
                    return EventResult::Consumed;
                }
                moveCursorUp(w.theme(), w);
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            return EventResult::Ignored;
        case GLFW_KEY_DOWN:
            if (multiline_) {
                if (!focused_) return EventResult::Ignored;
                if (!shift && hasSelection()) {
                    cursor_pos_ = selEnd();
                    sel_anchor_ = cursor_pos_;
                    selecting_  = false;
                    resetCursorBlink();
                    ensureCursorVisible(w.theme(), w);
                    return EventResult::Consumed;
                }
                moveCursorDown(w.theme(), w);
                ensureCursorVisible(w.theme(), w);
                return EventResult::Consumed;
            }
            return EventResult::Ignored;
        default:
            return EventResult::Consumed;
        }
    }
    case EventKind::Char: {
        if (!focused_) return EventResult::Ignored;
        const auto& ce = std::get<CharEvent>(e);
        const char32_t cp = ce.codepoint;
        if (cp < 0x20 || (cp >= 0x7F && cp <= 0x9F))
            return EventResult::Consumed;
        insertChar(cp);
        ensureCursorVisible(w.theme(), w);
        return EventResult::Consumed;
    }
    default:
        return EventResult::Ignored;
    }
}
}  // namespace fluffy
