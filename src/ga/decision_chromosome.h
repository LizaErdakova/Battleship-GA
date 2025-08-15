#pragma once

#include <vector>
#include "../utils/rng.h"
#include <array>
#include "constants.h"

/**
 * @brief Класс, представляющий хромосому принятия решений в генетическом алгоритме
 * 
 * Хромосома содержит 20 вещественных генов для весов тепловой карты,
 * которые используются для стратегии стрельбы
 */
class DecisionChromosome {
public:
    // Публичные поля для прямого доступа
    std::array<double, DECISION_GENES> weights{};  // 20 весов признаков
    double fitness = 0.0;          // значение фитнеса
    double meanShots = 0.0;        // среднее число выстрелов
    double stdDevShots = 0.0;      // стандартное отклонение числа выстрелов
    
    /**
     * @brief Конструктор по умолчанию
     */
    DecisionChromosome() = default;
    
    /**
     * @brief Конструктор, создающий случайную хромосому принятия решений
     * @param rng Генератор случайных чисел
     */
    explicit DecisionChromosome(RNG& rng);

    /**
     * @brief Конструктор, создающий хромосому с заданными генами
     * @param genes Гены стратегии
     */
    explicit DecisionChromosome(const std::vector<double>& genes);

    /**
     * @brief Конструктор копирования
     * @param other Копируемая хромосома
     */
    DecisionChromosome(const DecisionChromosome& other);

    /**
     * @brief Оператор присваивания
     * @param other Копируемая хромосома
     * @return Ссылка на текущую хромосому
     */
    DecisionChromosome& operator=(const DecisionChromosome& other);

    /**
     * @brief Получает гены стратегии
     * @return Вектор генов стратегии
     */
    const std::vector<double>& getGenes() const { return m_genes; }

    /**
     * @brief Устанавливает гены стратегии
     * @param genes Новые гены стратегии
     */
    void setGenes(const std::vector<double>& genes) { m_genes = genes; }

    /**
     * @brief Получает значение фитнеса
     * @return Значение фитнеса
     */
    double getFitness() const { return m_fitness; }

    /**
     * @brief Устанавливает значение фитнеса
     * @param fitness Новое значение фитнеса
     */
    void setFitness(double fitness) { m_fitness = fitness; }

    /**
     * @brief Получает среднее число ходов
     * @return Среднее число ходов
     */
    double getMeanShots() const { return m_meanShots; }

    /**
     * @brief Устанавливает среднее число ходов
     * @param meanShots Новое среднее число ходов
     */
    void setMeanShots(double meanShots) { m_meanShots = meanShots; }

    /**
     * @brief Получает стандартное отклонение числа ходов
     * @return Стандартное отклонение числа ходов
     */
    double getStdDevShots() const { return m_stdDevShots; }

    /**
     * @brief Устанавливает стандартное отклонение числа ходов
     * @param stdDevShots Новое стандартное отклонение числа ходов
     */
    void setStdDevShots(double stdDevShots) { m_stdDevShots = stdDevShots; }

    /**
     * @brief Получает вес для указанного признака
     * @param featureIndex Индекс признака
     * @return Вес признака
     */
    double getWeight(int featureIndex) const;

private:
    /**
     * @brief Генерирует случайные гены стратегии
     * @param rng Генератор случайных чисел
     */
    void generateRandomGenes(RNG& rng);

private:
    // Гены стратегии (20 вещественных чисел для весов тепловой карты)
    std::vector<double> m_genes;
    
    // Значение фитнеса
    double m_fitness = 0.0;
    
    // Среднее число ходов
    double m_meanShots = 0.0;
    
    // Стандартное отклонение числа ходов
    double m_stdDevShots = 0.0;

public:
    // Константы
    static constexpr int GENES_COUNT = DECISION_GENES;  // Для обратной совместимости
}; 