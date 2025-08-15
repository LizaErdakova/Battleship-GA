#include "features.h"
#include <random>
#include <algorithm>
#include <cmath>
#include <memory>
#include "../models/fleet.h"

Features::Features(
    const Board& board,
    const std::vector<std::pair<Cell, ShotResult>>& history,
    const PlacementPool& pool,
    int currentIteration
) : board(board),
    history(history),
    pool(pool),
    currentIteration(currentIteration) {}

std::array<double, Features::FEATURE_COUNT> Features::getFeatures(const Cell& cell) const {
    std::array<double, FEATURE_COUNT> features{};
    
    // 1. Heat - доля расстановок из PH, где клетка занята
    features[0] = calculateHeat(cell);
    
    // 2. HitNeighbor - 4-сторонние соседи с попаданиями
    features[1] = hasHitNeighbor(cell);
    
    // 3. DiagHitNeighbor - диагональные соседи с попаданиями
    features[2] = hasDiagHitNeighbor(cell);
    
    // 4. Parity - шахматный паттерн
    features[3] = getParity(cell);
    
    // 5. DistLastHit - близость к последнему попаданию
    features[4] = getDistLastHit(cell);
    
    // 6. MissCluster - плотность промахов
    features[5] = getMissCluster(cell);
    
    // 7. RowFree - доля свободных клеток в строке
    features[6] = getRowFree(cell);
    
    // 8. ColFree - доля свободных клеток в столбце
    features[7] = getColFree(cell);
    
    // 9. CenterBias - близость к центру
    features[8] = getCenterBias(cell);
    
    // 10. EdgeBias - близость к краям
    features[9] = getEdgeBias(cell);
    
    // 11. Corner - угловая клетка
    features[10] = isCorner(cell);
    
    // 12-15. Fit для разных типов кораблей
    features[11] = canFitShip(cell, 4); // L4Fit
    features[12] = canFitShip(cell, 3); // L3Fit
    features[13] = canFitShip(cell, 2); // L2Fit
    features[14] = canFitShip(cell, 1); // SubFit
    
    // 16. RecentMissPenalty - штраф за близость к свежему промаху
    features[15] = getRecentMissPenalty(cell);
    
    // 17. TimeDecayHit - затухание влияния старых попаданий
    features[16] = getTimeDecayHit(cell);
    
    // 18. TimeDecayMiss - затухание влияния старых промахов
    features[17] = getTimeDecayMiss(cell);
    
    // 19. RandNoise - случайный шум
    features[18] = getRandNoise();
    
    // 20. IterParityFlip - чередование четности
    features[19] = getIterParityFlip(cell);
    
    return features;
}

double Features::calculateHeat(const Cell& cell) const {
    if (history.empty()) {
        return 0.5; // На старте все клетки равновероятны
    }

    int count = 0;
    const auto& validPlacements = pool.getBestPlacements();
    
    for (const auto& placement : validPlacements) {
        auto fleet = placement.decodeFleet();
        if (fleet && fleet->hasShipAt(cell.x, cell.y)) {
            count++;
        }
    }
    
    return static_cast<double>(count) / validPlacements.size();
}

double Features::hasHitNeighbor(const Cell& cell) const {
    // 4-сторонние соседи
    const std::array<std::pair<int, int>, 4> neighbors = {{
        {-1, 0}, // влево
        {1, 0},  // вправо
        {0, -1}, // вверх
        {0, 1}   // вниз
    }};
    
    for (const auto& [dx, dy] : neighbors) {
        int x = cell.x + dx;
        int y = cell.y + dy;
        
        if (isValidCell(x, y)) {
            for (const auto& [hitCell, result] : history) {
                if (hitCell.x == x && hitCell.y == y && isHit(result)) {
                    return 1.0;
                }
            }
        }
    }
    
    return 0.0;
}

double Features::hasDiagHitNeighbor(const Cell& cell) const {
    // Диагональные соседи
    const std::array<std::pair<int, int>, 4> diagNeighbors = {{
        {-1, -1}, // влево-вверх
        {-1, 1},  // влево-вниз
        {1, -1},  // вправо-вверх
        {1, 1}    // вправо-вниз
    }};
    
    for (const auto& [dx, dy] : diagNeighbors) {
        int x = cell.x + dx;
        int y = cell.y + dy;
        
        if (isValidCell(x, y)) {
            for (const auto& [hitCell, result] : history) {
                if (hitCell.x == x && hitCell.y == y && isHit(result)) {
                    return 1.0;
                }
            }
        }
    }
    
    return 0.0;
}

double Features::getParity(const Cell& cell) const {
    return (cell.x + cell.y) % 2;
}

double Features::getDistLastHit(const Cell& cell) const {
    double minDist = 100.0;
    
    for (const auto& [hitCell, result] : history) {
        if (isHit(result)) {
            double dist = distance(cell, hitCell);
            minDist = std::min(minDist, dist);
        }
    }
    
    return 1.0 / (1.0 + minDist);
}

double Features::getMissCluster(const Cell& cell) const {
    int missCount = 0;
    int totalCells = 0;
    const int radius = 2;
    
    for (int dx = -radius; dx <= radius; ++dx) {
        for (int dy = -radius; dy <= radius; ++dy) {
            int x = cell.x + dx;
            int y = cell.y + dy;
            
            if (isValidCell(x, y)) {
                totalCells++;
                for (const auto& [missCell, result] : history) {
                    if (missCell.x == x && missCell.y == y && isMiss(result)) {
                        missCount++;
                        break;
                    }
                }
            }
        }
    }
    
    return static_cast<double>(missCount) / totalCells;
}

bool Features::isValidCell(int x, int y) const {
    return x >= 0 && x < 10 && y >= 0 && y < 10;
}

double Features::distance(const Cell& a, const Cell& b) const {
    return std::sqrt(std::pow(a.x - b.x, 2) + std::pow(a.y - b.y, 2));
}

bool Features::isHit(const ShotResult& result) const {
    return result == ShotResult::HIT || result == ShotResult::KILL;
}

bool Features::isMiss(const ShotResult& result) const {
    return result == ShotResult::MISS;
}

bool Features::isKill(const ShotResult& result) const {
    return result == ShotResult::KILL;
}

double Features::getRowFree(const Cell& cell) const {
    int freeCount = 0;
    for (int x = 0; x < 10; ++x) {
        if (board.isCellFree(x, cell.y)) {
            freeCount++;
        }
    }
    return static_cast<double>(freeCount) / 10;
}

double Features::getColFree(const Cell& cell) const {
    int freeCount = 0;
    for (int y = 0; y < 10; ++y) {
        if (board.isCellFree(cell.x, y)) {
            freeCount++;
        }
    }
    return static_cast<double>(freeCount) / 10;
}

double Features::getCenterBias(const Cell& cell) const {
    const double centerX = 4.5;
    const double centerY = 4.5;
    double dist = std::sqrt(std::pow(cell.x - centerX, 2) + std::pow(cell.y - centerY, 2));
    return 1.0 - (dist / 7.07); // 7.07 = sqrt(50) - максимальное расстояние до центра
}

double Features::getEdgeBias(const Cell& cell) const {
    return (cell.x == 0 || cell.x == 9 || cell.y == 0 || cell.y == 9) ? 1.0 : 0.0;
}

double Features::isCorner(const Cell& cell) const {
    return ((cell.x == 0 || cell.x == 9) && (cell.y == 0 || cell.y == 9)) ? 1.0 : 0.0;
}

double Features::canFitShip(const Cell& cell, int size) const {
    // Проверяем горизонтальное размещение
    bool canFitHorizontal = true;
    for (int dx = 0; dx < size && canFitHorizontal; ++dx) {
        int x = cell.x + dx;
        if (!isValidCell(x, cell.y) || !board.isCellFree(x, cell.y)) {
            canFitHorizontal = false;
        }
    }
    
    // Проверяем вертикальное размещение
    bool canFitVertical = true;
    for (int dy = 0; dy < size && canFitVertical; ++dy) {
        int y = cell.y + dy;
        if (!isValidCell(cell.x, y) || !board.isCellFree(cell.x, y)) {
            canFitVertical = false;
        }
    }
    
    return (canFitHorizontal || canFitVertical) ? 1.0 : 0.0;
}

double Features::getRecentMissPenalty(const Cell& cell) const {
    if (history.empty()) return 0.0;
    
    const int recentHistorySize = std::min(5, static_cast<int>(history.size()));
    const auto& recentHistory = std::vector<std::pair<Cell, ShotResult>>(
        history.end() - recentHistorySize,
        history.end()
    );
    
    for (const auto& [missCell, result] : recentHistory) {
        if (isMiss(result) && distance(cell, missCell) <= 2.0) {
            return 1.0;
        }
    }
    
    return 0.0;
}

double Features::getTimeDecayHit(const Cell& cell) const {
    if (history.empty()) return 0.0;
    
    double maxInfluence = 0.0;
    const int historySize = static_cast<int>(history.size());
    
    for (int i = 0; i < historySize; ++i) {
        const auto& [hitCell, result] = history[i];
        if (isHit(result)) {
            double dist = distance(cell, hitCell);
            double timeDecay = static_cast<double>(i) / historySize;
            double influence = std::exp(-dist) * (1.0 - timeDecay);
            maxInfluence = std::max(maxInfluence, influence);
        }
    }
    
    return maxInfluence;
}

double Features::getTimeDecayMiss(const Cell& cell) const {
    if (history.empty()) return 0.0;
    
    double maxInfluence = 0.0;
    const int historySize = static_cast<int>(history.size());
    
    for (int i = 0; i < historySize; ++i) {
        const auto& [missCell, result] = history[i];
        if (isMiss(result)) {
            double dist = distance(cell, missCell);
            double timeDecay = static_cast<double>(i) / historySize;
            double influence = std::exp(-dist) * (1.0 - timeDecay);
            maxInfluence = std::max(maxInfluence, influence);
        }
    }
    
    return maxInfluence;
}

double Features::getRandNoise() const {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_real_distribution<> dis(0.0, 0.1);
    return dis(gen);
}

double Features::getIterParityFlip(const Cell& cell) const {
    return ((cell.x + cell.y + currentIteration) % 2) ? 1.0 : 0.0;
} 