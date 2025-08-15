#include "evaluator.h"
#include "../strategies/random_strategy.h"
#include "../strategies/checkerboard_strategy.h"
#include "../ga/fitness.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <iostream>
#include <thread>

Evaluator::Evaluator(int boardSize, 
                     bool strictAdjacency, 
                     int numThreads,
                     int gamesPerEvaluation)
    : boardSize(boardSize), 
      strictAdjacency(strictAdjacency),
      numThreads(numThreads),
      gamesPerEvaluation(gamesPerEvaluation) {
          
    // Если количество потоков не задано, используем доступное количество
    if (this->numThreads <= 0) {
        this->numThreads = std::max(1, (int)std::thread::hardware_concurrency() - 1);
    }
    
    // По умолчанию добавляем стратегии-противники
    addOpponent(std::make_unique<RandomStrategy>(boardSize));      // π₁
    addOpponent(std::make_unique<CheckerboardStrategy>(boardSize)); // π₂
    // Monte-Carlo-1000 будет добавлен позже
}

void Evaluator::addOpponent(std::unique_ptr<Strategy> opponent) {
    opponents.push_back(std::move(opponent));
}

std::unique_ptr<Strategy> Evaluator::createStrategyFromChromosome(const PlacementChromosome& chromosome) {
    // В будущем здесь будет создание GeneticStrategy
    // Пока используем CheckerboardStrategy как заглушку
    return std::make_unique<CheckerboardStrategy>(boardSize);
}

void Evaluator::evaluateChromosome(PlacementChromosome& chromosome) {
    // Декодируем флот из хромосомы
    auto fleet = chromosome.decodeFleet();
    
    // Если флот невалидный, применяем штраф
    if (!fleet || !fleet->isValid()) {
        chromosome.setFitness(-100.0);
        return;
    }
    
    // Количество игр для каждой стратегии из §2.2.1
    std::vector<int> gamesPerStrategy = {15, 15}; // M₁=15, M₂=15 (M₃=10 будет добавлен позже)
    std::vector<double> shotsPerStrategy(2, 0.0); // μ₁, μ₂ (μ₃ будет добавлен позже)
    
    // Для каждого противника запускаем игры
    for (size_t i = 0; i < opponents.size(); ++i) {
        std::vector<int> strategyShots;
        
        // Моделируем M_q партий для каждой стратегии π^(q)
        for (int g = 0; g < gamesPerStrategy[i]; ++g) {
            // Создаем новую игру с клоном противника
            std::unique_ptr<Strategy> opponentClone;
            if (opponents[i]->getName() == "Random") {
                opponentClone = std::make_unique<RandomStrategy>(boardSize);
            } else {
                opponentClone = std::make_unique<CheckerboardStrategy>(boardSize);
            }
            
            // Создаем пустую стратегию для второго игрока (она не будет использоваться)
            auto dummyStrategy = std::make_unique<RandomStrategy>(boardSize);
            
            Game currentGame(std::move(opponentClone), 
                           std::move(dummyStrategy),
                           boardSize, 
                           strictAdjacency);
            
            // Инициализируем игру с флотом из хромосомы
            Fleet randomFleet; // Используем конструктор по умолчанию
            RNG rng; // Создаем RNG
            randomFleet.createStandardFleet(rng, boardSize);
            
            if (currentGame.initialize(*fleet, randomFleet)) {
                currentGame.simulate();
                if (currentGame.isGameOver()) {
                    strategyShots.push_back(currentGame.getPlayer1Shots());
            }
        }
    }
    
        // Вычисляем среднее μ_q для текущей стратегии
        if (!strategyShots.empty()) {
            double mean = std::accumulate(strategyShots.begin(), strategyShots.end(), 0.0) / strategyShots.size();
            shotsPerStrategy[i] = mean;
        }
    }
    
    // Сохраняем средние значения для каждой стратегии
    chromosome.setMeanShotsRandom(shotsPerStrategy[0]);   // μ₁
    chromosome.setMeanShotsChecker(shotsPerStrategy[1]);  // μ₂
    chromosome.setMeanShotsMC(0.0);                       // μ₃ (будет добавлен позже)
    
    // Вычисляем общий фитнес по формуле F_p из §2.2.3:
    // F_p = w₁μ₁ + w₂μ₂ + w₃μ₃, где w₁=0.40, w₂=0.30, w₃=0.30
    double fitness = Fitness::calculatePlacementFitness(
        chromosome,
        shotsPerStrategy[0],  // μ₁ (Random)
        shotsPerStrategy[1],  // μ₂ (Checkerboard)
        0.0                   // μ₃ (Monte-Carlo-1000, будет добавлен позже)
    );
    
        chromosome.setFitness(fitness);
}

void Evaluator::evaluationWorker(std::vector<PlacementChromosome>& chromosomes, 
                                int startIdx, 
                                int endIdx, 
                                int threadId) {
    for (int i = startIdx; i < endIdx; ++i) {
        evaluateChromosome(chromosomes[i]);
    }
}

void Evaluator::evaluatePopulation(std::vector<PlacementChromosome>& chromosomes) {
    if (chromosomes.empty()) {
        return;
    }
    
    int populationSize = chromosomes.size();
    int chromosomesPerThread = populationSize / numThreads;
    
    std::vector<std::thread> threads;
    
    // Запускаем потоки для оценки
    for (int i = 0; i < numThreads; ++i) {
        int startIdx = i * chromosomesPerThread;
        int endIdx = (i == numThreads - 1) ? populationSize : (i + 1) * chromosomesPerThread;
        
        threads.emplace_back(&Evaluator::evaluationWorker, this, 
                           std::ref(chromosomes), startIdx, endIdx, i);
    }
    
    // Ждем завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }
} 