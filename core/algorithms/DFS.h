#pragma once

#include <utility>
#include "PathResult.h"
#include "../Graph.h"

/*
Поиск в глубину (DFS) — итеративный, на явном стеке.

Рекурсивная реализация падает на глубоких графах (цепочка
из 100k вершин), поэтому используется собственный стек.
Соседи помещаются в обратном порядке — так порядок обхода
совпадает с классическим рекурсивным DFS.

dist[v] = глубина вершины в дереве обхода (НЕ кратчайшее
расстояние!). Для кратчайших путей используется BFS.

Сложность: O(V + E).
*/


namespace dfs_detail {

struct Frame {
    size_t         vertex = 0;
    Option<size_t> parent;
    size_t         depth = 0;

    Frame() = default;
    Frame(size_t v, Option<size_t> p, size_t d)
        : vertex(v), parent(std::move(p)), depth(d) {}
};

} // namespace dfs_detail

inline PathResult dfs(const Graph& g, size_t source) {
    const size_t n = g.vertex_count();
    if (source >= n) throw IndexOutOfRange(__func__);

    PathResult r(n, source, "DFS");

    DynamicArray<bool> visited;
    for (size_t i = 0; i < n; ++i) visited.push_back(false);

    DynamicArray<dfs_detail::Frame> stack;
    stack.push_back(dfs_detail::Frame(source, None<size_t>(), 0));

    while (stack.size() > 0) {
        const dfs_detail::Frame f = stack[stack.size() - 1];
        stack.pop_back();

        if (visited[f.vertex]) continue;   // ленивое удаление устаревших записей
        visited[f.vertex] = true;

        r.dist[f.vertex]   = Some<double>(static_cast<double>(f.depth));
        r.parent[f.vertex] = f.parent;
        r.order.push_back(f.vertex);
        ++r.reached;

        const DynamicArray<Edge>& nb = g.neighbors(f.vertex);
        for (size_t i = nb.size(); i > 0; --i) {
            const size_t u = nb[i - 1].to;
            if (!visited[u]) {
                stack.push_back(dfs_detail::Frame(u, Some<size_t>(f.vertex), f.depth + 1));
            }
        }
    }
    return r;
}

inline DynamicArray<size_t> dfs_order(const Graph& g, size_t source) {
    return dfs(g, source).order;
}

// Компоненты связности неориентированного графа (М-2.6).
// Каждая вершина получает идентификатор компоненты 0..count-1.

struct ComponentsResult {
    DynamicArray<size_t> component;   // component[v] — номер компоненты
    DynamicArray<size_t> sizes;       // sizes[c] — число вершин в компоненте
    size_t count = 0;
    double elapsed_ms = 0.0;
};

inline ComponentsResult connected_components(const Graph& g) {
    const size_t n = g.vertex_count();
    ComponentsResult res;

    DynamicArray<bool> visited;
    for (size_t i = 0; i < n; ++i) {
        visited.push_back(false);
        res.component.push_back(static_cast<size_t>(0));
    }

    DynamicArray<size_t> stack;

    for (size_t start = 0; start < n; ++start) {
        if (visited[start]) continue;

        const size_t comp_id = res.count++;
        res.sizes.push_back(static_cast<size_t>(0));

        visited[start] = true;
        stack.push_back(start);

        while (stack.size() > 0) {
            const size_t v = stack[stack.size() - 1];
            stack.pop_back();

            res.component[v] = comp_id;
            ++res.sizes[comp_id];

            const DynamicArray<Edge>& nb = g.neighbors(v);
            for (size_t i = 0; i < nb.size(); ++i) {
                const size_t u = nb[i].to;
                if (!visited[u]) {
                    visited[u] = true;
                    stack.push_back(u);
                }
            }
        }
    }
    return res;
}

inline bool is_connected(const Graph& g) {
    return connected_components(g).count <= 1;
}