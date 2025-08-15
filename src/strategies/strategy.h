#pragma once

#include <utility>
#include <vector>
#include <string>
#include "../models/board.h"

/**
 * @brief Абстрактный базовый класс для всех стратегий стрельбы
 * 
 * Этот класс определяет интерфейс, который должны реализовать
 * все конкретные стратегии стрельбы (Random, Checkerboard, HeatMap, Genetic).
 */
class Strategy {
public:
    /**
     * @brief Виртуальный деструктор
     */
    virtual ~Strategy() = default;
    
    /**
     * @brief Определяет клетку для следующего выстрела
     * 
     * @param board Текущее состояние игрового поля
     * @return Пара координат (x, y) для следующего выстрела
     */
    virtual std::pair<int, int> getNextShot(const Board& board) = 0;
    
    /**
     * @brief Уведомление о результате выстрела
     * 
     * @param x X-координата выстрела
     * @param y Y-координата выстрела
     * @param hit true, если попадание, false, если промах
     * @param sunk true, если корабль потоплен, false иначе
     * @param board Текущее состояние игрового поля (для более эффективного добивания)
     */
    virtual void notifyShotResult(int x, int y, bool hit, bool sunk, const Board& board) = 0;
    
    /**
     * @brief Сброс стратегии для новой игры
     */
    virtual void reset() = 0;
    
    /**
     * @brief Получает список всех сделанных выстрелов
     * 
     * @return Вектор пар координат (x, y)
     */
    virtual std::vector<std::pair<int, int>> getAllShots() const = 0;
    
    /**
     * @brief Получает имя стратегии
     * 
     * @return Строка с именем стратегии
     */
    virtual std::string getName() const = 0;
}; 