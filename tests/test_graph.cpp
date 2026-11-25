#include <iostream>
#include "../core/Graph.h"
#include "../lib/Exceptions.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

TEST(graph_create_empty) {
    Graph g;
    ASSERT(g.vertex_count() == 0);
    ASSERT(g.edge_count() == 0);
}

TEST(graph_add_vertices) {
    Graph g;
    size_t v0 = g.add_vertex();
    size_t v1 = g.add_vertex();
    size_t v2 = g.add_vertex();

    ASSERT(g.vertex_count() == 3);
    ASSERT(v0 == 0);
    ASSERT(v1 == 1);
    ASSERT(v2 == 2);
}

TEST(graph_add_vertices_with_names) {
    Graph g;
    g.add_vertex("A");
    g.add_vertex("B");
    g.add_vertex("C");

    ASSERT(g.vertex_count() == 3);
    ASSERT(g.vertex_id("A") == 0);
    ASSERT(g.vertex_id("B") == 1);
    ASSERT(g.vertex_id("C") == 2);
    ASSERT(g.has_vertex("A"));
    ASSERT(!g.has_vertex("D"));
}

TEST(graph_add_duplicate_name_returns_same_id) {
    Graph g;
    size_t v0 = g.add_vertex("A");
    size_t v1 = g.add_vertex("A");

    ASSERT(v0 == v1);
    ASSERT(g.vertex_count() == 1);
}

TEST(graph_add_edges) {
    Graph g(3);
    g.add_edge(0, 1, Value(10));
    g.add_edge(1, 2, Value(20));
    g.add_edge(0, 2, Value(30));

    ASSERT(g.edge_count() == 3);
    ASSERT(g.has_edge(0, 1));
    ASSERT(g.has_edge(1, 0));
    ASSERT(g.has_edge(1, 2));
    ASSERT(g.has_edge(0, 2));
}

TEST(graph_edge_weights) {
    Graph g(3);
    g.add_edge(0, 1, Value(10));
    g.add_edge(1, 2, Value(20.5));

    ASSERT(g.get_edge_weight(0, 1) == Value(10));
    ASSERT(g.get_edge_weight(1, 0) == Value(10));
    ASSERT(g.get_edge_weight(1, 2) == Value(20.5));
}

TEST(graph_neighbors) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(0, 2, Value(2));
    g.add_edge(0, 3, Value(3));

    const auto& neighbors = g.neighbors(0);
    ASSERT(neighbors.size() == 3);
    ASSERT(neighbors[0].to == 1);
    ASSERT(neighbors[1].to == 2);
    ASSERT(neighbors[2].to == 3);
}

TEST(graph_edge_count) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(2));
    g.add_edge(2, 3, Value(3));
    g.add_edge(3, 0, Value(4));

    ASSERT(g.edge_count() == 4);
}

TEST(graph_out_of_range_throws) {
    Graph g(2);
    bool caught = false;
    try {
        g.add_edge(0, 5, Value(1));
    } catch (const IndexOutOfRange&) {
        caught = true;
    }
    ASSERT(caught);
}

TEST(graph_missing_edge_throws) {
    Graph g(2);
    g.add_edge(0, 1, Value(1));

    // Ребро есть — не должно бросить
    bool caught = false;
    try {
        (void)g.get_edge_weight(0, 1);
        (void)g.get_edge_weight(1, 0);
    } catch (...) {
        caught = true;
    }
    ASSERT(!caught);

    // Петли нет — должно бросить
    caught = false;
    try {
        (void)g.get_edge_weight(0, 0);
    } catch (const EdgeNotFound&) {
        caught = true;
    }
    ASSERT(caught);
}

TEST(graph_vertex_not_found_throws) {
    Graph g;
    g.add_vertex("A");

    bool caught = false;
    try {
        g.vertex_id("Z");
    } catch (const VertexNotFound&) {
        caught = true;
    }
    ASSERT(caught);
}

TEST(graph_base_exception_catch) {
    // Все наши исключения ловятся через GraphException
    Graph g(1);
    bool caught = false;
    try {
        g.add_edge(0, 10, Value(1));
    } catch (const GraphException&) {
        caught = true;
    }
    ASSERT(caught);
}

TEST(graph_remove_edge_basic) {
    Graph g(3);
    g.add_edge(0, 1, Value(5));
    g.add_edge(1, 2, Value(3));

    ASSERT(g.remove_edge(0, 1));
    ASSERT(!g.has_edge(0, 1));
    ASSERT(!g.has_edge(1, 0));   // неориентированный
    ASSERT(g.edge_count() == 1);

    // Повторное удаление — вернёт false
    ASSERT(!g.remove_edge(0, 1));
}

TEST(graph_remove_edge_missing) {
    Graph g(3);
    g.add_edge(0, 1, Value(5));
    ASSERT(!g.remove_edge(0, 2));
    ASSERT(g.edge_count() == 1);
}

TEST(graph_remove_vertex_updates_edges) {
    Graph g(4);
    g.add_edge(0, 1, Value(1));
    g.add_edge(1, 2, Value(2));
    g.add_edge(2, 3, Value(3));
    g.add_edge(0, 3, Value(4));

    g.remove_vertex(1);

    ASSERT(g.vertex_count() == 3);
    ASSERT(g.edge_count() == 2);   // остались 2-3 и 0-3 (с новыми индексами)
    // После удаления вершины 1: бывшая вершина 2 стала 1, бывшая 3 стала 2
    ASSERT(g.has_edge(0, 2));      // старое 0-3
    ASSERT(g.has_edge(1, 2));      // старое 2-3
    ASSERT(!g.has_edge(0, 1));     // старое 0-1 удалено вместе с вершиной
}

TEST(graph_remove_vertex_updates_names) {
    Graph g;
    g.add_vertex("A");
    g.add_vertex("B");
    g.add_vertex("C");

    g.remove_vertex(0);   // удаляем "A"

    ASSERT(g.vertex_count() == 2);
    ASSERT(!g.has_vertex("A"));
    ASSERT(g.has_vertex("B"));
    ASSERT(g.has_vertex("C"));
    // "B" теперь имеет индекс 0, "C" — индекс 1
    ASSERT(g.vertex_id("B") == 0);
    ASSERT(g.vertex_id("C") == 1);
}

TEST(graph_remove_vertex_out_of_range) {
    Graph g(2);
    bool caught = false;
    try {
        g.remove_vertex(5);
    } catch (const IndexOutOfRange&) {
        caught = true;
    }
    ASSERT(caught);
}

int main() {
    std::cout << "=== Тесты структуры графа ===\n" << std::endl;

    RUN_TEST(graph_create_empty);
    RUN_TEST(graph_add_vertices);
    RUN_TEST(graph_add_vertices_with_names);
    RUN_TEST(graph_add_duplicate_name_returns_same_id);
    RUN_TEST(graph_add_edges);
    RUN_TEST(graph_edge_weights);
    RUN_TEST(graph_neighbors);
    RUN_TEST(graph_edge_count);
    RUN_TEST(graph_out_of_range_throws);
    RUN_TEST(graph_missing_edge_throws);
    RUN_TEST(graph_vertex_not_found_throws);
    RUN_TEST(graph_base_exception_catch);
    RUN_TEST(graph_remove_edge_basic);
    RUN_TEST(graph_remove_edge_missing);
    RUN_TEST(graph_remove_vertex_updates_edges);
    RUN_TEST(graph_remove_vertex_updates_names);
    RUN_TEST(graph_remove_vertex_out_of_range);

    std::cout << "\n=== Все тесты структуры графа пройдены ===\n";
    return 0;
}