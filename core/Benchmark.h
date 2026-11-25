#pragma once

#include <chrono>
#include <fstream>
#include <string>
#include "Graph.h"
#include "GraphGenerator.h"
#include "algorithms/PathResult.h"
#include "algorithms/BFS.h"
#include "algorithms/Dijkstra.h"
#include "algorithms/BellmanFord.h"
#include "algorithms/Kruskal.h"
#include "algorithms/Prim.h"
#include "../lib/DynamicArray.h"

/*
Слой бенчмарков: измерение времени, сравнение алгоритмов,
выгрузка результатов в CSV.

Реализует требования методички:
  - функция измерения времени выполнения алгоритма
  - функция сравнения алгоритмов на одних входных данных
  - выгрузка данных в открытом формате (CSV) для графиков

Три случая данных из сноски 1 методички реализованы через
WeightOrder::Asc / Desc / Random — распределение весов.
*/


struct BenchmarkResult {
    std::string algorithm;
    std::string topology;
    std::string weight_order;
    size_t vertices = 0;
    size_t edges = 0;
    double elapsed_ms = 0.0;
    double result_value = 0.0;   // агрегированная метрика результата
    bool success = true;
    std::string note;            // описание ошибки, если успех == false
};

// Измерение времени выполнения функции в миллисекундах
template <typename Func>
inline double measure_ms(Func&& func) {
    const auto start = std::chrono::high_resolution_clock::now();
    func();
    const auto end = std::chrono::high_resolution_clock::now();
    return std::chrono::duration<double, std::milli>(end - start).count();
}

inline std::string topology_to_string(GraphTopology t) {
    switch (t) {
        case GraphTopology::Random:     return "random";
        case GraphTopology::Ring:       return "ring";
        case GraphTopology::Path:       return "path";
        case GraphTopology::Star:       return "star";
        case GraphTopology::Complete:   return "complete";
        case GraphTopology::RandomTree: return "random_tree";
    }
    return "unknown";
}

inline std::string order_to_string(WeightOrder o) {
    switch (o) {
        case WeightOrder::Asc:    return "asc";
        case WeightOrder::Desc:   return "desc";
        case WeightOrder::Random: return "random";
    }
    return "unknown";
}

// Сумма расстояний до всех достижимых вершин — агрегированная метрика
inline double sum_distances(const PathResult& pr) {
    double sum = 0.0;
    for (size_t i = 0; i < pr.dist.size(); ++i) {
        if (pr.dist[i].has_value()) sum += pr.dist[i].value();
    }
    return sum;
}

// =========================================================
// Бенчмарк кратчайших путей: Дейкстра, Форд-Беллман, BFS
// на одном графе — прямое сравнение по времени.
// =========================================================
inline DynamicArray<BenchmarkResult> benchmark_shortest_paths(
        const Graph& g, size_t source,
        const std::string& topology, const std::string& weight_order) {

    DynamicArray<BenchmarkResult> results;

    // Дейкстра
    {
        BenchmarkResult r;
        r.algorithm = "DIJKSTRA";
        r.topology = topology;
        r.weight_order = weight_order;
        r.vertices = g.vertex_count();
        r.edges = g.edge_count();
        try {
            PathResult pr;
            r.elapsed_ms = measure_ms([&]() { pr = dijkstra(g, source); });
            r.result_value = sum_distances(pr);
            r.success = true;
        } catch (const GraphException& e) {
            r.success = false;
            r.note = e.what();
        }
        results.push_back(r);
    }

    // Форд-Беллман
    {
        BenchmarkResult r;
        r.algorithm = "BELLMAN-FORD";
        r.topology = topology;
        r.weight_order = weight_order;
        r.vertices = g.vertex_count();
        r.edges = g.edge_count();
        try {
            PathResult pr;
            r.elapsed_ms = measure_ms([&]() { pr = bellman_ford(g, source); });
            r.result_value = sum_distances(pr);
            r.success = true;
        } catch (const GraphException& e) {
            r.success = false;
            r.note = e.what();
        }
        results.push_back(r);
    }

    // BFS
    {
        BenchmarkResult r;
        r.algorithm = "BFS";
        r.topology = topology;
        r.weight_order = weight_order;
        r.vertices = g.vertex_count();
        r.edges = g.edge_count();
        try {
            PathResult pr;
            r.elapsed_ms = measure_ms([&]() { pr = bfs(g, source); });
            r.result_value = sum_distances(pr);
            r.success = true;
        } catch (const GraphException& e) {
            r.success = false;
            r.note = e.what();
        }
        results.push_back(r);
    }

    return results;
}

// Бенчмарк минимального остова: Крускал, Прим
// на одном графе — прямое сравнение по времени.
inline DynamicArray<BenchmarkResult> benchmark_mst(
        const Graph& g,
        const std::string& topology, const std::string& weight_order) {

    DynamicArray<BenchmarkResult> results;

    // Крускал
    {
        BenchmarkResult r;
        r.algorithm = "KRUSKAL";
        r.topology = topology;
        r.weight_order = weight_order;
        r.vertices = g.vertex_count();
        r.edges = g.edge_count();
        try {
            MstResult mr;
            r.elapsed_ms = measure_ms([&]() { mr = kruskal(g); });
            r.result_value = mr.total_weight;
            r.success = true;
        } catch (const GraphException& e) {
            r.success = false;
            r.note = e.what();
        }
        results.push_back(r);
    }

    // Прим
    {
        BenchmarkResult r;
        r.algorithm = "PRIM";
        r.topology = topology;
        r.weight_order = weight_order;
        r.vertices = g.vertex_count();
        r.edges = g.edge_count();
        try {
            MstResult mr;
            r.elapsed_ms = measure_ms([&]() { mr = prim(g); });
            r.result_value = mr.total_weight;
            r.success = true;
        } catch (const GraphException& e) {
            r.success = false;
            r.note = e.what();
        }
        results.push_back(r);
    }

    return results;
}

// Полный набор бенчмарков: все топологии + все распределения
// весов + все алгоритмы. Даёт материал для графиков и
// закрывает требование о трёх случаях данных.
inline DynamicArray<BenchmarkResult> benchmark_suite(
        size_t vertices, size_t edges, unsigned seed) {

    DynamicArray<BenchmarkResult> all;

    const GraphTopology topos[] = {
        GraphTopology::Random, GraphTopology::Ring, GraphTopology::Path,
        GraphTopology::Star, GraphTopology::RandomTree
    };
    const WeightOrder orders[] = {
        WeightOrder::Asc, WeightOrder::Desc, WeightOrder::Random
    };

    for (size_t t = 0; t < 5; ++t) {
        for (size_t o = 0; o < 3; ++o) {
            GraphGeneratorConfig cfg;
            cfg.vertices = vertices;
            cfg.edges = edges;
            cfg.min_weight = 1;
            cfg.max_weight = 100;
            cfg.order = orders[o];
            cfg.seed = seed;
            Graph g = GraphGenerator::generate(topos[t], cfg);

            const std::string topo_name = topology_to_string(topos[t]);
            const std::string order_name = order_to_string(orders[o]);

            const auto sp = benchmark_shortest_paths(g, 0, topo_name, order_name);
            for (size_t i = 0; i < sp.size(); ++i) all.push_back(sp[i]);

            const auto mst = benchmark_mst(g, topo_name, order_name);
            for (size_t i = 0; i < mst.size(); ++i) all.push_back(mst[i]);
        }
    }

    return all;
}

// Выгрузка результатов в CSV для построения графиков.
inline bool export_benchmark_csv(const DynamicArray<BenchmarkResult>& results,
                                 const std::string& path) {
    std::ofstream f(path);
    if (!f.is_open()) return false;

    f << "algorithm,topology,weight_order,vertices,edges,elapsed_ms,result_value,success,note\n";
    for (size_t i = 0; i < results.size(); ++i) {
        const BenchmarkResult& r = results[i];
        f << r.algorithm << ","
          << r.topology << ","
          << r.weight_order << ","
          << r.vertices << ","
          << r.edges << ","
          << r.elapsed_ms << ","
          << r.result_value << ","
          << (r.success ? "1" : "0") << ","
          << "\"" << r.note << "\"\n";
    }
    return true;
}