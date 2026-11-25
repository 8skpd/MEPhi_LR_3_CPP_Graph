#include <iostream>
#include <fstream>
#include <cstdio>
#include "../core/Benchmark.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

static Graph make_simple_positive_graph() {
    GraphGeneratorConfig cfg;
    cfg.vertices = 20;
    cfg.edges = 40;
    cfg.min_weight = 1;
    cfg.max_weight = 10;
    cfg.order = WeightOrder::Random;
    cfg.seed = 42;
    return GraphGenerator::generate(GraphTopology::Random, cfg);
}

TEST(bench_shortest_paths_returns_three_algorithms) {
    const Graph g = make_simple_positive_graph();
    const auto results = benchmark_shortest_paths(g, 0, "random", "random");
    ASSERT(results.size() == 3);
    ASSERT(results[0].algorithm == "DIJKSTRA");
    ASSERT(results[1].algorithm == "BELLMAN-FORD");
    ASSERT(results[2].algorithm == "BFS");
}

TEST(bench_mst_returns_two_algorithms) {
    const Graph g = make_simple_positive_graph();
    const auto results = benchmark_mst(g, "random", "random");
    ASSERT(results.size() == 2);
    ASSERT(results[0].algorithm == "KRUSKAL");
    ASSERT(results[1].algorithm == "PRIM");
}

TEST(bench_dijkstra_matches_bellman_ford_result) {
    const Graph g = make_simple_positive_graph();
    const auto results = benchmark_shortest_paths(g, 0, "random", "random");
    ASSERT(results[0].success);
    ASSERT(results[1].success);
    // Сумма расстояний должна совпадать
    const double d = results[0].result_value - results[1].result_value;
    ASSERT(d < 1e-6 && d > -1e-6);
}

TEST(bench_kruskal_matches_prim_result) {
    const Graph g = make_simple_positive_graph();
    const auto results = benchmark_mst(g, "random", "random");
    ASSERT(results[0].success);
    ASSERT(results[1].success);
    // Суммарный вес остова должен совпадать
    const double d = results[0].result_value - results[1].result_value;
    ASSERT(d < 1e-6 && d > -1e-6);
}

TEST(bench_elapsed_non_negative) {
    const Graph g = make_simple_positive_graph();
    const auto sp = benchmark_shortest_paths(g, 0, "random", "random");
    const auto mst = benchmark_mst(g, "random", "random");
    for (size_t i = 0; i < sp.size(); ++i) ASSERT(sp[i].elapsed_ms >= 0.0);
    for (size_t i = 0; i < mst.size(); ++i) ASSERT(mst[i].elapsed_ms >= 0.0);
}

TEST(bench_metadata_filled) {
    const Graph g = make_simple_positive_graph();
    const auto results = benchmark_shortest_paths(g, 0, "random", "random");
    for (size_t i = 0; i < results.size(); ++i) {
        ASSERT(results[i].topology == "random");
        ASSERT(results[i].weight_order == "random");
        ASSERT(results[i].vertices == g.vertex_count());
        ASSERT(results[i].edges == g.edge_count());
    }
}

TEST(bench_handles_negative_weight_gracefully) {
    Graph g(3);
    g.add_edge(0, 1, Value(-5));
    g.add_edge(1, 2, Value(3));
    const auto results = benchmark_shortest_paths(g, 0, "manual", "manual");
    // Дейкстра должна упасть с ошибкой, но бенчмарк не падает
    ASSERT(results[0].algorithm == "DIJKSTRA");
    ASSERT(!results[0].success);
    ASSERT(!results[0].note.empty());
}

TEST(bench_suite_returns_expected_count) {
    // 5 топологий × 3 порядка весов × 5 алгоритмов = 75 результатов
    const auto results = benchmark_suite(10, 15, 42);
    ASSERT(results.size() == 75);
}

TEST(bench_suite_covers_three_weight_orders) {
    // Требование сноски 1 методички: три случая данных
    const auto results = benchmark_suite(10, 15, 42);
    bool has_asc = false, has_desc = false, has_random = false;
    for (size_t i = 0; i < results.size(); ++i) {
        if (results[i].weight_order == "asc") has_asc = true;
        if (results[i].weight_order == "desc") has_desc = true;
        if (results[i].weight_order == "random") has_random = true;
    }
    ASSERT(has_asc);
    ASSERT(has_desc);
    ASSERT(has_random);
}

TEST(bench_export_csv_creates_file) {
    DynamicArray<BenchmarkResult> results;
    BenchmarkResult r;
    r.algorithm = "TEST";
    r.topology = "random";
    r.weight_order = "random";
    r.vertices = 10;
    r.edges = 15;
    r.elapsed_ms = 1.5;
    r.result_value = 42.0;
    r.success = true;
    results.push_back(r);

    const std::string path = "assets/test_benchmark.csv";
    ASSERT(export_benchmark_csv(results, path));

    // Проверка содержимого
    std::ifstream f(path);
    ASSERT(f.is_open());
    std::string header;
    std::getline(f, header);
    ASSERT(header.find("algorithm") != std::string::npos);
    ASSERT(header.find("elapsed_ms") != std::string::npos);

    std::string row;
    std::getline(f, row);
    ASSERT(row.find("TEST") != std::string::npos);
    f.close();

    std::remove(path.c_str());
}

TEST(bench_export_csv_handles_empty) {
    DynamicArray<BenchmarkResult> results;
    const std::string path = "assets/test_empty_benchmark.csv";
    ASSERT(export_benchmark_csv(results, path));
    std::remove(path.c_str());
}

TEST(bench_measure_ms_returns_positive) {
    double elapsed = measure_ms([]() {
        volatile int x = 0;
        for (int i = 0; i < 100000; ++i) x += i;
    });
    ASSERT(elapsed >= 0.0);
}

int main() {
    std::cout << "=== Тесты слоя бенчмарков ===\n" << std::endl;

    RUN_TEST(bench_shortest_paths_returns_three_algorithms);
    RUN_TEST(bench_mst_returns_two_algorithms);
    RUN_TEST(bench_dijkstra_matches_bellman_ford_result);
    RUN_TEST(bench_kruskal_matches_prim_result);
    RUN_TEST(bench_elapsed_non_negative);
    RUN_TEST(bench_metadata_filled);
    RUN_TEST(bench_handles_negative_weight_gracefully);
    RUN_TEST(bench_suite_returns_expected_count);
    RUN_TEST(bench_suite_covers_three_weight_orders);
    RUN_TEST(bench_export_csv_creates_file);
    RUN_TEST(bench_export_csv_handles_empty);
    RUN_TEST(bench_measure_ms_returns_positive);

    std::cout << "\n=== Все тесты бенчмарков пройдены ===\n";
    return 0;
}