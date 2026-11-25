#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include <cmath>
#include <algorithm>
#include "GraphLayout.h"
#include "../core/GraphHighlight.h"
#include "../lib/DynamicArray.h"

/*
Окно визуализации графа на SFML 2.x.

Управление:
  ЛКМ по вершине  — перетащить вершину
  ЛКМ по пустому  — панорамирование
  Колесо мыши     — масштаб
  R               — пересобрать раскладку
  F               — вписать граф в окно
  W               — показать/скрыть веса рёбер
  H               — включить/выключить подсветку результата
*/


class GraphWindow {
private:
    sf::RenderWindow window_;
    sf::Font font_;
    bool font_ok_ = false;

    GraphLayout layout_;
    GraphHighlight highlight_;

    float zoom_ = 1.0f;
    sf::Vector2f pan_{0.0f, 0.0f};

    bool dragging_node_ = false;
    size_t dragged_index_ = 0;
    bool panning_ = false;
    sf::Vector2i last_mouse_;

    bool show_weights_ = true;
    bool show_highlight_ = true;

    std::string status_line_;
    std::string command_line_;
    size_t vertex_count_ = 0;
    size_t edge_count_ = 0;

    static bool try_load_font(sf::Font& f) {
        const char* candidates[] = {
            "assets/fonts/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/TTF/DejaVuSans.ttf",
            "/usr/share/fonts/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/truetype/noto/NotoSans-Regular.ttf",
            "/System/Library/Fonts/Supplemental/Arial Unicode.ttf"
        };
        for (const char* path : candidates) {
            if (f.loadFromFile(path)) return true;
        }
        return false;
    }

    sf::Vector2f to_screen(float wx, float wy) const {
        return sf::Vector2f(wx * zoom_ + pan_.x, wy * zoom_ + pan_.y);
    }

    sf::Vector2f to_world(sf::Vector2i screen) const {
        return sf::Vector2f((static_cast<float>(screen.x) - pan_.x) / zoom_,
                            (static_cast<float>(screen.y) - pan_.y) / zoom_);
    }

    // Толстая линия как четырёхугольник
    void draw_thick_line(sf::Vector2f a, sf::Vector2f b, float thickness,
                         const sf::Color& color) {
        sf::Vector2f d = b - a;
        const float len = std::sqrt(d.x * d.x + d.y * d.y);
        if (len < 0.001f) return;
        const sf::Vector2f n(-d.y / len * thickness * 0.5f, d.x / len * thickness * 0.5f);
        sf::Vertex quad[4];
        quad[0].position = a + n; quad[0].color = color;
        quad[1].position = b + n; quad[1].color = color;
        quad[2].position = b - n; quad[2].color = color;
        quad[3].position = a - n; quad[3].color = color;
        window_.draw(quad, 4, sf::Quads);
    }

    void draw_text(const std::string& s, sf::Vector2f pos, unsigned size,
                   const sf::Color& color, bool centered) {
        if (!font_ok_) return;
        sf::Text t;
        t.setFont(font_);
        t.setString(sf::String::fromUtf8(s.begin(), s.end()));
        t.setCharacterSize(size);
        t.setFillColor(color);
        if (centered) {
            const sf::FloatRect b = t.getLocalBounds();
            t.setOrigin(b.left + b.width * 0.5f, b.top + b.height * 0.5f);
        }
        t.setPosition(pos);
        window_.draw(t);
    }

    static sf::Color component_color(size_t id) {
        static const sf::Color palette[] = {
            sf::Color(74, 158, 255), sf::Color(255, 138, 101),
            sf::Color(129, 199, 132), sf::Color(186, 104, 200),
            sf::Color(255, 213, 79),  sf::Color(77, 208, 225),
            sf::Color(240, 98, 146),  sf::Color(174, 213, 129)
        };
        return palette[id % 8];
    }

    void fit_view() {
        if (layout_.node_count() == 0) {
            zoom_ = 1.0f;
            pan_ = sf::Vector2f(window_.getSize().x * 0.5f, window_.getSize().y * 0.5f);
            return;
        }
        float min_x = layout_.node(0).x, max_x = min_x;
        float min_y = layout_.node(0).y, max_y = min_y;
        for (size_t i = 1; i < layout_.node_count(); ++i) {
            min_x = std::min(min_x, layout_.node(i).x);
            max_x = std::max(max_x, layout_.node(i).x);
            min_y = std::min(min_y, layout_.node(i).y);
            max_y = std::max(max_y, layout_.node(i).y);
        }
        const float w = std::max(60.0f, max_x - min_x);
        const float h = std::max(60.0f, max_y - min_y);
        const float sx = (window_.getSize().x - 160.0f) / w;
        const float sy = (window_.getSize().y - 200.0f) / h;
        zoom_ = std::max(0.12f, std::min(2.2f, std::min(sx, sy)));
        const float cx = (min_x + max_x) * 0.5f;
        const float cy = (min_y + max_y) * 0.5f;
        pan_ = sf::Vector2f(window_.getSize().x * 0.5f - cx * zoom_,
                            window_.getSize().y * 0.5f - cy * zoom_);
    }

    void handle_events() {
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed) {
                window_.close();
            } else if (event.type == sf::Event::Resized) {
                sf::FloatRect view(0.f, 0.f,
                                   static_cast<float>(event.size.width),
                                   static_cast<float>(event.size.height));
                window_.setView(sf::View(view));
            } else if (event.type == sf::Event::MouseButtonPressed) {
                last_mouse_ = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
                if (event.mouseButton.button == sf::Mouse::Left) {
                    const sf::Vector2f w = to_world(last_mouse_);
                    auto hit = layout_.pick(w.x, w.y, 26.0f / zoom_);
                    if (hit.has_value()) {
                        dragging_node_ = true;
                        dragged_index_ = hit.value();
                        layout_.node(dragged_index_).pinned = true;
                        layout_.reheat();
                    } else {
                        panning_ = true;
                    }
                }
            } else if (event.type == sf::Event::MouseButtonReleased) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    if (dragging_node_) {
                        layout_.node(dragged_index_).pinned = false;
                        dragging_node_ = false;
                    }
                    panning_ = false;
                }
            } else if (event.type == sf::Event::MouseMoved) {
                const sf::Vector2i now(event.mouseMove.x, event.mouseMove.y);
                if (dragging_node_) {
                    const sf::Vector2f w = to_world(now);
                    layout_.node(dragged_index_).x = w.x;
                    layout_.node(dragged_index_).y = w.y;
                    layout_.node(dragged_index_).vx = 0.0f;
                    layout_.node(dragged_index_).vy = 0.0f;
                    layout_.reheat();
                } else if (panning_) {
                    pan_ += sf::Vector2f(static_cast<float>(now.x - last_mouse_.x),
                                         static_cast<float>(now.y - last_mouse_.y));
                }
                last_mouse_ = now;
            } else if (event.type == sf::Event::MouseWheelScrolled) {
                const sf::Vector2i m(event.mouseWheelScroll.x, event.mouseWheelScroll.y);
                const sf::Vector2f before = to_world(m);
                const float factor = (event.mouseWheelScroll.delta > 0) ? 1.12f : 0.89f;
                zoom_ = std::max(0.06f, std::min(6.0f, zoom_ * factor));
                const sf::Vector2f after = to_world(m);
                pan_ += sf::Vector2f((after.x - before.x) * zoom_,
                                     (after.y - before.y) * zoom_);
            } else if (event.type == sf::Event::KeyPressed) {
                switch (event.key.code) {
                    case sf::Keyboard::R: layout_.scatter(); break;
                    case sf::Keyboard::F: fit_view(); break;
                    case sf::Keyboard::W: show_weights_ = !show_weights_; break;
                    case sf::Keyboard::H: show_highlight_ = !show_highlight_; break;
                    default: break;
                }
            }
        }
    }

    void render() {
        window_.clear(sf::Color(20, 22, 34));

        const float node_r = std::max(9.0f, 21.0f * std::min(1.4f, zoom_));
        const bool label_visible = font_ok_ && zoom_ > 0.30f;
        const bool weights_visible = show_weights_ && font_ok_ &&
                                     zoom_ > 0.45f && layout_.edge_count() <= 400;

        // --- рёбра ---
        for (size_t k = 0; k < layout_.edge_count(); ++k) {
            const LayoutEdge& e = layout_.edge(k);
            if (e.from >= layout_.node_count() || e.to >= layout_.node_count()) continue;

            sf::Color color(66, 72, 96);
            float thickness = 1.8f;

            if (show_highlight_ && highlight_.has_path &&
                highlight_.is_path_edge(e.from_name, e.to_name)) {
                color = sf::Color(255, 167, 38); thickness = 4.2f;
            } else if (show_highlight_ && highlight_.has_mst &&
                       highlight_.is_mst_edge(e.from_name, e.to_name)) {
                color = sf::Color(102, 187, 106); thickness = 3.6f;
            }

            const sf::Vector2f a = to_screen(layout_.node(e.from).x, layout_.node(e.from).y);
            const sf::Vector2f b = to_screen(layout_.node(e.to).x, layout_.node(e.to).y);

            if (e.from == e.to) {
                // петля: рисуем окружность рядом с вершиной
                sf::CircleShape loop(node_r * 0.85f);
                loop.setFillColor(sf::Color::Transparent);
                loop.setOutlineColor(color);
                loop.setOutlineThickness(thickness);
                loop.setOrigin(node_r * 0.85f, node_r * 0.85f);
                loop.setPosition(a.x + node_r * 1.1f, a.y - node_r * 1.1f);
                window_.draw(loop);
            } else {
                draw_thick_line(a, b, thickness, color);
            }

            if (weights_visible && e.from != e.to) {
                const sf::Vector2f mid((a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f);
                draw_text(e.weight, sf::Vector2f(mid.x + 4.0f, mid.y - 9.0f),
                          12, sf::Color(168, 176, 196), false);
            }
        }

        // --- вершины ---
        for (size_t i = 0; i < layout_.node_count(); ++i) {
            const LayoutNode& v = layout_.node(i);
            const sf::Vector2f p = to_screen(v.x, v.y);

            sf::Color fill(74, 158, 255);
            if (show_highlight_ && highlight_.has_components &&
                i < highlight_.components.size()) {
                fill = component_color(highlight_.components[i]);
            }
            if (show_highlight_ && highlight_.has_path &&
                highlight_.is_path_vertex(v.name)) {
                fill = sf::Color(255, 167, 38);
            }
            if (dragging_node_ && dragged_index_ == i) {
                fill = sf::Color(255, 238, 88);
            }

            sf::CircleShape circle(node_r);
            circle.setOrigin(node_r, node_r);
            circle.setPosition(p);
            circle.setFillColor(fill);
            circle.setOutlineColor(sf::Color(232, 236, 244));
            circle.setOutlineThickness(1.6f);
            window_.draw(circle);

            if (label_visible) {
                std::string label = v.name;
                if (label.size() > 14) {
                    // обрезаем по символам UTF-8, а не по байтам
                    size_t chars = 0, bytes = 0;
                    while (bytes < label.size() && chars < 6) {
                        if ((static_cast<unsigned char>(label[bytes]) & 0xC0) != 0x80) ++chars;
                        ++bytes;
                    }
                    label = label.substr(0, bytes) + "..";
                }
                draw_text(label, p, static_cast<unsigned>(std::max(10.0f, node_r * 0.62f)),
                          sf::Color(250, 250, 252), true);
            }
        }

        render_hud();
        window_.display();
    }

    void render_hud() {
        // верхняя панель
        sf::RectangleShape top(sf::Vector2f(static_cast<float>(window_.getSize().x), 62.0f));
        top.setFillColor(sf::Color(28, 31, 46, 235));
        window_.draw(top);

        draw_text("Граф: вершин " + std::to_string(vertex_count_) +
                  "   рёбер " + std::to_string(edge_count_),
                  sf::Vector2f(16.0f, 10.0f), 16, sf::Color(236, 240, 248), false);

        std::string mode = "подсветка: ";
        if (!show_highlight_) mode += "выключена";
        else if (highlight_.has_path) mode += "кратчайший путь";
        else if (highlight_.has_mst) mode += "минимальный остов";
        else if (highlight_.has_components)
            mode += "компоненты связности (" + std::to_string(highlight_.component_count) + ")";
        else mode += "нет результата";

        draw_text(mode, sf::Vector2f(16.0f, 34.0f), 13, sf::Color(150, 200, 255), false);

        // нижняя панель: последняя команда и подсказки
        const float h = static_cast<float>(window_.getSize().y);
        sf::RectangleShape bottom(sf::Vector2f(static_cast<float>(window_.getSize().x), 58.0f));
        bottom.setPosition(0.0f, h - 58.0f);
        bottom.setFillColor(sf::Color(28, 31, 46, 235));
        window_.draw(bottom);

        draw_text("> " + command_line_, sf::Vector2f(16.0f, h - 52.0f), 14,
                  sf::Color(255, 213, 128), false);
        draw_text(status_line_, sf::Vector2f(16.0f, h - 32.0f), 12,
                  sf::Color(178, 186, 204), false);

        const std::string keys = "ЛКМ: тащить вершину / панорама   колесо: масштаб   "
                                 "R: пересборка   F: вписать   W: веса   H: подсветка";
        draw_text(keys, sf::Vector2f(16.0f, h - 15.0f), 11,
                  sf::Color(120, 128, 146), false);

        if (!font_ok_) {
            draw_text("font missing", sf::Vector2f(16.0f, 80.0f), 14,
                      sf::Color(255, 120, 120), false);
        }
    }

public:
    GraphWindow()
        : window_(sf::VideoMode(1280, 820), "Графовые алгоритмы — ЛР-3",
                  sf::Style::Default) {
        window_.setFramerateLimit(60);
        font_ok_ = try_load_font(font_);
        pan_ = sf::Vector2f(640.0f, 410.0f);
    }

    bool is_open() const { return window_.isOpen(); }

    // layout_update вызывается ПОД мьютексом: получает const Graph&
    // и обновляет раскладку. Остальное берётся из уже скопированного снапшота.
    template <typename LayoutUpdateFn, typename SnapshotFn>
    void frame(LayoutUpdateFn&& layout_update, SnapshotFn&& take_snapshot) {
        handle_events();

        layout_update(layout_);          // внутри — lock_guard и layout_.sync(...)

        const auto snap = take_snapshot();   // короткие копии под мьютексом
        highlight_ = snap.highlight;
        status_line_ = snap.status;
        command_line_ = snap.command;
        vertex_count_ = snap.vertices;
        edge_count_ = snap.edges;
        if (snap.structure_changed && !first_fit_done_ && layout_.node_count() > 0) {
            fit_view();
            first_fit_done_ = true;
        }

        layout_.step();
        render();
    }

private:
    bool first_fit_done_ = false;
};