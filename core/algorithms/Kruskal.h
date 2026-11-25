#pragma once

#include "MstResult.h"
#include "../Graph.h"
#include "../DSU.h"
#include "../PriorityQueue.h"


/*
Алгоритм Крускала: минимальный остов (или остовный лес).

Идея: сортируем все рёбра по весу и жадно добавляем каждое,
если оно не создаёт цикл. Проверка цикла — через DSU.

Сортировка рёбер выполнена через нашу бинарную кучу
(PriorityQueue как приоритетная очередь): это переиспользование
инфраструктуры ЛР-3 и гарантированное O(E log E) без риска
деградации на отсортированных входных данных.

Сложность: O(E log E).
Отрицательные веса допускаются.
*/

inline MstResult kruskal(const Graph& g) {
    const size_t n = g.vertex_count();

    MstResult res;
    res.method = "KRUSKAL";
    res.vertex_count = n;

    const DynamicArray<GraphEdge> all = g.edges();
    for (size_t i = 0; i < all.size(); ++i) {
        if (!all[i].weight.is_numeric()) {
            throw TypeMismatch(__func__, "вес ребра не является числом");
        }
    }

    // Сортировка рёбер по возрастанию веса через бинарную кучу
    PriorityQueue<GraphEdge, double> sorter;
    for (size_t i = 0; i < all.size(); ++i) {
        sorter.enqueue(all[i], all[i].weight.to_double());
    }

    DSU dsu(n);
    while (!sorter.empty()) {
        // Как только набрано V-1 ребро, остов завершён (граф связен)
        if (n > 0 && res.edges.size() == n - 1) break;

        const GraphEdge e = sorter.dequeue().value();
        if (dsu.union_sets(e.from, e.to)) {          // ребро не создаёт цикл
            res.edges.push_back(e);
            res.total_weight += e.weight.to_double();
        }
    }

    res.connected = (n <= 1) || (res.edges.size() == n - 1);
    return res;
}