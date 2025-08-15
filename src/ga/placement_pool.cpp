#include "placement_pool.h"
#include "fitness.h"
#include <algorithm>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include "../utils/rng.h"
#include "../strategies/random_strategy.h"
#include "../strategies/checkerboard_strategy.h"
#include "../strategies/monte_carlo_strategy.h"

PlacementPool::PlacementPool(
    size_t bestPoolSize,
    size_t randPoolSize,
    double bestProb
) : bestPoolSize(bestPoolSize),
    randPoolSize(randPoolSize),
    bestProb(bestProb) 
{
    if (bestProb < 0.0 || bestProb > 1.0) {
        throw std::invalid_argument("Вероятность должна быть в диапазоне [0, 1]");
    }
}

void PlacementPool::setBestPlacements(const std::vector<PlacementChromosome>& placements) {
    if (placements.size() != bestPoolSize) {
        throw std::invalid_argument(
            "Размер P_best должен быть равен " + 
            std::to_string(bestPoolSize)
        );
    }
    
    bestPlacements = placements;
}

void PlacementPool::setRandomPlacements(const std::vector<PlacementChromosome>& placements) {
    if (placements.size() != randPoolSize) {
        throw std::invalid_argument(
            "Размер P_rand должен быть равен " + 
            std::to_string(randPoolSize)
        );
    }
    
    randomPlacements = placements;
}

void PlacementPool::addPlacement(const PlacementChromosome& placement) {
    // По умолчанию добавляем в случайные расстановки, чтобы не портить лучшие
    // Если нужно поддерживать размер пула, можно удалить худшую расстановку
    if (randomPlacements.size() >= randPoolSize && randomPlacements.size() > 0) {
        randomPlacements.erase(randomPlacements.begin());
    }
    
    randomPlacements.push_back(placement);
}

const PlacementChromosome& PlacementPool::getPlacement(size_t index) const {
    if (index < bestPlacements.size()) {
        return bestPlacements[index];
    } else if (index < bestPlacements.size() + randomPlacements.size()) {
        return randomPlacements[index - bestPlacements.size()];
    } else {
        throw std::out_of_range("Индекс вне допустимого диапазона");
    }
}

PlacementChromosome PlacementPool::getRandomPlacement() {
    // Проверяем, что оба пула не пусты
    if (bestPlacements.empty() || randomPlacements.empty()) {
        throw std::runtime_error("Пулы расстановок не инициализированы");
    }
    
    // Выбираем пул согласно вероятности из §2.7.2
    double roll = rng.uniformReal(0.0, 1.0);
    
    if (roll < bestProb) {
        // Выбираем случайную расстановку из P_best (вероятность 0.7)
        size_t idx = rng.uniformInt(0, bestPlacements.size() - 1);
        return bestPlacements[idx];
    } else {
        // Выбираем случайную расстановку из P_rand (вероятность 0.3)
        size_t idx = rng.uniformInt(0, randomPlacements.size() - 1);
        return randomPlacements[idx];
    }
}

// --- Реализация класса ShooterPool ---

ShooterPool::ShooterPool(int randomGames, int checkerGames, int mcGames, int mcIterations)
    : m_randomGames(randomGames), 
      m_checkerGames(checkerGames), 
      m_mcGames(mcGames), 
      m_mcIterations(mcIterations)
{
}

double ShooterPool::random(const PlacementChromosome& chromosome) {
    if (!chromosome.isValid()) {
        return 0.0; // Невалидные расстановки имеют 0 выживаемость
    }
    
    // Создаем флот из хромосомы
    auto fleet = chromosome.decodeFleet();
    
    // Проводим m_randomGames игр и считаем среднее количество ходов
    int totalTurns = 0;
    
    for (int i = 0; i < m_randomGames; ++i) {
        // Создаем случайную стратегию стрельбы
        auto shooter = std::make_unique<RandomStrategy>(m_rng);
        
        // Создаем игровую доску и размещаем флот
        Board board;
        
        // Размещаем корабли из флота на доске
        for (const auto& ship : fleet->getShips()) {
            // Получаем координаты клеток корабля
            auto cells = ship.getCells();
            for (const auto& cell : cells) {
                board.placeShip(cell.first, cell.second);
            }
        }
        
        // Симулируем стрельбу до потопления всех кораблей
        int shots = 0;
        while (!board.allShipsSunk() && shots < 100) {
            auto target = shooter->getNextShot(board);
            if (target.first == -1 && target.second == -1) break; // Проверка сигнала окончания игры
            
            bool hit = board.shoot(target.first, target.second);
            bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
            shooter->notifyShotResult(target.first, target.second, hit, sunk, board);
            shots++;
        }
        
        totalTurns += shots;
    }
    
    return static_cast<double>(totalTurns) / m_randomGames;
}

double ShooterPool::checker(const PlacementChromosome& chromosome) {
    if (!chromosome.isValid()) {
        return 0.0;
    }
    
    auto fleet = chromosome.decodeFleet();
    int totalTurns = 0;
    
    for (int i = 0; i < m_checkerGames; ++i) {
        // Создаем стратегию шахматной доски
        auto shooter = std::make_unique<CheckerboardStrategy>(m_rng);
        
        // Создаем игровую доску и размещаем флот
        Board board;
        
        // Размещаем корабли из флота на доске
        for (const auto& ship : fleet->getShips()) {
            // Получаем координаты клеток корабля
            auto cells = ship.getCells();
            for (const auto& cell : cells) {
                board.placeShip(cell.first, cell.second);
            }
        }
        
        // Симулируем стрельбу до потопления всех кораблей
        int shots = 0;
        while (!board.allShipsSunk() && shots < 100) {
            auto target = shooter->getNextShot(board);
            if (target.first == -1 && target.second == -1) break; // Проверка сигнала окончания игры
            
            bool hit = board.shoot(target.first, target.second);
            bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
            shooter->notifyShotResult(target.first, target.second, hit, sunk, board);
            shots++;
        }
        
        totalTurns += shots;
    }
    
    return static_cast<double>(totalTurns) / m_checkerGames;
}

double ShooterPool::montecarlo(const PlacementChromosome& chromosome) {
    if (!chromosome.isValid()) {
        return 0.0;
    }
    
    auto fleet = chromosome.decodeFleet();
    int totalTurns = 0;
    
    for (int i = 0; i < m_mcGames; ++i) {
        // Используем настоящую стратегию Монте-Карло вместо случайной
        auto shooter = std::make_unique<MonteCarloStrategy>(m_rng, m_mcIterations);
        
        // Создаем игровую доску и размещаем флот
        Board board;
        
        // Размещаем корабли из флота на доске
        for (const auto& ship : fleet->getShips()) {
            // Получаем координаты клеток корабля
            auto cells = ship.getCells();
            for (const auto& cell : cells) {
                board.placeShip(cell.first, cell.second);
            }
        }
        
        // Симулируем стрельбу до потопления всех кораблей
        int shots = 0;
        while (!board.allShipsSunk() && shots < 100) { // Добавлен лимит в 100 для защиты от зацикливания
            auto target = shooter->getNextShot(board);
            if (target.first == -1 && target.second == -1) break; // Проверка сигнала окончания игры
            
            bool hit = board.shoot(target.first, target.second);
            bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
            shooter->notifyShotResult(target.first, target.second, hit, sunk, board);
            shots++;
        }
        
        totalTurns += shots;
    }
    
    return static_cast<double>(totalTurns) / m_mcGames;
}

double ShooterPool::evaluate(PlacementChromosome& chromosome) {
    if (!chromosome.isValid()) {
        return -100.0; // Большой отрицательный фитнес для невалидных расстановок
    }
    
    // Проводим оценку против разных стрелков
    double shotsRandom = random(chromosome);
    double shotsChecker = checker(chromosome);
    double shotsMC = montecarlo(chromosome);
    
    // Сохраняем результаты в хромосому для дальнейшего использования
    chromosome.setMeanShotsRandom(shotsRandom);
    chromosome.setMeanShotsChecker(shotsChecker);
    chromosome.setMeanShotsMC(shotsMC);
    
    // Рассчитываем итоговый фитнес по формуле
    double fitness = Fitness::calculatePlacementFitness(
        chromosome, shotsRandom, shotsChecker, shotsMC);
    
    return fitness;
} 