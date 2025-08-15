#pragma once

#include <vector>
#include <memory>
#include <functional>
#include "placement_chromosome.h"
#include "placement_generator.h"
#include "../utils/rng.h"

/**
 * @brief Параметры генетического алгоритма
 */
struct GAParams {
    int    popSize      = 300;   // Размер популяции
    double crossoverP   = 0.9;   // Вероятность кроссовера
    double mutationP    = 0.04;  // Вероятность мутации
    int    tournamentK  = 3;     // Размер турнира
    int    eliteCount   = 8;     // Количество элитных особей
};

/**
 * @brief Класс, реализующий генетический алгоритм для оптимизации расстановки кораблей
 * 
 * Включает:
 * - Инициализация популяции случайных валидных расстановок
 * - Ship-swap crossover (p_cx = 0.8)
 * - Shift/Rotate mutation (p_mut = 0.04)
 * - Функция Repair для исправления расстановок
 * - Турнирная селекция (k=3)
 * - Элитарность (E_p = 2)
 */
class PlacementGA {
public:
    /**
     * @brief Конструктор генетического алгоритма расстановки
     * @param populationSize Размер популяции
     * @param crossoverRate Вероятность кроссовера (0.0-1.0)
     * @param mutationRate Вероятность мутации (0.0-1.0)
     * @param tournamentSize Размер турнира для селекции
     * @param eliteCount Количество элитных особей
     * @param initialPenalty Начальный штраф за нарушение правил
     * @param penaltyAlpha Коэффициент увеличения штрафа
     */
    PlacementGA(
        int populationSize = 100,
        double crossoverRate = 0.8,
        double mutationRate = 0.04,
        int tournamentSize = 3,
        int eliteCount = 2,
        double initialPenalty = 95.0,
        double penaltyAlpha = 0.05
    );

    /**
     * @brief Инициализирует популяцию и запускает генетический алгоритм
     * @param maxGenerations Максимальное число поколений
     * @param targetFitness Целевое значение фитнеса для ранней остановки
     * @param fitnessFunction Функция вычисления фитнеса для хромосомы
     * @return Лучшая найденная хромосома
     */
    PlacementChromosome evolve(
        int maxGenerations,
        double targetFitness,
        const std::function<double(PlacementChromosome&)>& fitnessFunction
    );

    /**
     * @brief Эволюционирует популяцию с использованием заданных параметров
     * @param params Параметры генетического алгоритма
     * @param fitnessFunction Функция вычисления фитнеса для хромосомы
     * @param maxGenerations Максимальное число поколений
     * @param targetFitness Целевое значение фитнеса для ранней остановки
     * @return Лучшая найденная хромосома
     */
    PlacementChromosome evolveWithParams(
        const GAParams& params,
        const std::function<double(PlacementChromosome&)>& fitnessFunction,
        int maxGenerations,
        double targetFitness
    );

    /**
     * @brief Инициализирует популяцию случайными валидными хромосомами
     * @param fitnessFunction Функция вычисления фитнеса для хромосомы
     */
    void initializePopulation(
        const std::function<double(PlacementChromosome&)>& fitnessFunction
    );

    /**
     * @brief Инициализирует GA с уже существующей популяцией
     * @param population Готовая популяция хромосом
     */
    void initializeWithPopulation(const std::vector<PlacementChromosome>& population) {
        m_population = population;
        m_currentGeneration = 0;
        m_currentPenalty = m_initialPenalty;
        m_regeneratedCount = 0;
    }

    /**
     * @brief Выполняет одно поколение генетического алгоритма
     * @param fitnessFunction Функция вычисления фитнеса для хромосомы
     * @return Лучшая хромосома в текущем поколении
     */
    PlacementChromosome evolvePopulation(
        const std::function<double(PlacementChromosome&)>& fitnessFunction
    );

    /**
     * @brief Получает лучшую хромосому из текущей популяции
     * @return Лучшая хромосома
     */
    PlacementChromosome getBestChromosome() const;

    /**
     * @brief Получает текущее поколение
     * @return Номер текущего поколения
     */
    int getCurrentGeneration() const { return m_currentGeneration; }

    /**
     * @brief Получает лучший фитнес текущего поколения
     * @return Лучший фитнес
     */
    double getBestFitness() const;

    /**
     * @brief Получает средний фитнес текущего поколения
     * @return Средний фитнес
     */
    double getAverageFitness() const;

    /**
     * @brief Получает количество перегенерированных невалидных особей
     * @return Количество перегенерированных невалидных особей
     */
    int getRegeneratedCount() const { return m_regeneratedCount; }

    /**
     * @brief Получает текущую популяцию
     * @return Ссылка на вектор хромосом
     */
    const std::vector<PlacementChromosome>& getPopulation() const { return m_population; }

    /**
     * @brief Получает текущий коэффициент мутации
     * @return Коэффициент мутации
     */
    double getMutationRate() const { return m_mutationRate; }

    /**
     * @brief Получает топ N лучших хромосом
     * @param n Количество лучших хромосом
     * @return Вектор лучших хромосом
     */
    std::vector<PlacementChromosome> getTopChromosomes(int n) const {
        // Копируем популяцию для сортировки
        std::vector<PlacementChromosome> sortedPop = m_population;
        // Сортируем по убыванию фитнеса
        std::sort(sortedPop.begin(), sortedPop.end(),
            [](const PlacementChromosome& a, const PlacementChromosome& b) {
                return a.getFitness() > b.getFitness();
            });
        
        // Возвращаем первые n хромосом или все, если population.size() < n
        int count = std::min(n, static_cast<int>(sortedPop.size()));
        return std::vector<PlacementChromosome>(sortedPop.begin(), sortedPop.begin() + count);
    }

    /**
     * @brief Проводит турнирную селекцию для выбора родителя
     * @param k Размер турнира (количество участников)
     * @return Ссылка на выбранную хромосому
     */
    const PlacementChromosome& tournamentSelection(int k) const;

private:
    /**
     * @brief Применяет оператор кроссовера Ship-swap
     * @param parent1 Первый родитель
     * @param parent2 Второй родитель
     * @return Потомок после кроссовера
     */
    PlacementChromosome crossover(
        const PlacementChromosome& parent1,
        const PlacementChromosome& parent2
    );

    /**
     * @brief Применяет оператор мутации Shift/Rotate
     * @param chromosome Хромосома для мутации
     */
    void mutate(PlacementChromosome& chromosome);

    /**
     * @brief Выполняет турнирную селекцию
     * @return Выбранная хромосома
     */
    PlacementChromosome selectParent();

    /**
     * @brief Исправляет невалидную хромосому (Repair)
     * @param chromosome Хромосома для исправления
     * @return true, если исправление успешно
     */
    bool repair(PlacementChromosome& chromosome);
    
    /**
     * @brief Проверяет валидность всех хромосом в популяции
     * @return true, если все хромосомы валидны, false иначе
     */
    bool verifyPopulationValidity() const;

private:
    // Текущая популяция хромосом
    std::vector<PlacementChromosome> m_population;
    
    // Размер популяции
    int m_populationSize;
    
    // Вероятность кроссовера (0.0-1.0)
    double m_crossoverRate;
    
    // Вероятность мутации (0.0-1.0)
    double m_mutationRate;
    
    // Размер турнира для селекции
    int m_tournamentSize;
    
    // Количество элитных особей
    int m_eliteCount;
    
    // Текущее поколение
    int m_currentGeneration;
    
    // Начальный штраф за нарушение правил
    double m_initialPenalty;
    
    // Текущий штраф за нарушение правил
    double m_currentPenalty;
    
    // Коэффициент увеличения штрафа
    double m_penaltyAlpha;
    
    // Генератор случайных чисел
    RNG m_rng;
    
    // Счетчик перегенерированных невалидных особей
    int m_regeneratedCount = 0;
}; 
