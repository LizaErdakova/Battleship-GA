#pragma once

#include <array>
#include "cell.h"
#include "ship.h"
#include "fleet.h"
#include <vector>

/**
 * @enum CellState
 * @brief Перечисление для представления состояния клетки игрового поля.
 */
enum class CellState {
    SEA,        ///< Пустая клетка (море)
    SHIP,       ///< Клетка содержит корабль (неповрежденный)
    HIT,        ///< Клетка содержит попадание по кораблю
    MISS,       ///< Клетка содержит промах (выстрел в море)
    SUNK        ///< Клетка содержит потопленный корабль (для маркировки)
};

/**
 * @class Board
 * @brief Класс, представляющий игровое поле в игре "Морской бой".
 * 
 * Игровое поле представляет собой двумерную сетку 10x10 клеток,
 * каждая из которых может содержать часть корабля или быть пустой.
 * Класс хранит состояние каждой клетки и предоставляет методы для
 * выстрелов и проверки состояния поля.
 */
class Board {
public:
    static constexpr int BOARD_SIZE = 10;
    
    Board();
    
    /**
     * @brief Конструктор с указанием размера (для совместимости)
     * @param boardSize Размер игрового поля (игнорируется, всегда используется BOARD_SIZE)
     */
    explicit Board(int boardSize) : Board() {}
    
    /**
     * @brief Проверяет, свободна ли клетка
     */
    bool isCellFree(int x, int y) const;
    
    /**
     * @brief Проверяет, свободна ли клетка
     */
    bool isCellFree(const Cell& cell) const;
    
    /**
     * @brief Помечает клетку как занятую
     */
    void markCell(int x, int y);
    
    /**
     * @brief Помечает клетку как занятую
     */
    void markCell(const Cell& cell);
    
    /**
     * @brief Очищает клетку
     */
    void clearCell(int x, int y);
    
    /**
     * @brief Очищает клетку
     */
    void clearCell(const Cell& cell);
    
    /**
     * @brief Очищает все поле
     */
    void clear();
    
    /**
     * @brief Установить клетку в состояние "содержит корабль" (устаревший, не использовать для полноценных кораблей)
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если операция выполнена успешно, false иначе
     */
    bool placeShip(int x, int y);

    /**
     * @brief Размещает корабль на доске
     * @param ship Корабль для размещения
     * @return true, если корабль успешно размещен, false иначе
     */
    bool placeShip(const Ship& ship);
    
    /**
     * @brief Размещает флот на доске
     * @param fleet Флот для размещения
     * @return true, если флот успешно размещен, false иначе
     */
    bool placeFleet(const Fleet& fleet);
    
    /**
     * @brief Выстрел по клетке
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если попадание по кораблю, false, если промах или повторный выстрел
     */
    bool shoot(int x, int y);
    
    /**
     * @brief Получить состояние клетки
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return Состояние клетки
     */
    CellState getCell(int x, int y) const;
    
    /**
     * @brief Проверить, была ли клетка уже обстреляна (HIT или MISS)
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если клетка уже обстреляна, false иначе
     */
    bool isShot(int x, int y) const;
    
    /**
     * @brief Проверить, все ли корабли потоплены
     * @return true, если все корабли потоплены, false иначе
     */
    bool allShipsSunk() const;
    
    /**
     * @brief Получить размер поля
     * @return Размер поля
     */
    int getSize() const { return BOARD_SIZE; }
    
    /**
     * @brief Печатает игровое поле в консоль
     * @param showShips true, если нужно показать неповрежденные корабли, false иначе
     */
    void print(bool showShips = false) const;
    
    /**
     * @brief Проверить, была ли клетка уже обстреляна (алиас для isShot)
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если клетка уже обстреляна, false иначе
     */
    bool wasShotAt(int x, int y) const;
    
    /**
     * @brief Проверить, был ли потоплен корабль в данной клетке
     * 
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если в клетке был корабль и он потоплен, false иначе
     */
    bool wasShipSunkAt(int x, int y) const;

    /**
     * @brief Возвращает список клеток с кораблями, по которым еще не было попаданий
     * @return Вектор пар (x, y) с координатами неповрежденных частей кораблей
     */
    std::vector<std::pair<int, int>> getRemainingShipCells() const {
        std::vector<std::pair<int, int>> result;
        for (int y = 0; y < BOARD_SIZE; ++y) {
            for (int x = 0; x < BOARD_SIZE; ++x) {
                if (getCell(x, y) == CellState::SHIP) {
                    result.emplace_back(x, y);
                }
            }
        }
        return result;
    }

    /**
     * @brief Отметить выстрел по клетке для проверочной доски (не для реальной игры)
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если операция выполнена успешно, false иначе
     */
    bool markShot(int x, int y);

    /**
     * @brief Возвращает размер наибольшего не потопленного корабля
     * @return Длина самого большого оставшегося корабля или 0, если кораблей нет
     */
    int largestRemainingShipSize() const;

private:
    std::array<std::array<CellState, BOARD_SIZE>, BOARD_SIZE> m_grid;
    std::vector<Ship> m_ships; // Храним размещенные корабли
    int m_sunkShipCells;       // Общее количество потопленных палуб (для allShipsSunk)
    int m_totalShipCells;      // Общее количество палуб всех размещенных кораблей

    // Вспомогательная функция для проверки, потоплен ли корабль
    bool isShipSunk(const Ship& ship) const;
    // Вспомогательная функция для обновления состояния клеток потопленного корабля
    void markShipAsSunk(const Ship& ship);
}; 