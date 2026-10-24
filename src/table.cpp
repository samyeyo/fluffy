// src/table.cpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Table — editable grid with selection and cell editing
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#include <fluffy/table.hpp>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <string>

#include <nanovg.h>

#include <GLFW/glfw3.h>

#include <fluffy/checkbox.hpp>
#include <fluffy/combobox.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/window.hpp>
#include <fluffy/yaml_fwd.hpp>

namespace fluffy {

// ––––––––[ Table ] constructs
Table::Table(Rect bounds)
    : Widget(bounds) {}

// ––––––––[ setColumns ] replaces columns
void Table::setColumns(std::vector<Column> cols) {
    columns_ = std::move(cols);
    for (auto& r : rows_) r.resize(columns_.size());
    sel_row_ = sel_col_ = -1;
    editing_ = false;
}

// ––––––––[ addColumn ] appends
void Table::addColumn(Column col) {
    // If headerInline empty, init from label for glyph support
    if (col.headerInline.empty() && !col.label.empty()) col.headerInline = Inline{} << col.label;
    columns_.push_back(std::move(col));
    for (auto& r : rows_) r.emplace_back();
}

// ––––––––[ setHeader ] sets header Inline
void Table::setHeader(int col, Inline h) {
    if (col < 0 || col >= std::ssize(columns_)) return;
    columns_[static_cast<std::size_t>(col)].headerInline = std::move(h);
    if (!columns_[static_cast<std::size_t>(col)].headerInline.empty()) {
        // Keep label in sync for yaml
        std::string txt;
        for (auto& run : columns_[static_cast<std::size_t>(col)].headerInline.runs()) if (run.type == Inline::Run::Type::Text) txt += run.utf8;
        columns_[static_cast<std::size_t>(col)].label = txt;
    }
}

// ––––––––[ setRows ] replaces rows
void Table::setRows(std::vector<std::vector<std::string>> rows) {
    rows_.clear();
    rows_.reserve(rows.size());
    for (auto& r : rows) {
        std::vector<Inline> ir;
        ir.reserve(columns_.size());
        for (auto& s : r) ir.push_back(Inline{} << s);
        ir.resize(columns_.size());
        rows_.push_back(std::move(ir));
    }
    sel_row_ = sel_col_ = -1;
    editing_ = false;
}

// ––––––––[ setRowsInline ] replaces Inline rows
void Table::setRowsInline(std::vector<std::vector<Inline>> rows) {
    rows_ = std::move(rows);
    for (auto& r : rows_) r.resize(columns_.size());
    sel_row_ = sel_col_ = -1;
    editing_ = false;
}

// ––––––––[ addRow ] appends
void Table::addRow(std::vector<std::string> row) {
    std::vector<Inline> ir;
    ir.reserve(columns_.size());
    for (auto& s : row) ir.push_back(Inline{} << s);
    ir.resize(columns_.size());
    rows_.push_back(std::move(ir));
}

// ––––––––[ addRowInline ] appends Inline row
void Table::addRowInline(std::vector<Inline> row) {
    row.resize(columns_.size());
    rows_.push_back(std::move(row));
}

// ––––––––[ setSelected ] selects cell
void Table::setSelected(int row, int col) {
    if (row < 0 || row >= std::ssize(rows_)) return;
    if (col < 0 || col >= std::ssize(columns_)) return;
    sel_row_ = row; sel_col_ = col;
    if (on_select_) on_select_(row, col);
}

// ––––––––[ setCell ] sets
void Table::setCell(int row, int col, std::string value) {
    if (row < 0 || row >= std::ssize(rows_)) return;
    if (col < 0 || col >= std::ssize(columns_)) return;
    rows_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = Inline{} << std::move(value);
}

// ––––––––[ setCellInline ] sets Inline
void Table::setCellInline(int row, int col, Inline value) {
    if (row < 0 || row >= std::ssize(rows_)) return;
    if (col < 0 || col >= std::ssize(columns_)) return;
    rows_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = std::move(value);
}

// ––––––––[ cell ] returns plain text
std::string Table::cell(int row, int col) const {
    if (row < 0 || row >= std::ssize(rows_)) return "";
    if (col < 0 || col >= std::ssize(columns_)) return "";
    const auto& inl = rows_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
    std::string out;
    for (auto& r : inl.runs()) if (r.type == Inline::Run::Type::Text) out += r.utf8;
    return out;
}

// ––––––––[ cellInline ] returns Inline
Inline Table::cellInline(int row, int col) const {
    if (row < 0 || row >= std::ssize(rows_)) return Inline{};
    if (col < 0 || col >= std::ssize(columns_)) return Inline{};
    return rows_[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
}

// ––––––––[ header_h ] header height
float Table::header_h(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.input_padding_y * 2.0f + 6.0f;
}

// ––––––––[ row_h ] row height
float Table::row_h(const Theme& theme) const noexcept {
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    return size + theme.metrics.input_padding_y * 2.0f + 4.0f;
}

// ––––––––[ col_x ] x of column
float Table::col_x(int col) const noexcept {
    float x = 0.0f;
    for (int i = 0; i < col && i < std::ssize(columns_); ++i) x += columns_[static_cast<std::size_t>(i)].width;
    return x;
}

// ––––––––[ hitCell ] hit test
void Table::hitCell(float mx, float my, const Theme& theme, int& out_row, int& out_col) const noexcept {
    out_row = out_col = -1;
    float hh = header_h(theme);
    float rh = row_h(theme);
    float content_y = bounds_.y + hh;
    if (my < bounds_.y || my >= bounds_.y + bounds_.h) return;
    if (my < content_y) {
        // header
        float x = bounds_.x - scroll_x_;
        for (int c = 0; c < std::ssize(columns_); ++c) {
            float cw = columns_[static_cast<std::size_t>(c)].width;
            if (mx >= x && mx < x + cw) { out_col = c; out_row = -1; return; }
            x += cw;
        }
        return;
    }
    float y = content_y - scroll_y_;
    int row = static_cast<int>(std::floor((my - y) / rh));
    if (row < 0 || row >= std::ssize(rows_)) return;
    float x = bounds_.x - scroll_x_;
    for (int c = 0; c < std::ssize(columns_); ++c) {
        float cw = columns_[static_cast<std::size_t>(c)].width;
        if (mx >= x && mx < x + cw) { out_row = row; out_col = c; return; }
        x += cw;
    }
}

// ––––––––[ ensureVisible ] scroll to show cell
void Table::ensureVisible(int row, int col, const Theme& theme) {
    float hh = header_h(theme);
    float rh = row_h(theme);
    float content_h = bounds_.h - hh;
    if (row >= 0) {
        float y = static_cast<float>(row) * rh;
        if (y < scroll_y_) scroll_y_ = y;
        else if (y + rh > scroll_y_ + content_h) scroll_y_ = y + rh - content_h;
    }
    if (col >= 0) {
        float x = col_x(col);
        float cw = columns_[static_cast<std::size_t>(col)].width;
        float content_w = bounds_.w;
        if (x < scroll_x_) scroll_x_ = x;
        else if (x + cw > scroll_x_ + content_w) scroll_x_ = x + cw - content_w;
    }
    float total_w = 0.0f;
    for (auto& c : columns_) total_w += c.width;
    float total_h = static_cast<float>(rows_.size()) * rh;
    float max_x = std::max(0.0f, total_w - bounds_.w);
    float max_y = std::max(0.0f, total_h - (bounds_.h - hh));
    scroll_x_ = std::clamp(scroll_x_, 0.0f, max_x);
    scroll_y_ = std::clamp(scroll_y_, 0.0f, max_y);
}

// ––––––––[ startEdit ] begin editing (inline TextInput/ComboBox, Bool toggles)
void Table::startEdit(int row, int col, const Window& w) {
    if (editing_) return;
    if (row < 0 || col < 0) return;
    if (row >= std::ssize(rows_) || col >= std::ssize(columns_)) return;
    auto& colDef = columns_[static_cast<std::size_t>(col)];
    if (!colDef.editable) return;
    // Bool: toggle directly, no editor
    if (colDef.kind == Column::Kind::Bool) {
        std::string cur = cell(row, col);
        std::string nxt = (cur == "true" || cur == "1" || cur == "checked") ? "false" : "true";
        setCell(row, col, nxt);
        if (on_change_) on_change_(row, col, nxt);
        return;
    }
    editing_ = true;
    edit_row_ = row;
    edit_col_ = col;
    float hh = header_h(w.theme());
    float rh = row_h(w.theme());
    float x = bounds_.x - scroll_x_ + col_x(col);
    float y = bounds_.y + hh - scroll_y_ + static_cast<float>(row) * rh;
    float cw = colDef.width;
    x = std::clamp(x, bounds_.x, bounds_.x + bounds_.w - cw);
    y = std::clamp(y, bounds_.y + hh, bounds_.y + bounds_.h - rh);
    if (colDef.kind == Column::Kind::Choice) {
        combo_editor_ = std::make_unique<ComboBox>(Rect{x, y, cw, rh});
        combo_editor_->setItems(colDef.choices);
        std::string cur = cell(row, col);
        int sel = -1;
        for (size_t i = 0; i < colDef.choices.size(); ++i) if (colDef.choices[i] == cur) { sel = static_cast<int>(i); break; }
        combo_editor_->setSelectedIndex(sel);
        combo_editor_->setWindow(window_);
        combo_editor_->setFocused(true);
        if (window_) window_->requestFocus(combo_editor_.get());
        combo_editor_->setOnChange([this, row, col](int idx, const std::string& v) {
            (void)idx;
            pending_choice_ = true;
            pending_row_ = row;
            pending_col_ = col;
            pending_value_ = v;
        });
        combo_auto_open_ = true;
        ensureVisible(row, col, w.theme());
        return;
    }
    // Text
    editor_ = std::make_unique<TextInput>(Rect{x, y, cw, rh}, "");
    editor_->setText(cell(row, col));
    editor_->setWindow(window_);
    editor_->setFocused(true);
    if (window_) window_->requestFocus(editor_.get());
    editor_->setOnSubmit([this, row, col](const std::string& txt) {
        setCell(row, col, txt);
        if (on_change_) on_change_(row, col, txt);
        editing_ = false;
        edit_row_ = edit_col_ = -1;
        if (window_) window_->requestFocus(this);
    });
    editor_->setOnChange([](const std::string&) {});
    ensureVisible(row, col, w.theme());
}

// ––––––––[ commitEdit ] save
void Table::commitEdit() {
    editing_ = false;
    edit_row_ = edit_col_ = -1;
}

// ––––––––[ cancelEdit ] abort
void Table::cancelEdit() {
    editing_ = false;
    edit_row_ = edit_col_ = -1;
}

// ––––––––[ naturalWidth ] sum of column widths + border
float Table::naturalWidth(const Theme&, const Window&) const {
    float total = 0.0f;
    for (auto& c : columns_) total += c.width;
    if (total < 100.0f) total = 100.0f;
    return total + 2.0f;
}

// ––––––––[ naturalHeight ] header + rows
float Table::naturalHeight(const Theme& theme, const Window&) const {
    float hh = header_h(theme);
    float rh = row_h(theme);
    int n = std::min(static_cast<int>(std::ssize(rows_)), 8);
    if (n == 0) n = 3;
    return hh + rh * static_cast<float>(n) + 4.0f;
}

// ––––––––[ setWindow ] propagate
void Table::setWindow(Window* w) noexcept {
    Widget::setWindow(w);
    if (editor_) editor_->setWindow(w);
}

// ––––––––[ draw ] grid
void Table::draw(NVGcontext* vg, const Theme& theme, const Window& w) {
    if (!visible_) return;
    float hh = header_h(theme);
    float rh = row_h(theme);
    // Background
    nvgBeginPath(vg);
    nvgRoundedRect(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h, theme.metrics.corner_radius);
    nvgFillColor(vg, nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
    nvgFill(vg);
    nvgStrokeWidth(vg, theme.metrics.border_width);
    nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, theme.colors.border.a));
    nvgStroke(vg);
    if (focused_) drawFocusRing(vg, bounds_, theme);
    // Clip to table bounds
    nvgSave(vg);
    nvgIntersectScissor(vg, bounds_.x, bounds_.y, bounds_.w, bounds_.h);
    // Header — customizable bk, text color, font, with glyphs (drawn first, rows clipped below)
    float hx = bounds_.x - scroll_x_;
    for (int c = 0; c < std::ssize(columns_); ++c) {
        float cw = columns_[static_cast<std::size_t>(c)].width;
        float x = hx + col_x(c);
        // Header bg — custom or theme
        Color hbg = header_bg_set_ ? header_bg_ : theme.colors.surface;
        nvgBeginPath(vg);
        nvgRect(vg, x, bounds_.y, cw, hh);
        nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, hbg.a));
        // Slight tint if not custom
        if (!header_bg_set_ && c % 2 == 0) {
            nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, 8));
            nvgFill(vg);
            nvgBeginPath(vg);
            nvgRect(vg, x, bounds_.y, cw, hh);
            nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, 180));
            nvgFill(vg);
        } else {
            nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, hbg.a));
            nvgFill(vg);
        }
        // Header text Inline (glyphs) or plain label
        const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
        std::string spec = !header_font_.empty() ? header_font_ : (font_.empty() ? theme.font_family : font_);
        std::string face;
        w.prepareFont(vg, spec, face);
        nvgFontSize(vg, size);
        nvgFontFace(vg, face.c_str());
        Color hcol = header_text_set_ ? header_text_ : theme.colors.on_surface;
        // Use headerInline if set, else label
        const auto& col = columns_[static_cast<std::size_t>(c)];
        if (!col.headerInline.empty()) {
            nvgSave(vg);
            nvgIntersectScissor(vg, x, bounds_.y, cw, hh);
            renderInline(vg, col.headerInline.runs(), x + 6.0f, bounds_.y + hh * 0.5f,
                         NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE, size,
                         theme.metrics.animation.icon_size_gain, theme.metrics.animation.icon_padding,
                         face.c_str(), hcol, hcol);
            nvgRestore(vg);
        } else {
            nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
            nvgFillColor(vg, nvgRGBA(hcol.r, hcol.g, hcol.b, hcol.a));
            std::string bold_face = face + "-bold";
            bool has_bold = nvgFindFont(vg, bold_face.c_str()) != -1;
            if (has_bold) nvgFontFace(vg, bold_face.c_str());
            nvgText(vg, x + 6.0f, bounds_.y + hh * 0.5f, col.label.c_str(), nullptr);
            if (has_bold) nvgFontFace(vg, face.c_str());
        }
        // Vertical grid line
        nvgBeginPath(vg);
        nvgMoveTo(vg, x + cw, bounds_.y);
        nvgLineTo(vg, x + cw, bounds_.y + bounds_.h);
        nvgStrokeWidth(vg, 1.0f);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, 80));
        nvgStroke(vg);
    }
    // Trailing header area — fill with same bg as last column
    {
        float total_w = 0.0f;
        for (auto& col : columns_) total_w += col.width;
        float trailing_x = hx + total_w;
        float trailing_w = (bounds_.x + bounds_.w) - trailing_x;
        if (trailing_w > 0.5f && !columns_.empty()) {
            int last = static_cast<int>(std::ssize(columns_)) - 1;
            Color hbg = header_bg_set_ ? header_bg_ : theme.colors.surface;
            // Apply same tint as last column if not custom
            bool is_tinted = !header_bg_set_ && (last % 2 == 0);
            nvgBeginPath(vg);
            nvgRect(vg, trailing_x, bounds_.y, trailing_w, hh);
            if (is_tinted) {
                // Match last column's tinted bg: base 8 alpha + 180
                nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, hbg.a));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRect(vg, trailing_x, bounds_.y, trailing_w, hh);
                nvgFillColor(vg, nvgRGBA(theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, 8));
                nvgFill(vg);
                nvgBeginPath(vg);
                nvgRect(vg, trailing_x, bounds_.y, trailing_w, hh);
                nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, 180));
                nvgFill(vg);
            } else {
                nvgFillColor(vg, nvgRGBA(hbg.r, hbg.g, hbg.b, hbg.a));
                nvgFill(vg);
            }
        }
    }
    // Header bottom line — thin accent line to distinguish header
    nvgBeginPath(vg);
    nvgMoveTo(vg, bounds_.x, bounds_.y + hh);
    nvgLineTo(vg, bounds_.x + bounds_.w, bounds_.y + hh);
    nvgStrokeWidth(vg, 1.5f);
    nvgStrokeColor(vg, nvgRGBA(accentColor(theme).r, accentColor(theme).g, accentColor(theme).b, 200));
    nvgStroke(vg);
    // Rows — clipped to content area below header (prevents overriding header)
    nvgSave(vg);
    nvgIntersectScissor(vg, bounds_.x, bounds_.y + hh, bounds_.w, bounds_.h - hh);
    const float size = textSize_ > 0.0f ? textSize_ : theme.metrics.text_size;
    std::string spec = font_.empty() ? theme.font_family : font_;
    std::string face;
    w.prepareFont(vg, spec, face);
    nvgFontSize(vg, size);
    nvgFontFace(vg, face.c_str());
    nvgTextAlign(vg, NVG_ALIGN_LEFT | NVG_ALIGN_MIDDLE);
    const int row_count = static_cast<int>(std::ssize(rows_));
    int first = static_cast<int>(std::floor(scroll_y_ / rh));
    int last = static_cast<int>(std::ceil((scroll_y_ + (bounds_.h - hh)) / rh));
    first = std::clamp(first, 0, row_count);
    last = std::clamp(last, 0, row_count);
    for (int r = first; r < last; ++r) {
        float y = bounds_.y + hh - scroll_y_ + static_cast<float>(r) * rh;
        bool row_sel = r == sel_row_;
        // Row bg — customizable
        Color row_bg = Color{0,0,0,0};
        bool has_row_bg = false;
        if (row_sel) {
            nvgBeginPath(vg);
            nvgRect(vg, bounds_.x, y, bounds_.w, rh);
            nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 30));
            nvgFill(vg);
        } else if (r % 2 == 1 && cell_bg_alt_set_) {
            row_bg = cell_bg_alt_; has_row_bg = true;
        } else if (r % 2 == 0 && cell_bg_set_) {
            row_bg = cell_bg_; has_row_bg = true;
        } else if (r % 2 == 1) {
            row_bg = Color{theme.colors.on_surface.r, theme.colors.on_surface.g, theme.colors.on_surface.b, 6}; has_row_bg = true;
        }
        if (has_row_bg && !row_sel) {
            nvgBeginPath(vg);
            nvgRect(vg, bounds_.x, y, bounds_.w, rh);
            Color cb = row_bg;
            // For alt rows, if custom not set, use subtle tint, else use custom
            if (!cell_bg_set_ && !cell_bg_alt_set_ && r % 2 == 1) {
                nvgFillColor(vg, nvgRGBA(cb.r, cb.g, cb.b, cb.a));
            } else {
                nvgFillColor(vg, nvgRGBA(cb.r, cb.g, cb.b, cb.a));
            }
            nvgFill(vg);
        } else if (!row_sel && (cell_bg_set_ || cell_bg_alt_set_)) {
            // Even rows with custom bg
            Color cb = (r % 2 == 0 && cell_bg_set_) ? cell_bg_ : (cell_bg_alt_set_ ? cell_bg_alt_ : Color{0,0,0,0});
            if (cell_bg_set_ && r % 2 == 0) {
                nvgBeginPath(vg);
                nvgRect(vg, bounds_.x, y, bounds_.w, rh);
                nvgFillColor(vg, nvgRGBA(cb.r, cb.g, cb.b, cb.a));
                nvgFill(vg);
            }
        }
        // Cells
        float cx = bounds_.x - scroll_x_;
        for (int col = 0; col < std::ssize(columns_); ++col) {
            float cw = columns_[static_cast<std::size_t>(col)].width;
            float x = cx + col_x(col);
            bool is_sel = r == sel_row_ && col == sel_col_;
            if (is_sel) {
                const float cr = std::min(theme.metrics.corner_radius * 0.66f, std::min(cw, rh) * 0.35f);
                const float inset = 1.0f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, x + inset, y + inset, cw - 2.0f * inset, rh - 2.0f * inset, cr);
                nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 60));
                nvgFill(vg);
                nvgStrokeWidth(vg, theme.metrics.input_focus_width);
                nvgStrokeColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 140));
                nvgStroke(vg);
            }
            // Hover
            if (!is_sel && r == hover_row_ && col == hover_col_) {
                nvgBeginPath(vg);
                nvgRect(vg, x, y, cw, rh);
                nvgFillColor(vg, nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, 20));
                nvgFill(vg);
            }
                    // Text / Inline with glyphs — Bool shows only box
            const auto& colDef = columns_[static_cast<std::size_t>(col)];
            if (colDef.kind == Column::Kind::Bool) {
                bool checked = false;
                if (r < std::ssize(rows_) && col < std::ssize(rows_[static_cast<std::size_t>(r)])) {
                    std::string v = cell(r, col);
                    checked = (v == "true" || v == "1" || v == "checked");
                }
                if (editing_ && r == edit_row_ && col == edit_col_) checked = false;
                // Draw checkbox centered
                const float box = 16.0f;
                float bx = x + (cw - box) * 0.5f;
                float by = y + (rh - box) * 0.5f;
                nvgBeginPath(vg);
                nvgRoundedRect(vg, bx, by, box, box, 3.0f);
                nvgFillColor(vg, checked ? nvgRGBA(theme.colors.accent.r, theme.colors.accent.g, theme.colors.accent.b, theme.colors.accent.a)
                                         : nvgRGBA(theme.colors.surface.r, theme.colors.surface.g, theme.colors.surface.b, theme.colors.surface.a));
                nvgFill(vg);
                nvgStrokeWidth(vg, 1.0f);
                nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, theme.colors.border.a));
                nvgStroke(vg);
                if (checked) {
                    NVGcolor cc = nvgRGBA(255,255,255,255);
                    // Check mark via two lines
                    nvgBeginPath(vg);
                    nvgMoveTo(vg, bx + 4, by + 8);
                    nvgLineTo(vg, bx + 7, by + 11);
                    nvgLineTo(vg, bx + 12, by + 5);
                    nvgStrokeWidth(vg, 2.0f);
                    nvgStrokeColor(vg, cc);
                    nvgStroke(vg);
                }
            } else {
                Inline cell_inl;
                if (r < std::ssize(rows_) && col < std::ssize(rows_[static_cast<std::size_t>(r)])) {
                    cell_inl = rows_[static_cast<std::size_t>(r)][static_cast<std::size_t>(col)];
                }
                if (editing_ && r == edit_row_ && col == edit_col_) cell_inl = Inline{};
                if (!cell_inl.empty()) {
                    nvgSave(vg);
                    nvgIntersectScissor(vg, x, y, cw, rh);
                    const float icon_gain = theme.metrics.animation.icon_size_gain;
                    const float icon_pad = theme.metrics.animation.icon_padding;
                    std::string cell_spec = !cell_font_.empty() ? cell_font_ : spec;
                    std::string cell_face = face;
                    if (!cell_font_.empty()) {
                        w.prepareFont(vg, cell_spec, cell_face);
                        nvgFontFace(vg, cell_face.c_str());
                    }
                    Color cell_col = cell_text_set_ ? cell_text_ : theme.colors.on_surface;
                    renderInline(vg, cell_inl.runs(), x + 6.0f, y + rh * 0.5f,
                                 NVG_ALIGN_LEFT, NVG_ALIGN_MIDDLE,
                                 size, icon_gain, icon_pad,
                                 cell_face.c_str(), cell_col, cell_col);
                    if (!cell_font_.empty()) nvgFontFace(vg, face.c_str());
                    nvgRestore(vg);
                } else {
                    (void)size;
                }
            }
            // Vertical line
            nvgBeginPath(vg);
            nvgMoveTo(vg, x + cw, y);
            nvgLineTo(vg, x + cw, y + rh);
            nvgStrokeWidth(vg, 1.0f);
            nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, 40));
            nvgStroke(vg);
        }
        // Horizontal line
        nvgBeginPath(vg);
        nvgMoveTo(vg, bounds_.x, y + rh);
        nvgLineTo(vg, bounds_.x + bounds_.w, y + rh);
        nvgStrokeWidth(vg, 1.0f);
        nvgStrokeColor(vg, nvgRGBA(theme.colors.border.r, theme.colors.border.g, theme.colors.border.b, 40));
        nvgStroke(vg);
    }
    nvgRestore(vg);
    // Scrollbars
    float total_w = 0.0f;
    for (auto& c : columns_) total_w += c.width;
    float total_h = static_cast<float>(rows_.size()) * rh;
    bool need_h = total_w > bounds_.w + 1.0f;
    bool need_v = total_h > (bounds_.h - hh) + 1.0f;
    float alpha_h = scroll_anim_h_.value();
    float alpha_v = scroll_anim_v_.value();
    if (need_h && alpha_h > 0.01f) {
        float tw = bounds_.w * (bounds_.w / total_w);
        tw = std::max(20.0f, tw);
        float tx = bounds_.x + (scroll_x_ / (total_w - bounds_.w)) * (bounds_.w - tw);
        uint8_t a = static_cast<uint8_t>(100.0f * alpha_h);
        // Track
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x, bounds_.y + bounds_.h - 4, bounds_.w, 3, 1.5f);
        nvgFillColor(vg, nvgRGBA(128, 128, 128, (uint8_t)(20 * alpha_h)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, tx, bounds_.y + bounds_.h - 4, tw, 3, 1.5f);
        nvgFillColor(vg, nvgRGBA(128, 128, 128, a));
        nvgFill(vg);
    }
    if (need_v && alpha_v > 0.01f) {
        float th = (bounds_.h - hh) * ((bounds_.h - hh) / total_h);
        th = std::max(12.0f, th);
        float ty = bounds_.y + hh + (scroll_y_ / (total_h - (bounds_.h - hh))) * ((bounds_.h - hh) - th);
        uint8_t a = static_cast<uint8_t>(100.0f * alpha_v);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x + bounds_.w - 4, bounds_.y + hh, 3, bounds_.h - hh, 1.5f);
        nvgFillColor(vg, nvgRGBA(128, 128, 128, (uint8_t)(20 * alpha_v)));
        nvgFill(vg);
        nvgBeginPath(vg);
        nvgRoundedRect(vg, bounds_.x + bounds_.w - 4, ty, 3, th, 1.5f);
        nvgFillColor(vg, nvgRGBA(128, 128, 128, a));
        nvgFill(vg);
    }
    // Inline editors (Text and Choice)
    if (editing_) {
        float hh2 = header_h(theme);
        float rh2 = row_h(theme);
        float ex = bounds_.x - scroll_x_ + col_x(edit_col_);
        float ey = bounds_.y + hh2 - scroll_y_ + static_cast<float>(edit_row_) * rh2;
        float cw = columns_[static_cast<std::size_t>(edit_col_)].width;
        if (editor_) {
            editor_->setBounds(Rect{ex, ey, cw, rh2});
            editor_->draw(vg, theme, w);
        }
        if (combo_editor_) {
            combo_editor_->setBounds(Rect{ex, ey, cw, rh2});
            combo_editor_->draw(vg, theme, w);
        }
    }
    nvgRestore(vg);
}

// ––––––––[ updateHover ] hover + scrollbar hover
void Table::updateHover(float mx, float my) noexcept {
    if (editing_ && (editor_ || combo_editor_)) {
        if (editor_) editor_->updateHover(mx, my);
        if (combo_editor_) combo_editor_->updateHover(mx, my);
        return;
    }
    int r, c;
    hitCell(mx, my, window_ ? window_->theme() : Theme::defaultLight(), r, c);
    hover_row_ = r;
    hover_col_ = c;
    // Scrollbar hover
    float hh = header_h(window_ ? window_->theme() : Theme::defaultLight());
    float rh = row_h(window_ ? window_->theme() : Theme::defaultLight());
    float total_w = 0.0f; for (auto& col : columns_) total_w += col.width;
    float total_h = static_cast<float>(rows_.size()) * rh;
    bool need_h = total_w > bounds_.w + 1.0f;
    bool need_v = total_h > (bounds_.h - hh) + 1.0f;
    scrollbar_hovered_h_ = false;
    scrollbar_hovered_v_ = false;
    if (need_h) {
        float tw = bounds_.w * (bounds_.w / total_w);
        tw = std::max(20.0f, tw);
        float tx = bounds_.x + (total_w > bounds_.w ? (scroll_x_ / (total_w - bounds_.w)) * (bounds_.w - tw) : 0);
        float ty = bounds_.y + bounds_.h - 4;
        if (mx >= tx && mx <= tx + tw && my >= ty - 2 && my <= ty + 5) scrollbar_hovered_h_ = true;
        else if (mx >= bounds_.x && mx <= bounds_.x + bounds_.w && my >= ty - 2 && my <= ty + 5) scrollbar_hovered_h_ = true;
    }
    if (need_v) {
        float th = (bounds_.h - hh) * ((bounds_.h - hh) / total_h);
        th = std::max(12.0f, th);
        float ty = bounds_.y + hh + (total_h > (bounds_.h - hh) ? (scroll_y_ / (total_h - (bounds_.h - hh))) * ((bounds_.h - hh) - th) : 0);
        float tx = bounds_.x + bounds_.w - 4;
        if (mx >= tx - 2 && mx <= tx + 5 && my >= bounds_.y + hh && my <= bounds_.y + bounds_.h) scrollbar_hovered_v_ = true;
        else if (mx >= tx - 2 && mx <= tx + 5 && my >= ty && my <= ty + th) scrollbar_hovered_v_ = true;
    }
    if ((scrollbar_hovered_h_ || scrollbar_hovered_v_) && window_) {
        // Show scrollbar on hover
        wheel_active_ = true;
        wheel_timer_ = 1.0f;
    }
}

// ––––––––[ tick ] tick editors, handle deferred reset + scrollbar fade
void Table::tick(float dt, const Theme& theme) noexcept {
    if (pending_choice_) {
        pending_choice_ = false;
        setCell(pending_row_, pending_col_, pending_value_);
        if (on_change_) on_change_(pending_row_, pending_col_, pending_value_);
        editing_ = false; edit_row_ = edit_col_ = -1;
        if (window_) window_->requestFocus(this);
        combo_editor_.reset();
        if (window_) window_->requestFocus(this);
    }
    if (!editing_) {
        if (editor_) {
            if (window_) window_->requestFocus(this);
            editor_.reset();
        }
        if (combo_editor_ && !pending_choice_) {
            if (window_) window_->requestFocus(this);
            combo_editor_.reset();
        }
        combo_auto_open_ = false;
    } else {
        if (editor_) editor_->tick(dt, theme);
        if (combo_editor_) {
            combo_editor_->tick(dt, theme);
            if (combo_auto_open_ && window_) {
                combo_auto_open_ = false;
                combo_editor_->openPopup(*window_);
            }
            // If popup was closed without selection (click outside), cancel edit
            if (!pending_choice_ && window_ && !window_->hasPopup()) {
                // Check if the ComboBox's popup was just closed without a selection
                // We can detect by seeing if the ComboBox's selected index hasn't changed and no pending
                // For now, just keep editing until explicit cancel or select
                // If the user clicked outside the table entirely, the above handleEvent would have already handled
                // But if they clicked outside the popup but inside the window, the popup is closed but editing remains
                // We should cancel in that case after a short delay
                // For now, do nothing — editing remains until next interaction
            }
        }
    }
    if (wheel_timer_ > 0.0f) {
        wheel_timer_ -= dt;
        if (wheel_timer_ <= 0.0f) wheel_active_ = false;
    }
    bool show_h = scrollbar_hovered_h_ || dragging_h_ || wheel_active_;
    bool show_v = scrollbar_hovered_v_ || dragging_v_ || wheel_active_;
    scroll_anim_h_.setTarget(show_h ? 1.0f : 0.0f);
    scroll_anim_v_.setTarget(show_v ? 1.0f : 0.0f);
    scroll_anim_h_.tick(dt, theme.metrics.animation.hover_tau);
    scroll_anim_v_.tick(dt, theme.metrics.animation.hover_tau);
}

// ––––––––[ setFocused ] focus
void Table::setFocused(bool f) noexcept {
    focused_ = f;
    if (editor_) editor_->setFocused(f);
    if (combo_editor_) combo_editor_->setFocused(f);
    // Don't auto-cancel editing on blur — let click-outside logic handle via pending_choice / tick
    // This avoids deleting combo_editor_ while Window::requestFocus is still on stack for that editor
}

// ––––––––[ handleEvent ] selection, edit, scroll
EventResult Table::handleEvent(const Event& e, const Window& w) {
    float dbg_mx{}, dbg_my{}; w.cursorPosition(dbg_mx, dbg_my);
    fflush(stderr);
    // If editing with TextInput
    if (editing_ && editor_) {
        if (kind_of(e) == EventKind::MouseDown) {
            float mx2{}, my2{}; w.cursorPosition(mx2, my2);
            bool inside_editor = editor_->containsPoint(mx2, my2);
            bool inside_table = containsPoint(mx2, my2);
            if (!inside_editor) {
                std::string txt; if (editor_) txt = editor_->text();
                int er = edit_row_, ec = edit_col_;
                auto* win = window_;
                editing_ = false; edit_row_ = edit_col_ = -1;
                std::string saved_txt = txt;
                std::unique_ptr<TextInput> old_editor = std::move(editor_);
                if (win) win->requestFocus(this);
                if (er >= 0 && ec >= 0) { setCell(er, ec, saved_txt); if (on_change_) on_change_(er, ec, saved_txt); }
                if (!inside_table) return EventResult::Consumed;
            } else {
                EventResult er = editor_->handleEvent(e, w);
                if (er == EventResult::Consumed) return er;
                return EventResult::Consumed;
            }
        } else {
            EventResult er = editor_->handleEvent(e, w);
            if (er == EventResult::Consumed) return er;
            if (kind_of(e) == EventKind::KeyDown) {
                const auto& ke = std::get<KeyDownEvent>(e);
                if (ke.key == GLFW_KEY_ESCAPE) {
                    editing_ = false; edit_row_ = edit_col_ = -1; editor_.reset();
                    if (window_) window_->requestFocus(this);
                    return EventResult::Consumed;
                }
            }
        }
    }
    // If editing with ComboBox
    if (editing_ && combo_editor_) {
        bool has_popup_before = window_ && window_->hasPopup();
        EventResult er = combo_editor_->handleEvent(e, w);
        if (er == EventResult::Consumed) return er;
        bool has_popup_after = window_ && window_->hasPopup();
        if (has_popup_before && !has_popup_after) {
            // Popup closed without selection (outside click) — cancel edit, transfer focus to Table
            pending_choice_ = false;
            editing_ = false; edit_row_ = edit_col_ = -1;
            if (window_) window_->requestFocus(this);
            return EventResult::Consumed;
        }
        if (kind_of(e) == EventKind::MouseDown) {
            float mx2{}, my2{}; w.cursorPosition(mx2, my2);
            bool inside_combo = combo_editor_->containsPoint(mx2, my2);
            bool has_popup_now = window_ && window_->hasPopup();
            if (has_popup_now) return EventResult::Consumed;
            if (!inside_combo) {
                pending_choice_ = false;
                editing_ = false; edit_row_ = edit_col_ = -1;
                if (window_) window_->requestFocus(this);
                float mx3{}, my3{}; w.cursorPosition(mx3, my3);
                bool inside_table = containsPoint(mx3, my3);
                if (!inside_table) return EventResult::Consumed;
            } else {
                return EventResult::Consumed;
            }
        } else if (kind_of(e) == EventKind::KeyDown) {
            const auto& ke = std::get<KeyDownEvent>(e);
            if (ke.key == GLFW_KEY_ESCAPE) {
                editing_ = false; edit_row_ = edit_col_ = -1;
                // Defer reset
                if (window_) window_->requestFocus(this);
                return EventResult::Consumed;
            }
        }
    }
    if (!enabled_) return EventResult::Ignored;
    float mx{}, my{};
    w.cursorPosition(mx, my);
    bool inside = containsPoint(mx, my);
    switch (kind_of(e)) {
    case EventKind::MouseMove: {
        if (dragging_h_ || dragging_v_) {
            float hh = header_h(w.theme());
            float rh = row_h(w.theme());
            float total_w = 0.0f; for (auto& col : columns_) total_w += col.width;
            float total_h = static_cast<float>(rows_.size()) * rh;
            if (dragging_h_) {
                float tw = bounds_.w * (bounds_.w / total_w);
                tw = std::max(20.0f, tw);
                float dx = mx - drag_start_x_;
                float max_x = std::max(1.0f, total_w - bounds_.w);
                float max_drag = bounds_.w - tw;
                if (max_drag > 1.0f) scroll_x_ = drag_orig_x_ + (dx / max_drag) * max_x;
                float max_scroll_x = std::max(0.0f, total_w - bounds_.w);
                scroll_x_ = std::clamp(scroll_x_, 0.0f, max_scroll_x);
            }
            if (dragging_v_) {
                float th = (bounds_.h - hh) * ((bounds_.h - hh) / total_h);
                th = std::max(12.0f, th);
                float dy = my - drag_start_y_;
                float max_y = std::max(1.0f, total_h - (bounds_.h - hh));
                float max_drag = (bounds_.h - hh) - th;
                if (max_drag > 1.0f) scroll_y_ = drag_orig_y_ + (dy / max_drag) * max_y;
                float max_scroll_y = std::max(0.0f, total_h - (bounds_.h - hh));
                scroll_y_ = std::clamp(scroll_y_, 0.0f, max_scroll_y);
            }
            return EventResult::Consumed;
        }
        break;
    }
    case EventKind::MouseDown: {
        // Check scrollbar thumb first
        {
            float hh = header_h(w.theme());
            float rh = row_h(w.theme());
            float total_w = 0.0f; for (auto& col : columns_) total_w += col.width;
            float total_h = static_cast<float>(rows_.size()) * rh;
            bool need_h = total_w > bounds_.w + 1.0f;
            bool need_v = total_h > (bounds_.h - hh) + 1.0f;
            if (need_h) {
                float tw = bounds_.w * (bounds_.w / total_w);
                tw = std::max(20.0f, tw);
                float tx = bounds_.x + (total_w > bounds_.w ? (scroll_x_ / (total_w - bounds_.w)) * (bounds_.w - tw) : 0);
                float ty = bounds_.y + bounds_.h - 4;
                if (mx >= tx && mx <= tx + tw && my >= ty - 2 && my <= ty + 5) {
                    dragging_h_ = true;
                    drag_start_x_ = mx;
                    drag_orig_x_ = scroll_x_;
                    if (window_) window_->setCursorShape(CursorShape::Arrow);
                    return EventResult::Consumed;
                } else if (need_h && mx >= bounds_.x && mx <= bounds_.x + bounds_.w && my >= ty - 2 && my <= ty + 5) {
                    // Click on track — page
                    float max_x = std::max(0.0f, total_w - bounds_.w);
                    if (mx < tx) scroll_x_ = std::max(0.0f, scroll_x_ - bounds_.w * 0.8f);
                    else scroll_x_ = std::min(max_x, scroll_x_ + bounds_.w * 0.8f);
                    wheel_active_ = true; wheel_timer_ = 1.0f;
                    return EventResult::Consumed;
                }
            }
            if (need_v) {
                float th = (bounds_.h - hh) * ((bounds_.h - hh) / total_h);
                th = std::max(12.0f, th);
                float ty = bounds_.y + hh + (total_h > (bounds_.h - hh) ? (scroll_y_ / (total_h - (bounds_.h - hh))) * ((bounds_.h - hh) - th) : 0);
                float tx = bounds_.x + bounds_.w - 4;
                if (mx >= tx - 2 && mx <= tx + 5 && my >= ty && my <= ty + th) {
                    dragging_v_ = true;
                    drag_start_y_ = my;
                    drag_orig_y_ = scroll_y_;
                    if (window_) window_->setCursorShape(CursorShape::Arrow);
                    return EventResult::Consumed;
                } else if (need_v && mx >= tx - 2 && mx <= tx + 5 && my >= bounds_.y + hh && my <= bounds_.y + bounds_.h) {
                    float max_y = std::max(0.0f, total_h - (bounds_.h - hh));
                    if (my < ty) scroll_y_ = std::max(0.0f, scroll_y_ - (bounds_.h - hh) * 0.8f);
                    else scroll_y_ = std::min(max_y, scroll_y_ + (bounds_.h - hh) * 0.8f);
                    wheel_active_ = true; wheel_timer_ = 1.0f;
                    return EventResult::Consumed;
                }
            }
        }
        if (!inside) return EventResult::Ignored;
        if (window_) window_->requestFocus(this);
        int r, c;
        hitCell(mx, my, w.theme(), r, c);
        if (r >= 0 && c >= 0) {
            // Bool: single click toggles box only
            if (columns_[static_cast<std::size_t>(c)].kind == Column::Kind::Bool) {
                sel_row_ = r; sel_col_ = c;
                if (on_select_) on_select_(r, c);
                ensureVisible(r, c, w.theme());
                std::string cur = cell(r, c);
                std::string nxt = (cur == "true" || cur == "1" || cur == "checked") ? "false" : "true";
                setCell(r, c, nxt);
                if (on_change_) on_change_(r, c, nxt);
                return EventResult::Consumed;
            }
            sel_row_ = r; sel_col_ = c;
            if (on_select_) on_select_(r, c);
            ensureVisible(r, c, w.theme());
            static double last_click = 0.0;
            static int last_r = -1, last_c = -1;
            double now = glfwGetTime();
            bool is_double = (now - last_click < 0.35 && last_r == r && last_c == c);
            last_click = now; last_r = r; last_c = c;
            if (is_double) {
                startEdit(r, c, w);
            }
        } else if (r == -1 && c >= 0) {
            // Header click — could sort, ignore for now
        }
        return EventResult::Consumed;
    }
    case EventKind::MouseUp:
        if (dragging_h_ || dragging_v_) {
            dragging_h_ = dragging_v_ = false;
            if (window_) window_->setCursorShape(CursorShape::Arrow);
            return EventResult::Consumed;
        }
        return inside ? EventResult::Consumed : EventResult::Ignored;
    case EventKind::MouseScroll: {
        if (!inside) return EventResult::Ignored;
        const auto& se = std::get<MouseScrollEvent>(e);
        float dx = se.dx;
        float dy = se.dy;
        // Trackpad deadzone + dominant-axis filter — vertical scroll often carries
        // small spurious dx jitter that would drift scroll_x and hide first column.
        if (std::abs(dx) < 0.12f) dx = 0.0f;
        if (std::abs(dy) < 0.12f) dy = 0.0f;
        if (dx != 0.0f && dy != 0.0f) {
            if (std::abs(dy) > std::abs(dx) * 2.0f) dx = 0.0f;
            else if (std::abs(dx) > std::abs(dy) * 2.0f) dy = 0.0f;
        } else if (dx != 0.0f && std::abs(dy) < 0.12f) {
            // pure horizontal already handled by deadzone
        }
        if (dx == 0.0f && dy == 0.0f) return EventResult::Ignored;
        scroll_y_ -= dy * 16.0f;
        scroll_x_ += dx * 16.0f;
        float total_w = 0.0f; for (auto& col : columns_) total_w += col.width;
        float total_h = static_cast<float>(rows_.size()) * row_h(w.theme());
        float max_x = std::max(0.0f, total_w - bounds_.w);
        float max_y = std::max(0.0f, total_h - (bounds_.h - header_h(w.theme())));
        scroll_x_ = std::clamp(scroll_x_, 0.0f, max_x);
        scroll_y_ = std::clamp(scroll_y_, 0.0f, max_y);
        wheel_active_ = true;
        wheel_timer_ = 1.0f;
        return EventResult::Consumed;
    }
    case EventKind::KeyDown: {
        if (!focused_) return EventResult::Ignored;
        const auto& ke = std::get<KeyDownEvent>(e);
        if (editing_) {
            if (ke.key == GLFW_KEY_ESCAPE) { cancelEdit(); return EventResult::Consumed; }
            if (ke.key == GLFW_KEY_ENTER || ke.key == GLFW_KEY_KP_ENTER) {
                // Commit via editor — we need to get text from editor popup
                // For now, just cancel (editor will handle)
                return EventResult::Consumed;
            }
            // Forward to editor popup if exists
            if (window_ && window_->hasPopup() && window_->popup() && window_->popup()->isTooltip() == false) {
                // The editor is a popup TextInput, let it handle
                // Find editor popup (assume it's a Container with TextInput)
                // For simplicity, forward event to popup
                // This will be handled via Window's popup handling, not here
            }
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_UP) {
            if (sel_row_ > 0) { sel_row_--; if (on_select_) on_select_(sel_row_, sel_col_); ensureVisible(sel_row_, sel_col_, w.theme()); }
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_DOWN) {
            if (sel_row_ + 1 < std::ssize(rows_)) { sel_row_++; if (on_select_) on_select_(sel_row_, sel_col_); ensureVisible(sel_row_, sel_col_, w.theme()); }
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_LEFT) {
            if (sel_col_ > 0) { sel_col_--; if (on_select_) on_select_(sel_row_, sel_col_); ensureVisible(sel_row_, sel_col_, w.theme()); }
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_RIGHT) {
            if (sel_col_ + 1 < std::ssize(columns_)) { sel_col_++; if (on_select_) on_select_(sel_row_, sel_col_); ensureVisible(sel_row_, sel_col_, w.theme()); }
            return EventResult::Consumed;
        }
        if (ke.key == GLFW_KEY_ENTER || ke.key == GLFW_KEY_KP_ENTER) {
            if (sel_row_ >= 0 && sel_col_ >= 0) { startEdit(sel_row_, sel_col_, w); return EventResult::Consumed; }
        }
        if (ke.key == GLFW_KEY_TAB) {
            if (sel_col_ + 1 < std::ssize(columns_)) { sel_col_++; } else if (sel_row_ + 1 < std::ssize(rows_)) { sel_row_++; sel_col_ = 0; }
            if (on_select_) on_select_(sel_row_, sel_col_);
            ensureVisible(sel_row_, sel_col_, w.theme());
            return EventResult::Consumed;
        }
        // Typing starts edit
        if (ke.key >= 32 && ke.key < 127) {
            if (sel_row_ >= 0 && sel_col_ >= 0) { startEdit(sel_row_, sel_col_, w); return EventResult::Consumed; }
        }
        break;
    }
    case EventKind::Char: {
        if (!focused_ || editing_) break;
        const auto& ce = std::get<CharEvent>(e);
        if (ce.codepoint >= 32 && sel_row_ >= 0 && sel_col_ >= 0) {
            startEdit(sel_row_, sel_col_, w);
            // Forward char to editor — editor popup will handle next Char
            return EventResult::Consumed;
        }
        break;
    }
    default: break;
    }
    return inside ? EventResult::Consumed : EventResult::Ignored;
}

// ––––––––[ setProperty ] yaml
void Table::setProperty(std::string_view name, const YamlNode& value) {
    if (name == "columns") {
        std::vector<Column> cols;
        if (value.IsSequence()) {
            for (const auto& ch : value) {
                Column c;
                c.label = ch.asString();
                cols.push_back(std::move(c));
            }
        } else if (value.IsScalar()) {
            // Single string comma-separated?
            std::string s = value.asString();
            // Simple split by comma
            std::string cur;
            for (char ch : s) {
                if (ch == ',') { if (!cur.empty()) { cols.push_back({cur}); cur.clear(); } }
                else cur.push_back(ch);
            }
            if (!cur.empty()) cols.push_back({cur});
        }
        setColumns(std::move(cols));
        return;
    }
    if (name == "rows") {
        // Not implemented yaml rows
        return;
    }
    Widget::setProperty(name, value);
}

}  // namespace fluffy
