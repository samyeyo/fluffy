// include/fluffy/textinput.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// TextInput — single-line editable text field with cursor, placeholder, focus.

#pragma once
#include <fluffy/inline.hpp>
#include <fluffy/widget.hpp>
#include <functional>
#include <string>
#include <vector>
namespace fluffy {
class TextInput final : public Widget {
public:
    using ChangeHandler = std::function<void(const std::string&)>;

    explicit TextInput(Rect bounds = Rect{}, std::string placeholder = "");
    // ––––––––[ TextInput ] plain-text convenience without a rect
    explicit TextInput(std::string placeholder) : TextInput(Rect{}, std::move(placeholder)) {}

    void         draw(NVGcontext* vg, const Theme& theme, const Window& w) override;
    EventResult  handleEvent(const Event& e, const Window& w) override;
    void         tick(float dt, const Theme& theme) noexcept override;
    void         updateHover(float mx, float my) noexcept override;
    void         setProperty(std::string_view name, const YamlNode& value) override;
    float naturalWidth(const Theme& theme, const Window& w) const override;
    float naturalHeight(const Theme& theme, const Window& w) const override;

    const std::string& text()        const noexcept { return text_; }
    void               setText(std::string t);

    const std::string& placeholder() const noexcept { return placeholder_; }
    void               setPlaceholder(std::string p) { placeholder_ = std::move(p); }

    void               setPrefix(Inline content) { prefix_ = content.takeRuns(); }

    bool              isFocused() const noexcept { return focused_; }
    bool              isFocusable() const noexcept override { return true; }
    void              setFocused(bool f) noexcept override;

    bool              isMultiline() const noexcept { return multiline_; }
    void              setMultiline(bool m) noexcept { multiline_ = m; }

    bool              growable() const noexcept { return growable_; }
    void              setGrowable(bool g) noexcept { growable_ = g; }

    float             scrollX() const noexcept { return scrollX_; }
    float             scrollY() const noexcept { return scrollY_; }
    void              scrollToX(float x) noexcept { scrollX_ = (std::max)(0.0f, x); }
    void              scrollToY(float y) noexcept { scrollY_ = (std::max)(0.0f, y); }

    using SubmitHandler = std::function<void(const std::string&)>;

    void setOnChange(ChangeHandler h) { on_change_ = std::move(h); }
    void setOnSubmit(SubmitHandler h) { on_submit_ = std::move(h); }
    // ––––––––[ onChange ] chainable change handler, preferred over setOnChange
    TextInput& onChange(ChangeHandler h) { on_change_ = std::move(h); return *this; }
    // ––––––––[ onSubmit ] chainable submit handler, preferred over setOnSubmit
    TextInput& onSubmit(SubmitHandler h) { on_submit_ = std::move(h); return *this; }

private:
    struct UndoState {
        std::string text;
        int         cursor_pos  = 0;
        int         sel_anchor  = 0;
    };
    static constexpr std::size_t kMaxUndo = 20;

    std::string  text_;
    std::string  placeholder_;
    std::vector<Inline::Run> prefix_;
    bool         multiline_           = false;
    bool         growable_            = true;
    float        scrollX_             = 0.0f;
    float        scrollY_             = 0.0f;
    bool         focused_             = false;
    bool         hovered_             = false;
    int          cursor_pos_          = 0;
    float        cursor_blink_timer_  = 0.0f;
    bool         cursor_visible_      = true;
    int          sel_anchor_          = 0;
    bool         selecting_           = false;
    ChangeHandler  on_change_;
    SubmitHandler  on_submit_;
    std::vector<UndoState> undo_stack_;
    std::vector<UndoState> redo_stack_;
    // ––––––––[ line_starts_ ] cached byte offsets of '\n'+1 for each line.
    // Rebuilt on text mutation; consumed by cursorLine()/cursorXFromIndex()
    // so arrow-key navigation and per-frame cursor rendering stay O(log N)
    // on long pastes instead of an O(line_length) backward scan.
    std::vector<int>      line_starts_;
    // ––––––––[ last_click_time_ ] time of last mouse down for double-click detection.
    double                last_click_time_ = -1e9;
    // ––––––––[ last_click_idx_ ] byte index of last mouse down.
    int                   last_click_idx_  = -1;

    void ensureCursorVisible(const Theme& theme, const Window& w);
    float contentWidth(const Theme& theme, const Window& w) const;
    float contentHeight(const Theme& theme, const Window& w) const;
    void insertChar(char32_t cp);
    void backspace();
    void deleteForward();
    void moveCursorLeft();
    void moveCursorRight();
    void moveCursorHome();
    void moveCursorEnd();
    int  cursorIndexFromXY(float x, float y, const Theme& theme, const Window& w) const;
    float prefixWidth(const Theme& theme, const Window& w) const;
    float cursorXFromIndex(int idx, const Theme& theme, const Window& w) const;
    int  cursorLine() const noexcept;
    int  cursorCol()  const noexcept;
    int  lineStart(int line) const noexcept;
    int  lineEnd(int line)   const noexcept;
    int  lineCount() const noexcept;
    void moveCursorUp(const Theme& theme, const Window& w);
    void moveCursorDown(const Theme& theme, const Window& w);
    float textX(const Theme& theme, const Window& w) const;
    void resetCursorBlink();
    void deleteSelection();
    void pushState();
    void undo();
    void redo();
    void selectAll();
    void copySelection(const Window& w) const;
    void pasteText(const char* str);
    void rebuildLineIndex() noexcept;
    void selectWordAt(int idx);
    int  selStart() const noexcept { return (std::min)(sel_anchor_, cursor_pos_); }
    int  selEnd()   const noexcept { return (std::max)(sel_anchor_, cursor_pos_); }
    bool hasSelection() const noexcept { return sel_anchor_ != cursor_pos_; }
};

}  // namespace fluffy
