#include <iostream>
#include <vector>
#include <algorithm>
#include "../core/PriorityQueue.h"
#include "../lib/Exceptions.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

TEST(pq_empty) {
    PriorityQueue<int, int> pq;
    ASSERT(pq.empty());
    ASSERT(pq.size() == 0);
    ASSERT(!pq.peek().has_value());
    ASSERT(!pq.dequeue().has_value());
}

TEST(pq_single_element) {
    PriorityQueue<int, int> pq;
    pq.enqueue(42, 10);

    ASSERT(!pq.empty());
    ASSERT(pq.size() == 1);
    ASSERT(pq.peek().value() == 42);

    auto v = pq.dequeue();
    ASSERT(v.has_value() && v.value() == 42);
    ASSERT(pq.empty());
}

TEST(pq_min_property) {
    PriorityQueue<int, int> pq;
    pq.enqueue(10, 5);
    pq.enqueue(20, 1);
    pq.enqueue(30, 10);
    pq.enqueue(40, 3);

    // Извлечение должно идти в порядке возрастания приоритета
    ASSERT(pq.dequeue().value() == 20);  // priority 1
    ASSERT(pq.dequeue().value() == 40);  // priority 3
    ASSERT(pq.dequeue().value() == 10);  // priority 5
    ASSERT(pq.dequeue().value() == 30);  // priority 10
    ASSERT(pq.empty());
}

TEST(pq_with_same_priorities) {
    PriorityQueue<int, int> pq;
    pq.enqueue(1, 5);
    pq.enqueue(2, 5);
    pq.enqueue(3, 5);

    ASSERT(pq.size() == 3);
    // Все должны извлечься (порядок при равных приоритетах не гарантирован)
    std::vector<int> extracted;
    while (!pq.empty()) {
        extracted.push_back(pq.dequeue().value());
    }
    ASSERT(extracted.size() == 3);
    std::sort(extracted.begin(), extracted.end());
    ASSERT(extracted == std::vector<int>({1, 2, 3}));
}

TEST(pq_double_priorities) {
    PriorityQueue<int, double> pq;
    pq.enqueue(1, 1.5);
    pq.enqueue(2, 0.5);
    pq.enqueue(3, 2.5);

    ASSERT(pq.dequeue().value() == 2);
    ASSERT(pq.dequeue().value() == 1);
    ASSERT(pq.dequeue().value() == 3);
}

TEST(pq_large_dataset) {
    PriorityQueue<int, int> pq;
    const int N = 1000;

    // Вставляем в обратном порядке
    for (int i = N - 1; i >= 0; --i) {
        pq.enqueue(i, i);
    }

    ASSERT(pq.size() == static_cast<size_t>(N));

    // Должны извлекаться в порядке 0, 1, 2, ...
    for (int i = 0; i < N; ++i) {
        auto v = pq.dequeue();
        ASSERT(v.has_value() && v.value() == i);
    }
    ASSERT(pq.empty());
}

TEST(pq_random_order) {
    PriorityQueue<int, int> pq;
    std::vector<int> priorities = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};

    for (int p : priorities) {
        pq.enqueue(p * 10, p);
    }

    // Ожидаемый порядок извлечения: по возрастанию приоритета
    std::vector<int> expected = priorities;
    std::sort(expected.begin(), expected.end());

    for (int p : expected) {
        auto v = pq.dequeue();
        ASSERT(v.has_value() && v.value() == p * 10);
    }
}

TEST(pq_clear) {
    PriorityQueue<int, int> pq;
    pq.enqueue(1, 1);
    pq.enqueue(2, 2);
    pq.enqueue(3, 3);

    pq.clear();
    ASSERT(pq.empty());
    ASSERT(pq.size() == 0);
}

TEST(pq_peek_does_not_modify) {
    PriorityQueue<int, int> pq;
    pq.enqueue(10, 5);
    pq.enqueue(20, 1);

    ASSERT(pq.peek().value() == 20);
    ASSERT(pq.peek().value() == 20);  // повторный peek
    ASSERT(pq.size() == 2);           // размер не изменился
}

int main() {
    std::cout << "=== Тесты PriorityQueue ===\n" << std::endl;

    RUN_TEST(pq_empty);
    RUN_TEST(pq_single_element);
    RUN_TEST(pq_min_property);
    RUN_TEST(pq_with_same_priorities);
    RUN_TEST(pq_double_priorities);
    RUN_TEST(pq_large_dataset);
    RUN_TEST(pq_random_order);
    RUN_TEST(pq_clear);
    RUN_TEST(pq_peek_does_not_modify);

    std::cout << "\n=== Все тесты PriorityQueue пройдены ===\n";
    return 0;
}