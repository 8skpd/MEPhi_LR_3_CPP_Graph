#pragma once

#include <limits>
#include "MstResult.h"
#include "../Graph.h"
#include "../PriorityQueue.h"

/*
Алгоритм Прима: минимальный остов (или остовный лес).

Идея: растим одно дерево, на каждом шаге добавляя минимальное
ребро, соединяющее дерево с новой вершиной. Реализация с
«ленивым удалением» через PriorityQueue (без decrease_key).

Для несвязного графа запускаемся от каждой непосещённой вершины,
получая минимальный остовный лес.

Сложность: O(E log V).
Отрицательные веса допускаются.
*/


inline MstResult prim(const Graph& g) {
    const size_t n = g.vertex_count();

    MstResult res;
    res.method = "PRIM";
    res.vertex_count = n;

    if (n == 0) {
        res.connected = true;
        return res;
    }

    const DynamicArray<GraphEdge> all = g.edges();
    for (size_t i = 0; i < all.size(); ++i) {
        if (!all[i].weight.is_numeric()) {
            throw TypeMismatch(__func__, "вес ребра не является числом");
        }
    }

    DynamicArray<bool> in_tree;
    DynamicArray<double> best;                 // минимальный вес ребра в дерево
    DynamicArray<Option<size_t>> parent;       // вершина дерева, откуда пришли

    for (size_t i = 0; i < n; ++i) {
        in_tree.push_back(false);
        best.push_back(std::numeric_limits<double>::infinity());
        parent.push_back(None<size_t>());
    }

    PriorityQueue<size_t, double> pq;

    // Внешний цикл — для построения остовного леса в несвязном графе
    for (size_t start = 0; start < n; ++start) {
        if (in_tree[start]) continue;

        best[start] = 0.0;
        pq.enqueue(start, 0.0);

        while (!pq.empty()) {
            const size_t u = pq.dequeue().value();
            if (in_tree[u]) continue;          // устаревшая запись очереди
            in_tree[u] = true;

            if (parent[u].has_value()) {
                // best[u] — вес минимального ребра, соединившего u с деревом.
                // Используем именно его: корректно даже при параллельных рёбрах.
                res.edges.push_back(GraphEdge(parent[u].value(), u, Value(best[u])));
                res.total_weight += best[u];
            }

            const DynamicArray<Edge>& nb = g.neighbors(u);
            for (size_t i = 0; i < nb.size(); ++i) {
                const size_t v = nb[i].to;
                if (in_tree[v]) continue;
                const double w = nb[i].weight.to_double();
                if (w < best[v]) {
                    best[v] = w;
                    parent[v] = Some<size_t>(u);
                    pq.enqueue(v, w);          // ленивое обновление приоритета
                }
            }
        }
    }

    res.connected = (res.edges.size() == n - 1);
    return res;
}