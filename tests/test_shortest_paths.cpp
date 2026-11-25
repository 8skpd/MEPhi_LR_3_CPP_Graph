#include <iostream>
#include <random>
#include <vector>
#include <limits>
#include <algorithm>
#include "../core/algorithms/BFS.h"
#include "../core/algorithms/Dijkstra.h"
#include "../core/algorithms/BellmanFord.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

static bool approx(double a, double b) {
    const double d = a - b;
    return d < 1e-9 && d > -1e-9;
}

static std::vector<size_t> to_vec(const DynamicArray<size_t>& a) {
    std::vector<size_t> v;
    for (size_t i = 0; i < a.size(); ++i) v.push_back(a[i]);
    return v;
}

static Graph make_weighted_chain(size_t n, int w) {
    Graph g(n);
    for (size_t i = 0; i + 1 < n; ++i) g.add_edge(i, i + 1, Value(w));
    return g;
}

// Минимальный вес среди всех рёбер между двумя вершинами.
// Дейкстра и Форд-Беллман всегда выбирают оптимальное ребро,
// поэтому для проверки пути достаточно минимального веса.
static double min_edge_weight(const Graph& g, size_t from, size_t to) {
    const DynamicArray<Edge>& nb = g.neighbors(from);
    double best = std::numeric_limits<double>::infinity();
    bool found = false;
    for (size_t i = 0; i < nb.size(); ++i) {
        if (nb[i].to == to) {
            const double w = nb[i].weight.to_double();
            if (w < best) best = w;
            found = true;
        }
    }
    if (!found) throw EdgeNotFound(__func__);
    return best;
}

static double path_weight(const Graph& g, const std::vector<size_t>& p) {
    double s = 0.0;
    for (size_t i = 0; i + 1 < p.size(); ++i) {
        s += min_edge_weight(g, p[i], p[i + 1]);
    }
    return s;
}

// =========================================================
// Дейкстра: базовое поведение
// =========================================================

TEST(dijkstra_single_vertex) {
    Graph g(1);
    const PathResult r = dijkstra(g, 0);
    ASSERT(r.reached == 1);
    ASSERT(r.dist[0].value() == 0.0);
    ASSERT(!r.parent[0].has_value());
    ASSERT(r.method == "DIJKSTRA");
}

TEST(dijkstra_weighted_chain) {
    const Graph g = make_weighted_chain(6, 3);
    const PathResult r = dijkstra(g, 0);
    ASSERT(r.reached == 6);
    for (size_t i = 0; i < 6; ++i) {
        ASSERT(approx(r.dist[i].value(), 3.0 * static_cast<double>(i)));
    }
}

TEST(dijkstra_prefers_cheap_over_short) {
    // 0-1 напрямую стоит 10; через вершину 2 — 1+1=2
    Graph g(3);
    g.add_edge(0, 1, Value(10));
    g.add_edge(0, 2, Value(1));
    g.add_edge(2, 1, Value(1));

    const PathResult d = dijkstra(g, 0);
    ASSERT(approx(d.dist[1].value(), 2.0));
    ASSERT(to_vec(reconstruct_path(d, 1).value()) == std::vector<size_t>({0,2,1}));

    // BFS считает рёбра и выбирает прямой путь — принципиальная разница
    const PathResult b = bfs(g, 0);
    ASSERT(approx(b.dist[1].value(), 1.0));
}

TEST(dijkstra_double_weights) {
    Graph g(3);
    g.add_edge(0, 1, Value(0.5));
    g.add_edge(1, 2, Value(0.25));
    const PathResult r = dijkstra(g, 0);
    ASSERT(approx(r.dist[2].value(), 0.75));
}

TEST(dijkstra_mixed_int_double_weights) {
    Graph g(3);
    g.add_edge(0, 1, Value(2));       // int
    g.add_edge(1, 2, Value(0.5));     // double
    const PathResult r = dijkstra(g, 0);
    ASSERT(approx(r.dist[2].value(), 2.5));
}

TEST(dijkstra_zero_weights) {
    Graph g(4);
    g.add_edge(0, 1, Value(0));
    g.add_edge(1, 2, Value(0));
    g.add_edge(2, 3, Value(0));
    const PathResult r = dijkstra(g, 0);
    ASSERT(r.reached == 4);
    for (size_t i = 0; i < 4; ++i) ASSERT(approx(r.dist[i].value(), 0.0));
}

TEST(dijkstra_unreachable_is_none) {
    Graph g(4);
    g.add_edge(0, 1, Value(5));
    const PathResult r = dijkstra(g, 0);
    ASSERT(r.reached == 2);
    ASSERT(r.has_path(1));
    ASSERT(!r.has_path(2));
    ASSERT(!reconstruct_path(r, 3).has_value());
}

TEST(dijkstra_negative_weight_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value(-5));
    bool caught = false;
    try { (void)dijkstra(g, 0); } catch (const NegativeWeight&) { caught = true; }
    ASSERT(caught);
}

TEST(dijkstra_negative_weight_unreachable_also_throws) {
    // сознательное решение: валидация по всему графу
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(2, 3, Value(-1));
    bool caught = false;
    try { (void)dijkstra(g, 0); } catch (const NegativeWeight&) { caught = true; }
    ASSERT(caught);
}

TEST(dijkstra_non_numeric_weight_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value("дорого"));
    bool caught = false;
    try { (void)dijkstra(g, 0); } catch (const TypeMismatch&) { caught = true; }
    ASSERT(caught);
}

TEST(dijkstra_out_of_range_throws) {
    const Graph g = make_weighted_chain(3, 1);
    bool caught = false;
    try { (void)dijkstra(g, 10); } catch (const IndexOutOfRange&) { caught = true; }
    ASSERT(caught);
}

TEST(dijkstra_named_vertices) {
    Graph g;
    g.add_vertex("A"); g.add_vertex("B"); g.add_vertex("C");
    g.add_edge("A", "B", Value(4));
    g.add_edge("B", "C", Value(6));
    g.add_edge("A", "C", Value(11));
    const PathResult r = dijkstra(g, g.vertex_id("A"));
    ASSERT(approx(r.dist[g.vertex_id("C")].value(), 10.0));
}

TEST(dijkstra_large_chain) {
    const Graph g = make_weighted_chain(20000, 1);
    const PathResult r = dijkstra(g, 0);
    ASSERT(r.reached == 20000);
    ASSERT(approx(r.dist[19999].value(), 19999.0));
}

// =========================================================
// Форд-Беллман
// =========================================================

TEST(bf_single_vertex) {
    Graph g(1);
    const PathResult r = bellman_ford(g, 0);
    ASSERT(r.reached == 1);
    ASSERT(approx(r.dist[0].value(), 0.0));
    ASSERT(r.method == "BELLMAN-FORD");
}

TEST(bf_weighted_chain) {
    const Graph g = make_weighted_chain(6, 3);
    const PathResult r = bellman_ford(g, 0);
    for (size_t i = 0; i < 6; ++i) {
        ASSERT(approx(r.dist[i].value(), 3.0 * static_cast<double>(i)));
    }
}

TEST(bf_prefers_cheap_over_short) {
    Graph g(3);
    g.add_edge(0, 1, Value(10));
    g.add_edge(0, 2, Value(1));
    g.add_edge(2, 1, Value(1));
    const PathResult r = bellman_ford(g, 0);
    ASSERT(approx(r.dist[1].value(), 2.0));
}

TEST(bf_unreachable_is_none) {
    Graph g(4);
    g.add_edge(0, 1, Value(5));
    const PathResult r = bellman_ford(g, 0);
    ASSERT(r.reached == 2);
    ASSERT(!r.has_path(2));
}

TEST(bf_negative_edge_is_negative_cycle_in_undirected) {
    // В неориентированном графе ребро веса -5 даёт цикл u->v->u веса -10
    Graph g(3);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(-5));
    bool caught = false;
    try { (void)bellman_ford(g, 0); } catch (const NegativeCycle&) { caught = true; }
    ASSERT(caught);
}

TEST(bf_negative_edge_unreachable_is_ok) {
    Graph g(4);
    g.add_edge(0, 1, Value(2));
    g.add_edge(2, 3, Value(-5));   // недостижимая компонента
    const PathResult r = bellman_ford(g, 0);
    ASSERT(r.reached == 2);
    ASSERT(approx(r.dist[1].value(), 2.0));
    ASSERT(!r.has_path(2));
}

TEST(bf_zero_weight_cycle_is_ok) {
    Graph g(3);
    g.add_edge(0, 1, Value(0));
    g.add_edge(1, 2, Value(0));
    g.add_edge(0, 2, Value(0));
    const PathResult r = bellman_ford(g, 0);
    ASSERT(r.reached == 3);
    for (size_t i = 0; i < 3; ++i) ASSERT(approx(r.dist[i].value(), 0.0));
}

TEST(bf_out_of_range_throws) {
    const Graph g = make_weighted_chain(2, 1);
    bool caught = false;
    try { (void)bellman_ford(g, 7); } catch (const IndexOutOfRange&) { caught = true; }
    ASSERT(caught);
}

TEST(bf_non_numeric_weight_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value("abc"));
    bool caught = false;
    try { (void)bellman_ford(g, 0); } catch (const TypeMismatch&) { caught = true; }
    ASSERT(caught);
}

// =========================================================
// Дифференциальные тесты: три алгоритма на одних данных
// =========================================================

TEST(dijkstra_matches_bellman_ford_random) {
    std::mt19937 rng(2026);
    const size_t n = 120;
    Graph g(n);
    for (int i = 0; i < 500; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a == b) continue;
        g.add_edge(a, b, Value(static_cast<int>(rng() % 50) + 1));
    }

    for (size_t src = 0; src < n; src += 17) {
        const PathResult d = dijkstra(g, src);
        const PathResult f = bellman_ford(g, src);
        ASSERT(d.reached == f.reached);
        for (size_t v = 0; v < n; ++v) {
            ASSERT(d.has_path(v) == f.has_path(v));
            if (d.has_path(v)) ASSERT(approx(d.dist[v].value(), f.dist[v].value()));
        }
    }
}

TEST(dijkstra_matches_bfs_on_unit_weights) {
    std::mt19937 rng(777);
    const size_t n = 150;
    Graph g(n);
    for (int i = 0; i < 450; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a != b) g.add_edge(a, b, Value(1));
    }
    const PathResult d = dijkstra(g, 0);
    const PathResult b = bfs(g, 0);
    ASSERT(d.reached == b.reached);
    for (size_t v = 0; v < n; ++v) {
        ASSERT(d.has_path(v) == b.has_path(v));
        if (d.has_path(v)) ASSERT(approx(d.dist[v].value(), b.dist[v].value()));
    }
}

TEST(dijkstra_paths_are_valid_and_weights_match) {
    std::mt19937 rng(4242);
    const size_t n = 100;
    Graph g(n);
    for (int i = 0; i < 400; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a == b) continue;
        g.add_edge(a, b, Value(static_cast<int>(rng() % 30) + 1));
    }

    const PathResult r = dijkstra(g, 0);
    for (size_t v = 0; v < n; ++v) {
        if (!r.has_path(v)) continue;
        const auto p = reconstruct_path(r, v);
        ASSERT(p.has_value());
        const std::vector<size_t> path = to_vec(p.value());
        ASSERT(path.front() == 0);
        ASSERT(path.back() == v);
        for (size_t i = 0; i + 1 < path.size(); ++i) {
            ASSERT(g.has_edge(path[i], path[i + 1]));
        }
        // сумма весов вдоль пути равна заявленному расстоянию
        ASSERT(approx(path_weight(g, path), r.dist[v].value()));
    }
}

TEST(bf_paths_are_valid_and_weights_match) {
    std::mt19937 rng(31337);
    const size_t n = 80;
    Graph g(n);
    for (int i = 0; i < 250; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a == b) continue;
        g.add_edge(a, b, Value(static_cast<int>(rng() % 20) + 1));
    }
    const PathResult r = bellman_ford(g, 0);
    for (size_t v = 0; v < n; ++v) {
        if (!r.has_path(v)) continue;
        const std::vector<size_t> path = to_vec(reconstruct_path(r, v).value());
        ASSERT(path.front() == 0);
        ASSERT(path.back() == v);
        ASSERT(approx(path_weight(g, path), r.dist[v].value()));
    }
}

TEST(dijkstra_triangle_inequality_on_edges) {
    // Для любого ребра (u,v): dist[u] <= dist[v] + w(u,v)
    std::mt19937 rng(555);
    const size_t n = 90;
    Graph g(n);
    for (int i = 0; i < 350; ++i) {
        const size_t a = rng() % n, b = rng() % n;
        if (a == b) continue;
        g.add_edge(a, b, Value(static_cast<int>(rng() % 40) + 1));
    }
    const PathResult r = dijkstra(g, 0);
    for (size_t v = 0; v < n; ++v) {
        if (!r.has_path(v)) continue;
        const auto& nb = g.neighbors(v);
        for (size_t i = 0; i < nb.size(); ++i) {
            const size_t u = nb[i].to;
            ASSERT(r.has_path(u));
            const double w = nb[i].weight.to_double();
            ASSERT(r.dist[u].value() <= r.dist[v].value() + w + 1e-9);
        }
    }
}

TEST(graph_edges_lists_each_edge_once) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(2));
    g.add_edge(2, 3, Value(3));
    const auto es = g.edges();
    ASSERT(es.size() == g.edge_count());
    ASSERT(es.size() == 3);
    for (size_t i = 0; i < es.size(); ++i) ASSERT(es[i].from < es[i].to);
}

int main() {
    std::cout << "=== Тесты кратчайших путей (Дейкстра, Форд-Беллман) ===\n" << std::endl;

    RUN_TEST(dijkstra_single_vertex);
    RUN_TEST(dijkstra_weighted_chain);
    RUN_TEST(dijkstra_prefers_cheap_over_short);
    RUN_TEST(dijkstra_double_weights);
    RUN_TEST(dijkstra_mixed_int_double_weights);
    RUN_TEST(dijkstra_zero_weights);
    RUN_TEST(dijkstra_unreachable_is_none);
    RUN_TEST(dijkstra_negative_weight_throws);
    RUN_TEST(dijkstra_negative_weight_unreachable_also_throws);
    RUN_TEST(dijkstra_non_numeric_weight_throws);
    RUN_TEST(dijkstra_out_of_range_throws);
    RUN_TEST(dijkstra_named_vertices);
    RUN_TEST(dijkstra_large_chain);

    RUN_TEST(bf_single_vertex);
    RUN_TEST(bf_weighted_chain);
    RUN_TEST(bf_prefers_cheap_over_short);
    RUN_TEST(bf_unreachable_is_none);
    RUN_TEST(bf_negative_edge_is_negative_cycle_in_undirected);
    RUN_TEST(bf_negative_edge_unreachable_is_ok);
    RUN_TEST(bf_zero_weight_cycle_is_ok);
    RUN_TEST(bf_out_of_range_throws);
    RUN_TEST(bf_non_numeric_weight_throws);

    RUN_TEST(dijkstra_matches_bellman_ford_random);
    RUN_TEST(dijkstra_matches_bfs_on_unit_weights);
    RUN_TEST(dijkstra_paths_are_valid_and_weights_match);
    RUN_TEST(bf_paths_are_valid_and_weights_match);
    RUN_TEST(dijkstra_triangle_inequality_on_edges);
    RUN_TEST(graph_edges_lists_each_edge_once);

    std::cout << "\n=== Все тесты кратчайших путей пройдены ===\n";
    return 0;
}