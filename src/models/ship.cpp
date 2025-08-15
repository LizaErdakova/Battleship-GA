#include "ship.h"
#include "../utils/rng.h"
#include <algorithm>
#include <cmath>
#include <random>

Ship::Ship(int x, int y, int length, bool horizontal)
    : position(x, y), length(length), horizontal(horizontal) {
}

Ship::Ship(const Cell& pos, int length, bool horizontal)
    : position(pos), length(length), horizontal(horizontal) {
}

Cell Ship::getEndPosition() const {
    if (horizontal) {
        return Cell(position.x + length - 1, position.y);
    } else {
        return Cell(position.x, position.y + length - 1);
    }
}

void Ship::rotate() {
    horizontal = !horizontal;
}

void Ship::move(int dx, int dy) {
    position.x += dx;
    position.y += dy;
}

std::vector<Cell> Ship::getAllCells() const {
    std::vector<Cell> cells;
    cells.reserve(length);
    
    for (int i = 0; i < length; ++i) {
        if (!horizontal) {
            cells.emplace_back(position.x, position.y + i);
        } else {
            cells.emplace_back(position.x + i, position.y);
        }
    }
    
    return cells;
}

bool Ship::occupies(int x, int y) const {
    if (horizontal) {
        return (y == position.y && x >= position.x && x < position.x + length);
    } else {
        return (x == position.x && y >= position.y && y < position.y + length);
    }
}

bool Ship::occupies(const Cell& cell) const {
    return occupies(cell.x, cell.y);
}

bool Ship::intersects(const Ship& other) const {
    // Получаем все клетки, занимаемые кораблями
    auto thisCells = getAllCells();
    auto otherCells = other.getAllCells();
    
    // Проверяем, есть ли общие клетки
    for (const auto& cell1 : thisCells) {
        for (const auto& cell2 : otherCells) {
            if (cell1 == cell2) {
                return true;
            }
        }
    }
    
    return false;
}

bool Ship::touches(const Ship& other) const {
    // Получаем все клетки, занимаемые кораблями
    auto thisCells = getAllCells();
    auto otherCells = other.getAllCells();
    
    // Проверяем метрику Чебышева для всех пар клеток
    for (const auto& cell1 : thisCells) {
        for (const auto& cell2 : otherCells) {
            // Расстояние Чебышева = max(|x1 - x2|, |y1 - y2|)
            int chebyshevDist = std::max(std::abs(cell1.x - cell2.x), 
                                         std::abs(cell1.y - cell2.y));
            if (chebyshevDist <= 1 && cell1 != cell2) {
                return true;
            }
        }
    }
    
    return false;
}

bool Ship::isWithinBounds(int boardSize) const {
    // Проверяем, что начальная точка корабля находится в пределах поля
    if (position.x < 0 || position.y < 0 || position.x >= boardSize || position.y >= boardSize) {
        return false;
    }
    
    // Проверяем, что конечная точка корабля находится в пределах поля
    Cell endPos = getEndPosition();
    
    return endPos.x < boardSize && endPos.y < boardSize;
}

void Ship::randomShift(int maxShift) {
    // Сдвиг по X
    int dx = RNG::getInt(-maxShift, maxShift);
    position.x += dx;
    
    // Сдвиг по Y
    int dy = RNG::getInt(-maxShift, maxShift);
    position.y += dy;
} 