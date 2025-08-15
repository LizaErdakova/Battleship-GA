#pragma once

#include <array>
#include <vector>
#include <utility>
#include "../models/cell.h"
#include "../models/board.h"
#include "../ga/placement_pool.h"

/**
 * @brief Перечисление результатов выстрела
 */
enum class ShotResult {
    MISS,   ///< Промах
    HIT,    ///< Попадание
    KILL    ///< Потопление корабля
};

/**
 * @brief Класс для вычисления признаков клеток для DecisionGA
 */
class Features {
public:
    static constexpr size_t FEATURE_COUNT = 20;

    /**
     * @brief Конструктор класса признаков
     */
    Features(
        const Board& board,
        const std::vector<std::pair<Cell, ShotResult>>& history,
        const PlacementPool& pool,
        int currentIteration
    );

    /**
     * @brief Вычисляет все признаки для заданной клетки
     */
    std::array<double, FEATURE_COUNT> getFeatures(const Cell& cell) const;

private:
    const Board& board;
    const std::vector<std::pair<Cell, ShotResult>>& history;
    const PlacementPool& pool;
    int currentIteration;

    // Методы для вычисления отдельных признаков
    double calculateHeat(const Cell& cell) const;
    double hasHitNeighbor(const Cell& cell) const;
    double hasDiagHitNeighbor(const Cell& cell) const;
    double getParity(const Cell& cell) const;
    double getDistLastHit(const Cell& cell) const;
    double getMissCluster(const Cell& cell) const;
    double getRowFree(const Cell& cell) const;
    double getColFree(const Cell& cell) const;
    double getCenterBias(const Cell& cell) const;
    double getEdgeBias(const Cell& cell) const;
    double isCorner(const Cell& cell) const;
    double canFitShip(const Cell& cell, int size) const;
    double getRecentMissPenalty(const Cell& cell) const;
    double getTimeDecayHit(const Cell& cell) const;
    double getTimeDecayMiss(const Cell& cell) const;
    double getRandNoise() const;
    double getIterParityFlip(const Cell& cell) const;

    // Вспомогательные методы
    bool isValidCell(int x, int y) const;
    double distance(const Cell& a, const Cell& b) const;
    bool isHit(const ShotResult& result) const;
    bool isMiss(const ShotResult& result) const;
    bool isKill(const ShotResult& result) const;
};