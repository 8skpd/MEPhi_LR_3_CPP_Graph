#pragma once

#include <utility>
#include "../lib/DynamicArray.h"
#include "../lib/Option.h"
#include "../lib/Exceptions.h"


/*
Бинарная min-куча на DynamicArray.

Хранит пары (элемент, приоритет). Извлечение минимума
возвращает элемент с наименьшим приоритетом.

Используется в алгоритмах Дейкстры и Прима.

ВАЖНО: эта реализация НЕ поддерживает decrease_key.
Для Дейкстры это означает, что при обновлении расстояния
мы просто добавляем новую пару в очередь, а устаревшие
записи игнорируем при извлечении (ленивое удаление).
Это стандартный подход, дающий O((V+E) log V) вместо O(E log V).
*/


template <typename TElement, typename TPriority>
class PriorityQueue {
private:
    struct Entry {
        TElement element;
        TPriority priority;

        Entry() = default;
        Entry(const TElement& e, const TPriority& p) : element(e), priority(p) {}
    };

    DynamicArray<Entry> heap_;

    // Индексы в бинарной куче:
    // parent(i)      = (i - 1) / 2
    // left_child(i)  = 2*i + 1
    // right_child(i) = 2*i + 2

    static size_t parent(size_t i) { return (i - 1) / 2; }
    static size_t left(size_t i)   { return 2 * i + 1; }
    static size_t right(size_t i)  { return 2 * i + 2; }

    void sift_up(size_t i) {
        while (i > 0 && heap_[parent(i)].priority > heap_[i].priority) {
            std::swap(heap_[parent(i)], heap_[i]);
            i = parent(i);
        }
    }

    void sift_down(size_t i) {
        const size_t n = heap_.size();
        while (true) {
            size_t smallest = i;
            const size_t l = left(i);
            const size_t r = right(i);

            if (l < n && heap_[l].priority < heap_[smallest].priority) smallest = l;
            if (r < n && heap_[r].priority < heap_[smallest].priority) smallest = r;

            if (smallest == i) break;
            std::swap(heap_[i], heap_[smallest]);
            i = smallest;
        }
    }

public:
    PriorityQueue() = default;

    // Добавление элемента с приоритетом
    void enqueue(const TElement& element, const TPriority& priority) {
        heap_.push_back(Entry(element, priority));
        sift_up(heap_.size() - 1);
    }

    // Извлечение элемента с минимальным приоритетом
    Option<TElement> dequeue() {
        if (heap_.size() == 0) return None<TElement>();

        const TElement result = heap_[0].element;
        const size_t last = heap_.size() - 1;

        if (last > 0) {
            heap_[0] = heap_[last];
            heap_.pop_back();
            sift_down(0);
        } else {
            heap_.pop_back();
        }
        return Some<TElement>(result);
    }

    // Просмотр минимального элемента без извлечения
    Option<TElement> peek() const {
        if (heap_.size() == 0) return None<TElement>();
        return Some<TElement>(heap_[0].element);
    }

    bool empty() const { return heap_.size() == 0; }
    size_t size() const { return heap_.size(); }

    void clear() {
        heap_ = DynamicArray<Entry>();
    }
};