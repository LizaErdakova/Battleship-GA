#pragma once

#include <vector>
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include "../ga/placement_chromosome.h"
#include "../ga/decision_chromosome.h"
#include "../strategies/strategy.h"
#include "game.h"

/**
 * @brief Класс для многопоточной оценки хромосом
 * 
 * Evaluator позволяет оценивать фитнес хромосом путем запуска нескольких
 * симуляций игр с различными противниками. Используется многопоточность для
 * ускорения процесса оценки.
 */
class Evaluator {
private:
    int boardSize;                   ///< Размер игрового поля
    bool strictAdjacency;            ///< Строгое правило о недопустимости касания кораблей
    int numThreads;                  ///< Количество рабочих потоков
    int gamesPerEvaluation;          ///< Количество игр для оценки каждой хромосомы
    std::vector<std::unique_ptr<Strategy>> opponents; ///< Список стратегий-противников
    
    std::mutex resultMutex;          ///< Мьютекс для защиты общих данных
    
    /**
     * @brief Создает стратегию на основе хромосомы расстановки
     * 
     * @param chromosome Хромосома расстановки
     * @return Указатель на созданную стратегию
     */
    std::unique_ptr<Strategy> createStrategyFromChromosome(const PlacementChromosome& chromosome);
    
    /**
     * @brief Рабочая функция для потоков оценки
     * 
     * @param chromosomes Вектор хромосом расстановки для оценки
     * @param startIdx Начальный индекс
     * @param endIdx Конечный индекс
     * @param threadId Идентификатор потока
     */
    void evaluationWorker(std::vector<PlacementChromosome>& chromosomes, 
                          int startIdx, 
                          int endIdx, 
                          int threadId);
    
    /**
     * @brief Оценивает одну хромосому расстановки
     * 
     * @param chromosome Хромосома расстановки для оценки
     */
    void evaluateChromosome(PlacementChromosome& chromosome);
    
public:
    /**
     * @brief Конструктор оценщика
     * 
     * @param boardSize Размер игрового поля (обычно 10)
     * @param strictAdjacency Строгое правило о недопустимости касания кораблей
     * @param numThreads Количество рабочих потоков (0 - авто)
     * @param gamesPerEvaluation Количество игр для оценки каждой хромосомы
     */
    Evaluator(int boardSize = 10, 
              bool strictAdjacency = true,
              int numThreads = 0,
              int gamesPerEvaluation = 5);
    
    /**
     * @brief Добавляет стратегию-противника
     * 
     * @param opponent Стратегия-противник
     */
    void addOpponent(std::unique_ptr<Strategy> opponent);
    
    /**
     * @brief Оценивает популяцию хромосом расстановки
     * 
     * @param chromosomes Вектор хромосом расстановки для оценки
     */
    void evaluatePopulation(std::vector<PlacementChromosome>& chromosomes);
    
    /**
     * @brief Устанавливает количество игр для оценки
     * 
     * @param games Количество игр
     */
    void setGamesPerEvaluation(int games) { gamesPerEvaluation = games; }
    
    /**
     * @brief Получает количество игр для оценки
     * 
     * @return Количество игр
     */
    int getGamesPerEvaluation() const { return gamesPerEvaluation; }
}; 