#include "decision_ga.h"
#include "../utils/logger.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <random>
#include <stdexcept>

// Заменяем вложенный тип на обычный
// using DecisionGA::DecisionChromosome = DecisionChromosome;
using Chrom = DecisionChromosome;

DecisionGA::DecisionGA(
    int populationSize,
    double crossoverRate,
    double mutationRate,
    int tournamentSize,
    int eliteCount,
    double initialSigma,
    double finalSigma,
    double weightBound
)
    : m_populationSize(populationSize)
    , m_crossoverRate(crossoverRate)
    , m_mutationRate(mutationRate)
    , m_tournamentSize(tournamentSize)
    , m_eliteCount(eliteCount)
    , m_initialSigma(initialSigma)
    , m_finalSigma(finalSigma)
    , m_weightBound(weightBound)
    , m_currentGeneration(0)
    , m_rng()
{
    // Инициализируем генератор случайных чисел, если это еще не сделано
    // Вызываем любой метод RNG, чтобы убедиться, что он инициализирован
    RNG::getInt(0, 1);
    
    // Проверка параметров
    if (m_populationSize <= 0 || m_tournamentSize <= 0) {
        throw std::invalid_argument("Размеры популяции и турнира должны быть положительными");
    }
    
    if (m_crossoverRate < 0.0 || m_crossoverRate > 1.0 || 
        m_mutationRate < 0.0 || m_mutationRate > 1.0) {
        throw std::invalid_argument("Вероятности кроссовера и мутации должны быть в диапазоне [0, 1]");
    }
    
    if (m_eliteCount < 0 || m_eliteCount > m_populationSize) {
        throw std::invalid_argument("Количество элит должно быть в диапазоне [0, размер популяции]");
    }
    
    if (m_initialSigma < 0.0 || m_finalSigma < 0.0) {
        throw std::invalid_argument("Параметры сигма должны быть неотрицательными");
    }
}

DecisionGA::Chromosome DecisionGA::run(
    int maxGenerations,
    double targetFitness,
    const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction
) {
    // Инициализируем популяцию
    initializePopulation();
    
    // Создаем пул расстановок для оценки стратегий
    PlacementPool pool;
    
    // Оцениваем начальную популяцию
    for (auto& chromosome : m_population) {
        fitnessFunction(chromosome, pool);
    }
    
    // Сортируем популяцию по убыванию фитнеса
    std::sort(m_population.begin(), m_population.end(),
              [](const Chromosome& a, const Chromosome& b) {
                  return a.fitness > b.fitness;
              });
    
    // Получаем лучшую хромосому в начальной популяции
    Chromosome bestChromosome = getBestChromosome();
    
    std::cout << "Поколение 0: Лучший фитнес = " << bestChromosome.fitness 
              << ", Среднее число выстрелов = " << bestChromosome.meanShots
              << ", СКО = " << bestChromosome.stdDevShots << std::endl;
              
    // Логирование начального поколения
    double sigmaNow = m_initialSigma;
    Logger::instance().logDecisionGen(
            0,  // поколение 0
            bestChromosome.fitness,
            getAverageFitness(),
            sigmaNow);
    
    // Основной цикл генетического алгоритма
    for (int gen = 1; gen <= maxGenerations; ++gen) {
        // Устанавливаем текущее поколение для использования в других методах
        m_currentGeneration = gen;
        
        // Вычисляем текущую sigma для мутации
        sigmaNow = calculateMutationSigma(gen);
        
        // Эволюция популяции на одно поколение
        bestChromosome = evolvePopulation(fitnessFunction);
        
        // Логирование информации о текущем поколении
        Logger::instance().logDecisionGen(
            gen, 
            bestChromosome.fitness, 
            getAverageFitness(),
            sigmaNow);
        
        // Выводим информацию о текущем поколении
        std::cout << "Поколение " << gen 
                  << ": Лучший фитнес = " << bestChromosome.fitness
                  << ", Среднее число выстрелов = " << bestChromosome.meanShots
                  << ", СКО = " << bestChromosome.stdDevShots
                  << ", Средний фитнес = " << getAverageFitness() << std::endl;
        
        // Проверяем условие ранней остановки
        if (bestChromosome.fitness >= targetFitness) {
            std::cout << "Целевой фитнес достигнут в поколении " 
                      << gen << std::endl;
            break;
        }
    }
    
    std::cout << "Генетический алгоритм завершен." << std::endl;
    std::cout << "Лучший фитнес: " << bestChromosome.fitness << std::endl;
    std::cout << "Среднее число выстрелов: " << bestChromosome.meanShots << std::endl;
    std::cout << "Стандартное отклонение: " << bestChromosome.stdDevShots << std::endl;
    std::cout << "Веса признаков:" << std::endl;
    
    for (size_t i = 0; i < bestChromosome.weights.size(); ++i) {
        std::cout << "  θ_" << (i + 1) << " = " << bestChromosome.weights[i] << std::endl;
    }
    
    return bestChromosome;
}

void DecisionGA::initializePopulation() {
    m_population.clear();
    m_population.reserve(m_populationSize);
    
    // Диапазоны инициализации для разных признаков
    std::vector<std::pair<double, double>> initRanges = {
        {0.0, 1.0},    // Heat - доля расстановок из PH
        {1.0, 3.0},    // HitNeighbor - 4-сторонние соседи с попаданиями
        {0.5, 2.0},    // DiagHitNeighbor - диагональные соседи с попаданиями
        {-1.0, 1.0},   // Parity - шахматный паттерн
        {0.0, 2.0},    // DistLastHit - близость к последнему попаданию
        {-2.0, 0.0},   // MissCluster - плотность промахов
        {0.0, 1.0},    // RowFree - доля свободных клеток в строке
        {0.0, 1.0},    // ColFree - доля свободных клеток в столбце
        {-1.0, 1.0},   // CenterBias - близость к центру
        {-1.0, 1.0},   // EdgeBias - близость к краям
        {-1.0, 1.0},   // Corner - угловая клетка
        {0.0, 2.0},    // L4Fit - возможность разместить линкор
        {0.0, 1.5},    // L3Fit - возможность разместить крейсер
        {0.0, 1.0},    // L2Fit - возможность разместить эсминец
        {-0.5, 0.5},   // SubFit - возможность разместить подлодку
        {-2.0, 0.0},   // RecentMissPenalty - штраф за близость к свежему промаху
        {-1.0, 1.0},   // TimeDecayHit - затухание влияния старых попаданий
        {-1.0, 1.0},   // TimeDecayMiss - затухание влияния старых промахов
        {0.0, 0.2},    // RandNoise - случайный шум
        {-0.5, 0.5}    // IterParityFlip - чередование четности
    };
    
    // Создаем начальную популяцию со случайными весами
    for (int i = 0; i < m_populationSize; ++i) {
        std::vector<double> weights(DECISION_GENES);
        
        // Инициализируем веса случайными значениями в соответствующих диапазонах
        for (int j = 0; j < DECISION_GENES; ++j) {
            double minVal = initRanges[j].first;
            double maxVal = initRanges[j].second;
            weights[j] = m_rng.uniformReal(minVal, maxVal);
        }
        
        m_population.emplace_back(weights);
    }
}

DecisionGA::Chromosome DecisionGA::evolvePopulation(
    const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction
) {
    // Создаем пул расстановок для оценки стратегий
    PlacementPool pool;
    
    // Создаем новую популяцию
    std::vector<Chromosome> newPopulation;
    newPopulation.reserve(m_populationSize);
    
    // Сохраняем элитных особей
    for (int i = 0; i < m_eliteCount && i < static_cast<int>(m_population.size()); ++i) {
        newPopulation.push_back(m_population[i]);
    }
    
    // Заполняем оставшуюся часть новой популяции потомками
    while (newPopulation.size() < m_populationSize) {
        // Выбираем родителей
        Chromosome parent1 = selectParent();
        Chromosome parent2 = selectParent();
        
        // С вероятностью crossoverRate выполняем кроссовер
        Chromosome offspring = 
            (m_rng.uniformReal(0.0, 1.0) < m_crossoverRate) ?
            crossover(parent1, parent2) : parent1;
        
        // С вероятностью mutationRate выполняем мутацию
        if (m_rng.uniformReal(0.0, 1.0) < m_mutationRate) {
            mutate(offspring);
        }
        
        // Вычисляем фитнес для нового потомка
        fitnessFunction(offspring, pool);
        
        // Добавляем потомка в новую популяцию
        newPopulation.push_back(offspring);
    }
    
    // Заменяем текущую популяцию новой
    m_population = std::move(newPopulation);
    
    // Сортируем популяцию по убыванию фитнеса
    std::sort(m_population.begin(), m_population.end(),
              [](const Chromosome& a, const Chromosome& b) {
                  return a.fitness > b.fitness;
              });
    
    // Возвращаем лучшую хромосому
    return m_population.front();
}

DecisionGA::Chromosome DecisionGA::selectParent() {
    // Реализуем турнирную селекцию
    
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    // Выбираем m_tournamentSize случайных хромосом из популяции
    std::vector<Chromosome> tournament;
    tournament.reserve(m_tournamentSize);
    
    for (int i = 0; i < m_tournamentSize; ++i) {
        int index = m_rng.uniformInt(0, m_population.size() - 1);
        tournament.push_back(m_population[index]);
    }
    
    // Находим хромосому с максимальным фитнесом в турнире
    auto it = std::max_element(tournament.begin(), tournament.end(),
                               [](const Chromosome& a, const Chromosome& b) {
                                   return a.fitness < b.fitness;
                               });
    
    return *it;
}

DecisionGA::Chromosome DecisionGA::crossover(
    const Chromosome& parent1,
    const Chromosome& parent2
) {
    // Реализуем арифметический кроссовер
    // θ_k^child = α θ_k^A + (1-α) θ_k^B, α ~ U(0,1)
    
    // Проверяем размеры векторов весов
    if (parent1.weights.size() != DECISION_GENES || parent2.weights.size() != DECISION_GENES) {
        throw std::invalid_argument("Неверный размер вектора весов");
    }
    
    // Генерируем случайный коэффициент α
    double alpha = m_rng.uniformReal(0.0, 1.0);
    
    // Создаем вектор весов для потомка
    std::vector<double> childWeights(DECISION_GENES);
    
    // Применяем арифметический кроссовер с единым α для всех генов
    for (size_t i = 0; i < DECISION_GENES; ++i) {
        childWeights[i] = alpha * parent1.weights[i] + (1.0 - alpha) * parent2.weights[i];
    }
    
    return Chromosome(childWeights);
}

void DecisionGA::mutate(Chromosome& chromosome) {
    // Реализуем гауссовскую мутацию с затухающей дисперсией
    
    // Проверяем размер вектора весов
    if (chromosome.weights.size() != DECISION_GENES) {
        throw std::invalid_argument("Неверный размер вектора весов");
    }
    
    // Вычисляем текущее стандартное отклонение мутации
    double sigma = calculateMutationSigma(m_currentGeneration);
    
    // Применяем мутацию к каждому гену
    for (size_t i = 0; i < DECISION_GENES; ++i) {
        // Добавляем гауссовский шум
        chromosome.weights[i] += m_rng.normalReal(0.0, sigma);
        
        // Ограничиваем значение веса
        chromosome.weights[i] = std::clamp(chromosome.weights[i], 
                                          -m_weightBound, m_weightBound);
    }
}

double DecisionGA::calculateMutationSigma(int generation) const {
    // Линейное уменьшение стандартного отклонения от m_initialSigma до m_finalSigma
    // σ_g(g) = σ_0 - (σ_0 - σ_min) * g / G_d
    
    // Установим правильное максимальное количество поколений
    int maxGenerations = 100; // Ранее было жестко закодировано значение 10
    
    double progress = static_cast<double>(generation) / static_cast<double>(maxGenerations);
    progress = std::min(1.0, std::max(0.0, progress)); // Ограничиваем в диапазоне [0, 1]
    
    return m_initialSigma - (m_initialSigma - m_finalSigma) * progress;
}

DecisionGA::Chromosome DecisionGA::getBestChromosome() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    // Находим хромосому с максимальным фитнесом
    auto it = std::max_element(m_population.begin(), m_population.end(),
                               [](const Chromosome& a, const Chromosome& b) {
                                   return a.fitness < b.fitness;
                               });
    
    return *it;
}

double DecisionGA::getBestFitness() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    return m_population.front().fitness;
}

double DecisionGA::getAverageFitness() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    double sum = std::accumulate(m_population.begin(), m_population.end(), 0.0,
                                [](double sum, const Chromosome& chromosome) {
                                    return sum + chromosome.fitness;
                                });
    
    return sum / m_population.size();
} 