#pragma once

#include "PathResult.h"
#include "../Graph.h"

/*
Алгоритм Форда-Беллмана: кратчайшие пути от одной вершины,
допускает отрицательные веса и обнаруживает отрицательные циклы.

Сложность: O(V * E). Реализована ранняя остановка: если за
очередной проход ни одно расстояние не улучшилось, алгоритм
завершается — дальнейшие проходы ничего не изменят.

ВАЖНО про неориентированный граф: ребро (u,v) веса w < 0
образует цикл u->v->u веса 2w < 0. Поэтому любое отрицательное
ребро, достижимое из source, — это отрицательный цикл, и
алгоритм сообщает о нём. Отрицательные рёбра в недостижимой
компоненте на результат не влияют и допускаются.

Каждое ребро просматривается в обоих направлениях, так как
граф неориентированный.
*/


inline PathResult bellman_ford(const Graph& g, size_t source) {
    const size_t n = g.vertex_count();
    if (source >= n) throw IndexOutOfRange(__func__);

    const DynamicArray<GraphEdge> es = g.edges();
    for (size_t i = 0; i < es.size(); ++i) {
        if (!es[i].weight.is_numeric()) {
            throw TypeMismatch(__func__, "вес ребра не является числом");
        }
    }

    PathResult r(n, source, "BELLMAN-FORD");
    r.dist[source] = Some<double>(0.0);
    r.order.push_back(source);
    r.reached = 1;

    // Ослабление ребра from -> to. true, если расстояние улучшено.
    auto relax = [&](size_t from, size_t to, double w) -> bool {
        if (!r.dist[from].has_value()) return false;
        const double candidate = r.dist[from].value() + w;

        if (!r.dist[to].has_value()) {
            r.dist[to]   = Some<double>(candidate);
            r.parent[to] = Some<size_t>(from);
            r.order.push_back(to);           // порядок первого достижения
            ++r.reached;
            return true;
        }
        if (candidate < r.dist[to].value()) {
            r.dist[to]   = Some<double>(candidate);
            r.parent[to] = Some<size_t>(from);
            return true;
        }
        return false;
    };

    // не более V-1 проходов по всем рёбрам
    for (size_t round = 0; round + 1 < n; ++round) {
        bool changed = false;
        for (size_t i = 0; i < es.size(); ++i) {
            const GraphEdge& e = es[i];
            const double w = e.weight.to_double();
            const bool a = relax(e.from, e.to, w);
            const bool b = relax(e.to, e.from, w);
            if (a || b) changed = true;
        }
        if (!changed) break;                 // ранняя остановка
    }

    // контроль: если ещё что-то улучшается, есть отрицательный цикл
    for (size_t i = 0; i < es.size(); ++i) {
        const GraphEdge& e = es[i];
        const double w = e.weight.to_double();
        if (r.dist[e.from].has_value() &&
            r.dist[e.from].value() + w < r.dist[e.to].value_or(0.0) &&
            r.dist[e.to].has_value()) {
            throw NegativeCycle(__func__,
                "обнаружен отрицательный цикл, достижимый из источника");
        }
        if (r.dist[e.to].has_value() &&
            r.dist[e.to].value() + w < r.dist[e.from].value_or(0.0) &&
            r.dist[e.from].has_value()) {
            throw NegativeCycle(__func__,
                "обнаружен отрицательный цикл, достижимый из источника");
        }
        // отрицательная петля
        if (e.from == e.to && r.dist[e.from].has_value() && w < 0.0) {
            throw NegativeCycle(__func__, "отрицательная петля");
        }
    }

    return r;
}