#pragma once

#include <exception>
#include <string>


// Иерархия исключений для проекта «Графовые алгоритмы».
// Базовый класс наследует std::exception (из <exception>,
// не из <stdexcept>), все конкретные классы — наши собственные.


class GraphException : public std::exception {
protected:
    std::string message_;

public:
    GraphException(const char* where, const std::string& msg)
        : message_(std::string(where) + ": " + msg) {}

    const char* what() const noexcept override {
        return message_.c_str();
    }
};

// Предметные исключения

class VertexNotFound : public GraphException {
public:
    VertexNotFound(const char* where, const std::string& name)
        : GraphException(where, "вершина не найдена: " + name) {}
};

class EdgeNotFound : public GraphException {
public:
    EdgeNotFound(const char* where)
        : GraphException(where, "ребро не найдено") {}
};

class IndexOutOfRange : public GraphException {
public:
    IndexOutOfRange(const char* where)
        : GraphException(where, "индекс вне диапазона") {}
};

class TypeMismatch : public GraphException {
public:
    TypeMismatch(const char* where, const std::string& msg)
        : GraphException(where, msg) {}
};

class EmptyOption : public GraphException {
public:
    EmptyOption(const char* where)
        : GraphException(where, "значение отсутствует") {}
};

class DuplicateKey : public GraphException {
public:
    DuplicateKey(const char* where)
        : GraphException(where, "ключ уже существует") {}
};

// Отрицательный вес там, где алгоритм его не допускает (Дейкстра)
class NegativeWeight : public GraphException {
public:
    NegativeWeight(const char* where, const std::string& msg)
        : GraphException(where, msg) {}
};

// Обнаружен отрицательный цикл (Форд-Беллман)
class NegativeCycle : public GraphException {
public:
    NegativeCycle(const char* where, const std::string& msg)
        : GraphException(where, msg) {}
};