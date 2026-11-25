#pragma once

#include <string>
#include <algorithm>
#include "../lib/DynamicArray.h"

/*
Структурированный результат последней команды для
визуализации. Не зависит от графической библиотеки.

Позволяет GUI подсвечивать:
  - найденный путь
  - рёбра минимального остова
  - раскраску по компонентам связности
*/


struct GraphHighlight {
    // Путь: последовательность имён вершин
    DynamicArray<std::string> path;
    bool has_path = false;

    // Остов: пары имён, нормализованные (меньшее имя первым)
    DynamicArray<std::string> mst_a;
    DynamicArray<std::string> mst_b;
    bool has_mst = false;

    // Компоненты: номер компоненты для каждой вершины по индексу
    DynamicArray<size_t> components;
    size_t component_count = 0;
    bool has_components = false;

    // Нормализованный ключ ребра — не зависит от направления
    static std::string edge_key(const std::string& a, const std::string& b) {
        return (a < b) ? (a + "\x1F" + b) : (b + "\x1F" + a);
    }

    void add_mst_edge(const std::string& a, const std::string& b) {
        if (a < b) { mst_a.push_back(a); mst_b.push_back(b); }
        else       { mst_a.push_back(b); mst_b.push_back(a); }
    }

    bool is_mst_edge(const std::string& a, const std::string& b) const {
        const std::string key = edge_key(a, b);
        for (size_t i = 0; i < mst_a.size(); ++i) {
            if (edge_key(mst_a[i], mst_b[i]) == key) return true;
        }
        return false;
    }

    bool is_path_edge(const std::string& a, const std::string& b) const {
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            if (edge_key(path[i], path[i + 1]) == edge_key(a, b)) return true;
        }
        return false;
    }

    bool is_path_vertex(const std::string& name) const {
        for (size_t i = 0; i < path.size(); ++i) {
            if (path[i] == name) return true;
        }
        return false;
    }
};