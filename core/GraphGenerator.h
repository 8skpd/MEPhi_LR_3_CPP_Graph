#pragma once

#include <random>
#include <algorithm>
#include <utility>
#include "Graph.h"
#include "../lib/DynamicArray.h"
#include "../lib/HashTable.h"
#include "../lib/Value.h"

/*
Генератор графов заданной топологии и размера.

Поддерживаются топологии: случайный граф (модель Эрдёша-Реньи),
кольцо, путь, звезда, полный граф, случайное дерево.

Распределение весов управляется через WeightOrder:
  - Asc:    веса назначаются по возрастанию вдоль порядка генерации
  - Desc:   веса назначаются по убыванию
  - Random: веса выбираются случайно из диапазона

Это закрывает требование методички о трёх случаях данных
(отсортировано, обратно, случайно) для бенчмарков.
*/


enum class GraphTopology {
    Random,       // модель Эрдёша-Реньи
    Ring,         // цикл
    Path,         // цепочка
    Star,         // одна центральная вершина
    Complete,     // полный граф
    RandomTree    // случайное дерево
};

enum class WeightOrder {
    Asc,
    Desc,
    Random
};

struct GraphGeneratorConfig {
    size_t      vertices   = 10;
    size_t      edges      = 15;   // целевое число рёбер (для Random)
    int         min_weight = 1;
    int         max_weight = 10;
    WeightOrder order      = WeightOrder::Random;
    unsigned    seed       = 0;
};

class GraphGenerator {
private:
    // Назначение весов рёбрам согласно заданному распределению
    static void assign_weights(DynamicArray<GraphEdge>& edges,
                               int min_w, int max_w,
                               WeightOrder order,
                               std::mt19937& rng) {
        const size_t m = edges.size();
        if (m == 0) return;

        if (order == WeightOrder::Random) {
            std::uniform_int_distribution<int> dist(min_w, max_w);
            for (size_t i = 0; i < m; ++i) {
                edges[i].weight = Value(dist(rng));
            }
        } else {
            for (size_t i = 0; i < m; ++i) {
                const double t = (m == 1) ? 0.0
                    : static_cast<double>(i) / static_cast<double>(m - 1);
                int w;
                if (order == WeightOrder::Asc) {
                    w = min_w + static_cast<int>(t * (max_w - min_w));
                } else {
                    w = max_w - static_cast<int>(t * (max_w - min_w));
                }
                edges[i].weight = Value(w);
            }
        }
    }

    // Построение графа из списка рёбер
    static Graph build_graph(size_t n, const DynamicArray<GraphEdge>& edges) {
        Graph g(n);
        for (size_t i = 0; i < edges.size(); ++i) {
            g.add_edge(edges[i].from, edges[i].to, edges[i].weight);
        }
        return g;
    }

    // Добавление ребра без дубликатов (для неориентированного графа)
    static bool try_add_edge(DynamicArray<GraphEdge>& edges,
                             HashTable<size_t, bool>& seen,
                             size_t a, size_t b, size_t n) {
        if (a == b) return false;
        const size_t lo = std::min(a, b);
        const size_t hi = std::max(a, b);
        const size_t key = lo * n + hi;
        if (seen.ContainsKey(key)) return false;
        seen.Add(key, true);
        edges.push_back(GraphEdge(lo, hi, Value(0)));
        return true;
    }

public:
    // Случайный граф (модель Эрдёша-Реньи)
    static Graph generate_random(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        const size_t max_edges = (n <= 1) ? 0 : n * (n - 1) / 2;
        const size_t target = std::min(cfg.edges, max_edges);

        DynamicArray<GraphEdge> edges;
        std::mt19937 rng(cfg.seed);

        if (target > max_edges / 2) {
            // Плотный граф: генерируем все рёбра, перемешиваем, берём первые
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    edges.push_back(GraphEdge(i, j, Value(0)));
                }
            }
            // Фишер-Йетс
            for (size_t i = edges.size(); i > 1; --i) {
                const size_t j = rng() % i;
                std::swap(edges[i - 1], edges[j]);
            }
            while (edges.size() > target) edges.pop_back();
        } else {
            // Разреженный граф: случайные пары без дубликатов
            HashTable<size_t, bool> seen;
            while (edges.size() < target) {
                const size_t a = rng() % n;
                const size_t b = rng() % n;
                try_add_edge(edges, seen, a, b, n);
            }
        }

        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Кольцо (цикл)
    static Graph generate_ring(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        DynamicArray<GraphEdge> edges;
        HashTable<size_t, bool> seen;
        for (size_t i = 0; i < n; ++i) {
            try_add_edge(edges, seen, i, (i + 1) % n, n);
        }
        std::mt19937 rng(cfg.seed);
        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Путь (цепочка)
    static Graph generate_path(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        DynamicArray<GraphEdge> edges;
        for (size_t i = 0; i + 1 < n; ++i) {
            edges.push_back(GraphEdge(i, i + 1, Value(0)));
        }
        std::mt19937 rng(cfg.seed);
        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Звезда (вершина 0 соединена со всеми)
    static Graph generate_star(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        DynamicArray<GraphEdge> edges;
        for (size_t i = 1; i < n; ++i) {
            edges.push_back(GraphEdge(0, i, Value(0)));
        }
        std::mt19937 rng(cfg.seed);
        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Полный граф
    static Graph generate_complete(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        DynamicArray<GraphEdge> edges;
        for (size_t i = 0; i < n; ++i) {
            for (size_t j = i + 1; j < n; ++j) {
                edges.push_back(GraphEdge(i, j, Value(0)));
            }
        }
        std::mt19937 rng(cfg.seed);
        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Случайное дерево (каждая новая вершина соединяется со случайной существующей)
    static Graph generate_random_tree(const GraphGeneratorConfig& cfg) {
        const size_t n = cfg.vertices;
        DynamicArray<GraphEdge> edges;
        std::mt19937 rng(cfg.seed);
        for (size_t i = 1; i < n; ++i) {
            const size_t parent = rng() % i;
            edges.push_back(GraphEdge(parent, i, Value(0)));
        }
        assign_weights(edges, cfg.min_weight, cfg.max_weight, cfg.order, rng);
        return build_graph(n, edges);
    }

    // Общая точка входа
    static Graph generate(GraphTopology topo, const GraphGeneratorConfig& cfg) {
        switch (topo) {
            case GraphTopology::Random:     return generate_random(cfg);
            case GraphTopology::Ring:       return generate_ring(cfg);
            case GraphTopology::Path:       return generate_path(cfg);
            case GraphTopology::Star:       return generate_star(cfg);
            case GraphTopology::Complete:   return generate_complete(cfg);
            case GraphTopology::RandomTree: return generate_random_tree(cfg);
        }
        return generate_random(cfg);
    }
};