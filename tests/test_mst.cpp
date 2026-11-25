#include <iostream>
#include <random>
#include <vector>
#include <algorithm>
#include "../core/algorithms/Kruskal.h"
#include "../core/algorithms/Prim.h"
#include "../core/DSU.h"

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

// Связный случайный граф: цепочка (гарантия связности) + случайные рёбра
static Graph make_connected_random(size_t n, size_t extra, std::mt19937& rng) {
    Graph g(n);
    for (size_t i = 0; i + 1 < n; ++i) {
        g.add_edge(i, i + 1, Value(static_cast<int>(rng() % 50) + 1));
    }
    for (size_t k = 0; k < extra; ++k) {
        const size_t a = rng() % n, b = rng() % n;
        if (a != b) g.add_edge(a, b, Value(static_cast<int>(rng() % 50) + 1));
    }
    return g;
}

// Проверка, что рёбра результата образуют корректный остовный лес:
// все рёбра существуют в графе и не образуют циклов.
static bool spanning_forest_is_valid(const Graph& g, const MstResult& r) {
    const size_t n = g.vertex_count();
    DSU dsu(n);
    size_t unions = 0;
    for (size_t i = 0; i < r.edges.size(); ++i) {
        const GraphEdge& e = r.edges[i];
        if (!g.has_edge(e.from, e.to)) return false;   // ребра нет в графе
        if (!dsu.union_sets(e.from, e.to)) return false; // образовали цикл
        ++unions;
    }
    return r.component_count() == n - unions;
}

// =========================================================
// Крускал
// =========================================================

TEST(kruskal_empty_graph) {
    Graph g;
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 0);
    ASSERT(r.connected);
    ASSERT(approx(r.total_weight, 0.0));
}

TEST(kruskal_single_vertex) {
    Graph g(1);
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 0);
    ASSERT(r.connected);
    ASSERT(r.component_count() == 1);
}

TEST(kruskal_triangle_picks_two_cheapest) {
    Graph g(3);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(2));
    g.add_edge(0, 2, Value(3));
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(approx(r.total_weight, 3.0));   // 1 + 2
    ASSERT(r.connected);
}

TEST(kruskal_disconnected_gives_forest) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(2, 3, Value(2));
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(!r.connected);
    ASSERT(r.component_count() == 2);
    ASSERT(approx(r.total_weight, 3.0));
}

TEST(kruskal_negative_weights_allowed) {
    Graph g(3);
    g.add_edge(0, 1, Value(-5));
    g.add_edge(1, 2, Value(3));
    g.add_edge(0, 2, Value(1));
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(approx(r.total_weight, -4.0));  // -5 + 1
}

TEST(kruskal_parallel_edges_uses_cheapest) {
    Graph g(2);
    g.add_edge(0, 1, Value(10));
    g.add_edge(0, 1, Value(3));
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 1);
    ASSERT(approx(r.total_weight, 3.0));
}

TEST(kruskal_self_loop_ignored) {
    Graph g(2);
    g.add_edge(0, 0, Value(1));
    g.add_edge(0, 1, Value(5));
    const MstResult r = kruskal(g);
    ASSERT(r.edge_count() == 1);
    ASSERT(approx(r.total_weight, 5.0));
}

TEST(kruskal_non_numeric_weight_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value("вес"));
    bool caught = false;
    try { (void)kruskal(g); } catch (const TypeMismatch&) { caught = true; }
    ASSERT(caught);
}

// =========================================================
// Прим
// =========================================================

TEST(prim_empty_graph) {
    Graph g;
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 0);
    ASSERT(r.connected);
}

TEST(prim_single_vertex) {
    Graph g(1);
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 0);
    ASSERT(r.connected);
}

TEST(prim_triangle_picks_two_cheapest) {
    Graph g(3);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(2));
    g.add_edge(0, 2, Value(3));
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(approx(r.total_weight, 3.0));
    ASSERT(r.connected);
}

TEST(prim_disconnected_gives_forest) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(2, 3, Value(2));
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(!r.connected);
    ASSERT(r.component_count() == 2);
    ASSERT(approx(r.total_weight, 3.0));
}

TEST(prim_negative_weights_allowed) {
    Graph g(3);
    g.add_edge(0, 1, Value(-5));
    g.add_edge(1, 2, Value(3));
    g.add_edge(0, 2, Value(1));
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 2);
    ASSERT(approx(r.total_weight, -4.0));
}

TEST(prim_parallel_edges_uses_cheapest) {
    Graph g(2);
    g.add_edge(0, 1, Value(10));
    g.add_edge(0, 1, Value(3));
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 1);
    ASSERT(approx(r.total_weight, 3.0));
}

TEST(prim_self_loop_ignored) {
    Graph g(2);
    g.add_edge(0, 0, Value(1));
    g.add_edge(0, 1, Value(5));
    const MstResult r = prim(g);
    ASSERT(r.edge_count() == 1);
    ASSERT(approx(r.total_weight, 5.0));
}

TEST(prim_non_numeric_weight_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value("вес"));
    bool caught = false;
    try { (void)prim(g); } catch (const TypeMismatch&) { caught = true; }
    ASSERT(caught);
}

// =========================================================
// Дифференциальные тесты: Крускал против Прима
// =========================================================

TEST(kruskal_vs_prim_connected_random) {
    for (unsigned seed = 1; seed <= 20; ++seed) {
        std::mt19937 rng(seed);
        const Graph g = make_connected_random(120, 350, rng);

        const MstResult k = kruskal(g);
        const MstResult p = prim(g);

        ASSERT(k.connected);
        ASSERT(p.connected);
        ASSERT(k.edge_count() == g.vertex_count() - 1);
        ASSERT(p.edge_count() == g.vertex_count() - 1);
        ASSERT(approx(k.total_weight, p.total_weight));
        ASSERT(spanning_forest_is_valid(g, k));
        ASSERT(spanning_forest_is_valid(g, p));
    }
}

TEST(kruskal_vs_prim_disconnected_random) {
    for (unsigned seed = 1; seed <= 15; ++seed) {
        std::mt19937 rng(seed + 1000);
        Graph g(80);   // без цепочки -> почти наверняка несвязный
        for (int i = 0; i < 60; ++i) {
            const size_t a = rng() % 80, b = rng() % 80;
            if (a != b) g.add_edge(a, b, Value(static_cast<int>(rng() % 40) + 1));
        }

        const MstResult k = kruskal(g);
        const MstResult p = prim(g);

        ASSERT(k.edge_count() == p.edge_count());
        ASSERT(k.component_count() == p.component_count());
        ASSERT(approx(k.total_weight, p.total_weight));
        ASSERT(spanning_forest_is_valid(g, k));
        ASSERT(spanning_forest_is_valid(g, p));
    }
}

TEST(kruskal_vs_prim_with_negative_weights) {
    std::mt19937 rng(2026);
    Graph g(60);
    for (size_t i = 0; i + 1 < 60; ++i) {
        // веса в диапазоне [-25, 25]
        g.add_edge(i, i + 1, Value(static_cast<int>(rng() % 51) - 25));
    }
    for (int i = 0; i < 100; ++i) {
        const size_t a = rng() % 60, b = rng() % 60;
        if (a != b) g.add_edge(a, b, Value(static_cast<int>(rng() % 51) - 25));
    }

    const MstResult k = kruskal(g);
    const MstResult p = prim(g);
    ASSERT(approx(k.total_weight, p.total_weight));
    ASSERT(k.edge_count() == 59);
    ASSERT(p.edge_count() == 59);
}

TEST(mst_large_chain_sanity) {
    const size_t n = 20000;
    Graph g(n);
    for (size_t i = 0; i + 1 < n; ++i) g.add_edge(i, i + 1, Value(1));

    const MstResult k = kruskal(g);
    const MstResult p = prim(g);
    ASSERT(k.edge_count() == n - 1);
    ASSERT(p.edge_count() == n - 1);
    ASSERT(approx(k.total_weight, static_cast<double>(n - 1)));
    ASSERT(approx(p.total_weight, static_cast<double>(n - 1)));
}

int main() {
    std::cout << "=== Тесты минимального остова (Крускал, Прим) ===\n" << std::endl;

    RUN_TEST(kruskal_empty_graph);
    RUN_TEST(kruskal_single_vertex);
    RUN_TEST(kruskal_triangle_picks_two_cheapest);
    RUN_TEST(kruskal_disconnected_gives_forest);
    RUN_TEST(kruskal_negative_weights_allowed);
    RUN_TEST(kruskal_parallel_edges_uses_cheapest);
    RUN_TEST(kruskal_self_loop_ignored);
    RUN_TEST(kruskal_non_numeric_weight_throws);

    RUN_TEST(prim_empty_graph);
    RUN_TEST(prim_single_vertex);
    RUN_TEST(prim_triangle_picks_two_cheapest);
    RUN_TEST(prim_disconnected_gives_forest);
    RUN_TEST(prim_negative_weights_allowed);
    RUN_TEST(prim_parallel_edges_uses_cheapest);
    RUN_TEST(prim_self_loop_ignored);
    RUN_TEST(prim_non_numeric_weight_throws);

    RUN_TEST(kruskal_vs_prim_connected_random);
    RUN_TEST(kruskal_vs_prim_disconnected_random);
    RUN_TEST(kruskal_vs_prim_with_negative_weights);
    RUN_TEST(mst_large_chain_sanity);

    std::cout << "\n=== Все тесты минимального остова пройдены ===\n";
    return 0;
}