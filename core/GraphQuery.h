#pragma once

#include <string>
#include <variant>
#include "Graph.h"
#include "GraphGenerator.h"
#include "../lib/Value.h"

// AST команд графового интерфейса.
// каждая команда — структура, всё вместе —
// std::variant. Исполнитель обрабатывает через std::visit.

struct CreateGraphQuery {};

struct DropGraphQuery {};

struct AddVertexQuery {
    std::string name;            // пустое имя -> автогенерация
};

struct AddEdgeQuery {
    std::string from;
    std::string to;
    Value weight;
};

struct ShowGraphQuery {};        // общая сводка: вершины, рёбра, связность
struct ShowVerticesQuery {};
struct ShowEdgesQuery {};

struct GenerateQuery {
    GraphTopology topology = GraphTopology::Random;
    size_t vertices = 10;
    size_t edges = 15;
    unsigned seed = 0;
    WeightOrder order = WeightOrder::Random;
    int min_weight = 1;
    int max_weight = 100;
};

// PATH BFS|DFS|DIJKSTRA|BELLMAN from [to]
struct PathQuery {
    std::string algorithm;
    std::string from;
    std::string to;              // пустое -> только обход (для DFS)
};

// SPANNING KRUSKAL|PRIM
struct SpanningQuery {
    std::string algorithm;
};

struct ComponentsQuery {};

// BENCHMARK SHORTEST|MST|SUITE [параметры]
struct BenchmarkQuery {
    std::string type;            // SHORTEST, MST, SUITE
    std::string from;            // для SHORTEST
    size_t vertices = 100;       // для SUITE
    size_t edges = 300;          // для SUITE
    unsigned seed = 42;          // для SUITE
};

// EXPORT CSV <path>
struct ExportQuery {
    std::string path;
};

struct HelpQuery {};
struct ExitQuery {};

// VIEW [path] — визуализация графа в браузере
struct ViewQuery {
    std::string path;   // пустое -> открыть в браузере, иначе сохранить по пути
};

struct RemoveVertexQuery {
    std::string name;
};

struct RemoveEdgeQuery {
    std::string from;
    std::string to;
};


using GraphQuery = std::variant<
    CreateGraphQuery, DropGraphQuery, 
    AddVertexQuery, AddEdgeQuery,
    RemoveVertexQuery, RemoveEdgeQuery,
    ShowGraphQuery, ShowVerticesQuery, ShowEdgesQuery,
    GenerateQuery, PathQuery, SpanningQuery, ComponentsQuery,
    BenchmarkQuery, ExportQuery,
    ViewQuery,
    HelpQuery, ExitQuery
>;