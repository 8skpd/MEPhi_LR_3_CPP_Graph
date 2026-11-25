#!/bin/bash

mkdir -p bin

CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -I."

# Проверка доступности SFML: пробуем собрать минимальную программу
sfml_available() {
    cat > /tmp/sfml_probe_$$.cpp << 'EOF'
#include <SFML/Graphics.hpp>
int main() { sf::RenderWindow w(sf::VideoMode(64,64), "probe"); return 0; }
EOF
    g++ -std=c++20 /tmp/sfml_probe_$$.cpp -o /tmp/sfml_probe_$$ \
        -lsfml-graphics -lsfml-window -lsfml-system 2>/dev/null
    local ok=$?
    rm -f /tmp/sfml_probe_$$.cpp /tmp/sfml_probe_$$
    return $ok
}

build_gui() {
    echo "Сборка с графическим интерфейсом (SFML)..."
    g++ $CXXFLAGS -pthread gui/main.cpp -o bin/graph_cli \
        -lsfml-graphics -lsfml-window -lsfml-system
    if [ $? -ne 0 ]; then
        echo "Ошибка компиляции GUI"
        echo "Код рассчитан на SFML 2.5/2.6. Проверьте: sudo apt install libsfml-dev"
        return 1
    fi
    echo "Готово. Запуск:"
    echo "  ./bin/graph_cli        — консольный режим"
    echo "  ./bin/graph_cli gui    — окно + консоль"
    return 0
}

build_console() {
    g++ $CXXFLAGS cli/main.cpp -o bin/graph_cli
    if [ $? -ne 0 ]; then echo "Ошибка компиляции"; return 1; fi
    return 0
}

if [ "$1" == "1" ]; then
    g++ $CXXFLAGS tests/test_graph.cpp -o bin/test_graph && ./bin/test_graph || exit 1

elif [ "$1" == "2" ]; then
    g++ $CXXFLAGS tests/test_priority_queue.cpp -o bin/test_pq && ./bin/test_pq || exit 1

elif [ "$1" == "3" ]; then
    g++ $CXXFLAGS tests/test_dsu.cpp -o bin/test_dsu && ./bin/test_dsu || exit 1

elif [ "$1" == "4" ]; then
    g++ $CXXFLAGS tests/test_bfs_dfs.cpp -o bin/test_bfs_dfs && ./bin/test_bfs_dfs || exit 1

elif [ "$1" == "5" ]; then
    g++ $CXXFLAGS tests/test_shortest_paths.cpp -o bin/test_shortest_paths && ./bin/test_shortest_paths || exit 1

elif [ "$1" == "6" ]; then
    g++ $CXXFLAGS tests/test_mst.cpp -o bin/test_mst && ./bin/test_mst || exit 1

elif [ "$1" == "7" ]; then
    g++ $CXXFLAGS tests/test_generator.cpp -o bin/test_generator && ./bin/test_generator || exit 1

elif [ "$1" == "8" ]; then
    g++ $CXXFLAGS tests/test_benchmark.cpp -o bin/test_benchmark && ./bin/test_benchmark || exit 1

elif [ "$1" == "9" ]; then
    g++ $CXXFLAGS tests/test_graph_parser.cpp -o bin/test_graph_parser && ./bin/test_graph_parser || exit 1

elif [ "$1" == "10" ]; then
    g++ $CXXFLAGS tests/test_executor.cpp -o bin/test_executor && ./bin/test_executor || exit 1

elif [ "$1" == "11" ]; then
    if sfml_available; then
        build_gui || exit 1
    else
        echo "SFML не найден — собираю только консольный режим."
        echo "Для GUI: sudo apt install libsfml-dev fonts-dejavu-core"
        build_console || exit 1
    fi
    ./bin/graph_cli

elif [ "$1" == "gui" ]; then
    if ! sfml_available; then
        echo "SFML не найден. Установите: sudo apt install libsfml-dev fonts-dejavu-core"
        exit 1
    fi
    build_gui || exit 1
    ./bin/graph_cli gui

elif [ "$1" == "all" ]; then
    for n in 1 2 3 4 5 6 7 8 9 10; do ./c.sh $n || exit 1; done

else
    echo "Использование: ./c.sh <номер|gui|all>"
    echo "  1   - тесты структуры графа"
    echo "  2   - тесты PriorityQueue"
    echo "  3   - тесты DSU"
    echo "  4   - тесты BFS, DFS и компонент связности"
    echo "  5   - тесты кратчайших путей"
    echo "  6   - тесты минимального остова"
    echo "  7   - тесты генератора графов"
    echo "  8   - тесты слоя бенчмарков"
    echo "  9   - тесты парсера команд"
    echo "  10  - сквозные тесты"
    echo "  11  - запуск клиента (GUI если доступен SFML)"
    echo "  gui - принудительная сборка и запуск GUI"
    echo "  all - все тесты"
fi