#pragma once

#include <sstream>
#include <iomanip>
#include <string>
#include "Graph.h"
#include "GraphQuery.h"
#include "GraphParser.h"
#include "Benchmark.h"
#include "GraphViewer.h"
#include "algorithms/BFS.h"
#include "algorithms/DFS.h"
#include "algorithms/Dijkstra.h"
#include "algorithms/BellmanFord.h"
#include "algorithms/Kruskal.h"
#include "algorithms/Prim.h"

/*
Исполнитель команд графового интерфейса.
Хранит текущий граф, выполняет команды через std::visit,
форматирует результаты для консольного вывода.

Закрывает требования методички:
  - просмотр исходных данных (SHOW)
  - промежуточные результаты (порядок обхода DFS)
  - конечные результаты (пути, остовы, компоненты)
  - ручной ввод (ADD) и автоматический (GENERATE)
*/

template <class... Ts>
struct Overload : Ts... { using Ts::operator()...; };
template <class... Ts>
Overload(Ts...) -> Overload<Ts...>;

// Подсчёт визуальной длины строки в символах.
// В UTF-8 байт является началом символа, если он не имеет вид 10xxxxxx.
inline size_t visual_length(const std::string& s) {
    size_t count = 0;
    for (unsigned char c : s) {
        if ((c & 0xC0) != 0x80) ++count;
    }
    return count;
}

// Выравнивание строки по левому краю до заданной визуальной ширины
inline std::string pad_right(const std::string& s, size_t width) {
    const size_t len = visual_length(s);
    if (len >= width) return s;
    return s + std::string(width - len, ' ');
}

class GraphExecutor {
private:
    Graph graph_;
    DynamicArray<BenchmarkResult> last_benchmark_;
    bool exit_requested_ = false;

    // форматирование пути
    std::string format_path(const DynamicArray<size_t>& path) const {
        std::ostringstream oss;
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) oss << " -> ";
            oss << graph_.vertex_name(path[i]);
        }
        return oss.str();
    }

    std::string format_double(double v) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << v;
        return oss.str();
    }

    // выполнение команд

    std::string exec_create() {
        graph_ = Graph();
        last_benchmark_ = DynamicArray<BenchmarkResult>();
        return "Создан новый пустой граф";
    }

    std::string exec_add_vertex(const AddVertexQuery& q) {
        size_t id;
        if (q.name.empty()) {
            id = graph_.add_vertex();
        } else {
            id = graph_.add_vertex(q.name);
        }
        return "Добавлена вершина '" + graph_.vertex_name(id) + "' (id=" + std::to_string(id) + ")";
    }

    std::string exec_add_edge(const AddEdgeQuery& q) {
        // Создаём вершины, если их ещё нет
        graph_.add_vertex(q.from);
        graph_.add_vertex(q.to);
        graph_.add_edge(q.from, q.to, q.weight);
        return "Добавлено ребро '" + q.from + "' - '" + q.to +
               "' (вес " + q.weight.to_string() + ")";
    }

    std::string exec_show_graph() const {
        if (graph_.vertex_count() == 0) return "Граф пуст";
        const bool connected = is_connected(graph_);
        std::ostringstream oss;
        oss << "Граф:\n"
            << "  Вершин: " << graph_.vertex_count() << "\n"
            << "  Рёбер:  " << graph_.edge_count() << "\n"
            << "  Связный: " << (connected ? "да" : "нет");
        if (!connected) {
            const auto comps = connected_components(graph_);
            oss << " (компонент: " << comps.count << ")";
        }
        return oss.str();
    }

    std::string exec_show_vertices() const {
        if (graph_.vertex_count() == 0) return "Граф пуст";
        std::ostringstream oss;
        oss << "Вершины (" << graph_.vertex_count() << "):\n";
        oss << pad_right("Имя", 15) << " | "
            << pad_right("ID", 5) << " | Степень\n";
        oss << std::string(30, '-') << "\n";
        for (size_t i = 0; i < graph_.vertex_count(); ++i) {
            oss << pad_right(graph_.vertex_name(i), 15)
                << " | " << pad_right(std::to_string(i), 5)
                << " | " << graph_.neighbors(i).size() << "\n";
        }
        return oss.str();
    }

    std::string exec_show_edges() const {
        if (graph_.edge_count() == 0) return "Рёбер нет";
        const auto edges = graph_.edges();
        std::ostringstream oss;
        oss << "Рёбра (" << edges.size() << "):\n";
        oss << pad_right("От", 15) << " | "
            << pad_right("До", 15) << " | Вес\n";
        oss << std::string(40, '-') << "\n";
        for (size_t i = 0; i < edges.size(); ++i) {
            oss << pad_right(graph_.vertex_name(edges[i].from), 15)
                << " | " << pad_right(graph_.vertex_name(edges[i].to), 15)
                << " | " << edges[i].weight.to_string() << "\n";
        }
        return oss.str();
    }

    std::string exec_generate(const GenerateQuery& q) {
        GraphGeneratorConfig cfg;
        cfg.vertices = q.vertices;
        cfg.edges = q.edges;
        cfg.min_weight = q.min_weight;
        cfg.max_weight = q.max_weight;
        cfg.order = q.order;
        cfg.seed = q.seed;
        graph_ = GraphGenerator::generate(q.topology, cfg);
        last_benchmark_ = DynamicArray<BenchmarkResult>();

        std::ostringstream oss;
        oss << "Сгенерирован граф: вершин=" << graph_.vertex_count()
            << ", рёбер=" << graph_.edge_count()
            << ", топология=" << topology_to_string(q.topology)
            << ", веса=" << order_to_string(q.order);
        return oss.str();
    }

    std::string exec_path(const PathQuery& q) {
        if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";
        if (!graph_.has_vertex(q.from)) {
            return "Ошибка: вершина '" + q.from + "' не найдена";
        }
        const size_t from_id = graph_.vertex_id(q.from);

        // DFS
        if (q.algorithm == "DFS") {
            const PathResult r = dfs(graph_, from_id);
            if (q.to.empty()) {
                // Промежуточный результат: порядок обхода
                std::ostringstream oss;
                oss << "Алгоритм: DFS (поиск в глубину)\n";
                oss << "Порядок обхода от '" << q.from << "':\n  ";
                for (size_t i = 0; i < r.order.size(); ++i) {
                    if (i > 0) oss << ", ";
                    oss << graph_.vertex_name(r.order[i]);
                }
                oss << "\nПосещено вершин: " << r.reached
                    << " из " << graph_.vertex_count();
                return oss.str();
            }
            if (!graph_.has_vertex(q.to)) {
                return "Ошибка: вершина '" + q.to + "' не найдена";
            }
            const size_t to_id = graph_.vertex_id(q.to);
            const auto path = reconstruct_path(r, to_id);
            if (!path.has_value()) {
                return "Вершина '" + q.to + "' недостижима из '" + q.from + "'";
            }
            std::ostringstream oss;
            oss << "Алгоритм: DFS (путь не гарантирует кратчайший)\n";
            oss << "Путь: " << format_path(path.value()) << "\n";
            oss << "Глубина: " << format_double(r.dist[to_id].value());
            return oss.str();
        }

        // BFS, DIJKSTRA, BELLMAN
        PathResult r;
        try {
            if (q.algorithm == "BFS") {
                r = bfs(graph_, from_id);
            } else if (q.algorithm == "DIJKSTRA") {
                r = dijkstra(graph_, from_id);
            } else if (q.algorithm == "BELLMAN") {
                r = bellman_ford(graph_, from_id);
            }
        } catch (const GraphException& e) {
            return std::string("Ошибка: ") + e.what();
        }

        if (q.to.empty()) {
            // Расстояния до всех вершин
            std::ostringstream oss;
            oss << "Алгоритм: " << q.algorithm << "\n";
            oss << "Расстояния от '" << q.from << "':\n";
            for (size_t i = 0; i < graph_.vertex_count(); ++i) {
                oss << "  " << graph_.vertex_name(i) << ": ";
                if (r.dist[i].has_value()) {
                    oss << format_double(r.dist[i].value());
                } else {
                    oss << "недостижима";
                }
                oss << "\n";
            }
            oss << "Достижимо вершин: " << r.reached
                << " из " << graph_.vertex_count();
            return oss.str();
        }

        if (!graph_.has_vertex(q.to)) {
            return "Ошибка: вершина '" + q.to + "' не найдена";
        }
        const size_t to_id = graph_.vertex_id(q.to);
        const auto path = reconstruct_path(r, to_id);
        if (!path.has_value()) {
            return "Вершина '" + q.to + "' недостижима из '" + q.from + "'";
        }
        std::ostringstream oss;
        oss << "Алгоритм: " << q.algorithm << "\n";
        oss << "Путь: " << format_path(path.value()) << "\n";
        oss << "Расстояние: " << format_double(r.dist[to_id].value());
        return oss.str();
    }

    std::string exec_spanning(const SpanningQuery& q) {
        if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";

        MstResult r;
        try {
            if (q.algorithm == "KRUSKAL") {
                r = kruskal(graph_);
            } else {
                r = prim(graph_);
            }
        } catch (const GraphException& e) {
            return std::string("Ошибка: ") + e.what();
        }

        std::ostringstream oss;
        oss << "Алгоритм: " << q.algorithm << " (минимальный остов)\n";
        if (r.edges.size() == 0) {
            oss << "Рёбер в остове нет (граф из изолированных вершин)";
            return oss.str();
        }
        oss << "Рёбра остова:\n";
        for (size_t i = 0; i < r.edges.size(); ++i) {
            oss << "  " << graph_.vertex_name(r.edges[i].from)
                << " - " << graph_.vertex_name(r.edges[i].to)
                << " (вес " << r.edges[i].weight.to_string() << ")\n";
        }
        oss << "Суммарный вес: " << format_double(r.total_weight) << "\n";
        oss << "Связный граф: " << (r.connected ? "да" : "нет (остовный лес)");
        return oss.str();
    }

    std::string exec_components() {
        if (graph_.vertex_count() == 0) return "Граф пуст";
        const auto comps = connected_components(graph_);
        std::ostringstream oss;
        oss << "Компонент связности: " << comps.count << "\n";
        for (size_t c = 0; c < comps.count; ++c) {
            oss << "  Компонента " << c << ": " << comps.sizes[c] << " вершин\n";
        }
        return oss.str();
    }

    std::string exec_benchmark(const BenchmarkQuery& q) {
        if (q.type == "SUITE") {
            last_benchmark_ = benchmark_suite(q.vertices, q.edges, q.seed);
        } else if (q.type == "SHORTEST") {
            if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";
            if (!graph_.has_vertex(q.from)) {
                return "Ошибка: вершина '" + q.from + "' не найдена";
            }
            const size_t from_id = graph_.vertex_id(q.from);
            last_benchmark_ = benchmark_shortest_paths(graph_, from_id, "manual", "manual");
        } else if (q.type == "MST") {
            if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";
            last_benchmark_ = benchmark_mst(graph_, "manual", "manual");
        }

        std::ostringstream oss;
        oss << std::fixed << std::setprecision(3);
        oss << "Результаты бенчмарка (" << last_benchmark_.size() << "):\n";
        oss << pad_right("Алгоритм", 14) << " | "
            << pad_right("Время (мс)", 10) << " | "
            << pad_right("Результат", 14) << " | Статус\n";
        oss << std::string(60, '-') << "\n";
        for (size_t i = 0; i < last_benchmark_.size(); ++i) {
            const auto& r = last_benchmark_[i];
            std::ostringstream time_oss, result_oss;
            time_oss << std::fixed << std::setprecision(3) << r.elapsed_ms;
            result_oss << std::fixed << std::setprecision(2) << r.result_value;
            oss << pad_right(r.algorithm, 14) << " | "
                << pad_right(time_oss.str(), 10) << " | "
                << pad_right(result_oss.str(), 14) << " | "
                << (r.success ? "OK" : r.note) << "\n";
        }
        oss << "\nРезультаты доступны через EXPORT CSV";
        return oss.str();
    }

    std::string exec_export(const ExportQuery& q) {
        if (last_benchmark_.size() == 0) {
            return "Ошибка: нет данных бенчмарка. Сначала выполните BENCHMARK.";
        }
        if (export_benchmark_csv(last_benchmark_, q.path)) {
            return "Результаты сохранены в '" + q.path + "' (" +
                   std::to_string(last_benchmark_.size()) + " строк)";
        }
        return "Ошибка: не удалось открыть файл '" + q.path + "' для записи";
    }

    std::string exec_help() const {
        return
            "Команды графового интерфейса:\n"
            "\n"
            "  CREATE GRAPH                         - создать пустой граф\n"
            "  DROP GRAPH                           - удалить текущий граф\n"
            "  ADD VERTEX [имя]                     - добавить вершину\n"
            "  ADD EDGE <от> <до> <вес>             - добавить ребро\n"
            "  REMOVE VERTEX <имя>                - удалить вершину и её рёбра\n"
            "  REMOVE EDGE <от> <до>              - удалить ребро\n"
            "  SHOW GRAPH | VERTICES | EDGES        - просмотр данных\n"
            "\n"
            "  GENERATE <топология> <n> [m]\n"
            "           [SEED s] [WEIGHTS ASC|DESC|RANDOM min max]\n"
            "      топологии: RANDOM, RING, PATH, STAR, COMPLETE, TREE\n"
            "\n"
            "  PATH BFS|DFS|DIJKSTRA|BELLMAN <от> [до]\n"
            "                                       - кратчайший путь или обход\n"
            "  SPANNING KRUSKAL|PRIM                - минимальный остов\n"
            "  COMPONENTS                           - компоненты связности\n"
            "\n"
            "  BENCHMARK SHORTEST <от>              - бенчмарк путей от вершины\n"
            "  BENCHMARK MST                        - бенчмарк остова\n"
            "  BENCHMARK SUITE <n> <m> [seed]       - полный бенчмарк (3 случая весов)\n"
            "  EXPORT CSV <файл>                    - выгрузка бенчмарка в CSV\n"
            "\n"
            "  HELP                                 - эта справка\n"
            "  VIEW [файл]                          - визуализация графа в браузере\n"
            "  EXIT | QUIT                          - выход";
    }

    std::string exec_view(const ViewQuery& q) {
        if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";
        
        const std::string path = q.path.empty() ? "assets/graph_view.html" : q.path;
        if (!GraphViewer::save_html(graph_, path, "Граф")) {
            return "Ошибка: не удалось сохранить файл '" + path + "'";
        }
        
        if (q.path.empty()) {
            GraphViewer::open_in_browser(path);
            return "Граф сохранён в '" + path + "' и открыт в браузере";
        }
        return "Граф сохранён в '" + path + "'";
    }

    std::string exec_remove_vertex(const RemoveVertexQuery& q) {
        if (graph_.vertex_count() == 0) return "Ошибка: граф пуст";
        if (!graph_.has_vertex(q.name)) {
            return "Ошибка: вершина '" + q.name + "' не найдена";
        }
        const size_t id = graph_.vertex_id(q.name);
        graph_.remove_vertex(id);
        return "Удалена вершина '" + q.name + "'";
    }

    std::string exec_remove_edge(const RemoveEdgeQuery& q) {
        if (!graph_.has_vertex(q.from)) {
            return "Ошибка: вершина '" + q.from + "' не найдена";
        }
        if (!graph_.has_vertex(q.to)) {
            return "Ошибка: вершина '" + q.to + "' не найдена";
        }
        const size_t from_id = graph_.vertex_id(q.from);
        const size_t to_id = graph_.vertex_id(q.to);
        if (graph_.remove_edge(from_id, to_id)) {
            return "Удалено ребро '" + q.from + "' - '" + q.to + "'";
        }
        return "Ошибка: ребро '" + q.from + "' - '" + q.to + "' не найдено";
    }

    std::string exec_drop() {
        const bool was_empty = (graph_.vertex_count() == 0);
        graph_ = Graph();
        last_benchmark_ = DynamicArray<BenchmarkResult>();
        if (was_empty) {
            return "Граф уже был пуст";
        }
        return "Граф удалён";
    }

public:
    // Выполнить команду, вернуть форматированный результат
    std::string execute(const GraphQuery& q) {
        return std::visit(Overload{
            [&](const CreateGraphQuery&)    { return exec_create(); },
            [&](const DropGraphQuery&)        { return exec_drop(); },
            [&](const AddVertexQuery& aq)   { return exec_add_vertex(aq); },
            [&](const AddEdgeQuery& aq)     { return exec_add_edge(aq); },
            [&](const ShowGraphQuery&)      { return exec_show_graph(); },
            [&](const ShowVerticesQuery&)   { return exec_show_vertices(); },
            [&](const ShowEdgesQuery&)      { return exec_show_edges(); },
            [&](const GenerateQuery& gq)    { return exec_generate(gq); },
            [&](const PathQuery& pq)        { return exec_path(pq); },
            [&](const SpanningQuery& sq)    { return exec_spanning(sq); },
            [&](const ComponentsQuery&)     { return exec_components(); },
            [&](const BenchmarkQuery& bq)   { return exec_benchmark(bq); },
            [&](const ExportQuery& eq)      { return exec_export(eq); },
            [&](const HelpQuery&)           { return exec_help(); },
            [&](const ViewQuery& vq)          { return exec_view(vq); },
            [&](const RemoveVertexQuery& rq)  { return exec_remove_vertex(rq); },
            [&](const RemoveEdgeQuery& rq)    { return exec_remove_edge(rq); },
            [&](const ExitQuery&)           { exit_requested_ = true; return std::string("Выход..."); },
        }, q);
    }

    bool should_exit() const { return exit_requested_; }
    const Graph& graph() const { return graph_; }
};