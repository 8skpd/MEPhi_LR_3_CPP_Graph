#pragma once

#include <cctype>
#include <string>
#include "GraphQuery.h"
#include "../lib/DynamicArray.h"
#include "../lib/Exceptions.h"

// Парсер команд графового интерфейса.
// Разбивает строку на токены (слова / строки в кавычках)
// и разбирает их в AST. Комментарии начинаются с # или --.

class GraphParser {
private:
    DynamicArray<std::string> tokens_;
    size_t idx_ = 0;

    // токенизация
    static DynamicArray<std::string> tokenize(const std::string& s) {
        DynamicArray<std::string> out;
        std::string current;
        bool in_quotes = false;
        bool has_token = false;

        for (size_t i = 0; i < s.size(); ++i) {
            const char c = s[i];

            // комментарий до конца строки
            if (!in_quotes && (c == '#' || (c == '-' && i + 1 < s.size() && s[i + 1] == '-'))) {
                break;
            }

            if (c == '"') {
                in_quotes = !in_quotes;
                has_token = true;
                continue;
            }

            if (!in_quotes && std::isspace(static_cast<unsigned char>(c))) {
                if (has_token) {
                    out.push_back(current);
                    current.clear();
                    has_token = false;
                }
                continue;
            }

            current.push_back(c);
            has_token = true;
        }
        if (has_token) out.push_back(current);
        return out;
    }

    // навигация по токенам
    bool at_end() const { return idx_ >= tokens_.size(); }

    std::string peek() const {
        if (at_end()) return "";
        return tokens_[idx_];
    }

    std::string advance() {
        if (at_end()) throw GraphException("parse", "неожиданный конец команды");
        return tokens_[idx_++];
    }

    static std::string to_upper(const std::string& s) {
        std::string r = s;
        for (char& c : r) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return r;
    }

    bool match_keyword(const std::string& kw) {
        if (at_end()) return false;
        if (to_upper(tokens_[idx_]) == to_upper(kw)) {
            ++idx_;
            return true;
        }
        return false;
    }

    void expect_keyword(const std::string& kw) {
        if (!match_keyword(kw)) {
            throw GraphException("parse",
                "ожидалось ключевое слово '" + kw + "', получено '" +
                (at_end() ? std::string("<конец>") : tokens_[idx_]) + "'");
        }
    }

    std::string expect_identifier() {
        if (at_end()) {
            throw GraphException("parse", "ожидался идентификатор, получен конец команды");
        }
        return advance();
    }

    Value parse_weight() {
        const std::string tok = advance();
        char* end = nullptr;
        const double d = std::strtod(tok.c_str(), &end);
        if (end == tok.c_str() || *end != '\0') {
            throw GraphException("parse", "вес должен быть числом, получено '" + tok + "'");
        }
        // целое, если нет точки
        if (tok.find('.') == std::string::npos) {
            return Value(static_cast<int>(d));
        }
        return Value(d);
    }

    size_t parse_size() {
        const std::string tok = advance();
        char* end = nullptr;
        const long long v = std::strtoll(tok.c_str(), &end, 10);
        if (end == tok.c_str() || *end != '\0' || v < 0) {
            throw GraphException("parse", "ожидалось неотрицательное число, получено '" + tok + "'");
        }
        return static_cast<size_t>(v);
    }

    unsigned parse_seed() {
        const std::string tok = advance();
        char* end = nullptr;
        const unsigned long v = std::strtoul(tok.c_str(), &end, 10);
        if (end == tok.c_str() || *end != '\0') {
            throw GraphException("parse", "seed должен быть числом, получено '" + tok + "'");
        }
        return static_cast<unsigned>(v);
    }

    GraphTopology parse_topology() {
        const std::string tok = to_upper(advance());
        if (tok == "RANDOM") return GraphTopology::Random;
        if (tok == "RING")   return GraphTopology::Ring;
        if (tok == "PATH")   return GraphTopology::Path;
        if (tok == "STAR")   return GraphTopology::Star;
        if (tok == "COMPLETE") return GraphTopology::Complete;
        if (tok == "TREE")   return GraphTopology::RandomTree;
        throw GraphException("parse",
            "неизвестная топология '" + tok +
            "' (RANDOM, RING, PATH, STAR, COMPLETE, TREE)");
    }

    WeightOrder parse_order() {
        const std::string tok = to_upper(advance());
        if (tok == "ASC")    return WeightOrder::Asc;
        if (tok == "DESC")   return WeightOrder::Desc;
        if (tok == "RANDOM") return WeightOrder::Random;
        throw GraphException("parse",
            "неизвестное распределение весов '" + tok + "' (ASC, DESC, RANDOM)");
    }

    // разбор команд
    GraphQuery parse_command() {
        if (at_end()) throw GraphException("parse", "пустая команда");

        const std::string first = to_upper(peek());

        if (first == "HELP")       { advance(); return HelpQuery{}; }
        if (first == "EXIT" || first == "QUIT") { advance(); return ExitQuery{}; }
        if (first == "CREATE")     { return parse_create(); }
        if (first == "ADD")        { return parse_add(); }
        if (first == "SHOW")       { return parse_show(); }
        if (first == "GENERATE" || first == "GEN") { return parse_generate(); }
        if (first == "PATH")       { return parse_path(); }
        if (first == "SPANNING" || first == "MST") { return parse_spanning(); }
        if (first == "COMPONENTS" || first == "COMP") { advance(); return ComponentsQuery{}; }
        if (first == "BENCHMARK" || first == "BENCH") { return parse_benchmark(); }
        if (first == "EXPORT")     { return parse_export(); }
        if (first == "VIEW")       { return parse_view(); }

        throw GraphException("parse", "неизвестная команда '" + first + "' (HELP для списка)");
    }

    GraphQuery parse_create() {
        expect_keyword("CREATE");
        expect_keyword("GRAPH");
        return CreateGraphQuery{};
    }

    GraphQuery parse_add() {
        expect_keyword("ADD");
        if (match_keyword("VERTEX")) {
            AddVertexQuery q;
            if (!at_end()) q.name = advance();
            return q;
        }
        if (match_keyword("EDGE")) {
            AddEdgeQuery q;
            q.from = expect_identifier();
            q.to = expect_identifier();
            q.weight = parse_weight();
            return q;
        }
        throw GraphException("parse", "после ADD ожидалось VERTEX или EDGE");
    }

    GraphQuery parse_show() {
        expect_keyword("SHOW");
        if (match_keyword("GRAPH"))    return ShowGraphQuery{};
        if (match_keyword("VERTICES")) return ShowVerticesQuery{};
        if (match_keyword("EDGES"))    return ShowEdgesQuery{};
        throw GraphException("parse", "после SHOW ожидалось GRAPH, VERTICES или EDGES");
    }

    // GENERATE <topo> <n> [m] [SEED s] [WEIGHTS ASC|DESC|RANDOM min max]
    GraphQuery parse_generate() {
        advance();  // GENERATE
        GenerateQuery q;
        q.topology = parse_topology();
        q.vertices = parse_size();

        // число рёбер только для RANDOM
        if (q.topology == GraphTopology::Random && !at_end()
            && to_upper(peek()) != "SEED" && to_upper(peek()) != "WEIGHTS") {
            q.edges = parse_size();
        }

        while (!at_end()) {
            if (match_keyword("SEED")) {
                q.seed = parse_seed();
            } else if (match_keyword("WEIGHTS")) {
                q.order = parse_order();
                q.min_weight = static_cast<int>(parse_size());
                q.max_weight = static_cast<int>(parse_size());
            } else {
                throw GraphException("parse", "неожиданный токен '" + peek() + "'");
            }
        }
        return q;
    }

    // PATH BFS|DFS|DIJKSTRA|BELLMAN from [to]
    GraphQuery parse_path() {
        expect_keyword("PATH");
        PathQuery q;
        q.algorithm = to_upper(expect_identifier());
        if (q.algorithm != "BFS" && q.algorithm != "DFS" &&
            q.algorithm != "DIJKSTRA" && q.algorithm != "BELLMAN") {
            throw GraphException("parse",
                "неизвестный алгоритм '" + q.algorithm + "' (BFS, DFS, DIJKSTRA, BELLMAN)");
        }
        q.from = expect_identifier();
        if (!at_end()) q.to = expect_identifier();
        return q;
    }

    // SPANNING KRUSKAL|PRIM
    GraphQuery parse_spanning() {
        advance();  // SPANNING
        SpanningQuery q;
        q.algorithm = to_upper(expect_identifier());
        if (q.algorithm != "KRUSKAL" && q.algorithm != "PRIM") {
            throw GraphException("parse",
                "неизвестный алгоритм '" + q.algorithm + "' (KRUSKAL, PRIM)");
        }
        return q;
    }

    // BENCHMARK SHORTEST from | BENCHMARK MST | BENCHMARK SUITE n m [seed]
    GraphQuery parse_benchmark() {
        advance();  // BENCHMARK
        BenchmarkQuery q;
        q.type = to_upper(expect_identifier());

        if (q.type == "SHORTEST") {
            q.from = expect_identifier();
        } else if (q.type == "SUITE") {
            q.vertices = parse_size();
            q.edges = parse_size();
            if (!at_end()) q.seed = parse_seed();
        } else if (q.type != "MST") {
            throw GraphException("parse",
                "неизвестный тип бенчмарка '" + q.type + "' (SHORTEST, MST, SUITE)");
        }
        return q;
    }

    // EXPORT CSV <path>
    GraphQuery parse_export() {
        expect_keyword("EXPORT");
        expect_keyword("CSV");
        ExportQuery q;
        q.path = expect_identifier();
        return q;
    }

    // VIEW [path]
    GraphQuery parse_view() {
        expect_keyword("VIEW");
        ViewQuery q;
        if (!at_end()) q.path = expect_identifier();
        return q;
    }

public:
    GraphQuery parse(const std::string& input) {
        tokens_ = tokenize(input);
        idx_ = 0;
        const GraphQuery q = parse_command();
        if (!at_end()) {
            throw GraphException("parse", "неожиданный токен после конца команды: '" + peek() + "'");
        }
        return q;
    }
};