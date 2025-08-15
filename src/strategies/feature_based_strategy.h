#pragma once

#include <vector>
#include <utility>
#include <memory>
#include "strategy.h"
#include "features.h"
#include "../models/cell.h"
#include "../ga/placement_pool.h"

/**
 * @brief Стратегия стрельбы на основе взвешенных признаков
 *
 * Использует признаки из Features и веса из DecisionGA для принятия решений
 */
class FeatureBasedStrategy : public Strategy {
public:
    /**
     * @brief Конструктор стратегии с указанными весами признаков
     * @param weights Веса признаков (должен содержать 20 значений)
     */
    explicit FeatureBasedStrategy(const std::vector<double>& weights);

    /**
     * @brief Определяет клетку для следующего выстрела
     * @param board Текущее состояние игрового поля
     * @return Пара координат (x, y) для следующего выстрела
     */
    std::pair<int, int> getNextShot(const Board& board) override;

    /**
     * @brief Уведомление о результате выстрела
     * @param x X-координата выстрела
     * @param y Y-координата выстрела
     * @param hit true, если попадание, false, если промах
     * @param sunk true, если корабль потоплен, false иначе
     * @param board Текущее состояние игрового поля
     */
    void notifyShotResult(int x, int y, bool hit, bool sunk, const Board& board) override;

    /**
     * @brief Сброс стратегии для новой игры
     */
    void reset() override;

    /**
     * @brief Получает список всех сделанных выстрелов
     * @return Вектор пар координат (x, y)
     */
    std::vector<std::pair<int, int>> getAllShots() const override;

    /**
     * @brief Получает имя стратегии
     * @return Строка с именем стратегии
     */
    std::string getName() const override { return "Feature-Based"; }

private:
    // Веса признаков из DecisionGA
    std::vector<double> m_weights;
    
    // История выстрелов и результатов
    std::vector<std::pair<Cell, ShotResult>> m_shotHistory;
    
    // Счетчик итераций
    int m_iteration;
    
    // Пул расстановок для вычисления признаков
    PlacementPool m_pool;
    
    /**
     * @brief Вычисляет оценку для клетки на основе взвешенных признаков
     * @param cell Клетка для оценки
     * @param board Текущее состояние доски
     * @return Оценка клетки
     */
    double scoreCell(const Cell& cell, const Board& board) const;
}; 