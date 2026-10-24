// include/fluffy/yaml_fwd.hpp

// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

// YamlNode — thin wrapper around dyml (DirectYAML) row-level API.
// dyml.h is a 477-line header-only library (MIT).

#pragma once

#include <dyml/dyml.h>

#include <cstdlib>
#include <cstddef>
#include <string>
#include <string_view>

namespace fluffy {

// ––––––––[ parseYaml ] parse YAML text into a dyml::Directyaml
// The input buffer is modified in place (destructive parse).
// The returned Directyaml MUST outlive any YamlNode referencing it.
inline dyml::Directyaml parseYaml(std::string& buf) {
    dyml::Directyaml dy;
    dy.parse(&buf[0], false); // false = managed externally (buf stays alive)
    return dy;
}

// ––––––––[ YamlNode ] read-only node wrapping a dyml row index.
// row_ = -1 signals the virtual root.
class YamlNode {
public:
    YamlNode() = default;
    explicit YamlNode(const dyml::Directyaml& dy) : dy_(&dy), row_(-1) {}
    YamlNode(const dyml::Directyaml& dy, int row) : dy_(&dy), row_(row) {}

    // --- Type queries -----------------------------------------------------

    bool IsMap()      const noexcept { return ok() && childCount() > 0; }
    bool IsScalar()   const noexcept { return ok() && row_ >= 0 && hasValue(); }
    bool IsSequence() const noexcept { return false; }
    bool IsNull()     const noexcept { return !ok() || !hasValue(); }
    bool IsDefined()  const noexcept { return ok(); }

    // --- Children count ---------------------------------------------------

    std::size_t size() const noexcept {
        return static_cast<std::size_t>(childCount());
    }

    // --- Key / value ------------------------------------------------------

    std::string key() const {
        if (!ok() || row_ < 0) return {};
        auto k = dy_->rows()[static_cast<std::size_t>(row_)].key;
        return k ? std::string(k) : std::string();
    }

    std::string asString() const {
        if (!ok()) return {};
        auto v = row_ >= 0 ? dy_->rows()[static_cast<std::size_t>(row_)].val : nullptr;
        if (!v) return {};
        std::string s(v);
        // Strip surrounding double quotes if present.
        if (s.size() >= 2 && s.front() == '"' && s.back() == '"') {
            s = s.substr(1, s.size() - 2);
            // Unescape \" inside the string.
            for (std::size_t p = s.find("\\\""); p != std::string::npos;
                 p = s.find("\\\"", p + 1))
                s.replace(p, 2, "\"");
        }
        // Strip surrounding single quotes if present.
        else if (s.size() >= 2 && s.front() == '\'' && s.back() == '\'') {
            s = s.substr(1, s.size() - 2);
        }
        return s;
    }

    int asInt() const {
        auto s = asString();
        if (s.empty()) return 0;
        char* end = nullptr;
        long v = std::strtol(s.c_str(), &end, 0);
        if (end != s.c_str() + static_cast<std::ptrdiff_t>(s.size())) return 0;
        return static_cast<int>(v);
    }

    float asFloat() const {
        auto s = asString();
        if (s.empty()) return 0.0f;
        char* end = nullptr;
        float v = std::strtof(s.c_str(), &end);
        if (end != s.c_str() + static_cast<std::ptrdiff_t>(s.size())) return 0.0f;
        return v;
    }

    bool asBool() const {
        auto s = asString();
        if (s == "true"  || s == "True"  || s == "TRUE"  || s == "1") return true;
        if (s == "false" || s == "False" || s == "FALSE" || s == "0") return false;
        return false;
    }

    // --- Child access -----------------------------------------------------

    YamlNode operator[](std::string_view k) const {
        if (!ok()) return {};
        int targetLevel = row_ < 0 ? 0 : rowLevel() + 1;
        const auto& rows = dy_->rows();
        int n = static_cast<int>(rows.size());
        int start = row_ < 0 ? 0 : row_ + 1;
        for (int i = start; i < n; ++i) {
            if (row_ >= 0 && rows[static_cast<std::size_t>(i)].level <= rowLevel()) break;
            if (rows[static_cast<std::size_t>(i)].level == targetLevel &&
                rows[static_cast<std::size_t>(i)].key && k == rows[static_cast<std::size_t>(i)].key) {
                return YamlNode(*dy_, i);
            }
        }
        return {};
    }

    YamlNode operator[](std::size_t i) const {
        if (!ok()) return {};
        int targetLevel = row_ < 0 ? 0 : rowLevel() + 1;
        const auto& rows = dy_->rows();
        int n = static_cast<int>(rows.size());
        int start = row_ < 0 ? 0 : row_ + 1;
        int found = 0;
        for (int j = start; j < n; ++j) {
            if (row_ >= 0 && rows[static_cast<std::size_t>(j)].level <= rowLevel()) break;
            if (rows[static_cast<std::size_t>(j)].level == targetLevel) {
                if (found == static_cast<int>(i)) return YamlNode(*dy_, j);
                ++found;
            }
        }
        return {};
    }

    // --- Iteration --------------------------------------------------------

    class Iterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type        = YamlNode;
        using difference_type   = std::ptrdiff_t;

        Iterator() = default;
        Iterator(const dyml::Directyaml& dy, int startRow, int parentLevel)
            : dy_(&dy), row_(startRow), parentLevel_(parentLevel) {
            advanceToChild();
        }

        YamlNode  operator*()  const { return YamlNode(*dy_, row_); }
        Iterator& operator++() { ++row_; advanceToChild(); return *this; }
        bool operator!=(const Iterator& o) const { return row_ != o.row_; }

    private:
        const dyml::Directyaml* dy_ = nullptr;
        int row_                     = -1;
        int parentLevel_             = -2;

        void advanceToChild() {
            if (!dy_) return;
            const auto& rows = dy_->rows();
            int n = static_cast<int>(rows.size());
            while (row_ < n) {
                const auto& r = rows[static_cast<std::size_t>(row_)];
                if (r.level <= parentLevel_) break;
                if (r.level == parentLevel_ + 1) return;
                ++row_;
            }
            row_ = -1; // exhausted — match end() sentinel
        }
    };

    Iterator begin() const {
        if (!ok()) return {};
        int targetLevel = row_ < 0 ? 0 : rowLevel() + 1;
        int start = row_ < 0 ? 0 : row_ + 1;
        return Iterator(*dy_, start, targetLevel - 1);
    }

    Iterator end() const { return {}; }

private:
    const dyml::Directyaml* dy_ = nullptr;
    int row_                     = -1;

    bool ok() const noexcept { return dy_ != nullptr; }

    int rowLevel() const noexcept {
        if (row_ < 0 || static_cast<std::size_t>(row_) >= dy_->rows().size()) return -1;
        return dy_->rows()[static_cast<std::size_t>(row_)].level;
    }

    bool hasValue() const noexcept {
        if (row_ < 0 || static_cast<std::size_t>(row_) >= dy_->rows().size()) return false;
        return dy_->rows()[static_cast<std::size_t>(row_)].val != nullptr;
    }

    int childCount() const noexcept {
        if (!ok()) return 0;
        const auto& rows = dy_->rows();
        int lv = rowLevel();
        int n = static_cast<int>(rows.size());
        int count = 0;
        for (int i = row_ + 1; i < n; ++i) {
            if (rows[static_cast<std::size_t>(i)].level <= lv) break;
            if (rows[static_cast<std::size_t>(i)].level == lv + 1) ++count;
        }
        return count;
    }
};

} // namespace fluffy
