#pragma once

/**
 * @brief Структура для представления клетки на игровом поле
 */
struct Cell {
    int x;  ///< X-координата
    int y;  ///< Y-координата

    Cell(int x = 0, int y = 0) : x(x), y(y) {}

    bool operator==(const Cell& other) const {
        return x == other.x && y == other.y;
    }

    bool operator!=(const Cell& other) const {
        return !(*this == other);
    }
}; 