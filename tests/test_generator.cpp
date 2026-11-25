#include <iostream>
#include <climits>
#include <vector>
#include <algorithm>
#include "../core/GraphGenerator.h"
#include "../core/algorithms/DFS.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

static GraphGeneratorConfig make_cfg(size_t v, size_t e, unsigned seed) {
    GraphGeneratorConfig cfg;
    cfg.vertices = v;
    cfg.edges = e;
    cfg.min_weight = 1;
    cfg.max_weight = 10;
    cfg.order = WeightOrder::Random;
    cfg.seed = seed;
    return cfg;
}

// =========================================================
// Случайный граф
// =========================================================

TEST(gen_random_vertex_count) {
    const Graph g = GraphGenerator::generate_random(make_cfg(50, 100, 42));
    ASSERT(g.vertex_count() == 50);
}

TEST(gen_random_edge_count) {
    const Graph g = GraphGenerator::generate_random(make_cfg(50, 100, 42));
    ASSERT(g.edge_count() == 100);
}

TEST(gen_random_edge_count_capped) {
    // Запрошено больше рёбер, чем возможно: ограничиваем максимумом
    const Graph g = GraphGenerator::generate_random(make_cfg(10, 1000, 42));
    const size_t max_edges = 10 * 9 / 2;
    ASSERT(g.edge_count() == max_edges);
}

TEST(gen_random_no_loops_no_duplicates) {
    const Graph g = GraphGenerator::generate_random(make_cfg(30, 100, 42));
    const auto es = g.edges();
    ASSERT(es.size() == g.edge_count());
    for (size_t i = 0; i < es.size(); ++i) {
        ASSERT(es[i].from != es[i].to);       // нет петель
        ASSERT(es[i].from < es[i].to);        // нормализовано
    }
}

TEST(gen_random_dense_uses_shuffle) {
    // Плотный граф (больше половины максимума) идёт через перемешивание
    const Graph g = GraphGenerator::generate_random(make_cfg(20, 150, 42));
    ASSERT(g.edge_count() == 150);
}

// =========================================================
// Топологии: структура
// =========================================================

TEST(gen_path_structure) {
    const Graph g = GraphGenerator::generate_path(make_cfg(10, 0, 1));
    ASSERT(g.vertex_count() == 10);
    ASSERT(g.edge_count() == 9);
    ASSERT(is_connected(g));
}

TEST(gen_ring_structure) {
    const Graph g = GraphGenerator::generate_ring(make_cfg(10, 0, 1));
    ASSERT(g.vertex_count() == 10);
    ASSERT(g.edge_count() == 10);
    ASSERT(is_connected(g));
}

TEST(gen_ring_small_cases) {
    // n=1: рёбер нет; n=2: одно ребро
    ASSERT(GraphGenerator::generate_ring(make_cfg(1, 0, 1)).edge_count() == 0);
    ASSERT(GraphGenerator::generate_ring(make_cfg(2, 0, 1)).edge_count() == 1);
}

TEST(gen_star_structure) {
    const Graph g = GraphGenerator::generate_star(make_cfg(10, 0, 1));
    ASSERT(g.vertex_count() == 10);
    ASSERT(g.edge_count() == 9);
    ASSERT(g.neighbors(0).size() == 9);   // центр соединён со всеми
    ASSERT(is_connected(g));
}

TEST(gen_complete_structure) {
    const Graph g = GraphGenerator::generate_complete(make_cfg(8, 0, 1));
    ASSERT(g.vertex_count() == 8);
    ASSERT(g.edge_count() == 8 * 7 / 2);
    ASSERT(is_connected(g));
}

TEST(gen_random_tree_structure) {
    const Graph g = GraphGenerator::generate_random_tree(make_cfg(50, 0, 42));
    ASSERT(g.vertex_count() == 50);
    ASSERT(g.edge_count() == 49);          // дерево: V-1 рёбер
    ASSERT(is_connected(g));
}

// =========================================================
// Распределение весов
// =========================================================

TEST(gen_weights_in_range) {
    GraphGeneratorConfig cfg = make_cfg(30, 80, 42);
    cfg.min_weight = 5;
    cfg.max_weight = 20;
    const Graph g = GraphGenerator::generate_random(cfg);
    const auto es = g.edges();
    for (size_t i = 0; i < es.size(); ++i) {
        const int w = static_cast<int>(es[i].weight.to_double());
        ASSERT(w >= 5 && w <= 20);
    }
}

TEST(gen_weights_asc_spans_range) {
    GraphGeneratorConfig cfg = make_cfg(20, 50, 42);
    cfg.min_weight = 1;
    cfg.max_weight = 100;
    cfg.order = WeightOrder::Asc;
    const Graph g = GraphGenerator::generate_random(cfg);
    const auto es = g.edges();
    int min_w = INT_MAX, max_w = INT_MIN;
    for (size_t i = 0; i < es.size(); ++i) {
        const int w = static_cast<int>(es[i].weight.to_double());
        if (w < min_w) min_w = w;
        if (w > max_w) max_w = w;
    }
    ASSERT(min_w == 1);
    ASSERT(max_w == 100);
}

TEST(gen_weights_desc_spans_range) {
    GraphGeneratorConfig cfg = make_cfg(20, 50, 42);
    cfg.min_weight = 1;
    cfg.max_weight = 100;
    cfg.order = WeightOrder::Desc;
    const Graph g = GraphGenerator::generate_random(cfg);
    const auto es = g.edges();
    int min_w = INT_MAX, max_w = INT_MIN;
    for (size_t i = 0; i < es.size(); ++i) {
        const int w = static_cast<int>(es[i].weight.to_double());
        if (w < min_w) min_w = w;
        if (w > max_w) max_w = w;
    }
    ASSERT(min_w == 1);
    ASSERT(max_w == 100);
}

// =========================================================
// Детерминированность
// =========================================================

TEST(gen_deterministic_same_seed) {
    const Graph g1 = GraphGenerator::generate_random(make_cfg(30, 60, 123));
    const Graph g2 = GraphGenerator::generate_random(make_cfg(30, 60, 123));
    ASSERT(g1.vertex_count() == g2.vertex_count());
    ASSERT(g1.edge_count() == g2.edge_count());
    const auto e1 = g1.edges();
    const auto e2 = g2.edges();
    ASSERT(e1.size() == e2.size());
    for (size_t i = 0; i < e1.size(); ++i) {
        ASSERT(e1[i].from == e2[i].from);
        ASSERT(e1[i].to == e2[i].to);
        ASSERT(e1[i].weight == e2[i].weight);
    }
}

TEST(gen_different_seeds_differ) {
    const Graph g1 = GraphGenerator::generate_random(make_cfg(30, 60, 1));
    const Graph g2 = GraphGenerator::generate_random(make_cfg(30, 60, 2));
    // Вероятностно: хотя бы одно ребро отличается
    const auto e1 = g1.edges();
    const auto e2 = g2.edges();
    bool any_diff = false;
    for (size_t i = 0; i < e1.size() && !any_diff; ++i) {
        if (e1[i].from != e2[i].from || e1[i].to != e2[i].to) any_diff = true;
    }
    ASSERT(any_diff);
}

// =========================================================
// Общая точка входа
// =========================================================

TEST(gen_dispatch_all_topologies) {
    GraphGeneratorConfig cfg = make_cfg(15, 30, 42);
    const GraphTopology topos[] = {
        GraphTopology::Random, GraphTopology::Ring, GraphTopology::Path,
        GraphTopology::Star, GraphTopology::Complete, GraphTopology::RandomTree
    };
    for (size_t i = 0; i < 6; ++i) {
        const Graph g = GraphGenerator::generate(topos[i], cfg);
        ASSERT(g.vertex_count() == 15);
    }
}

TEST(gen_empty_graph) {
    const Graph g = GraphGenerator::generate_random(make_cfg(0, 0, 1));
    ASSERT(g.vertex_count() == 0);
    ASSERT(g.edge_count() == 0);
}

TEST(gen_single_vertex) {
    const Graph g = GraphGenerator::generate_random(make_cfg(1, 0, 1));
    ASSERT(g.vertex_count() == 1);
    ASSERT(g.edge_count() == 0);
}

int main() {
    std::cout << "=== Тесты генератора графов ===\n" << std::endl;

    RUN_TEST(gen_random_vertex_count);
    RUN_TEST(gen_random_edge_count);
    RUN_TEST(gen_random_edge_count_capped);
    RUN_TEST(gen_random_no_loops_no_duplicates);
    RUN_TEST(gen_random_dense_uses_shuffle);

    RUN_TEST(gen_path_structure);
    RUN_TEST(gen_ring_structure);
    RUN_TEST(gen_ring_small_cases);
    RUN_TEST(gen_star_structure);
    RUN_TEST(gen_complete_structure);
    RUN_TEST(gen_random_tree_structure);

    RUN_TEST(gen_weights_in_range);
    RUN_TEST(gen_weights_asc_spans_range);
    RUN_TEST(gen_weights_desc_spans_range);

    RUN_TEST(gen_deterministic_same_seed);
    RUN_TEST(gen_different_seeds_differ);

    RUN_TEST(gen_dispatch_all_topologies);
    RUN_TEST(gen_empty_graph);
    RUN_TEST(gen_single_vertex);

    std::cout << "\n=== Все тесты генератора пройдены ===\n";
    return 0;
}