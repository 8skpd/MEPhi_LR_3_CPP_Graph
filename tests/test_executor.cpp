#include <iostream>
#include <cstdio>
#include <string>
#include "../core/GraphParser.h"
#include "../core/GraphExecutor.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

// Окружение: парсер + исполнитель, как в реальном клиенте
struct Env {
    GraphParser parser;
    GraphExecutor executor;

    std::string run(const std::string& cmd) {
        try {
            return executor.execute(parser.parse(cmd));
        } catch (const GraphException& e) {
            return std::string("Ошибка: ") + e.what();
        }
    }
};

static bool contains(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

// =========================================================
// Ручной ввод графа
// =========================================================

TEST(exec_create_and_add_vertices) {
    Env e;
    ASSERT(contains(e.run("CREATE GRAPH"), "пустой граф"));
    ASSERT(contains(e.run("ADD VERTEX A"), "A"));
    ASSERT(contains(e.run("ADD VERTEX B"), "B"));
    ASSERT(e.executor.graph().vertex_count() == 2);
}

TEST(exec_add_edge_creates_vertices) {
    Env e;
    e.run("ADD EDGE A B 5");
    ASSERT(e.executor.graph().vertex_count() == 2);
    ASSERT(e.executor.graph().edge_count() == 1);
}

TEST(exec_add_duplicate_vertex_is_idempotent) {
    Env e;
    e.run("ADD VERTEX A");
    e.run("ADD VERTEX A");
    ASSERT(e.executor.graph().vertex_count() == 1);
}

TEST(exec_show_graph_reports_structure) {
    Env e;
    e.run("ADD EDGE A B 5");
    const std::string r = e.run("SHOW GRAPH");
    ASSERT(contains(r, "Вершин: 2"));
    ASSERT(contains(r, "Рёбер:  1"));
    ASSERT(contains(r, "Связный: да"));
}

TEST(exec_show_vertices_lists_all) {
    Env e;
    e.run("ADD EDGE A B 5");
    e.run("ADD VERTEX C");
    const std::string r = e.run("SHOW VERTICES");
    ASSERT(contains(r, "A"));
    ASSERT(contains(r, "B"));
    ASSERT(contains(r, "C"));
}

TEST(exec_show_edges_lists_all) {
    Env e;
    e.run("ADD EDGE A B 5");
    const std::string r = e.run("SHOW EDGES");
    ASSERT(contains(r, "A"));
    ASSERT(contains(r, "B"));
    ASSERT(contains(r, "5"));
}

TEST(exec_show_empty_graph) {
    Env e;
    ASSERT(contains(e.run("SHOW GRAPH"), "пуст"));
}

// =========================================================
// Автоматическая генерация
// =========================================================

TEST(exec_generate_random) {
    Env e;
    const std::string r = e.run("GENERATE RANDOM 50 100 SEED 42");
    ASSERT(contains(r, "вершин=50"));
    ASSERT(e.executor.graph().vertex_count() == 50);
    ASSERT(e.executor.graph().edge_count() == 100);
}

TEST(exec_generate_ring) {
    Env e;
    e.run("GENERATE RING 10 SEED 1");
    ASSERT(e.executor.graph().vertex_count() == 10);
    ASSERT(e.executor.graph().edge_count() == 10);
}

TEST(exec_generate_with_weight_order) {
    Env e;
    const std::string r = e.run("GENERATE PATH 10 WEIGHTS ASC 1 100");
    ASSERT(contains(r, "веса=asc"));
    ASSERT(e.executor.graph().edge_count() == 9);
}

TEST(exec_generate_replaces_graph) {
    Env e;
    e.run("ADD EDGE A B 5");
    e.run("GENERATE RING 5 SEED 1");
    ASSERT(e.executor.graph().vertex_count() == 5);
    ASSERT(e.executor.graph().edge_count() == 5);
}

// =========================================================
// Алгоритмы: кратчайшие пути и обходы
// =========================================================

TEST(exec_path_dijkstra_finds_shortest) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 2");
    e.run("ADD EDGE A C 10");
    const std::string r = e.run("PATH DIJKSTRA A C");
    ASSERT(contains(r, "A -> B -> C"));
    ASSERT(contains(r, "Расстояние: 3"));
}

TEST(exec_path_bfs_unreachable) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD VERTEX Z");
    const std::string r = e.run("PATH BFS A Z");
    ASSERT(contains(r, "недостижима"));
}

TEST(exec_path_dfs_shows_traversal_order) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 1");
    const std::string r = e.run("PATH DFS A");
    ASSERT(contains(r, "Порядок обхода"));
    ASSERT(contains(r, "A"));
}

TEST(exec_path_bellman_finds_shortest) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 2");
    e.run("ADD EDGE A C 10");
    const std::string r = e.run("PATH BELLMAN A C");
    ASSERT(contains(r, "Расстояние: 3"));
}

TEST(exec_path_distances_to_all) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 2");
    const std::string r = e.run("PATH DIJKSTRA A");
    ASSERT(contains(r, "Расстояния от"));
    ASSERT(contains(r, "Достижимо вершин: 3"));
}

// =========================================================
// Алгоритмы: остов и компоненты
// =========================================================

TEST(exec_spanning_kruskal) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 2");
    e.run("ADD EDGE A C 10");
    const std::string r = e.run("SPANNING KRUSKAL");
    ASSERT(contains(r, "Суммарный вес: 3"));
}

TEST(exec_spanning_prim) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD EDGE B C 2");
    e.run("ADD EDGE A C 10");
    const std::string r = e.run("SPANNING PRIM");
    ASSERT(contains(r, "Суммарный вес: 3"));
}

TEST(exec_components_reports_count) {
    Env e;
    e.run("ADD EDGE A B 1");
    e.run("ADD VERTEX Z");
    const std::string r = e.run("COMPONENTS");
    ASSERT(contains(r, "Компонент связности: 2"));
}

// =========================================================
// Бенчмарки и выгрузка
// =========================================================

TEST(exec_benchmark_shortest_runs_three_algorithms) {
    Env e;
    e.run("GENERATE RANDOM 20 40 SEED 42");
    const std::string r = e.run("BENCHMARK SHORTEST v0");
    ASSERT(contains(r, "DIJKSTRA"));
    ASSERT(contains(r, "BELLMAN-FORD"));
    ASSERT(contains(r, "BFS"));
}

TEST(exec_benchmark_mst_runs_two_algorithms) {
    Env e;
    e.run("GENERATE RANDOM 20 40 SEED 42");
    const std::string r = e.run("BENCHMARK MST");
    ASSERT(contains(r, "KRUSKAL"));
    ASSERT(contains(r, "PRIM"));
}

TEST(exec_benchmark_suite_and_export) {
    Env e;
    const std::string r = e.run("BENCHMARK SUITE 10 15 42");
    ASSERT(contains(r, "75"));

    const std::string path = "assets/test_exec_export.csv";
    const std::string er = e.run("EXPORT CSV " + path);
    ASSERT(contains(er, "сохранены"));
    std::remove(path.c_str());
}

TEST(exec_export_without_benchmark_fails) {
    Env e;
    const std::string r = e.run("EXPORT CSV file.csv");
    ASSERT(contains(r, "нет данных"));
}

// =========================================================
// Обработка ошибок
// =========================================================

TEST(exec_error_unknown_vertex) {
    Env e;
    e.run("ADD EDGE A B 1");
    const std::string r = e.run("PATH DIJKSTRA X Y");
    ASSERT(contains(r, "Ошибка"));
}

TEST(exec_error_on_empty_graph) {
    Env e;
    const std::string r = e.run("PATH DIJKSTRA A");
    ASSERT(contains(r, "граф пуст"));
}

TEST(exec_help_lists_commands) {
    Env e;
    const std::string r = e.run("HELP");
    ASSERT(contains(r, "CREATE GRAPH"));
    ASSERT(contains(r, "GENERATE"));
    ASSERT(contains(r, "PATH"));
    ASSERT(contains(r, "BENCHMARK"));
}

TEST(exec_exit_sets_flag) {
    Env e;
    ASSERT(!e.executor.should_exit());
    e.run("EXIT");
    ASSERT(e.executor.should_exit());
}

// =========================================================
// Полный сценарий: ручной ввод -> алгоритм -> результат
// =========================================================

TEST(exec_full_scenario_manual_graph) {
    Env e;
    // Строим граф "транспортная сеть"
    e.run("CREATE GRAPH");
    e.run("ADD EDGE Москва Тверь 170");
    e.run("ADD EDGE Тверь Новгород 390");
    e.run("ADD EDGE Москва Новгород 600");
    e.run("ADD EDGE Новгород Псков 290");

    // Проверяем структуру
    ASSERT(contains(e.run("SHOW GRAPH"), "Вершин: 4"));
    ASSERT(contains(e.run("SHOW GRAPH"), "Рёбер:  4"));

    // Кратчайший путь: через Тверь дешевле (170+390=560 < 600)
    const std::string r = e.run("PATH DIJKSTRA Москва Новгород");
    ASSERT(contains(r, "Москва -> Тверь -> Новгород"));
    ASSERT(contains(r, "Расстояние: 560"));

    // Остов
    ASSERT(contains(e.run("SPANNING KRUSKAL"), "Суммарный вес: 850"));

    // Граф связный — одна компонента
    ASSERT(contains(e.run("COMPONENTS"), "Компонент связности: 1"));
}

TEST(exec_remove_vertex) {
    Env e;
    e.run("ADD EDGE A B 5");
    e.run("ADD EDGE B C 3");
    const std::string r = e.run("REMOVE VERTEX B");
    ASSERT(contains(r, "Удалена вершина"));
    ASSERT(e.executor.graph().vertex_count() == 2);
    ASSERT(e.executor.graph().edge_count() == 0);  // оба ребра удалены
}

TEST(exec_remove_edge) {
    Env e;
    e.run("ADD EDGE A B 5");
    e.run("ADD EDGE B C 3");
    const std::string r = e.run("REMOVE EDGE A B");
    ASSERT(contains(r, "Удалено ребро"));
    ASSERT(e.executor.graph().edge_count() == 1);
}

TEST(exec_remove_vertex_not_found) {
    Env e;
    e.run("ADD VERTEX A");
    const std::string r = e.run("REMOVE VERTEX Z");
    ASSERT(contains(r, "не найдена"));
}

TEST(exec_remove_edge_not_found) {
    Env e;
    e.run("ADD EDGE A B 5");
    const std::string r = e.run("REMOVE EDGE A C");
    ASSERT(contains(r, "не найдена") || contains(r, "не найдено"));
}

TEST(exec_drop_graph) {
    Env e;
    e.run("ADD EDGE A B 5");
    e.run("ADD EDGE B C 3");
    ASSERT(e.executor.graph().vertex_count() == 3);

    const std::string r = e.run("DROP GRAPH");
    ASSERT(contains(r, "Граф удалён"));
    ASSERT(e.executor.graph().vertex_count() == 0);
    ASSERT(e.executor.graph().edge_count() == 0);
}

TEST(exec_drop_empty_graph) {
    Env e;
    const std::string r = e.run("DROP GRAPH");
    ASSERT(contains(r, "уже был пуст"));
}

int main() {
    std::cout << "=== Сквозные тесты (парсер + исполнитель) ===\n" << std::endl;

    RUN_TEST(exec_create_and_add_vertices);
    RUN_TEST(exec_add_edge_creates_vertices);
    RUN_TEST(exec_add_duplicate_vertex_is_idempotent);
    RUN_TEST(exec_show_graph_reports_structure);
    RUN_TEST(exec_show_vertices_lists_all);
    RUN_TEST(exec_show_edges_lists_all);
    RUN_TEST(exec_show_empty_graph);

    RUN_TEST(exec_generate_random);
    RUN_TEST(exec_generate_ring);
    RUN_TEST(exec_generate_with_weight_order);
    RUN_TEST(exec_generate_replaces_graph);

    RUN_TEST(exec_path_dijkstra_finds_shortest);
    RUN_TEST(exec_path_bfs_unreachable);
    RUN_TEST(exec_path_dfs_shows_traversal_order);
    RUN_TEST(exec_path_bellman_finds_shortest);
    RUN_TEST(exec_path_distances_to_all);

    RUN_TEST(exec_spanning_kruskal);
    RUN_TEST(exec_spanning_prim);
    RUN_TEST(exec_components_reports_count);

    RUN_TEST(exec_benchmark_shortest_runs_three_algorithms);
    RUN_TEST(exec_benchmark_mst_runs_two_algorithms);
    RUN_TEST(exec_benchmark_suite_and_export);
    RUN_TEST(exec_export_without_benchmark_fails);

    RUN_TEST(exec_error_unknown_vertex);
    RUN_TEST(exec_error_on_empty_graph);
    RUN_TEST(exec_help_lists_commands);
    RUN_TEST(exec_exit_sets_flag);

    RUN_TEST(exec_full_scenario_manual_graph);

    RUN_TEST(exec_remove_vertex);
    RUN_TEST(exec_remove_edge);
    RUN_TEST(exec_remove_vertex_not_found);
    RUN_TEST(exec_remove_edge_not_found);

    RUN_TEST(exec_drop_graph);
    RUN_TEST(exec_drop_empty_graph);

    std::cout << "\n=== Все сквозные тесты пройдены ===\n";
    return 0;
}