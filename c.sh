#!/bin/bash

# Создаём директорию для бинарников
mkdir -p bin

if [ "$1" == "1" ]; then
    # Тесты структуры графа
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_graph.cpp -o bin/test_graph
    if [ $? -eq 0 ]; then ./bin/test_graph; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "2" ]; then
    # Тесты PriorityQueue
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_priority_queue.cpp -o bin/test_pq
    if [ $? -eq 0 ]; then ./bin/test_pq; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "3" ]; then
    # Тесты DSU
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_dsu.cpp -o bin/test_dsu
    if [ $? -eq 0 ]; then ./bin/test_dsu; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "4" ]; then
    # Тесты BFS, DFS и компонент связности
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_bfs_dfs.cpp -o bin/test_bfs_dfs
    if [ $? -eq 0 ]; then ./bin/test_bfs_dfs; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "5" ]; then
    # Тесты кратчайших путей (Дейкстра, Форд-Беллман)
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_shortest_paths.cpp -o bin/test_shortest_paths
    if [ $? -eq 0 ]; then ./bin/test_shortest_paths; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "6" ]; then
    # Тесты минимального остова (Крускал, Прим)
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_mst.cpp -o bin/test_mst
    if [ $? -eq 0 ]; then ./bin/test_mst; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "7" ]; then
    # Тесты генератора графов
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_generator.cpp -o bin/test_generator
    if [ $? -eq 0 ]; then ./bin/test_generator; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "8" ]; then
    # Тесты слоя бенчмарков
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_benchmark.cpp -o bin/test_benchmark
    if [ $? -eq 0 ]; then ./bin/test_benchmark; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "9" ]; then
    # Тесты парсера команд
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_graph_parser.cpp -o bin/test_graph_parser
    if [ $? -eq 0 ]; then ./bin/test_graph_parser; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "10" ]; then
    # Сквозные тесты (парсер + исполнитель)
    g++ -std=c++20 -O2 -Wall -Wextra -I. tests/test_executor.cpp -o bin/test_executor
    if [ $? -eq 0 ]; then ./bin/test_executor; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "11" ]; then
    # Интерактивный режим (клиент)
    g++ -std=c++20 -O2 -Wall -Wextra -I. cli/main.cpp -o bin/graph_cli
    if [ $? -eq 0 ]; then ./bin/graph_cli; else echo "Ошибка компиляции"; exit 1; fi

elif [ "$1" == "all" ]; then
    for n in 1 2 3 4 5 6 7 8 9 10; do ./c.sh $n || exit 1; done

else
    echo "Использование: ./c.sh <номер>"
    echo "  1   - тесты структуры графа"
    echo "  2   - тесты PriorityQueue"
    echo "  3   - тесты DSU"
    echo "  4   - тесты BFS, DFS и компонент связности"
    echo "  5   - тесты кратчайших путей (Дейкстра, Форд-Беллман)"
    echo "  6   - тесты минимального остова (Крускал, Прим)"
    echo "  7   - тесты генератора графов"
    echo "  all - все тесты"
fi

# ./bin/graph_cli < assets/scenarios/scenario_N.txt