#pragma once

#include <string>
#include "../../lib/DynamicArray.h"
#include "../Graph.h"

/*
Результат построения минимального остова (или остовного леса).

Для несвязного графа алгоритмы строят минимальный остовный ЛЕС:
по одному остову на каждую компоненту связности. Флаг connected
показывает, является ли результат единым деревом.

Инвариант леса: число рёбер = V - число_компонент.
Для связного графа: рёбер ровно V-1.

ВАЖНО: в отличие от кратчайших путей, остовные алгоритмы
корректно работают с отрицательными весами.
*/


struct MstResult {
    std::string method;
    double total_weight = 0.0;

    DynamicArray<GraphEdge> edges;   // рёбра остова в порядке добавления алгоритмом
    size_t vertex_count = 0;
    bool connected = true;           // false -> это остовный лес
    double elapsed_ms = 0.0;         // заполняется слоем бенчмарков

    size_t edge_count() const { return edges.size(); }

    size_t component_count() const {
        if (vertex_count == 0) return 0;
        return vertex_count - edges.size();
    }
};