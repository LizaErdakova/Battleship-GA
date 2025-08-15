#pragma once

#include "placement_chromosome.h"
#include "decision_chromosome.h"

/**
 * @brief Пространство имен для фитнес-функций
 */
namespace Fitness {

/**
 * @brief Расчет фитнеса для расстановки кораблей (PlacementGA)
 * 
 * Формула из §2.2.3: F_p = 0.40*μ₁ + 0.30*μ₂ + 0.30*μ₃
 * где:
 * - μ₁ - среднее число ходов против Random (15 игр)
 * - μ₂ - среднее число ходов против Checkerboard (15 игр)
 * - μ₃ - среднее число ходов против Monte-Carlo-1000 (10 игр)
 * 
 * Чем больше F_p, тем дольше живет флот против трех стилей атаки.
 * 
 * @param chromosome Хромосома расстановки для оценки
 * @param meanShotsRandom Среднее число ходов против Random (μ₁)
 * @param meanShotsChecker Среднее число ходов против Checkerboard (μ₂)
 * @param meanShotsMC Среднее число ходов против Monte-Carlo-1000 (μ₃)
 * @return Значение фитнеса (чем больше, тем лучше)
 */
double calculatePlacementFitness(
    const PlacementChromosome& chromosome,
    double meanShotsRandom = 0.0,
    double meanShotsChecker = 0.0,
    double meanShotsMC = 0.0
);

/**
 * @brief Расчет фитнеса для стратегии стрельбы (DecisionGA)
 * 
 * Формула из §3.5: F_d = -μ + 0.1σ
 * где:
 * - μ - среднее число ходов до победы
 * - σ - стандартное отклонение числа ходов
 * 
 * Чем больше F_d, тем лучше стратегия (меньше ходов до победы).
 * 
 * @param chromosome Хромосома стратегии для оценки
 * @param meanShots Среднее число ходов (μ)
 * @param stdDevShots Стандартное отклонение числа ходов (σ)
 * @return Значение фитнеса (чем больше, тем лучше)
 */
double calculateDecisionFitness(
    const DecisionChromosome& chromosome,
    double meanShots = 0.0,
    double stdDevShots = 0.0
);

/**
 * @brief Упрощенный расчет фитнеса для стратегии стрельбы (DecisionGA)
 * 
 * @param meanShots Среднее число ходов (μ)
 * @param stdDevShots Стандартное отклонение числа ходов (σ)
 * @return Значение фитнеса (чем больше, тем лучше)
 */
double calculateDecisionFitness(
    double meanShots,
    double stdDevShots
);

/**
 * @brief Расчет динамического штрафа
 * 
 * Формула из §2: λ(g) = λ₀·(1 + 0.05·g)
 * где:
 * - λ₀ - начальный штраф
 * - g - текущее поколение
 * 
 * @param initialPenalty Начальный штраф (λ₀)
 * @param generation Текущее поколение (g)
 * @param alpha Коэффициент увеличения штрафа (по умолчанию 0.05)
 * @return Значение динамического штрафа
 */
double calculateDynamicPenalty(
    double initialPenalty,
    int generation,
    double alpha = 0.05
);

// Для совместимости со старым кодом (будет удалено в будущем)
double calculateFitness(
    const PlacementChromosome& chromosome, 
    double penalty = 95.0,
    double meanShots = 0.0,
    double stdDevShots = 0.0
);

double calculateFitness(
    const DecisionChromosome& chromosome,
    double penalty = 0.0,
    double meanShots = 0.0,
    double stdDevShots = 0.0
);

} // namespace Fitness 