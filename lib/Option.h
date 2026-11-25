#pragma once

#include <optional>
#include <utility>
#include "Exceptions.h"

// Тип Option<T> — обёртка над std::optional с более
// выразительным API. Может содержать значение или быть пустым.

template <typename T>
class Option {
private:
    std::optional<T> data_;

public:
    Option() = default;
    Option(const T& value) : data_(value) {}
    Option(T&& value) : data_(std::move(value)) {}

    bool has_value() const { return data_.has_value(); }
    explicit operator bool() const { return data_.has_value(); }

    const T& value() const {
        if (!data_) throw EmptyOption(__func__);
        return *data_;
    }

    T& value() {
        if (!data_) throw EmptyOption(__func__);
        return *data_;
    }

    T value_or(const T& default_value) const {
        return data_.value_or(default_value);
    }

    const T& operator*() const { return value(); }
    T& operator*() { return value(); }

    const T* operator->() const { return &value(); }
    T* operator->() { return &value(); }
};

template <typename T>
Option<T> Some(T value) {
    return Option<T>(std::move(value));
}

template <typename T>
Option<T> None() {
    return Option<T>();
}