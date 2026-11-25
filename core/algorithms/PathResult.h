#pragma once

#include <string>
#include <utility>
#include "../../lib/DynamicArray.h"
#include "../../lib/Option.h"
#include "../../lib/Exceptions.h"

/*
Единый результат алгоритмов поиска путей от одной вершины.

dist[v]   — расстояние от source (None, если недостижима)
parent[v] — предшественник в дереве путей (None у source)
order     — порядок посещения вершин (для отображения
            промежуточных результатов в UI)

Расстояние хранится как double: это позволяет сравнивать
BFS (целые числа рёбер) и взвешенные алгоритмы напрямую.
*/


struct PathResult {
    size_t      source = 0;
    std::string method;

    DynamicArray<Option<double>> dist;
    DynamicArray<Option<size_t>> parent;
    DynamicArray<size_t>         order;

    size_t reached = 0;      // число достижимых вершин, включая source
    double elapsed_ms = 0.0; // заполняется слоем бенчмарков

    PathResult() = default;

    PathResult(size_t n, size_t src, std::string m)
        : source(src), method(std::move(m)) {
        for (size_t i = 0; i < n; ++i) {
            dist.push_back(None<double>());
            parent.push_back(None<size_t>());
        }
    }

    bool has_path(size_t v) const {
        return v < dist.size() && dist[v].has_value();
    }

    Option<double> distance_to(size_t v) const {
        if (v >= dist.size()) throw IndexOutOfRange(__func__);
        return dist[v];
    }
};

// Восстановление пути source -> target.
// None, если вершина недостижима.
inline Option<DynamicArray<size_t>> reconstruct_path(const PathResult& r, size_t target) {
    if (target >= r.dist.size()) throw IndexOutOfRange(__func__);
    if (!r.dist[target].has_value()) return None<DynamicArray<size_t>>();

    DynamicArray<size_t> reversed;
    size_t cur = target;
    while (true) {
        reversed.push_back(cur);
        if (cur == r.source) break;
        const Option<size_t> p = r.parent[cur];
        if (!p.has_value()) return None<DynamicArray<size_t>>();  // защита от разрыва цепи
        cur = p.value();
    }

    DynamicArray<size_t> path;
    for (size_t i = reversed.size(); i > 0; --i) {
        path.push_back(reversed[i - 1]);
    }
    return Some<DynamicArray<size_t>>(std::move(path));
}