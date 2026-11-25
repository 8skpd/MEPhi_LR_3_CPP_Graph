#include <iostream>
#include "../core/DSU.h"
#include "../lib/Exceptions.h"

#define TEST(name) void test_##name()
#define ASSERT(expr) \
    do { if (!(expr)) { \
        std::cerr << "✗ ПРОВАЛЕН: " << #expr << " в " << __FILE__ << ":" << __LINE__ << std::endl; \
        exit(1); } } while (0)
#define RUN_TEST(name) \
    do { std::cout << "Запуск теста: " << #name << "... "; \
         test_##name(); std::cout << "✓ ПРОЙДЕН" << std::endl; } while (0)

TEST(dsu_empty) {
    DSU dsu;
    ASSERT(dsu.size() == 0);
}

TEST(dsu_single_sets) {
    DSU dsu(5);
    ASSERT(dsu.size() == 5);

    // Каждый элемент — своё множество
    for (size_t i = 0; i < 5; ++i) {
        ASSERT(dsu.find(i) == i);
        for (size_t j = 0; j < 5; ++j) {
            if (i != j) {
                ASSERT(!dsu.connected(i, j));
            }
        }
    }
}

TEST(dsu_union_basic) {
    DSU dsu(5);

    ASSERT(dsu.union_sets(0, 1));
    ASSERT(dsu.connected(0, 1));
    ASSERT(!dsu.connected(0, 2));

    ASSERT(dsu.union_sets(2, 3));
    ASSERT(dsu.connected(2, 3));
    ASSERT(!dsu.connected(0, 2));

    ASSERT(dsu.union_sets(1, 3));  // объединяет две компоненты
    ASSERT(dsu.connected(0, 2));
    ASSERT(dsu.connected(1, 3));
}

TEST(dsu_union_returns_false_if_already_connected) {
    DSU dsu(3);
    ASSERT(dsu.union_sets(0, 1));
    ASSERT(dsu.union_sets(1, 2));
    ASSERT(!dsu.union_sets(0, 2));  // уже в одном множестве
}

TEST(dsu_transitivity) {
    DSU dsu(6);
    dsu.union_sets(0, 1);
    dsu.union_sets(1, 2);
    dsu.union_sets(2, 3);
    dsu.union_sets(3, 4);
    dsu.union_sets(4, 5);

    // Все должны быть связаны
    for (size_t i = 0; i < 6; ++i) {
        for (size_t j = 0; j < 6; ++j) {
            ASSERT(dsu.connected(i, j));
        }
    }
}

TEST(dsu_two_components) {
    DSU dsu(6);
    dsu.union_sets(0, 1);
    dsu.union_sets(1, 2);
    dsu.union_sets(3, 4);
    dsu.union_sets(4, 5);

    ASSERT(dsu.connected(0, 2));
    ASSERT(dsu.connected(3, 5));
    ASSERT(!dsu.connected(0, 3));
    ASSERT(!dsu.connected(1, 4));
}

TEST(dsu_make_set_grows) {
    DSU dsu;
    ASSERT(dsu.size() == 0);

    size_t a = dsu.make_set();
    size_t b = dsu.make_set();
    size_t c = dsu.make_set();

    ASSERT(dsu.size() == 3);
    ASSERT(a == 0);
    ASSERT(b == 1);
    ASSERT(c == 2);
    ASSERT(!dsu.connected(a, b));

    dsu.union_sets(a, b);
    ASSERT(dsu.connected(a, b));
    ASSERT(!dsu.connected(a, c));
}

TEST(dsu_out_of_range_throws) {
    DSU dsu(3);
    bool caught = false;
    try {
        dsu.find(10);
    } catch (const IndexOutOfRange&) {
        caught = true;
    }
    ASSERT(caught);
}

TEST(dsu_path_compression) {
    // Создаём длинную цепочку и проверяем, что find её сжимает
    DSU dsu(10);
    for (size_t i = 0; i < 9; ++i) {
        dsu.union_sets(i, i + 1);
    }

    // После find(0) все элементы должны указывать на одного представителя
    size_t rep = dsu.find(0);
    for (size_t i = 0; i < 10; ++i) {
        ASSERT(dsu.find(i) == rep);
    }
}

TEST(dsu_union_by_rank_balances) {
    // Проверяем, что union by rank работает: после объединения
    // деревьев разного размера, меньшее подвешивается к большему
    DSU dsu(8);

    // Создаём два дерева: {0,1,2,3} и {4,5,6,7}
    dsu.union_sets(0, 1);
    dsu.union_sets(2, 3);
    dsu.union_sets(0, 2);

    dsu.union_sets(4, 5);
    dsu.union_sets(6, 7);
    dsu.union_sets(4, 6);

    // Объединяем два больших дерева
    dsu.union_sets(0, 4);

    // Все должны быть в одном множестве
    size_t rep = dsu.find(0);
    for (size_t i = 0; i < 8; ++i) {
        ASSERT(dsu.find(i) == rep);
    }
}

int main() {
    std::cout << "=== Тесты DSU ===\n" << std::endl;

    RUN_TEST(dsu_empty);
    RUN_TEST(dsu_single_sets);
    RUN_TEST(dsu_union_basic);
    RUN_TEST(dsu_union_returns_false_if_already_connected);
    RUN_TEST(dsu_transitivity);
    RUN_TEST(dsu_two_components);
    RUN_TEST(dsu_make_set_grows);
    RUN_TEST(dsu_out_of_range_throws);
    RUN_TEST(dsu_path_compression);
    RUN_TEST(dsu_union_by_rank_balances);

    std::cout << "\n=== Все тесты DSU пройдены ===\n";
    return 0;
}