#pragma once

#include "../lib/DynamicArray.h"
#include "../lib/Exceptions.h"

/*
Система непересекающихся множеств (Disjoint Set Union).

Используется в алгоритме Крускала для проверки, лежат ли
концы ребра в одной компоненте связности.

Оптимизации:
  - path compression в find
  - union by rank в union_sets

Амортизированная сложность: O(α(N)) на операцию,
где α — обратная функция Аккермана (практически константа).
*/


class DSU {
private:
    DynamicArray<size_t> parent_;
    DynamicArray<size_t> rank_;

public:
    // Создание N одноэлементных множеств {0}, {1}, ..., {N-1}
    explicit DSU(size_t n = 0) {
        for (size_t i = 0; i < n; ++i) {
            parent_.push_back(i);
            rank_.push_back(static_cast<size_t>(0));
        }
    }

    // Добавить новый элемент как отдельное множество
    size_t make_set() {
        const size_t id = parent_.size();
        parent_.push_back(id);
        rank_.push_back(static_cast<size_t>(0));
        return id;
    }

    // Найти представителя множества (с path compression)
    size_t find(size_t x) {
        if (x >= parent_.size()) {
            throw IndexOutOfRange(__func__);
        }
        if (parent_[x] != x) {
            parent_[x] = find(parent_[x]);
        }
        return parent_[x];
    }

    // Принадлежат ли элементы одному множеству
    bool connected(size_t a, size_t b) {
        return find(a) == find(b);
    }

    // Объединить множества. false, если они уже были одним множеством.
    bool union_sets(size_t a, size_t b) {
        const size_t ra = find(a);
        const size_t rb = find(b);
        if (ra == rb) return false;

        if (rank_[ra] < rank_[rb]) {
            parent_[ra] = rb;
        } else if (rank_[ra] > rank_[rb]) {
            parent_[rb] = ra;
        } else {
            parent_[rb] = ra;
            ++rank_[ra];
        }
        return true;
    }

    size_t size() const { return parent_.size(); }
};