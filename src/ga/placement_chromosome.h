#pragma once

#include <vector>
#include <memory>
#include <array>
#include <string>
#include "../models/fleet.h"
#include "../utils/rng.h"
#include "constants.h"

/**
 * @brief Класс, представляющий хромосому размещения кораблей в генетическом алгоритме
 * 
 * Хромосома содержит 30 целых генов: x, y, ориентация для каждого из 10 кораблей
 */
class PlacementChromosome {
public:
    /**
     * @brief Конструктор по умолчанию
     */
    PlacementChromosome();

    /**
     * @brief Конструктор для создания хромосомы со случайными генами
     * @param rng Генератор случайных чисел
     */
    PlacementChromosome(RNG& rng);

    /**
     * @brief Конструктор для создания хромосомы с заданными генами
     * @param genes Вектор генов
     */
    PlacementChromosome(const std::vector<int>& genes);

    /**
     * @brief Конструктор копирования
     * @param other Копируемая хромосома
     */
    PlacementChromosome(const PlacementChromosome& other);

    /**
     * @brief Оператор присваивания
     * @param other Копируемая хромосома
     * @return Ссылка на текущую хромосому
     */
    PlacementChromosome& operator=(const PlacementChromosome& other);

    /**
     * @brief Проверяет, валидна ли хромосома
     * @return true, если хромосома валидна, иначе false
     */
    bool isValid() const;

    /**
     * @brief Декодирует хромосому в объект Fleet
     * @return Указатель на объект Fleet
     */
    std::shared_ptr<Fleet> decodeFleet() const;

    /**
     * @brief Получает гены хромосомы
     * @return Константная ссылка на вектор генов
     */
    const std::vector<int>& getGenes() const { return m_genes; }

    /**
     * @brief Устанавливает гены хромосомы
     * @param g Новый вектор генов
     */
    void setGenes(const std::vector<int>& g) { m_genes = g; }

    /**
     * @brief Получает среднее число выстрелов
     * @return Среднее число выстрелов
     */
    double getMeanShots() const { return m_meanShots; }

    /**
     * @brief Получает стандартное отклонение числа выстрелов
     * @return Стандартное отклонение числа выстрелов
     */
    double getStdDevShots() const { return m_stdDevShots; }

    /**
     * @brief Получает среднее число выстрелов для стратегии шахматной доски
     * @return Среднее число выстрелов для стратегии шахматной доски
     */
    double getMeanShotsChecker() const { return m_meanShotsCheckerboard; }

    /**
     * @brief Устанавливает среднее число выстрелов
     * @param v Новое значение среднего числа выстрелов
     */
    void setMeanShots(double v) { m_meanShots = v; }

    /**
     * @brief Устанавливает среднее число выстрелов для стратегии шахматной доски
     * @param v Новое значение среднего числа выстрелов для стратегии шахматной доски
     */
    void setMeanShotsChecker(double v) { m_meanShotsCheckerboard = v; }

    /**
     * @brief Синонимы для обратной совместимости
     */
    inline void setMeanShotsCheckerboard(double v) { m_meanShotsCheckerboard = v; }
    inline double getMeanShotsCheckerboard() const { return m_meanShotsCheckerboard; }

    /**
     * @brief Устанавливает стандартное отклонение числа выстрелов
     * @param v Новое значение стандартного отклонения числа выстрелов
     */
    void setStdDevShots(double v) { m_stdDevShots = v; }

    /**
     * @brief Получает значение фитнеса
     * @return Значение фитнеса
     */
    double getFitness() const { return m_fitness; }

    /**
     * @brief Устанавливает значение фитнеса
     * @param f Новое значение фитнеса
     */
    void setFitness(double f) { m_fitness = f; }

    /**
     * @brief Получает среднее число выстрелов для случайной стратегии
     * @return Среднее число выстрелов для случайной стратегии
     */
    double getMeanShotsRandom() const { return m_meanShotsRandom; }

    /**
     * @brief Устанавливает среднее число выстрелов для случайной стратегии
     * @param v Новое значение среднего числа выстрелов для случайной стратегии
     */
    void setMeanShotsRandom(double v) { m_meanShotsRandom = v; }

    /**
     * @brief Получает среднее число выстрелов для стратегии Монте-Карло
     * @return Среднее число выстрелов для стратегии Монте-Карло
     */
    double getMeanShotsMC() const { return m_meanShotsMC; }

    /**
     * @brief Устанавливает среднее число выстрелов для стратегии Монте-Карло
     * @param v Новое значение среднего числа выстрелов для стратегии Монте-Карло
     */
    void setMeanShotsMC(double v) { m_meanShotsMC = v; }

    /**
     * @brief Возвращает строковое представление хромосомы для сериализации
     */
    std::string serialize() const;

    // Статические методы для генерации хромосом
    static std::vector<int> generateValidRandomGenes(RNG& rng);
    static std::vector<int> generateCornerPlacement(RNG& rng);
    static std::vector<int> generateEdgePlacement(RNG& rng);
    static std::vector<int> generateCenterPlacement(RNG& rng);
    static std::vector<int> generateMixedPlacement(RNG& rng);

    // Константы
    static const int SHIP_COUNT = 10;
    static const int GENES_COUNT = PLACEMENT_GENES; // x, y, orientation для каждого корабля
    static const std::array<int, SHIP_COUNT> SHIP_LENGTHS;

private:
    std::vector<int> m_genes;    // гены: x1,y1,o1, x2,y2,o2, ...
    double m_fitness;            // значение функции приспособленности
    double m_meanShots;          // среднее число выстрелов для потопления
    double m_stdDevShots;        // стандартное отклонение числа выстрелов
    
    // Дополнительная статистика для различных стратегий
    double m_meanShotsRandom = 0.0;        // среднее для RandomStrategy
    double m_meanShotsCheckerboard = 0.0;  // среднее для CheckerboardStrategy
    double m_meanShotsMC = 0.0;            // среднее для MonteCarloStrategy

    void generateRandomGenes(RNG& rng);
}; 