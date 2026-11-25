#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include "../core/Queue.h"
#include "../core/DSU.h"
#include "../core/algorithms/BFS.h"
#include "../core/algorithms/DFS.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

static std::vector<size_t> to_vec(const DynamicArray<size_t>& a) {
    std::vector<size_t> v;
    for (size_t i = 0; i < a.size(); ++i) v.push_back(a[i]);
    return v;
}

// Цепочка 0-1-2-...-(n-1)
static Graph make_chain(size_t n) {
    Graph g(n);
    for (size_t i = 0; i + 1 < n; ++i) g.add_edge(i, i + 1, Value(1));
    return g;
}

// =========================================================
// Queue
// =========================================================

TEST(queue_fifo_order) {
    Queue<int> q;
    ASSERT(q.empty());
    ASSERT(!q.dequeue().has_value());

    for (int i = 0; i < 5; ++i) q.enqueue(i);
    ASSERT(q.size() == 5);
    ASSERT(q.front().value() == 0);

    for (int i = 0; i < 5; ++i) ASSERT(q.dequeue().value() == i);
    ASSERT(q.empty());
}

TEST(queue_wraparound_and_growth) {
    Queue<int> q;
    // Много циклов enqueue/dequeue — проверяем кольцевой буфер и рост
    int next_in = 0, next_out = 0;
    for (int round = 0; round < 100; ++round) {
        for (int k = 0; k < 7; ++k) q.enqueue(next_in++);
        for (int k = 0; k < 5; ++k) ASSERT(q.dequeue().value() == next_out++);
    }
    while (!q.empty()) ASSERT(q.dequeue().value() == next_out++);
    ASSERT(next_in == next_out);
}

// =========================================================
// BFS
// =========================================================

TEST(bfs_single_vertex) {
    Graph g(1);
    const PathResult r = bfs(g, 0);
    ASSERT(r.reached == 1);
    ASSERT(r.dist[0].value() == 0.0);
    ASSERT(!r.parent[0].has_value());
    ASSERT(to_vec(r.order) == std::vector<size_t>({0}));
}

TEST(bfs_chain_distances) {
    const Graph g = make_chain(6);
    const PathResult r = bfs(g, 0);
    ASSERT(r.reached == 6);
    for (size_t i = 0; i < 6; ++i) ASSERT(r.dist[i].value() == static_cast<double>(i));
    ASSERT(to_vec(r.order) == std::vector<size_t>({0,1,2,3,4,5}));
}

TEST(bfs_from_middle) {
    const Graph g = make_chain(7);
    const PathResult r = bfs(g, 3);
    const double want[] = {3, 2, 1, 0, 1, 2, 3};
    for (size_t i = 0; i < 7; ++i) ASSERT(r.dist[i].value() == want[i]);
}

TEST(bfs_ignores_weights) {
    Graph g(3);
    g.add_edge(0, 1, Value(1000));
    g.add_edge(1, 2, Value(0.001));
    const PathResult r = bfs(g, 0);
    // BFS считает рёбра, а не веса
    ASSERT(r.dist[2].value() == 2.0);
}

TEST(bfs_unreachable_is_none) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    // вершины 2 и 3 изолированы
    const PathResult r = bfs(g, 0);
    ASSERT(r.reached == 2);
    ASSERT(r.has_path(1));
    ASSERT(!r.has_path(2));
    ASSERT(!r.has_path(3));
    ASSERT(!reconstruct_path(r, 2).has_value());
}

TEST(bfs_path_reconstruction) {
    const Graph g = make_chain(5);
    const PathResult r = bfs(g, 0);
    const auto path = reconstruct_path(r, 4);
    ASSERT(path.has_value());
    ASSERT(to_vec(path.value()) == std::vector<size_t>({0,1,2,3,4}));

    const auto self = reconstruct_path(r, 0);
    ASSERT(to_vec(self.value()) == std::vector<size_t>({0}));
}

TEST(bfs_shortest_path_in_grid_like_graph) {
    // 0-1-2-3 и срезка 0-3: кратчайший путь 0->3 = 1 ребро
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(1));
    g.add_edge(2, 3, Value(1));
    g.add_edge(0, 3, Value(1));
    const PathResult r = bfs(g, 0);
    ASSERT(r.dist[3].value() == 1.0);
    ASSERT(to_vec(reconstruct_path(r, 3).value()) == std::vector<size_t>({0,3}));
}

TEST(bfs_out_of_range_throws) {
    const Graph g = make_chain(3);
    bool caught = false;
    try { (void)bfs(g, 10); } catch (const IndexOutOfRange&) { caught = true; }
    ASSERT(caught);
}

TEST(bfs_named_vertices) {
    Graph g;
    g.add_vertex("A"); g.add_vertex("B"); g.add_vertex("C");
    g.add_edge("A", "B", Value(1));
    g.add_edge("B", "C", Value(1));
    const PathResult r = bfs(g, g.vertex_id("A"));
    ASSERT(r.dist[g.vertex_id("C")].value() == 2.0);
}

// =========================================================
// DFS
// =========================================================

TEST(dfs_single_vertex) {
    Graph g(1);
    const PathResult r = dfs(g, 0);
    ASSERT(r.reached == 1);
    ASSERT(r.dist[0].value() == 0.0);
    ASSERT(to_vec(r.order) == std::vector<size_t>({0}));
}

TEST(dfs_chain_order_and_depth) {
    const Graph g = make_chain(6);
    const PathResult r = dfs(g, 0);
    ASSERT(to_vec(r.order) == std::vector<size_t>({0,1,2,3,4,5}));
    for (size_t i = 0; i < 6; ++i) ASSERT(r.dist[i].value() == static_cast<double>(i));
}

TEST(dfs_goes_deep_not_wide) {
    // 0 соединён с 1 и 2; 1 ведёт в длинную ветку
    Graph g(5);
    g.add_edge(0, 1, Value(1));
    g.add_edge(0, 2, Value(1));
    g.add_edge(1, 3, Value(1));
    g.add_edge(3, 4, Value(1));

    const PathResult d = dfs(g, 0);
    const PathResult b = bfs(g, 0);

    // DFS уходит вглубь по первому соседу до конца
    ASSERT(to_vec(d.order) == std::vector<size_t>({0,1,3,4,2}));
    // BFS обходит по уровням
    ASSERT(to_vec(b.order) == std::vector<size_t>({0,1,2,3,4}));
}

TEST(dfs_parent_tree_is_valid) {
    const Graph g = make_chain(10);
    const PathResult r = dfs(g, 0);
    for (size_t v = 1; v < 10; ++v) {
        ASSERT(r.parent[v].has_value());
        ASSERT(g.has_edge(r.parent[v].value(), v));
    }
    ASSERT(!r.parent[0].has_value());
}

TEST(dfs_deep_chain_no_stack_overflow) {
    // 50000 вершин: рекурсивный DFS здесь упал бы
    const Graph g = make_chain(50000);
    const PathResult r = dfs(g, 0);
    ASSERT(r.reached == 50000);
    ASSERT(r.dist[49999].value() == 49999.0);
}

TEST(dfs_unreachable_is_none) {
    Graph g(3);
    g.add_edge(0, 1, Value(1));
    const PathResult r = dfs(g, 0);
    ASSERT(r.reached == 2);
    ASSERT(!r.has_path(2));
}

TEST(dfs_out_of_range_throws) {
    const Graph g = make_chain(2);
    bool caught = false;
    try { (void)dfs(g, 5); } catch (const IndexOutOfRange&) { caught = true; }
    ASSERT(caught);
}

TEST(bfs_and_dfs_reach_same_set) {
    std::mt19937 rng(2026);
    Graph g(200);
    for (int i = 0; i < 400; ++i) {
        const size_t a = rng() % 200, b = rng() % 200;
        if (a != b) g.add_edge(a, b, Value(1));
    }
    const PathResult b = bfs(g, 0);
    const PathResult d = dfs(g, 0);
    ASSERT(b.reached == d.reached);
    for (size_t v = 0; v < 200; ++v) ASSERT(b.has_path(v) == d.has_path(v));
}

TEST(bfs_distance_property_on_edges) {
    // Для любого ребра (u,v) между достижимыми: |dist[u]-dist[v]| <= 1
    std::mt19937 rng(777);
    Graph g(150);
    for (int i = 0; i < 500; ++i) {
        const size_t a = rng() % 150, b = rng() % 150;
        if (a != b) g.add_edge(a, b, Value(1));
    }
    const PathResult r = bfs(g, 0);
    for (size_t v = 0; v < 150; ++v) {
        if (!r.has_path(v)) continue;
        const auto& nb = g.neighbors(v);
        for (size_t i = 0; i < nb.size(); ++i) {
            const size_t u = nb[i].to;
            ASSERT(r.has_path(u));
            const double diff = r.dist[v].value() - r.dist[u].value();
            ASSERT(diff <= 1.0 && diff >= -1.0);
        }
    }
}

TEST(bfs_paths_are_valid_and_minimal) {
    std::mt19937 rng(4242);
    Graph g(120);
    for (int i = 0; i < 350; ++i) {
        const size_t a = rng() % 120, b = rng() % 120;
        if (a != b) g.add_edge(a, b, Value(1));
    }
    const PathResult r = bfs(g, 0);
    for (size_t v = 0; v < 120; ++v) {
        if (!r.has_path(v)) continue;
        const auto p = reconstruct_path(r, v);
        ASSERT(p.has_value());
        const std::vector<size_t> path = to_vec(p.value());
        ASSERT(path.front() == 0);
        ASSERT(path.back() == v);
        // длина пути в рёбрах равна расстоянию
        ASSERT(static_cast<double>(path.size() - 1) == r.dist[v].value());
        // все переходы — по существующим рёбрам
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            ASSERT(g.has_edge(path[i], path[i + 1]));
        }
    }
}

// =========================================================
// Компоненты связности (М-2.6)
// =========================================================

TEST(components_empty_graph) {
    Graph g;
    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 0);
    ASSERT(is_connected(g));   // пустой граф считаем связным
}

TEST(components_single_vertex) {
    Graph g(1);
    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 1);
    ASSERT(c.sizes[0] == 1);
    ASSERT(c.component[0] == 0);
}

TEST(components_all_isolated) {
    Graph g(5);
    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 5);
    for (size_t i = 0; i < 5; ++i) ASSERT(c.sizes[i] == 1);
    ASSERT(!is_connected(g));
}

TEST(components_chain_is_one) {
    const Graph g = make_chain(100);
    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 1);
    ASSERT(c.sizes[0] == 100);
    ASSERT(is_connected(g));
}

TEST(components_two_triangles) {
    Graph g(6);
    g.add_edge(0, 1, Value(1)); g.add_edge(1, 2, Value(1)); g.add_edge(0, 2, Value(1));
    g.add_edge(3, 4, Value(1)); g.add_edge(4, 5, Value(1)); g.add_edge(3, 5, Value(1));

    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 2);
    ASSERT(c.sizes[0] == 3);
    ASSERT(c.sizes[1] == 3);
    ASSERT(c.component[0] == c.component[2]);
    ASSERT(c.component[3] == c.component[5]);
    ASSERT(c.component[0] != c.component[3]);
}

TEST(components_bridge_merges) {
    Graph g(6);
    g.add_edge(0, 1, Value(1)); g.add_edge(1, 2, Value(1));
    g.add_edge(3, 4, Value(1)); g.add_edge(4, 5, Value(1));
    ASSERT(connected_components(g).count == 2);

    g.add_edge(2, 3, Value(1));   // мост
    const ComponentsResult c = connected_components(g);
    ASSERT(c.count == 1);
    ASSERT(c.sizes[0] == 6);
}

TEST(components_match_dsu_differential) {
    // Эталон: строим DSU по всем рёбрам и сравниваем разбиение
    std::mt19937 rng(31337);
    const size_t n = 300;
    Graph g(n);
    DSU dsu(n);

    for (int i = 0; i < 250; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a == b) continue;
        g.add_edge(a, b, Value(1));
        dsu.union_sets(a, b);
    }

    const ComponentsResult c = connected_components(g);
    for (size_t a = 0; a < n; ++a) {
        for (size_t b = 0; b < n; ++b) {
            ASSERT((c.component[a] == c.component[b]) == dsu.connected(a, b));
        }
    }

    // число компонент = число различных представителей в DSU
    std::vector<size_t> reps;
    for (size_t v = 0; v < n; ++v) reps.push_back(dsu.find(v));
    std::sort(reps.begin(), reps.end());
    reps.erase(std::unique(reps.begin(), reps.end()), reps.end());
    ASSERT(c.count == reps.size());
}

TEST(components_sizes_sum_to_vertex_count) {
    std::mt19937 rng(99);
    Graph g(500);
    for (int i = 0; i < 300; ++i) {
        const size_t a = rng() % 500, b = rng() % 500;
        if (a != b) g.add_edge(a, b, Value(1));
    }
    const ComponentsResult c = connected_components(g);
    size_t total = 0;
    for (size_t i = 0; i < c.sizes.size(); ++i) total += c.sizes[i];
    ASSERT(total == 500);
    ASSERT(c.sizes.size() == c.count);
}

int main() {
    std::cout << "=== Тесты Queue, BFS, DFS и компонент связности ===\n" << std::endl;

    RUN_TEST(queue_fifo_order);
    RUN_TEST(queue_wraparound_and_growth);

    RUN_TEST(bfs_single_vertex);
    RUN_TEST(bfs_chain_distances);
    RUN_TEST(bfs_from_middle);
    RUN_TEST(bfs_ignores_weights);
    RUN_TEST(bfs_unreachable_is_none);
    RUN_TEST(bfs_path_reconstruction);
    RUN_TEST(bfs_shortest_path_in_grid_like_graph);
    RUN_TEST(bfs_out_of_range_throws);
    RUN_TEST(bfs_named_vertices);

    RUN_TEST(dfs_single_vertex);
    RUN_TEST(dfs_chain_order_and_depth);
    RUN_TEST(dfs_goes_deep_not_wide);
    RUN_TEST(dfs_parent_tree_is_valid);
    RUN_TEST(dfs_deep_chain_no_stack_overflow);
    RUN_TEST(dfs_unreachable_is_none);
    RUN_TEST(dfs_out_of_range_throws);

    RUN_TEST(bfs_and_dfs_reach_same_set);
    RUN_TEST(bfs_distance_property_on_edges);
    RUN_TEST(bfs_paths_are_valid_and_minimal);

    RUN_TEST(components_empty_graph);
    RUN_TEST(components_single_vertex);
    RUN_TEST(components_all_isolated);
    RUN_TEST(components_chain_is_one);
    RUN_TEST(components_two_triangles);
    RUN_TEST(components_bridge_merges);
    RUN_TEST(components_match_dsu_differential);
    RUN_TEST(components_sizes_sum_to_vertex_count);

    std::cout << "\n=== Все тесты обходов и компонент пройдены ===\n";
    return 0;
}