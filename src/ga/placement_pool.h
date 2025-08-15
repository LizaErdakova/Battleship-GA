#pragma once

#include <vector>
#include <random>
#include <memory>
#include "placement_chromosome.h"
#include "../utils/rng.h"
#include "../strategies/random_strategy.h"
#include "../strategies/checkerboard_strategy.h"
#include "../simulator/game.h"
#include <cmath>

/**
 * @brief Класс для управления гибридным пулом расстановок
 * 
 * Реализует механизм из §2.7.2 математической модели:
 * - P_best: 50 "ядовитых" схем от PlacementGA
 * - P_rand: 50 случайных корректных расстановок
 * - Вероятность выбора: 0.7 для P_best, 0.3 для P_rand
 */
class PlacementPool {
public:
    /**
     * @brief Конструктор пула расстановок
     * 
     * @param bestPoolSize Размер пула лучших расстановок (по умолчанию 50)
     * @param randPoolSize Размер пула случайных расстановок (по умолчанию 50)
     * @param bestProb Вероятность выбора из P_best (по умолчанию 0.7)
     */
    PlacementPool(
        size_t bestPoolSize = 50,
        size_t randPoolSize = 50,
        double bestProb = 0.7
    );

    /**
     * @brief Добавляет лучшие расстановки от PlacementGA
     * 
     * @param placements Вектор лучших расстановок
     */
    void setBestPlacements(const std::vector<PlacementChromosome>& placements);

    /**
     * @brief Добавляет случайные валидные расстановки
     * 
     * @param placements Вектор случайных расстановок
     */
    void setRandomPlacements(const std::vector<PlacementChromosome>& placements);

    /**
     * @brief Добавляет новую расстановку в пул
     * 
     * @param placement Хромосома для добавления
     */
    void addPlacement(const PlacementChromosome& placement);

    /**
     * @brief Возвращает размер общего пула расстановок
     * 
     * @return Общее количество расстановок в пуле
     */
    size_t size() const { return bestPlacements.size() + randomPlacements.size(); }

    /**
     * @brief Проверяет, пуст ли пул расстановок
     * 
     * @return true, если пул пуст, false иначе
     */
    bool empty() const noexcept { return bestPlacements.empty() && randomPlacements.empty(); }

    /**
     * @brief Возвращает расстановку по индексу
     *
     * @param index Индекс расстановки
     * @return Константная ссылка на хромосому
     */
    const PlacementChromosome& getPlacement(size_t index) const;
    
    /**
     * @brief Возвращает случайную расстановку из пула
     *
     * @return Копия хромосомы расстановки
     */
    PlacementChromosome getRandomPlacement();

    /**
     * @brief Возвращает все лучшие расстановки
     *
     * @return Вектор лучших расстановок
     */
    const std::vector<PlacementChromosome>& getBestPlacements() const { return bestPlacements; }

    /**
     * @brief Возвращает все случайные расстановки
     *
     * @return Вектор случайных расстановок
     */
    const std::vector<PlacementChromosome>& getRandomPlacements() const { return randomPlacements; }

private:
    std::vector<PlacementChromosome> bestPlacements;  ///< Пул лучших расстановок
    std::vector<PlacementChromosome> randomPlacements; ///< Пул случайных расстановок
    size_t bestPoolSize; ///< Размер пула лучших расстановок
    size_t randPoolSize; ///< Размер пула случайных расстановок
    double bestProb;     ///< Вероятность выбора из лучшего пула
    RNG rng;             ///< Генератор случайных чисел
};

/**
 * @brief Класс для тестирования расстановки против различных стрелков
 */
class ShooterPool {
public:
    /**
     * @brief Конструктор класса
     * @param randomGames Количество игр для стратегии Random
     * @param checkerGames Количество игр для стратегии Checkerboard
     * @param mcGames Количество игр для стратегии Monte-Carlo
     * @param mcIterations Количество итераций для метода Монте-Карло
     */
    ShooterPool(int randomGames = 15, int checkerGames = 15, int mcGames = 10, int mcIterations = 1000);
    
    /**
     * @brief Оценка хромосомы против случайного стрелка
     * @param chromosome Оцениваемая хромосома
     * @return Среднее количество ходов до поражения
     */
    double random(const PlacementChromosome& chromosome);
    
    /**
     * @brief Оценка хромосомы против стрелка с шахматной стратегией
     * @param chromosome Оцениваемая хромосома
     * @return Среднее количество ходов до поражения
     */
    double checker(const PlacementChromosome& chromosome);
    
    /**
     * @brief Оценка хромосомы против стрелка с стратегией Монте-Карло
     * @param chromosome Оцениваемая хромосома
     * @return Среднее количество ходов до поражения
     */
    double montecarlo(const PlacementChromosome& chromosome);
    
    /**
     * @brief Комплексная оценка хромосомы против всех стрелков с весами
     * @param chromosome Оцениваемая хромосома
     * @return Значение фитнеса от 0.0 до 100.0
     */
    double evaluate(PlacementChromosome& chromosome);

private:
    int m_randomGames;    // Количество игр для стратегии Random
    int m_checkerGames;   // Количество игр для стратегии Checkerboard
    int m_mcGames;        // Количество игр для стратегии Monte-Carlo
    int m_mcIterations;   // Количество итераций для метода Монте-Карло
    RNG m_rng;            // Генератор случайных чисел
}; 