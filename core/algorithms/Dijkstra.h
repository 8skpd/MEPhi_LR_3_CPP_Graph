#pragma once

#include "PathResult.h"
#include "../Graph.h"
#include "../PriorityQueue.h"

/*
Алгоритм Дейкстры: кратчайшие пути от одной вершины
в графе с неотрицательными весами.

Реализация с «ленивым удалением»: наша PriorityQueue не
поддерживает decrease_key, поэтому при улучшении расстояния
в очередь добавляется новая запись, а устаревшие отсеиваются
при извлечении по флагу finalized.

Сложность: O((V + E) log V).

Веса приводятся к double через Value::to_double(), что
удовлетворяет принципу общности (int и double веса).

Валидация весов выполняется по ВСЕМУ графу до начала работы:
отрицательный или нечисловой вес -> исключение, даже если он
лежит в недостижимой компоненте. Это делает поведение
предсказуемым: результат либо корректен, либо алгоритм
отказывается работать.
*/


inline PathResult dijkstra(const Graph& g, size_t source) {
    const size_t n = g.vertex_count();
    if (source >= n) throw IndexOutOfRange(__func__);

    // предусловие: все веса числовые и неотрицательные
    for (size_t v = 0; v < n; ++v) {
        const DynamicArray<Edge>& nb = g.neighbors(v);
        for (size_t i = 0; i < nb.size(); ++i) {
            if (!nb[i].weight.is_numeric()) {
                throw TypeMismatch(__func__, "вес ребра не является числом");
            }
            if (nb[i].weight.to_double() < 0.0) {
                throw NegativeWeight(__func__,
                    "алгоритм Дейкстры не допускает отрицательных весов");
            }
        }
    }

    PathResult r(n, source, "DIJKSTRA");

    DynamicArray<bool> finalized;
    for (size_t i = 0; i < n; ++i) finalized.push_back(false);

    PriorityQueue<size_t, double> pq;
    r.dist[source] = Some<double>(0.0);
    pq.enqueue(source, 0.0);

    while (!pq.empty()) {
        const size_t v = pq.dequeue().value();
        if (finalized[v]) continue;          // устаревшая запись очереди
        finalized[v] = true;

        r.order.push_back(v);                // порядок финализации — для UI
        ++r.reached;

        const double dv = r.dist[v].value();
        const DynamicArray<Edge>& nb = g.neighbors(v);
        for (size_t i = 0; i < nb.size(); ++i) {
            const size_t u = nb[i].to;
            if (finalized[u]) continue;

            const double candidate = dv + nb[i].weight.to_double();
            if (!r.dist[u].has_value() || candidate < r.dist[u].value()) {
                r.dist[u]   = Some<double>(candidate);
                r.parent[u] = Some<size_t>(v);
                pq.enqueue(u, candidate);    // ленивое обновление приоритета
            }
        }
    }
    return r;
}