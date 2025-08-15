#pragma once

#include <vector>
#include <functional>
#include <memory>
#include "../utils/rng.h"
#include "../strategies/features.h"
#include "placement_pool.h"
#include "decision_chromosome.h"
#include "../strategies/monte_carlo_strategy.h"

/**
 * @brief Класс для генетического алгоритма оптимизации стратегии стрельбы
 * 
 * Реализует DecisionGA из §2.9 математической модели:
 * - Оптимизирует вектор весов θ ∈ R^20 для признаков стратегии стрельбы
 * - Использует арифметический кроссовер и гауссовскую мутацию
 * - Минимизирует среднее число выстрелов до победы
 */
class DecisionGA {
public:
    using Chromosome = DecisionChromosome;   // локальный алиас
    
    /**
     * @brief Конструктор DecisionGA
     * 
     * @param populationSize Размер популяции (Q в модели)
     * @param crossoverRate Вероятность кроссовера (p_cx)
     * @param mutationRate Вероятность мутации (p_mut)
     * @param tournamentSize Размер турнира для селекции
     * @param eliteCount Количество элитных особей (E_d)
     * @param initialSigma Начальное стандартное отклонение мутации (σ_0)
     * @param finalSigma Конечное стандартное отклонение мутации (σ_min)
     * @param weightBound Ограничение на веса [-weightBound, +weightBound]
     */
    DecisionGA(
        int populationSize = 150,
        double crossoverRate = 0.8,
        double mutationRate = 0.3,
        int tournamentSize = 3,
        int eliteCount = 2,
        double initialSigma = 0.2,
        double finalSigma = 0.01,
        double weightBound = 5.0
    );
    
    /**
     * @brief Запускает генетический алгоритм
     * 
     * @param maxGenerations Максимальное количество поколений
     * @param targetFitness Целевое значение фитнеса для ранней остановки
     * @param fitnessFunction Функция оценки фитнеса
     * @return Лучшая хромосома
     */
    Chromosome run(
        int maxGenerations,
        double targetFitness,
        const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction
    );
    
    /**
     * @brief Получает лучшую хромосому в текущей популяции
     * 
     * @return Лучшая хромосома
     */
    Chromosome getBestChromosome() const;
    
    /**
     * @brief Получить фитнес лучшей хромосомы
     */
    double getBestFitness() const;
    
    /**
     * @brief Получить средний фитнес популяции
     */
    double getAverageFitness() const;
    
    /**
     * @brief Получает текущую популяцию
     * @return Ссылка на вектор хромосом
     */
    const std::vector<Chromosome>& getPopulation() const { return m_population; }
    
    /**
     * @brief Получает текущее значение сигма для мутации
     * @return Текущее значение стандартного отклонения мутации
     */
    double getSigma() const { return calculateMutationSigma(m_currentGeneration); }
    
    /**
     * @brief Получает текущий коэффициент мутации
     * @return Коэффициент мутации
     */
    double getMutationRate() const { return m_mutationRate; }
    
    /**
     * @brief Инициализирует GA с уже загруженной популяцией и функцией фитнеса
     * @param population Популяция хромосом
     * @param pool Пул расстановок для оценки фитнеса
     * @param fitnessFunction Функция оценки фитнеса
     */
    void initializeWithPopulation(
        const std::vector<Chromosome>& population, 
        const PlacementPool& pool,
        const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction) 
    {
        m_population = population;
        m_currentGeneration = 0;
    }
    
    /**
     * @brief Инициализирует начальную популяцию и вычисляет фитнесы 
     * @param fitnessFunction Функция оценки фитнеса
     * @param pool Пул расстановок для оценки фитнеса
     */
    void initialize(
        const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction,
        const PlacementPool& pool)
    {
        initializePopulation();
        
        // Вычисляем фитнес для каждой хромосомы в начальной популяции
        for (auto& chromosome : m_population) {
            fitnessFunction(chromosome, pool);
        }
    }
    
    /**
     * @brief Выполняет одно поколение эволюции
     * @param fitnessFunction Функция оценки фитнеса
     * @param pool Пул расстановок для оценки фитнеса
     */
    void evolveOneGeneration(
        const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction,
        const PlacementPool& pool)
    {
        evolvePopulation(fitnessFunction);
    }
    
    /**
     * @brief Получает топ N лучших хромосом
     * @param n Количество лучших хромосом
     * @return Вектор лучших хромосом
     */
    std::vector<Chromosome> getTopChromosomes(int n) const {
        // Создаем копию популяции для сортировки
        std::vector<Chromosome> sortedPop = m_population;
        // Сортируем по убыванию фитнеса
        std::sort(sortedPop.begin(), sortedPop.end(),
            [](const Chromosome& a, const Chromosome& b) {
                return a.getFitness() > b.getFitness();
            });
        
        // Возвращаем первые n хромосом или все, если population.size() < n
        int count = std::min(n, static_cast<int>(sortedPop.size()));
        return std::vector<Chromosome>(sortedPop.begin(), sortedPop.begin() + count);
    }
    
private:
    /**
     * @brief Инициализация начальной популяции
     */
    void initializePopulation();
    
    /**
     * @brief Эволюция популяции на одно поколение
     * 
     * @param fitnessFunction Функция оценки фитнеса
     * @return Лучшая хромосома в новом поколении
     */
    Chromosome evolvePopulation(
        const std::function<void(Chromosome&, const PlacementPool&)>& fitnessFunction
    );
    
    /**
     * @brief Выбор родительской хромосомы методом турнирной селекции
     * 
     * @return Выбранная хромосома
     */
    Chromosome selectParent();
    
    /**
     * @brief Применяет оператор кроссовера к двум родительским хромосомам
     * 
     * @param parent1 Первый родитель
     * @param parent2 Второй родитель
     * @return Хромосома-потомок
     */
    Chromosome crossover(const Chromosome& parent1, const Chromosome& parent2);
    
    /**
     * @brief Мутация хромосомы
     * 
     * @param chromosome Хромосома для мутации
     */
    void mutate(Chromosome& chromosome);
    
    /**
     * @brief Вычисление текущего стандартного отклонения мутации
     * 
     * @param generation Текущее поколение
     * @return Стандартное отклонение мутации
     */
    double calculateMutationSigma(int generation) const;

private:
    int m_populationSize;           ///< Размер популяции (Q)
    double m_crossoverRate;         ///< Вероятность кроссовера (p_cx)
    double m_mutationRate;          ///< Вероятность мутации (p_mut)
    int m_tournamentSize;           ///< Размер турнира для селекции
    int m_eliteCount;               ///< Количество элитных особей (E_d)
    double m_initialSigma;          ///< Начальное стандартное отклонение мутации (σ_0)
    double m_finalSigma;            ///< Конечное стандартное отклонение мутации (σ_min)
    double m_weightBound;           ///< Ограничение на веса [-weightBound, +weightBound]
    int m_currentGeneration;        ///< Текущее поколение
    
    std::vector<Chromosome> m_population;  ///< Популяция хромосом
    RNG m_rng;                                     ///< Генератор случайных чисел
    
    static constexpr int FEATURE_COUNT = 20;       ///< Количество признаков (размерность θ)
}; 