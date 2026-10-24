// include/fluffy/table.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Table — editable grid with rows/columns, selection and cell editing
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
#pragma once
#include <fluffy/animation.hpp>
#include <fluffy/combobox.hpp>
#include <fluffy/inline.hpp>
#include <fluffy/textinput.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <memory>
#include <string>
#include <vector>
namespace fluffy {
class Table final : public Widget {
public:
    // ––––––––[ Column ] header definition
    struct Column {
        std::string label;
        float width = 120.0f;
        bool editable = true;
        enum class Kind { Text, Choice, Bool } kind = Kind::Text;
        Inline headerInline;
        std::vector<std::string> choices;
    };

    explicit Table(Rect bounds = Rect{});

    // ––––––––[ setColumns ] replaces columns, resizes existing rows
    void setColumns(std::vector<Column> cols);

    // ––––––––[ addColumn ] appends column
    void addColumn(Column col);
    // ––––––––[ setHeader ] sets header Inline for column (glyphs)
    void setHeader(int col, Inline h);

    // ––––––––[ setRows ] replaces all rows
    void setRows(std::vector<std::vector<std::string>> rows);

    // ––––––––[ setRowsInline ] replaces all rows with Inline
    void setRowsInline(std::vector<std::vector<Inline>> rows);

    // ––––––––[ addRow ] appends row (padded to column count)
    void addRow(std::vector<std::string> row);

    // ––––––––[ addRowInline ] appends Inline row
    void addRowInline(std::vector<Inline> row);

    // ––––––––[ setCell ] sets cell text
    void setCell(int row, int col, std::string value);

    // ––––––––[ setCellInline ] sets cell Inline (with glyphs)
    void setCellInline(int row, int col, Inline value);

    // ––––––––[ cell ] returns cell text or empty
    std::string cell(int row, int col) const;

    // ––––––––[ cellInline ] returns cell Inline or empty
    Inline cellInline(int row, int col) const;

    // ––––––––[ setOnCellChange ] fired when cell edited
    void setOnCellChange(std::function<void(int row, int col, const std::string& value)> h) { on_change_ = std::move(h); }
    // ––––––––[ onCellChange ] chainable cell handler, preferred over setOnCellChange
    Table& onCellChange(std::function<void(int, int, const std::string&)> h) { on_change_ = std::move(h); return *this; }
    // ––––––––[ setOnSelect ] fired when selection changes
    void setOnSelect(std::function<void(int row, int col)> h) { on_select_ = std::move(h); }
    // ––––––––[ onSelect ] chainable selection handler, preferred over setOnSelect
    Table& onSelect(std::function<void(int, int)> h) { on_select_ = std::move(h); return *this; }

    int selectedRow() const noexcept { return sel_row_; }
    int selectedCol() const noexcept { return sel_col_; }
    int rowCount() const noexcept { return static_cast<int>(rows_.size()); }
    int columnCount() const noexcept { return static_cast<int>(columns_.size()); }
    void setSelected(int row, int col = 0);
    // ––––––––[ beginEdit ] starts editing a cell programmatically
    void beginEdit(int r, int c, const Window& w) { startEdit(r, c, w); }

    // ––––––––[ setHeaderBackground ] header bk
    void setHeaderBackground(Color c) noexcept { header_bg_ = c; header_bg_set_ = true; }
    // ––––––––[ setHeaderTextColor ] header text
    void setHeaderTextColor(Color c) noexcept { header_text_ = c; header_text_set_ = true; }
    // ––––––––[ setHeaderFont ] header font
    void setHeaderFont(std::string f) { header_font_ = std::move(f); }
    // ––––––––[ setCellBackground ] all cells bg (even rows)
    void setCellBackground(Color c) noexcept { cell_bg_ = c; cell_bg_set_ = true; }
    // ––––––––[ setCellBackgroundAlt ] alt row bg
    void setCellBackgroundAlt(Color c) noexcept { cell_bg_alt_ = c; cell_bg_alt_set_ = true; }
    // ––––––––[ setCellTextColor ] all cells text
    void setCellTextColor(Color c) noexcept { cell_text_ = c; cell_text_set_ = true; }
    // ––––––––[ setCellFont ] all cells font
    void setCellFont(std::string f) { cell_font_ = std::move(f); }

    void draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult handleEvent(const Event& e, const Window& w) override;
    void tick(float dt, const Theme& theme) noexcept override;
    void updateHover(float mx, float my) noexcept override;
    bool isFocusable() const noexcept override { return true; }
    void setFocused(bool f) noexcept override;
    void setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;
    void setWindow(Window* w) noexcept override;

private:
    // ––––––––[ columns_ ] headers
    std::vector<Column> columns_;
    // ––––––––[ rows_ ] cell data (Inline for glyphs)
    std::vector<std::vector<Inline>> rows_;
    // ––––––––[ sel_row_ ] selected row (-1 none)
    int sel_row_ = -1;
    // ––––––––[ sel_col_ ] selected col (-1 none)
    int sel_col_ = -1;
    // ––––––––[ hover_row_ ] hovered row
    int hover_row_ = -1;
    // ––––––––[ hover_col_ ] hovered col
    int hover_col_ = -1;
    // ––––––––[ scroll_x_ ] horizontal scroll
    float scroll_x_ = 0.0f;
    // ––––––––[ scroll_y_ ] vertical scroll
    float scroll_y_ = 0.0f;
    // ––––––––[ focused_ ] focus
    bool focused_ = false;
    // ––––––––[ editing_ ] editing state
    bool editing_ = false;
    // ––––––––[ edit_row_ ] editing row
    int edit_row_ = -1;
    // ––––––––[ edit_col_ ] editing col
    int edit_col_ = -1;
    // ––––––––[ on_change_ ] cell change callback
    std::function<void(int, int, const std::string&)> on_change_;
    // ––––––––[ on_select_ ] selection callback
    std::function<void(int, int)> on_select_;
    // ––––––––[ editor_ ] inline cell editor (Text)
    std::unique_ptr<TextInput> editor_;
    // ––––––––[ combo_editor_ ] Choice editor (ComboBox)
    std::unique_ptr<class ComboBox> combo_editor_;
    // ––––––––[ combo_auto_open_ ] defer popup to next tick to avoid reentrancy
    bool combo_auto_open_ = false;
    // ––––––––[ pending_choice_ ] deferred commit for Choice to avoid reentrancy
    bool pending_choice_ = false;
    int pending_row_ = -1;
    int pending_col_ = -1;
    std::string pending_value_;
    // ––––––––[ header_bg_ ] header background (if set)
    Color header_bg_ = Color{0,0,0,0};
    bool header_bg_set_ = false;
    // ––––––––[ header_text_ ] header text color
    Color header_text_ = Color{0,0,0,255};
    bool header_text_set_ = false;
    // ––––––––[ header_font_ ] header font
    std::string header_font_;
    // ––––––––[ cell_bg_ ] cell background
    Color cell_bg_ = Color{0,0,0,0};
    bool cell_bg_set_ = false;
    // ––––––––[ cell_bg_alt_ ] alt row background
    Color cell_bg_alt_ = Color{0,0,0,0};
    bool cell_bg_alt_set_ = false;
    // ––––––––[ cell_text_ ] cell text color
    Color cell_text_ = Color{0,0,0,255};
    bool cell_text_set_ = false;
    // ––––––––[ cell_font_ ] cell font
    std::string cell_font_;
    // ––––––––[ scrollbar state ] auto hide/drag
    SmoothAnim scroll_anim_h_;
    SmoothAnim scroll_anim_v_;
    bool scrollbar_hovered_h_ = false;
    bool scrollbar_hovered_v_ = false;
    bool dragging_h_ = false;
    bool dragging_v_ = false;
    float drag_start_x_ = 0.0f;
    float drag_start_y_ = 0.0f;
    float drag_orig_x_ = 0.0f;
    float drag_orig_y_ = 0.0f;
    bool wheel_active_ = false;
    float wheel_timer_ = 0.0f;
    // ––––––––[ header_h_ ] header height cache
    float header_h(const Theme& theme) const noexcept;
    // ––––––––[ row_h_ ] row height
    float row_h(const Theme& theme) const noexcept;
    // ––––––––[ col_x_ ] x of column start
    float col_x(int col) const noexcept;
    // ––––––––[ hitCell ] returns row/col under mx,my or -1
    void hitCell(float mx, float my, const Theme& theme, int& out_row, int& out_col) const noexcept;
    // ––––––––[ ensureVisible ] scroll to show cell
    void ensureVisible(int row, int col, const Theme& theme);
    // ––––––––[ startEdit ] begin editing cell
    void startEdit(int row, int col, const Window& w);
    // ––––––––[ commitEdit ] save and exit
    void commitEdit();
    // ––––––––[ cancelEdit ] abort editing
    void cancelEdit();
};
}  // namespace fluffy
