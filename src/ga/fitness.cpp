#include "fitness.h"

namespace Fitness {

// Фитнес для расстановки (PlacementGA)
// Цель: минимизировать среднее количество выстрелов для потопления флота.
// Поэтому, чем МЕНЬШЕ взвешенное среднее выстрелов, тем ВЫШЕ должен быть фитнес.
double calculatePlacementFitness(
    const PlacementChromosome& chromosome,
    double meanShotsRandom,     // μ₁ - среднее против Random
    double meanShotsChecker,    // μ₂ - среднее против Checkerboard
    double meanShotsMC          // μ₃ - среднее против Monte-Carlo
) {
    if (!chromosome.isValid()) {
        return -1000.0; // Очень низкий фитнес для невалидных расстановок
    }

    // Если значения не переданы явно, берем их из хромосомы
    // (хотя в main.cpp они теперь всегда передаются)
    // 주석 처리: Эта логика приводила к использованию старых значений из хромосомы, если переданные были нулевыми.
    // if (meanShotsRandom <= 0.001 && chromosome.getMeanShotsRandom() > 0.001) meanShotsRandom = chromosome.getMeanShotsRandom();
    // if (meanShotsChecker <= 0.001 && chromosome.getMeanShotsCheckerboard() > 0.001) meanShotsChecker = chromosome.getMeanShotsCheckerboard();
    // if (meanShotsMC <= 0.001 && chromosome.getMeanShotsMC() > 0.001) meanShotsMC = chromosome.getMeanShotsMC();

    // Веса важности (можно оставить как есть или скорректировать)
    const double w1 = 0.20; // Random
    const double w2 = 0.40; // Checkerboard
    const double w3 = 0.40; // Monte-Carlo

    // Взвешенное среднее число выстрелов
    double weightedAverageShots = w1 * meanShotsRandom +
                    w2 * meanShotsChecker +
                    w3 * meanShotsMC;

   
    double fitness = weightedAverageShots;
    
    // Можно добавить небольшой бонус/штраф за стандартное отклонение, если это нужно,
    // но пока что придерживаемся простой формулы.

    return fitness; 
}

// Фитнес для стратегии стрельбы (DecisionGA)
// Формула из §3.5: F_d = -μ + 0.1σ
double calculateDecisionFitness(
    const DecisionChromosome& chromosome,
    double meanShots,    // μ - среднее число ходов
    double stdDevShots   // σ - стандартное отклонение
) {
    // Если не предоставлены значения - берем из хромосомы
    if (meanShots == 0.0) meanShots = chromosome.getMeanShots();
    if (stdDevShots == 0.0) stdDevShots = chromosome.getStdDevShots();

    // Формула из §3.5: F_d = -μ + 0.1σ
    double fitness = -meanShots + 0.1 * stdDevShots;

    return fitness; // Чем больше, тем лучше (меньше ходов до победы)
}

/**
 * @brief Расчет фитнеса для стратегии стрельбы (DecisionGA) - перегрузка без хромосомы
 * 
 * @param meanShots Среднее число ходов до победы
 * @param stdDevShots Стандартное отклонение числа ходов
 * @return Значение фитнеса (чем больше, тем лучше)
 */
double Fitness::calculateDecisionFitness(double meanShots, double stdDevShots) {
    // Формула из §3.5: F_d = -μ + 0.1σ
    // Минимизируем среднее число ходов, но добавляем небольшую "премию" за стабильность
    return -meanShots + 0.1 * stdDevShots;
}

// Динамический штраф из §2
double calculateDynamicPenalty(
    double initialPenalty,
    int generation,
    double alpha
) {
    // λ(g) = λ₀·(1 + 0.05·g)
    return initialPenalty * (1.0 + alpha * generation);
}

// Для совместимости со старым кодом
double calculateFitness(
    const PlacementChromosome& chromosome, 
    double penalty,
    double meanShots,
    double stdDevShots
) {
    // Вызываем обновленную функцию, передавая 0 для неизвестных средних значений,
    // чтобы они взялись из хромосомы, если там что-то есть (но это старый путь вызова).
    return calculatePlacementFitness(chromosome, chromosome.getMeanShotsRandom(), chromosome.getMeanShotsCheckerboard(), chromosome.getMeanShotsMC());
}

double calculateFitness(
    const DecisionChromosome& chromosome,
    double penalty,
    double meanShots,
    double stdDevShots
) {
    // Игнорируем штраф для хромосомы принятия решений
    return calculateDecisionFitness(chromosome, 0.0, 0.0);
}

} // namespace Fitness 