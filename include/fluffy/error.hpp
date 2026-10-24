// include/fluffy/error.hpp
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––
// Result — lightweight Result<T, E> type with err() factory.
// Beginner path: check `if (!r)` then `.value()`; or `.unwrap()` to abort
// with a message when failure is unrecoverable (works with no exceptions).
//
// MIT License - Copyright (c) 2026 - Samir Tine
// ––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––––

#pragma once
#include <cstdio>
#include <cstdlib>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
namespace fluffy {
template <typename T, typename E = std::string>
class [[nodiscard]] Result {
public:
    Result(const T& v) : data_(std::in_place_index<0>, v)             {}
    Result(T&& v)      : data_(std::in_place_index<0>, std::move(v)) {}
    template <std::size_t I, typename... Args>
    explicit Result(std::in_place_index_t<I>, Args&&... args)
        : data_(std::in_place_index<I>, std::forward<Args>(args)...) {}
    static Result err(E e) {
        return Result(std::in_place_index<1>, std::move(e));
    }
    [[nodiscard]] bool            ok()     const noexcept { return data_.index() == 0; }
    [[nodiscard]] bool            has_value() const noexcept { return ok(); }
    [[nodiscard]] explicit operator bool() const noexcept { return ok(); }
    const T& value()  const&  { return std::get<0>(data_); }
    T&       value()       &  { return std::get<0>(data_); }
    T&&      value()      &&  { return std::move(std::get<0>(data_)); }
    const E& error()  const& noexcept { return std::get<1>(data_); }
    E&       error()      & noexcept { return std::get<1>(data_); }
    const T& operator*()  const&  { return value(); }
    T&       operator*()       &  { return value(); }
    const T* operator->() const   { return &value(); }
    T*       operator->()         { return &value(); }

    // ––––––––[ unwrap ] returns value or aborts with error on stderr
    T& unwrap() & {
        if (!ok()) {
            std::fputs(error().c_str(), stderr);
            std::fputc('\n', stderr);
            std::abort();
        }
        return value();
    }
    // ––––––––[ unwrap ] const lvalue overload aborts on error
    const T& unwrap() const& {
        if (!ok()) {
            std::fputs(error().c_str(), stderr);
            std::fputc('\n', stderr);
            std::abort();
        }
        return value();
    }
    // ––––––––[ unwrap ] rvalue overload moves value or aborts
    T&& unwrap() && {
        if (!ok()) {
            std::fputs(error().c_str(), stderr);
            std::fputc('\n', stderr);
            std::abort();
        }
        return std::move(value());
    }
    // ––––––––[ expect ] unwraps with a custom prefix message on failure
    T& expect(const char* msg) & {
        if (!ok()) {
            std::fputs(msg, stderr);
            std::fputs(": ", stderr);
            std::fputs(error().c_str(), stderr);
            std::fputc('\n', stderr);
            std::abort();
        }
        return value();
    }
    // ––––––––[ value_or ] returns the contained value or a fallback in error state
    template <typename U>
    T value_or(U&& fallback) const& {
        return ok() ? value() : static_cast<T>(std::forward<U>(fallback));
    }
    template <typename U>
    T value_or(U&& fallback) && {
        return ok() ? std::move(value()) : static_cast<T>(std::forward<U>(fallback));
    }

    // ––––––––[ map ] applies f to the value, propagating the error unchanged
    template <typename F>
    auto map(F&& f) const& -> Result<std::invoke_result_t<F, const T&>, E> {
        using U = std::invoke_result_t<F, const T&>;
        if (ok()) return Result<U, E>(f(value()));
        return Result<U, E>::err(error());
    }

    // ––––––––[ and_then ] chains f returning Result<U, E>, propagating errors
    template <typename F>
    auto and_then(F&& f) const& -> std::invoke_result_t<F, const T&> {
        using R = std::invoke_result_t<F, const T&>;
        if (ok()) return f(value());
        return R::err(error());
    }
private:
    std::variant<T, E> data_;
};
using NoneResult = Result<std::monostate>;
}  

