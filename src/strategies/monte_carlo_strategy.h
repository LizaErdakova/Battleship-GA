#pragma once

#include "strategy.h"
#include "../models/board.h"
#include "../utils/rng.h"
#include <string>
#include <vector>
#include <utility>
#include <array>
#include <memory>
#include <queue>
#include <set>

/**
 * @brief Стратегия стрельбы на основе метода Монте-Карло
 * 
 * Использует симуляции случайных расстановок кораблей для выбора 
 * наиболее вероятных клеток расположения кораблей противника.
 * Обычно дает около 40-50 ходов для потопления всех кораблей.
 */
class MonteCarloStrategy : public Strategy {
private:
    int m_samples;                          ///< Количество симуляций для каждого хода
    std::vector<std::pair<int, int>> shots; ///< История выстрелов
    RNG m_rng;                              ///< Генератор случайных чисел
    std::array<std::array<int, 10>, 10> prob_board{}; ///< Вероятностная доска ("тепловая карта")
    
    bool m_targeting_mode;                  ///< Режим добивания раненых кораблей
    std::queue<std::pair<int, int>> m_targets; ///< Очередь клеток для добивания
    std::vector<std::pair<int, int>> m_hits;   ///< Список клеток с попаданиями
    bool m_prob_board_valid;                ///< Флаг валидности вероятностной доски
    std::set<std::pair<int, int>> m_excluded_cells; ///< Клетки, исключенные из рассмотрения (вокруг потопленных кораблей)
    
    /**
     * @brief Вспомогательная структура для Монте-Карло симуляции расстановки
     */
    struct MCPlacement {
        std::array<std::array<int, 10>, 10> occ{};  ///< 1 если занято
    };
    
    /**
     * @brief Проверяет, находится ли клетка внутри границ поля
     * 
     * @param x X-координата
     * @param y Y-координата
     * @return true, если клетка внутри поля
     */
    bool inside(int x, int y) const {
        return x >= 0 && x < 10 && y >= 0 && y < 10; 
    }
    
    /**
     * @brief Проверяет, можно ли разместить корабль на позиции
     * 
     * @param x X-координата начала корабля
     * @param y Y-координата начала корабля
     * @param len Длина корабля
     * @param hor Ориентация (true - горизонтальная, false - вертикальная)
     * @param p Текущая симуляция размещения
     * @param board Реальное состояние игрового поля
     * @param hits Список попаданий
     * @return true, если размещение возможно
     */
    bool fits(int x, int y, int len, bool hor, 
              const MCPlacement& p, const Board& board,
              const std::vector<std::pair<int, int>>& hits) const;
    
    /**
     * @brief Размещает корабль на доске
     * 
     * @param x X-координата начала корабля
     * @param y Y-координата начала корабля
     * @param len Длина корабля
     * @param hor Ориентация (true - горизонтальная, false - вертикальная)
     * @param p Текущая симуляция размещения
     */
    void place(int x, int y, int len, bool hor, MCPlacement& p) const;
    
    /**
     * @brief Получает список оставшихся кораблей
     * 
     * @param board Текущее состояние игрового поля
     * @return Вектор длин оставшихся кораблей
     */
    std::vector<int> getRemainingShips(const Board& board) const;
    
    /**
     * @brief Инициализирует вероятностную доску нулями
     */
    void init_prob_board();

    /**
     * @brief Рассчитывает вероятности для всех клеток игрового поля
     * 
     * Создает "тепловую карту" частот положений кораблей на основе 
     * симуляций методом Монте-Карло
     * 
     * @param board Текущее состояние игрового поля
     */
    void build_probability(const Board& board);

    /**
     * @brief Обновляет список попаданий
     * 
     * @param board Текущее состояние игрового поля
     */
    void updateHitsList(const Board& board);

    /**
     * @brief Добавляет клетки в очередь для режима добивания
     * 
     * @param x X-координата попадания
     * @param y Y-координата попадания
     */
    void addTargetsAroundHit(int x, int y);

    /**
     * @brief Убирает клетку из вероятностной доски (при промахе)
     * 
     * @param x X-координата
     * @param y Y-координата
     */
    void removeFromProbBoard(int x, int y);
    
    /**
     * @brief Помечает клетки вокруг потопленного корабля как недоступные для выстрелов
     * Основан на правиле "no-touch" - корабли не могут касаться друг друга
     * 
     * @param board Текущее состояние игрового поля
     */
    void markSurroundingCellsAsUnavailable(const Board& board);

public:
    /**
     * @brief Конструктор стратегии Монте-Карло
     * 
     * @param samples Количество симуляций для каждого хода (по умолчанию 1000)
     */
    explicit MonteCarloStrategy(int samples = 1000)
        : m_samples(samples), m_rng(), m_targeting_mode(false), m_prob_board_valid(false) {
        reset();
    }

    /**
     * @brief Конструктор с передачей генератора случайных чисел
     * 
     * @param rng Генератор случайных чисел
     * @param samples Количество симуляций для каждого хода (по умолчанию 1000)
     */
    explicit MonteCarloStrategy(RNG& rng, int samples = 1000) 
        : m_samples(samples), m_rng(rng), m_targeting_mode(false), m_prob_board_valid(false) {
        reset();
    }

    /**
     * @brief Получает следующую клетку для выстрела
     * 
     * @param board Текущее состояние игрового поля
     * @return Пара координат (x, y) для следующего выстрела
     */
    std::pair<int, int> getNextShot(const Board& board) override;
    
    /**
     * @brief Уведомление о результате выстрела
     * 
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
     * 
     * @return Вектор пар координат (x, y)
     */
    std::vector<std::pair<int, int>> getAllShots() const override;

    /**
     * @brief Получает имя стратегии
     * 
     * @return Строка с именем стратегии
     */
    std::string getName() const override;
}; 