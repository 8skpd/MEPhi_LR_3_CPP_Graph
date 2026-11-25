#pragma once

#include "PathResult.h"
#include "../Graph.h"
#include "../Queue.h"

/*
Поиск в ширину (BFS).

Игнорирует веса рёбер: dist[v] = число рёбер от source.
Это кратчайший путь в невзвешенном понимании, и он совпадает
с результатом Дейкстры, если все веса равны 1.

Сложность: O(V + E).
*/

inline PathResult bfs(const Graph& g, size_t source) {
    const size_t n = g.vertex_count();
    if (source >= n) throw IndexOutOfRange(__func__);

    PathResult r(n, source, "BFS");

    DynamicArray<bool> visited;
    for (size_t i = 0; i < n; ++i) visited.push_back(false);

    Queue<size_t> q;
    visited[source] = true;
    r.dist[source] = Some<double>(0.0);
    r.reached = 1;
    q.enqueue(source);

    while (!q.empty()) {
        const size_t v = q.dequeue().value();
        r.order.push_back(v);

        const DynamicArray<Edge>& nb = g.neighbors(v);
        for (size_t i = 0; i < nb.size(); ++i) {
            const size_t u = nb[i].to;
            if (visited[u]) continue;
            visited[u] = true;
            r.dist[u]   = Some<double>(r.dist[v].value() + 1.0);
            r.parent[u] = Some<size_t>(v);
            ++r.reached;
            q.enqueue(u);
        }
    }
    return r;
}

// Только порядок обхода (для отображения/отладки)
inline DynamicArray<size_t> bfs_order(const Graph& g, size_t source) {
    return bfs(g, source).order;
}