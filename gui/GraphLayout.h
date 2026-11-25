#pragma once

#include <cmath>
#include <random>
#include <string>
#include "../lib/DynamicArray.h"
#include "../lib/HashTable.h"

/*
Сила-направленная раскладка графа.

Позиции хранятся по ИМЕНАМ вершин, а не по индексам:
удаление вершины сдвигает индексы, но имена стабильны,
поэтому раскладка не «прыгает» при редактировании графа.

Симуляция «остывает» (alpha), чтобы граф перестал дрожать
после укладки. Любое изменение графа снова нагревает её.
*/


struct LayoutNode {
    std::string name;
    float x = 0.0f, y = 0.0f;
    float vx = 0.0f, vy = 0.0f;
    bool pinned = false;

    LayoutNode() = default;
    LayoutNode(const std::string& n, float px, float py)
        : name(n), x(px), y(py) {}
};

struct LayoutEdge {
    size_t from = 0, to = 0;
    std::string weight;
    std::string from_name, to_name;

    LayoutEdge() = default;
    LayoutEdge(size_t f, size_t t, const std::string& w,
               const std::string& fn, const std::string& tn)
        : from(f), to(t), weight(w), from_name(fn), to_name(tn) {}
};

class GraphLayout {
private:
    DynamicArray<LayoutNode> nodes_;
    DynamicArray<LayoutEdge> edges_;
    HashTable<std::string, size_t> index_;
    std::mt19937 rng_{12345};

    float alpha_ = 1.0f;              // «температура» симуляции
    size_t version_seen_ = static_cast<size_t>(-1);

    static constexpr float REPULSION   = 22000.0f;
    static constexpr float ATTRACTION  = 0.020f;
    static constexpr float IDEAL_LEN   = 130.0f;
    static constexpr float CENTER_PULL = 0.0016f;
    static constexpr float DAMPING     = 0.86f;
    static constexpr float MAX_SPEED   = 14.0f;
    static constexpr size_t QUAD_LIMIT = 400;   // выше — отказ от O(V²)

    float rand_offset(float range) {
        return (static_cast<float>(rng_() % 10000) / 10000.0f - 0.5f) * 2.0f * range;
    }

public:
    // Синхронизация с текущим состоянием графа.
    // Сохраняет позиции существующих вершин, инициализирует новые.
    template <typename GraphT>
    void sync(const GraphT& g, size_t version) {
        if (version == version_seen_) return;
        version_seen_ = version;

        DynamicArray<LayoutNode> fresh;
        HashTable<std::string, size_t> fresh_index;

        for (size_t i = 0; i < g.vertex_count(); ++i) {
            const std::string name = g.vertex_name(i);
            auto old = index_.Get(name);
            if (old.has_value()) {
                // Вершина уже была — сохраняем её позицию
                const LayoutNode& prev = nodes_[old.value()];
                fresh.push_back(LayoutNode(name, prev.x, prev.y));
            } else {
                // Новая вершина: ставим рядом с уже размещёнными соседями
                float px = rand_offset(180.0f);
                float py = rand_offset(180.0f);
                const auto& nb = g.neighbors(i);
                size_t placed = 0;
                float sx = 0.0f, sy = 0.0f;
                for (size_t k = 0; k < nb.size(); ++k) {
                    auto pos = index_.Get(g.vertex_name(nb[k].to));
                    if (pos.has_value()) {
                        sx += nodes_[pos.value()].x;
                        sy += nodes_[pos.value()].y;
                        ++placed;
                    }
                }
                if (placed > 0) {
                    px = sx / static_cast<float>(placed) + rand_offset(60.0f);
                    py = sy / static_cast<float>(placed) + rand_offset(60.0f);
                }
                fresh.push_back(LayoutNode(name, px, py));
            }
            fresh_index.Set(name, fresh.size() - 1);
        }

        nodes_ = std::move(fresh);
        index_ = std::move(fresh_index);

        edges_ = DynamicArray<LayoutEdge>();
        const auto es = g.edges();
        for (size_t i = 0; i < es.size(); ++i) {
            const std::string a = g.vertex_name(es[i].from);
            const std::string b = g.vertex_name(es[i].to);
            edges_.push_back(LayoutEdge(es[i].from, es[i].to,
                                        es[i].weight.to_string(), a, b));
        }

        alpha_ = 1.0f;   // нагреваем: граф изменился
    }

    void reheat() { alpha_ = 1.0f; }

    void scatter() {
        for (size_t i = 0; i < nodes_.size(); ++i) {
            nodes_[i].x = rand_offset(320.0f);
            nodes_[i].y = rand_offset(320.0f);
            nodes_[i].vx = 0.0f;
            nodes_[i].vy = 0.0f;
        }
        alpha_ = 1.0f;
    }

    // Один шаг симуляции
    void step() {
        const size_t n = nodes_.size();
        if (n == 0 || alpha_ < 0.008f) return;

        // Отталкивание. Для больших графов O(V²) отключаем —
        // остаётся притяжение по рёбрам и центрирование.
        if (n <= QUAD_LIMIT) {
            for (size_t i = 0; i < n; ++i) {
                for (size_t j = i + 1; j < n; ++j) {
                    float dx = nodes_[j].x - nodes_[i].x;
                    float dy = nodes_[j].y - nodes_[i].y;
                    float d2 = dx * dx + dy * dy;
                    if (d2 < 1.0f) { d2 = 1.0f; dx = rand_offset(1.0f); dy = rand_offset(1.0f); }
                    const float d = std::sqrt(d2);
                    const float f = (REPULSION / d2) * alpha_;
                    const float fx = f * dx / d;
                    const float fy = f * dy / d;
                    nodes_[i].vx -= fx; nodes_[i].vy -= fy;
                    nodes_[j].vx += fx; nodes_[j].vy += fy;
                }
            }
        }

        // Притяжение по рёбрам
        for (size_t k = 0; k < edges_.size(); ++k) {
            const LayoutEdge& e = edges_[k];
            if (e.from >= n || e.to >= n || e.from == e.to) continue;
            LayoutNode& a = nodes_[e.from];
            LayoutNode& b = nodes_[e.to];
            const float dx = b.x - a.x;
            const float dy = b.y - a.y;
            const float d = std::sqrt(dx * dx + dy * dy) + 0.001f;
            const float f = ATTRACTION * (d - IDEAL_LEN) * alpha_;
            const float fx = f * dx / d;
            const float fy = f * dy / d;
            a.vx += fx; a.vy += fy;
            b.vx -= fx; b.vy -= fy;
        }

        // Интегрирование + слабое центрирование + затухание
        for (size_t i = 0; i < n; ++i) {
            LayoutNode& v = nodes_[i];
            if (v.pinned) { v.vx = 0.0f; v.vy = 0.0f; continue; }

            v.vx -= v.x * CENTER_PULL * alpha_;
            v.vy -= v.y * CENTER_PULL * alpha_;

            v.vx *= DAMPING;
            v.vy *= DAMPING;

            const float sp = std::sqrt(v.vx * v.vx + v.vy * v.vy);
            if (sp > MAX_SPEED) { v.vx *= MAX_SPEED / sp; v.vy *= MAX_SPEED / sp; }

            v.x += v.vx;
            v.y += v.vy;
        }

        alpha_ *= 0.994f;
    }

    size_t node_count() const { return nodes_.size(); }
    size_t edge_count() const { return edges_.size(); }
    const LayoutNode& node(size_t i) const { return nodes_[i]; }
    LayoutNode& node(size_t i) { return nodes_[i]; }
    const LayoutEdge& edge(size_t i) const { return edges_[i]; }

    Option<size_t> find(const std::string& name) const { return index_.Get(name); }

    // Ближайшая вершина к точке в мировых координатах
    Option<size_t> pick(float wx, float wy, float radius) const {
        Option<size_t> best = None<size_t>();
        float best_d2 = radius * radius;
        for (size_t i = 0; i < nodes_.size(); ++i) {
            const float dx = nodes_[i].x - wx;
            const float dy = nodes_[i].y - wy;
            const float d2 = dx * dx + dy * dy;
            if (d2 <= best_d2) { best_d2 = d2; best = Some<size_t>(i); }
        }
        return best;
    }
};