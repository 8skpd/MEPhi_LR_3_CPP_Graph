#pragma once

#include <fstream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include "Graph.h"

/*
Генератор интерактивной визуализации графа в формате HTML.
Использует силу-направленную раскладку на Canvas.

Переносимость: один файл без внешних зависимостей.
Не требует графических библиотек при компиляции.
*/


class GraphViewer {
private:
    // Экранирование строки для встраивания в JavaScript
    static std::string escape_js(const std::string& s) {
        std::string out;
        for (char c : s) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '"':  out += "\\\""; break;
                case '\'': out += "\\'";  break;
                case '\n': out += "\\n";  break;
                case '<':  out += "\\u003c"; break;
                case '>':  out += "\\u003e"; break;
                case '&':  out += "\\u0026"; break;
                default:   out += c;
            }
        }
        return out;
    }

public:
    // Генерация HTML с визуализацией графа
    static std::string generate_html(const Graph& g, const std::string& title) {
        // Собираем данные графа в JavaScript-массивы
        std::ostringstream nodes_js;
        nodes_js << "[";
        for (size_t i = 0; i < g.vertex_count(); ++i) {
            if (i > 0) nodes_js << ",";
            nodes_js << "{\"id\":" << i
                     << ",\"label\":\"" << escape_js(g.vertex_name(i)) << "\"}";
        }
        nodes_js << "]";

        std::ostringstream edges_js;
        edges_js << "[";
        const auto edges = g.edges();
        for (size_t i = 0; i < edges.size(); ++i) {
            if (i > 0) edges_js << ",";
            edges_js << "{\"from\":" << edges[i].from
                     << ",\"to\":" << edges[i].to
                     << ",\"weight\":\"" << escape_js(edges[i].weight.to_string()) << "\"}";
        }
        edges_js << "]";

        // HTML-шаблон с встроенным JavaScript
        std::ostringstream html;
        html << R"(<!DOCTYPE html>
<html lang="ru">
<head>
<meta charset="UTF-8">
<title>)" << escape_js(title) << R"(</title>
<style>
    body {
        margin: 0;
        background: #1a1a2e;
        font-family: 'Segoe UI', Arial, sans-serif;
        overflow: hidden;
    }
    #info {
        position: absolute;
        top: 10px;
        left: 10px;
        color: #e0e0e0;
        background: rgba(26, 26, 46, 0.8);
        padding: 10px 15px;
        border-radius: 8px;
        font-size: 14px;
    }
    canvas { display: block; }
</style>
</head>
<body>
<div id="info">
    <strong>)" << escape_js(title) << R"(</strong><br>
    Вершин: )" << g.vertex_count() << R"( | Рёбер: )" << edges.size() << R"(<br>
    <small>Перетаскивайте вершины мышью</small>
</div>
<canvas id="canvas"></canvas>
<script>
const nodes = )" << nodes_js.str() << R"(;
const edges = )" << edges_js.str() << R"(;

const canvas = document.getElementById('canvas');
const ctx = canvas.getContext('2d');
let width = canvas.width = window.innerWidth;
let height = canvas.height = window.innerHeight;

// Параметры раскладки
const NODE_RADIUS = 22;
const REPULSION = 8000;
const ATTRACTION = 0.01;
const IDEAL_LENGTH = 120;
const DAMPING = 0.85;

// Инициализация позиций
nodes.forEach(n => {
    n.x = width / 2 + (Math.random() - 0.5) * 300;
    n.y = height / 2 + (Math.random() - 0.5) * 300;
    n.vx = 0;
    n.vy = 0;
    n.dragging = false;
});

// Сила-направленная раскладка
function simulate() {
    // Отталкивание вершин друг от друга
    for (let i = 0; i < nodes.length; i++) {
        for (let j = i + 1; j < nodes.length; j++) {
            const dx = nodes[j].x - nodes[i].x;
            const dy = nodes[j].y - nodes[i].y;
            const distSq = dx * dx + dy * dy;
            const dist = Math.sqrt(distSq) || 1;
            const force = REPULSION / distSq;
            const fx = force * dx / dist;
            const fy = force * dy / dist;
            nodes[i].vx -= fx; nodes[i].vy -= fy;
            nodes[j].vx += fx; nodes[j].vy += fy;
        }
    }
    // Притяжение по рёбрам
    edges.forEach(e => {
        const a = nodes[e.from], b = nodes[e.to];
        const dx = b.x - a.x;
        const dy = b.y - a.y;
        const dist = Math.sqrt(dx * dx + dy * dy) || 1;
        const force = ATTRACTION * (dist - IDEAL_LENGTH);
        const fx = force * dx / dist;
        const fy = force * dy / dist;
        a.vx += fx; a.vy += fy;
        b.vx -= fx; b.vy -= fy;
    });
    // Обновление позиций
    nodes.forEach(n => {
        if (!n.dragging) {
            n.x += n.vx;
            n.y += n.vy;
            n.vx *= DAMPING;
            n.vy *= DAMPING;
            // Держим в пределах экрана
            n.x = Math.max(NODE_RADIUS, Math.min(width - NODE_RADIUS, n.x));
            n.y = Math.max(NODE_RADIUS, Math.min(height - NODE_RADIUS, n.y));
        }
    });
}

// Отрисовка
function draw() {
    ctx.fillStyle = '#1a1a2e';
    ctx.fillRect(0, 0, width, height);

    // Рёбра
    edges.forEach(e => {
        const a = nodes[e.from], b = nodes[e.to];
        ctx.beginPath();
        ctx.moveTo(a.x, a.y);
        ctx.lineTo(b.x, b.y);
        ctx.strokeStyle = '#4a5568';
        ctx.lineWidth = 2;
        ctx.stroke();
        // Вес ребра
        const mx = (a.x + b.x) / 2;
        const my = (a.y + b.y) / 2;
        ctx.fillStyle = '#a0aec0';
        ctx.font = '12px Arial';
        ctx.fillText(e.weight, mx + 5, my - 5);
    });

    // Вершины
    nodes.forEach(n => {
        ctx.beginPath();
        ctx.arc(n.x, n.y, NODE_RADIUS, 0, 2 * Math.PI);
        ctx.fillStyle = n.dragging ? '#f6ad55' : '#4a9eff';
        ctx.fill();
        ctx.strokeStyle = '#fff';
        ctx.lineWidth = 2;
        ctx.stroke();
        // Имя вершины
        ctx.fillStyle = '#fff';
        ctx.font = 'bold 11px Arial';
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';
        ctx.fillText(n.label, n.x, n.y);
    });
}

// Главный цикл
function animate() {
    simulate();
    draw();
    requestAnimationFrame(animate);
}

// Интерактивность: перетаскивание вершин
let draggedNode = null;
let mouseX = 0, mouseY = 0;

canvas.addEventListener('mousedown', e => {
    mouseX = e.clientX;
    mouseY = e.clientY;
    for (let i = nodes.length - 1; i >= 0; i--) {
        const dx = nodes[i].x - mouseX;
        const dy = nodes[i].y - mouseY;
        if (dx * dx + dy * dy < NODE_RADIUS * NODE_RADIUS) {
            draggedNode = nodes[i];
            draggedNode.dragging = true;
            break;
        }
    }
});

canvas.addEventListener('mousemove', e => {
    if (draggedNode) {
        draggedNode.x = e.clientX;
        draggedNode.y = e.clientY;
        draggedNode.vx = 0;
        draggedNode.vy = 0;
    }
});

canvas.addEventListener('mouseup', () => {
    if (draggedNode) {
        draggedNode.dragging = false;
        draggedNode = null;
    }
});

// Адаптация к изменению размера окна
window.addEventListener('resize', () => {
    width = canvas.width = window.innerWidth;
    height = canvas.height = window.innerHeight;
});

animate();
</script>
</body>
</html>)";

        return html.str();
    }

    // Сохранение графа в HTML-файл
    static bool save_html(const Graph& g, const std::string& path,
                          const std::string& title = "Граф") {
        namespace fs = std::filesystem;
        try {
            const fs::path p(path);
            if (p.has_parent_path()) {
                fs::create_directories(p.parent_path());
            }
        } catch (...) {
            return false;
        }

        std::ofstream f(path);
        if (!f.is_open()) return false;
        f << generate_html(g, title);
        return true;
    }

    // Открытие файла в браузере
    static bool open_in_browser(const std::string& path) {
        const std::string cmd = "xdg-open '" + path + "' > /dev/null 2>&1 &";
        return system(cmd.c_str()) == 0;
    }

    // Сохранение и открытие графа
    static bool show(const Graph& g, const std::string& path = "assets/graph_view.html",
                     const std::string& title = "Граф") {
        if (!save_html(g, path, title)) return false;
        open_in_browser(path);
        return true;
    }
};