#include "feature_based_strategy.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <random>
#include <iostream>

FeatureBasedStrategy::FeatureBasedStrategy(const std::vector<double>& weights)
    : m_weights(weights), m_iteration(0)
{
    // Проверка на соответствие количества весов 
    if (weights.size() != Features::FEATURE_COUNT) {
        throw std::invalid_argument("Неверное количество весов для стратегии на основе признаков");
    }
}

std::pair<int, int> FeatureBasedStrategy::getNextShot(const Board& board) {
    // Поиск непростреленной клетки с максимальной оценкой
    double maxScore = -std::numeric_limits<double>::max();
    Cell bestCell{0, 0};
    bool found = false;

    // Проверяем все клетки на доске
    int availableCells = 0;
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            // Пропускаем клетки, по которым уже стреляли
            if (board.isShot(x, y)) {
                continue;
            }
            
            availableCells++;

            Cell cell{x, y};
            double score = scoreCell(cell, board);

            if (score > maxScore) {
                maxScore = score;
                bestCell = cell;
                found = true;
            }
        }
    }

    // Если все клетки прострелены, возвращаем случайную
    if (!found) {
        std::cerr << "ПРЕДУПРЕЖДЕНИЕ: Не найдено доступных клеток для выстрела! Выбираем случайную." << std::endl;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 9);
        return {dis(gen), dis(gen)};
    }

    // Каждые 10 ходов выводим отладочную информацию
    if (m_iteration % 10 == 0) {
        std::cerr << "Ход #" << m_iteration << ": доступно " << availableCells 
                  << " клеток, выбрана (" << bestCell.x << "," << bestCell.y 
                  << ") с оценкой " << maxScore << std::endl;
    }

    m_iteration++; // Увеличиваем счетчик итераций
    return {bestCell.x, bestCell.y};
}

void FeatureBasedStrategy::notifyShotResult(int x, int y, bool hit, bool sunk, const Board& board) {
    // Добавляем результат выстрела в историю
    ShotResult result;
    if (hit) {
        result = sunk ? ShotResult::KILL : ShotResult::HIT;
    } else {
        result = ShotResult::MISS;
    }
    
    m_shotHistory.push_back({Cell{x, y}, result});
}

void FeatureBasedStrategy::reset() {
    m_shotHistory.clear();
    m_iteration = 0;
}

std::vector<std::pair<int, int>> FeatureBasedStrategy::getAllShots() const {
    std::vector<std::pair<int, int>> shots;
    shots.reserve(m_shotHistory.size());
    
    for (const auto& [cell, result] : m_shotHistory) {
        shots.emplace_back(cell.x, cell.y);
    }
    
    return shots;
}

double FeatureBasedStrategy::scoreCell(const Cell& cell, const Board& board) const {
    // Создаем объект Features для вычисления признаков
    Features features(board, m_shotHistory, m_pool, m_iteration);
    
    // Получаем значения всех признаков для клетки
    auto featureValues = features.getFeatures(cell);
    
    // Вычисляем взвешенную сумму
    double score = 0.0;
    for (size_t i = 0; i < featureValues.size() && i < m_weights.size(); ++i) {
        score += featureValues[i] * m_weights[i];
    }
    
    return score;
} 