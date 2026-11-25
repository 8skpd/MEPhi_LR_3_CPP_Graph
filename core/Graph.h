#pragma once

#include <string>
#include "../lib/DynamicArray.h"
#include "../lib/HashTable.h"
#include "../lib/Option.h"
#include "../lib/Value.h"
#include "../lib/Exceptions.h"

// Неориентированный взвешенный граф со списком смежности.
// Вершины идентифицируются индексами (0..N-1).

struct Edge {
    size_t to = 0;
    Value  weight;

    Edge() = default;
    Edge(size_t to, Value weight) : to(to), weight(weight) {}
};

// Ребро с обоими концами — для алгоритмов, работающих со списком рёбер
// (Форд-Беллман, Крускал), а не со списками смежности.
struct GraphEdge {
    size_t from = 0;
    size_t to   = 0;
    Value  weight;

    GraphEdge() = default;
    GraphEdge(size_t f, size_t t, Value w) : from(f), to(t), weight(w) {}
};

class Graph {
private:
    size_t vertex_count_ = 0;
    DynamicArray<DynamicArray<Edge>> adjacency_list_;
    HashTable<std::string, size_t> vertex_names_;
    DynamicArray<std::string> display_names_;

public:
    explicit Graph(size_t vertex_count = 0)
        : vertex_count_(vertex_count), adjacency_list_(vertex_count) {
        for (size_t i = 0; i < vertex_count; ++i) {
            const std::string auto_name = "v" + std::to_string(i);
            display_names_.push_back(auto_name);
            vertex_names_.Add(auto_name, i);
        }
    }

    size_t vertex_count() const { return vertex_count_; }


    size_t add_vertex() {
        size_t id = vertex_count_++;
        adjacency_list_.push_back(DynamicArray<Edge>());
        const std::string auto_name = "v" + std::to_string(id);
        display_names_.push_back(auto_name);
        vertex_names_.Add(auto_name, id);
        return id;
    }

    size_t add_vertex(const std::string& name) {
        auto existing = vertex_names_.Get(name);
        if (existing.has_value()) return existing.value();
        size_t id = add_vertex();
        vertex_names_.Remove("v" + std::to_string(id));
        display_names_[id] = name;
        vertex_names_.Add(name, id);
        return id;
    }

    std::string vertex_name(size_t id) const {
        if (id >= display_names_.size()) {
            throw IndexOutOfRange(__func__);
        }
        return display_names_[id];
    }

    size_t vertex_id(const std::string& name) const {
        auto opt = vertex_names_.Get(name);
        if (!opt.has_value()) {
            throw VertexNotFound(__func__, name);
        }
        return opt.value();
    }

    bool has_vertex(const std::string& name) const {
        return vertex_names_.ContainsKey(name);
    }

    void add_edge(size_t from, size_t to, Value weight) {
        if (from >= vertex_count_ || to >= vertex_count_) {
            throw IndexOutOfRange(__func__);
        }
        adjacency_list_[from].push_back(Edge(to, weight));
        adjacency_list_[to].push_back(Edge(from, weight));
    }

    void add_edge(const std::string& from_name, const std::string& to_name, Value weight) {
        size_t from = vertex_id(from_name);
        size_t to = vertex_id(to_name);
        add_edge(from, to, weight);
    }

    const DynamicArray<Edge>& neighbors(size_t vertex) const {
        if (vertex >= vertex_count_) {
            throw IndexOutOfRange(__func__);
        }
        return adjacency_list_[vertex];
    }

    size_t edge_count() const {
        size_t total = 0;
        for (size_t i = 0; i < vertex_count_; ++i) {
            total += adjacency_list_[i].size();
        }
        return total / 2;
    }

    bool has_edge(size_t from, size_t to) const {
        if (from >= vertex_count_ || to >= vertex_count_) return false;
        for (size_t i = 0; i < adjacency_list_[from].size(); ++i) {
            if (adjacency_list_[from][i].to == to) return true;
        }
        return false;
    }

    Value get_edge_weight(size_t from, size_t to) const {
        if (from >= vertex_count_ || to >= vertex_count_) {
            throw IndexOutOfRange(__func__);
        }
        for (size_t i = 0; i < adjacency_list_[from].size(); ++i) {
            if (adjacency_list_[from][i].to == to) {
                return adjacency_list_[from][i].weight;
            }
        }
        throw EdgeNotFound(__func__);
    }

    // Все рёбра графа, каждое ровно один раз.
    // Петли учитываются один раз (в списке смежности они хранятся дважды).
    DynamicArray<GraphEdge> edges() const {
        DynamicArray<GraphEdge> out;
        for (size_t v = 0; v < vertex_count_; ++v) {
            const DynamicArray<Edge>& nb = adjacency_list_[v];
            size_t loop_seen = 0;
            for (size_t i = 0; i < nb.size(); ++i) {
                if (nb[i].to > v) {
                    out.push_back(GraphEdge(v, nb[i].to, nb[i].weight));
                } else if (nb[i].to == v) {
                    // две записи одной петли идут подряд -> берём чётные
                    if (loop_seen % 2 == 0) {
                        out.push_back(GraphEdge(v, v, nb[i].weight));
                    }
                    ++loop_seen;
                }
            }
        }
        return out;
    }
};