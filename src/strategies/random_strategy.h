#pragma once

#include "strategy.h"
#include "../utils/rng.h"
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <queue>
#include <ctime> // для использования time()

/**
 * @brief Стратегия случайной стрельбы
 * 
 * Стратегия выбирает случайную еще не обстрелянную клетку для выстрела.
 * С учетом оптимизаций (четность шахматной доски и улучшенный алгоритм добивания)
 * обычно дает около 80-90 ходов для потопления всех кораблей.
 */
class RandomStrategy : public Strategy {
private:
    std::vector<std::pair<int, int>> shots;  ///< Список всех сделанных выстрелов
    int boardSize;                           ///< Размер игрового поля
    RNG m_rng;                               ///< Генератор случайных чисел
    std::queue<std::pair<int, int>> targetQueue;  ///< Очередь для координат добивания
    
    // Направления для проверки соседних клеток (вверх, вправо, вниз, влево)
    const std::vector<std::pair<int, int>> directions = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };

    // Последние попадания для определения ориентации корабля
    std::vector<std::pair<int, int>> lastHits;
    
    // Использовать ли четность шахматной доски
    bool useCheckerboardParity;

    /**
     * @brief Собирает все непроверенные клетки на доске с учетом четности
     * @param board Текущее состояние игрового поля
     * @param useParity Использовать ли четность шахматной доски
     * @return Вектор пар (x,y) непроверенных клеток
     */
    std::vector<std::pair<int, int>> collectUnknown(const Board& board, bool useParity = true) {
        std::vector<std::pair<int, int>> cells;
        
        // Если используем четность, то сперва отбираем только клетки с нужной четностью
        if (useParity && useCheckerboardParity) {
            // Выбираем черные клетки (сумма координат четная)
            bool parity = 0; // Четная сумма (черные клетки)
            for (int y = 0; y < boardSize; ++y) {
                for (int x = 0; x < boardSize; ++x) {
                    if (!board.wasShotAt(x, y) && ((x + y) % 2 == parity)) {
                        cells.emplace_back(x, y);
                    }
                }
            }
            // Если черных клеток нет, выбираем белые
            if (cells.empty()) {
                for (int y = 0; y < boardSize; ++y) {
                    for (int x = 0; x < boardSize; ++x) {
                        if (!board.wasShotAt(x, y) && ((x + y) % 2 != parity)) {
                            cells.emplace_back(x, y);
                        }
                    }
                }
            }
        } else {
            // Если не используем четность, добавляем все неотстрелянные клетки
            for (int y = 0; y < boardSize; ++y) {
                for (int x = 0; x < boardSize; ++x) {
                    if (!board.wasShotAt(x, y)) {
                        cells.emplace_back(x, y);
                    }
                }
            }
        }
        return cells;
    }

    /**
     * @brief Определяет ориентацию корабля по нескольким попаданиям
     * @return пара {isVertical, isHorizontal}, если удалось определить
     */
    std::pair<bool, bool> getOrientation() const {
        if (lastHits.size() < 2) return {false, false}; // Недостаточно данных
        
        bool sameX = true;
        bool sameY = true;
        int firstX = lastHits[0].first;
        int firstY = lastHits[0].second;
        
        for (size_t i = 1; i < lastHits.size(); ++i) {
            if (lastHits[i].first != firstX) sameX = false;
            if (lastHits[i].second != firstY) sameY = false;
        }
        
        return {sameX, sameY};
    }

    /**
     * @brief Проверяет, доступна ли клетка для выстрела
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если клетка внутри поля и по ней еще не стреляли
     */
    bool isAvailableCell(int x, int y) const {
        return x >= 0 && x < boardSize && y >= 0 && y < boardSize && 
               std::find(shots.begin(), shots.end(), std::make_pair(x, y)) == shots.end();
    }

    /**
     * @brief Обновляет очередь добивания с учетом ориентации
     * @param board Текущее состояние игрового поля
     */
    void updateTargetQueue(const Board& board) {
        if (lastHits.empty()) return;
        
        // Получаем возможную ориентацию корабля
        auto [isVertical, isHorizontal] = getOrientation();
        
        // Очищаем старую очередь
        std::queue<std::pair<int, int>> empty;
        targetQueue.swap(empty);
        
        // Находим крайние точки для поиска
        int minX = boardSize, maxX = -1, minY = boardSize, maxY = -1;
        for (const auto& hit : lastHits) {
            minX = std::min(minX, hit.first);
            maxX = std::max(maxX, hit.first);
            minY = std::min(minY, hit.second);
            maxY = std::max(maxY, hit.second);
        }
        
        // Для вертикальной ориентации
        if (isVertical) {
            // Проверка сверху
            if (minY > 0 && isAvailableCell(minX, minY - 1)) {
                targetQueue.push({minX, minY - 1});
            }
            // Проверка снизу
            if (maxY < boardSize - 1 && isAvailableCell(minX, maxY + 1)) {
                targetQueue.push({minX, maxY + 1});
            }
        } 
        // Для горизонтальной ориентации
        else if (isHorizontal) {
            // Проверка слева
            if (minX > 0 && isAvailableCell(minX - 1, minY)) {
                targetQueue.push({minX - 1, minY});
            }
            // Проверка справа
            if (maxX < boardSize - 1 && isAvailableCell(maxX + 1, minY)) {
                targetQueue.push({maxX + 1, minY});
            }
        }
        // Если ориентация не определена или только одно попадание
        else {
            // Для последнего попадания проверяем все 4 направления
            int lastX = lastHits.back().first;
            int lastY = lastHits.back().second;
            
            for (const auto& dir : directions) {
                int newX = lastX + dir.first;
                int newY = lastY + dir.second;
                
                if (isAvailableCell(newX, newY)) {
                    targetQueue.push({newX, newY});
                }
            }
        }
    }

public:
    /**
     * @brief Конструктор стратегии случайной стрельбы
     * 
     * @param boardSize Размер игрового поля (обычно 10)
     * @param useCheckerboard Использовать ли четность шахматной доски (по умолчанию true)
     */
    explicit RandomStrategy(int boardSize = 10, bool useCheckerboard = true) 
        : boardSize(boardSize), m_rng(), useCheckerboardParity(useCheckerboard) {
        std::srand(static_cast<unsigned int>(std::time(nullptr))); // Инициализация один раз здесь
        reset();
    }

    /**
     * @brief Конструктор с передачей генератора случайных чисел
     * 
     * @param rng Генератор случайных чисел
     * @param boardSize Размер игрового поля (обычно 10)
     * @param useCheckerboard Использовать ли четность шахматной доски (по умолчанию true)
     */
    explicit RandomStrategy(RNG& rng, int boardSize = 10, bool useCheckerboard = true) 
        : boardSize(boardSize), m_rng(rng), useCheckerboardParity(useCheckerboard) {
        reset();
    }

    /**
     * @brief Получает следующий выстрел
     * 
     * @param board Текущее состояние игрового поля
     * @return Пара координат (x, y) для следующего выстрела
     */
    std::pair<int, int> getNextShot(const Board& board) override {
        // если остались только однопалубники — отключаем фильтр четности
        if (board.largestRemainingShipSize() <= 2) {
            useCheckerboardParity = false;
        }
        // Проверка на конец игры
        if (board.allShipsSunk()) {
            return {-1, -1};
        }
        
        // Если есть координаты в очереди добивания, используем их
        if (!targetQueue.empty()) {
            auto nextShot = targetQueue.front();
            targetQueue.pop();
            
            // Убедимся, что в эту клетку еще не стреляли
            while (!targetQueue.empty() && board.wasShotAt(nextShot.first, nextShot.second)) {
                nextShot = targetQueue.front();
                targetQueue.pop();
            }
            
            // Если нашли валидную клетку
            if (!board.wasShotAt(nextShot.first, nextShot.second)) {
                shots.push_back(nextShot);
                return nextShot;
            }
        }
        
        // Если очередь пуста или все клетки в ней невалидны, стреляем случайно с учетом четности
        auto availableCells = collectUnknown(board, useCheckerboardParity);
        
        if (!availableCells.empty()) {
            int index = m_rng.uniformInt(0, availableCells.size() - 1);
            auto nextShot = availableCells[index];
            shots.push_back(nextShot);
            return nextShot;
        }
        
        // Если все клетки уже обстреляны, возвращаем невалидные координаты
        return {-1, -1};
    }

    /**
     * @brief Уведомление о результате выстрела
     * 
     * @param x X-координата выстрела
     * @param y Y-координата выстрела
     * @param hit true, если попадание, false, если промах
     * @param sunk true, если корабль потоплен, false иначе
     * @param board Текущее состояние игрового поля
     */
    void notifyShotResult(int x, int y, bool hit, bool sunk, const Board& board) override {
        // Если промах - ничего не делаем
        if (!hit) return;
        
        // Добавляем попадание в список
        lastHits.push_back({x, y});
            
        if (sunk) {
            // Если корабль потоплен, очищаем историю попаданий и очередь добивания
            lastHits.clear();
            std::queue<std::pair<int, int>> empty;
            targetQueue.swap(empty);
        } else {
            // Если попадание, но корабль не потоплен
            // Обновляем очередь добивания с учетом ориентации и реального состояния доски
            updateTargetQueue(board);
        }
        // если остались только однопалубники — отключаем фильтр четности
        if (board.largestRemainingShipSize() <= 2) {
            useCheckerboardParity = false;
        }
    }

    /**
     * @brief Сброс стратегии для новой игры
     */
    void reset() override {
        shots.clear();
        lastHits.clear();
        
        // Очистка очереди
        std::queue<std::pair<int, int>> empty;
        targetQueue.swap(empty);
    }

    /**
     * @brief Получает список всех сделанных выстрелов
     * 
     * @return Вектор пар координат (x, y)
     */
    std::vector<std::pair<int, int>> getAllShots() const override {
        return shots;
    }

    /**
     * @brief Получает имя стратегии
     * 
     * @return Строка с именем стратегии
     */
    std::string getName() const override {
        return "Random";
    }
    
    /**
     * @brief Включение/отключение использования четности шахматной доски
     * 
     * @param use true для включения, false для отключения
     */
    void setUseCheckerboardParity(bool use) {
        useCheckerboardParity = use;
    }
}; 