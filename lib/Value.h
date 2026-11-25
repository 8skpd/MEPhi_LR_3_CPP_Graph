#pragma once

#include <string>
#include <variant>
#include "Exceptions.h"

// Типизированное значение: int | double | string.
// Поддерживает сравнение с проверкой типов.

enum class ValueType { Int, Double, String };

class Value {
public:
    std::variant<int, double, std::string> data;

    Value() : data(0) {}
    Value(int v) : data(v) {}
    Value(double v) : data(v) {}
    Value(const std::string& v) : data(v) {}
    Value(const char* v) : data(std::string(v)) {}

    ValueType type() const {
        switch (data.index()) {
            case 0: return ValueType::Int;
            case 1: return ValueType::Double;
            case 2: return ValueType::String;
        }
        return ValueType::Int;
    }

    bool is_int() const    { return std::holds_alternative<int>(data); }
    bool is_double() const { return std::holds_alternative<double>(data); }
    bool is_string() const { return std::holds_alternative<std::string>(data); }

    bool is_numeric() const { return is_int() || is_double(); }

    // Приведение к double для арифметики весов
    double to_double() const {
        if (is_int())    return static_cast<double>(std::get<int>(data));
        if (is_double()) return std::get<double>(data);
        throw TypeMismatch(__func__, "нечисловое значение нельзя привести к double");
    }

    // Сравнение на равенство (разные типы — не равны)
    bool operator==(const Value& other) const {
        if (data.index() != other.data.index()) return false;
        return data == other.data;
    }

    bool operator!=(const Value& other) const {
        return !(*this == other);
    }

    // Лексикографическое сравнение (только внутри одного типа)
    bool operator<(const Value& other) const {
        if (data.index() != other.data.index()) {
            throw TypeMismatch(__func__, "сравнение значений разных типов");
        }
        return data < other.data;
    }

    bool operator>(const Value& other) const  { return other < *this; }
    bool operator<=(const Value& other) const { return !(other < *this); }
    bool operator>=(const Value& other) const { return !(*this < other); }

    // Сложение (для весов)
    Value operator+(const Value& other) const {
        if (is_int() && other.is_int()) {
            return Value(std::get<int>(data) + std::get<int>(other.data));
        }
        if (is_numeric() && other.is_numeric()) {
            return Value(to_double() + other.to_double());
        }
        throw TypeMismatch(__func__, "нельзя сложить значения");
    }

    // Строковое представление
    std::string to_string() const {
        if (is_int())    return std::to_string(std::get<int>(data));
        if (is_double()) return std::to_string(std::get<double>(data));
        return std::get<std::string>(data);
    }
};

// Хеш для Value (нужен для HashTable<Value, ...>)
namespace std {
    template <>
    struct hash<Value> {
        size_t operator()(const Value& v) const noexcept {
            switch (v.data.index()) {
                case 0: return hash<int>{}(std::get<int>(v.data));
                case 1: return hash<double>{}(std::get<double>(v.data));
                case 2: return hash<string>{}(std::get<string>(v.data));
            }
            return 0;
        }
    };
}