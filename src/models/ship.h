#pragma once

#include <vector>
#include <utility>
#include "cell.h"

/**
 * @class Ship
 * @brief Класс, представляющий корабль в игре "Морской бой".
 * 
 * Корабль имеет начальную позицию, размер и ориентацию.
 * Корабль может быть размещен горизонтально или вертикально.
 */
class Ship {
private:
    Cell position;      ///< Начальная позиция корабля (левый верхний угол)
    int length;         ///< Длина корабля (1-4)
    bool horizontal;    ///< Ориентация (true - горизонтально, false - вертикально)
    
public:
    /**
     * @brief Конструктор корабля
     * @param x X-координата начальной позиции
     * @param y Y-координата начальной позиции
     * @param length Длина корабля (1-4)
     * @param horizontal Ориентация (true - горизонтально, false - вертикально)
     */
    Ship(int x, int y, int length, bool horizontal);
    
    /**
     * @brief Конструктор корабля с использованием Cell
     * @param pos Начальная позиция
     * @param length Длина корабля (1-4)
     * @param horizontal Ориентация (true - горизонтально, false - вертикально)
     */
    Ship(const Cell& pos, int length, bool horizontal);
    
    /**
     * @brief Получить начальную позицию корабля
     * @return Начальная позиция
     */
    const Cell& getPosition() const { return position; }
    
    /**
     * @brief Получить X-координату начала корабля
     * @return X-координата
     */
    int getX() const { return position.x; }
    
    /**
     * @brief Получить Y-координату начала корабля
     * @return Y-координата
     */
    int getY() const { return position.y; }
    
    /**
     * @brief Получить длину корабля
     * @return Длина корабля
     */
    int getLength() const { return length; }
    
    /**
     * @brief Проверить, горизонтально ли расположен корабль
     * @return true, если корабль расположен горизонтально, false иначе
     */
    bool isHorizontal() const { return horizontal; }
    
    /**
     * @brief Проверить, вертикально ли расположен корабль (для совместимости)
     * @return true, если корабль расположен вертикально, false иначе
     */
    bool getIsVertical() const { return !horizontal; }
    
    /**
     * @brief Получить конечную позицию корабля
     * @return Конечная позиция
     */
    Cell getEndPosition() const;
    
    /**
     * @brief Проверить, занимает ли корабль указанную клетку
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если корабль занимает клетку, false иначе
     */
    bool occupies(int x, int y) const;
    
    /**
     * @brief Проверить, занимает ли корабль указанную клетку
     * @param cell Клетка
     * @return true, если корабль занимает клетку, false иначе
     */
    bool occupies(const Cell& cell) const;
    
    /**
     * @brief Получить все клетки, занимаемые кораблем
     * @return Вектор клеток
     */
    std::vector<Cell> getAllCells() const;
    
    /**
     * @brief Получить все клетки, занимаемые кораблем (для совместимости)
     * @return Вектор пар (x, y)
     */
    std::vector<std::pair<int, int>> getOccupiedCells() const {
        std::vector<std::pair<int, int>> result;
        std::vector<Cell> cells = getAllCells();
        for (const auto& cell : cells) {
            result.emplace_back(cell.x, cell.y);
        }
        return result;
    }
    
    /**
     * @brief Получить все клетки, занимаемые кораблем
     * @return Вектор пар (x, y)
     */
    inline std::vector<std::pair<int,int>> getCells() const {
        return getOccupiedCells();
    }
    
    /**
     * @brief Проверить, пересекается ли корабль с другим кораблем
     * @param other Другой корабль
     * @return true, если корабли пересекаются, false иначе
     */
    bool intersects(const Ship& other) const;
    
    /**
     * @brief Проверить, касается ли корабль другого корабля
     * @param other Другой корабль
     * @return true, если корабли касаются, false иначе
     */
    bool touches(const Ship& other) const;
    
    /**
     * @brief Сдвинуть корабль
     * @param dx Смещение по X
     * @param dy Смещение по Y
     */
    void move(int dx, int dy);
    
    /**
     * @brief Повернуть корабль
     */
    void rotate();
    
    /**
     * @brief Проверить, находится ли корабль полностью в границах поля
     * @param boardSize Размер игрового поля (обычно 10)
     * @return true, если корабль в границах, false иначе
     */
    bool isWithinBounds(int boardSize) const;
    
    /**
     * @brief Сдвинуть корабль случайным образом
     * @param maxShift Максимальный сдвиг (в клетках)
     */
    void randomShift(int maxShift = 1);

    // Сеттеры
    void setX(int x) { position.x = x; }
    void setY(int y) { position.y = y; }
    void setIsVertical(bool isVertical) { horizontal = !isVertical; } // horizontal - это !isVertical
}; 