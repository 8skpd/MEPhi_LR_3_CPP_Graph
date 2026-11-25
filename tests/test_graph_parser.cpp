#include <iostream>
#include "../core/GraphParser.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

template <class T>
static const T& as(const GraphQuery& q) { return std::get<T>(q); }

TEST(parse_create_graph) {
    GraphParser p;
    const GraphQuery q = p.parse("CREATE GRAPH");
    ASSERT(std::holds_alternative<CreateGraphQuery>(q));
}

TEST(parse_add_vertex_auto) {
    GraphParser p;
    const GraphQuery q = p.parse("ADD VERTEX");
    ASSERT(as<AddVertexQuery>(q).name.empty());
}

TEST(parse_add_vertex_named) {
    GraphParser p;
    const GraphQuery q = p.parse("ADD VERTEX Moscow");
    ASSERT(as<AddVertexQuery>(q).name == "Moscow");
}

TEST(parse_add_edge) {
    GraphParser p;
    const GraphQuery q = p.parse("ADD EDGE A B 5");
    const auto& e = as<AddEdgeQuery>(q);
    ASSERT(e.from == "A");
    ASSERT(e.to == "B");
    ASSERT(e.weight == Value(5));
}

TEST(parse_add_edge_double_weight) {
    GraphParser p;
    const GraphQuery q = p.parse("ADD EDGE A B 2.5");
    ASSERT(as<AddEdgeQuery>(q).weight == Value(2.5));
}

TEST(parse_add_edge_quoted_name) {
    GraphParser p;
    const GraphQuery q = p.parse("ADD EDGE \"New York\" Boston 3");
    const auto& e = as<AddEdgeQuery>(q);
    ASSERT(e.from == "New York");
    ASSERT(e.to == "Boston");
}

TEST(parse_show_commands) {
    GraphParser p;
    ASSERT(std::holds_alternative<ShowGraphQuery>(p.parse("SHOW GRAPH")));
    ASSERT(std::holds_alternative<ShowVerticesQuery>(p.parse("SHOW VERTICES")));
    ASSERT(std::holds_alternative<ShowEdgesQuery>(p.parse("SHOW EDGES")));
}

TEST(parse_generate_random) {
    GraphParser p;
    const GraphQuery q = p.parse("GENERATE RANDOM 100 250");
    const auto& g = as<GenerateQuery>(q);
    ASSERT(g.topology == GraphTopology::Random);
    ASSERT(g.vertices == 100);
    ASSERT(g.edges == 250);
}

TEST(parse_generate_ring_with_seed) {
    GraphParser p;
    const GraphQuery q = p.parse("GENERATE RING 20 SEED 42");
    const auto& g = as<GenerateQuery>(q);
    ASSERT(g.topology == GraphTopology::Ring);
    ASSERT(g.vertices == 20);
    ASSERT(g.seed == 42);
}

TEST(parse_generate_weights_order) {
    GraphParser p;
    const GraphQuery q = p.parse("GENERATE PATH 10 WEIGHTS ASC 1 100");
    const auto& g = as<GenerateQuery>(q);
    ASSERT(g.topology == GraphTopology::Path);
    ASSERT(g.order == WeightOrder::Asc);
    ASSERT(g.min_weight == 1);
    ASSERT(g.max_weight == 100);
}

TEST(parse_path_dijkstra) {
    GraphParser p;
    const GraphQuery q = p.parse("PATH DIJKSTRA A Z");
    const auto& pq = as<PathQuery>(q);
    ASSERT(pq.algorithm == "DIJKSTRA");
    ASSERT(pq.from == "A");
    ASSERT(pq.to == "Z");
}

TEST(parse_path_dfs_no_target) {
    GraphParser p;
    const GraphQuery q = p.parse("PATH DFS A");
    const auto& pq = as<PathQuery>(q);
    ASSERT(pq.algorithm == "DFS");
    ASSERT(pq.from == "A");
    ASSERT(pq.to.empty());
}

TEST(parse_spanning) {
    GraphParser p;
    ASSERT(as<SpanningQuery>(p.parse("SPANNING KRUSKAL")).algorithm == "KRUSKAL");
    ASSERT(as<SpanningQuery>(p.parse("SPANNING PRIM")).algorithm == "PRIM");
}

TEST(parse_components) {
    GraphParser p;
    ASSERT(std::holds_alternative<ComponentsQuery>(p.parse("COMPONENTS")));
    ASSERT(std::holds_alternative<ComponentsQuery>(p.parse("COMP")));
}

TEST(parse_benchmark_suite) {
    GraphParser p;
    const GraphQuery q = p.parse("BENCHMARK SUITE 200 500 42");
    const auto& b = as<BenchmarkQuery>(q);
    ASSERT(b.type == "SUITE");
    ASSERT(b.vertices == 200);
    ASSERT(b.edges == 500);
    ASSERT(b.seed == 42);
}

TEST(parse_benchmark_shortest) {
    GraphParser p;
    const GraphQuery q = p.parse("BENCHMARK SHORTEST A");
    const auto& b = as<BenchmarkQuery>(q);
    ASSERT(b.type == "SHORTEST");
    ASSERT(b.from == "A");
}

TEST(parse_export_csv) {
    GraphParser p;
    const GraphQuery q = p.parse("EXPORT CSV results.csv");
    ASSERT(as<ExportQuery>(q).path == "results.csv");
}

TEST(parse_help_exit) {
    GraphParser p;
    ASSERT(std::holds_alternative<HelpQuery>(p.parse("HELP")));
    ASSERT(std::holds_alternative<ExitQuery>(p.parse("EXIT")));
    ASSERT(std::holds_alternative<ExitQuery>(p.parse("QUIT")));
}

TEST(parse_case_insensitive) {
    GraphParser p;
    ASSERT(std::holds_alternative<CreateGraphQuery>(p.parse("create graph")));
    ASSERT(std::holds_alternative<HelpQuery>(p.parse("help")));
}

TEST(parse_comments_ignored) {
    GraphParser p;
    ASSERT(std::holds_alternative<HelpQuery>(p.parse("HELP # комментарий")));
    ASSERT(std::holds_alternative<ShowGraphQuery>(p.parse("SHOW GRAPH -- ещё комментарий")));
}

TEST(parse_error_unknown_command) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("FOOBAR"); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

TEST(parse_error_empty) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("   "); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

TEST(parse_error_add_missing_parts) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("ADD EDGE A"); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

TEST(parse_error_bad_weight) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("ADD EDGE A B notanumber"); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

TEST(parse_error_unknown_topology) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("GENERATE WHEEL 10"); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

TEST(parse_error_trailing_garbage) {
    GraphParser p;
    bool caught = false;
    try { (void)p.parse("HELP EXTRA"); } catch (const GraphException&) { caught = true; }
    ASSERT(caught);
}

int main() {
    std::cout << "=== Тесты парсера команд ===\n" << std::endl;

    RUN_TEST(parse_create_graph);
    RUN_TEST(parse_add_vertex_auto);
    RUN_TEST(parse_add_vertex_named);
    RUN_TEST(parse_add_edge);
    RUN_TEST(parse_add_edge_double_weight);
    RUN_TEST(parse_add_edge_quoted_name);
    RUN_TEST(parse_show_commands);
    RUN_TEST(parse_generate_random);
    RUN_TEST(parse_generate_ring_with_seed);
    RUN_TEST(parse_generate_weights_order);
    RUN_TEST(parse_path_dijkstra);
    RUN_TEST(parse_path_dfs_no_target);
    RUN_TEST(parse_spanning);
    RUN_TEST(parse_components);
    RUN_TEST(parse_benchmark_suite);
    RUN_TEST(parse_benchmark_shortest);
    RUN_TEST(parse_export_csv);
    RUN_TEST(parse_help_exit);
    RUN_TEST(parse_case_insensitive);
    RUN_TEST(parse_comments_ignored);
    RUN_TEST(parse_error_unknown_command);
    RUN_TEST(parse_error_empty);
    RUN_TEST(parse_error_add_missing_parts);
    RUN_TEST(parse_error_bad_weight);
    RUN_TEST(parse_error_unknown_topology);
    RUN_TEST(parse_error_trailing_garbage);

    std::cout << "\n=== Все тесты парсера пройдены ===\n";
    return 0;
}