#include <iostream>
#include <string>
#include "../core/GraphParser.h"
#include "../core/GraphExecutor.h"
#include "../lib/Exceptions.h"

// =========================================================
// Интерактивный клиент графовых алгоритмов (ЛР-3).
//
// Предоставляет пользователю:
//   - ручной ввод графа (ADD VERTEX / ADD EDGE)
//   - автоматическую генерацию (GENERATE)
//   - выбор алгоритма (PATH, SPANNING, COMPONENTS)
//   - просмотр данных и результатов (SHOW)
//   - бенчмарки и выгрузку в CSV (BENCHMARK, EXPORT)
// =========================================================

static bool is_blank(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

int main() {
    std::cout << "============================================\n";
    std::cout << "  Графовые алгоритмы — Лабораторная работа 3\n";
    std::cout << "============================================\n";
    std::cout << "Введите HELP для списка команд, EXIT для выхода.\n";
    std::cout << "Начните с GENERATE или с CREATE GRAPH + ADD EDGE.\n\n";

    GraphParser parser;
    GraphExecutor executor;

    while (!executor.should_exit()) {
        std::cout << "graph> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;   // EOF (Ctrl+D)

        if (is_blank(line)) continue;

        try {
            const GraphQuery q = parser.parse(line);
            const std::string result = executor.execute(q);
            std::cout << result << "\n\n";
        } catch (const GraphException& e) {
            std::cout << "Ошибка: " << e.what() << "\n\n";
        }
    }

    std::cout << "До свидания!\n";
    return 0;
}