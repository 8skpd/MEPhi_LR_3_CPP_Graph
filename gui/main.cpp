#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>

#include "../core/GraphParser.h"
#include "../core/GraphExecutor.h"
#include "../lib/Exceptions.h"
#include "GraphWindow.h"


/*
Точка входа:
  graph_cli        — обычный консольный режим
  graph_cli gui    — окно SFML + консольный ввод параллельно

Архитектура GUI-режима:
  главный поток  — event loop и отрисовка SFML
  второй поток   — чтение stdin, разбор и выполнение команд
  обмен          — мьютекс + счётчик версии графа
*/


struct SharedState {
    std::mutex mtx;
    GraphParser parser;
    GraphExecutor executor;
    size_t version = 1;
    std::string last_command = "(ожидание команды)";
    std::string last_status = "Введите команду в терминале. HELP — список команд.";
    std::atomic<bool> cli_done{false};
};

struct GuiSnapshot {
    GraphHighlight highlight;
    std::string command;
    std::string status;
    size_t vertices = 0;
    size_t edges = 0;
    bool structure_changed = false;
};

static bool is_blank(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

static std::string first_line(const std::string& s) {
    const size_t pos = s.find('\n');
    std::string r = (pos == std::string::npos) ? s : s.substr(0, pos);
    if (r.size() > 110) r = r.substr(0, 110) + "...";
    return r;
}

// обычный консольный режим

static int run_console() {
    std::cout << "============================================\n";
    std::cout << "  Графовые алгоритмы — Лабораторная работа 3\n";
    std::cout << "============================================\n";
    std::cout << "Введите HELP для списка команд, EXIT для выхода.\n";
    std::cout << "Для графического режима запустите: graph_cli gui\n\n";

    GraphParser parser;
    GraphExecutor executor;

    while (!executor.should_exit()) {
        std::cout << "graph> ";
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (is_blank(line)) continue;
        try {
            const GraphQuery q = parser.parse(line);
            std::cout << executor.execute(q) << "\n\n";
        } catch (const GraphException& e) {
            std::cout << "Ошибка: " << e.what() << "\n\n";
        }
    }
    std::cout << "До свидания!\n";
    return 0;
}

// поток CLI в GUI-режиме

static void cli_thread_main(SharedState& st) {
    std::cout << "=== Графический режим ===\n";
    std::cout << "Окно открыто. Вводите команды здесь: HELP, EXIT.\n";
    std::cout << "Окно обновляется автоматически при изменении графа.\n\n";

    while (true) {
        std::cout << "graph> " << std::flush;
        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (is_blank(line)) continue;

        std::string result;
        bool exit_requested = false;
        {
            std::lock_guard<std::mutex> lock(st.mtx);
            try {
                const GraphQuery q = st.parser.parse(line);
                result = st.executor.execute(q);
                exit_requested = st.executor.should_exit();
            } catch (const GraphException& e) {
                result = std::string("Ошибка: ") + e.what();
            }
            st.last_command = line;
            st.last_status = first_line(result);
            ++st.version;      // сигнал окну перечитать граф
        }
        std::cout << result << "\n\n";
        if (exit_requested) break;
    }
    st.cli_done = true;
}

// GUI-режим

static int run_gui() {
    SharedState st;
    GraphWindow window;

    std::thread cli(cli_thread_main, std::ref(st));
    cli.detach();   // stdin блокирующий: поток завершится вместе с процессом

    size_t seen_version = 0;

    while (window.is_open()) {
        // 1) обновление раскладки — под мьютексом, работаем с живым графом
        window.frame(
            [&](GraphLayout& layout) {
                std::lock_guard<std::mutex> lock(st.mtx);
                layout.sync(st.executor.graph(), st.version);
            },
            // 2) короткий снапшот метаданных — тоже под мьютексом
            [&]() {
                GuiSnapshot snap;
                std::lock_guard<std::mutex> lock(st.mtx);
                snap.highlight = st.executor.highlight();
                snap.command = st.last_command;
                snap.status = st.last_status;
                snap.vertices = st.executor.graph().vertex_count();
                snap.edges = st.executor.graph().edge_count();
                snap.structure_changed = (st.version != seen_version);
                seen_version = st.version;
                return snap;
            });

        if (st.cli_done) {
            // команда EXIT: закрываем окно
            break;
        }
    }

    std::cout << "Окно закрыто. Завершение работы.\n";
    std::exit(0);   // блокирующий stdin-поток не мешает завершению процесса
}

int main(int argc, char** argv) {
    const bool gui_mode = (argc > 1 && std::string(argv[1]) == "gui");
    if (gui_mode) return run_gui();
    return run_console();
}