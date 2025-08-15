#pragma once

#include "strategy.h"
#include "../utils/rng.h"
#include <string>
#include <vector>
#include <utility>
#include <queue>
#include <algorithm>
#include <set>

/**
 * @brief Стратегия шахматной доски с добиванием (Hunt-Target)
 * 
 * Стратегия стреляет по шахматному паттерну (только в клетки одного цвета),
 * так как минимальный размер корабля - 1 клетка. После первого попадания,
 * переходит в режим добивания, стреляя вокруг клетки по крестообразному паттерну.
 * Улучшенная версия обычно даёт около 50-60 ходов.
 */
class CheckerboardStrategy : public Strategy {
private:
    // Режимы работы стратегии
    enum class Mode {
        HUNT,   // Режим поиска (шахматный паттерн)
        TARGET  // Режим добивания (стрельба вокруг попадания)
    };
    
    int boardSize;                              ///< Размер игрового поля
    Mode currentMode;                           ///< Текущий режим стратегии
    std::vector<std::pair<int, int>> shots;     ///< Список всех сделанных выстрелов
    std::vector<std::pair<int, int>> hits;      ///< Список всех попаданий
    std::queue<std::pair<int, int>> targetQueue; ///< Очередь клеток для добивания
    RNG m_rng;                                  ///< Генератор случайных чисел
    bool preferEvenParity;                      ///< Предпочтение четной четности (черные клетки)
    std::set<std::pair<int, int>> excludedCells; ///< Клетки, исключенные из стрельбы (вокруг потопленных кораблей)
    
    // Направления для проверки соседних клеток (вверх, вправо, вниз, влево)
    std::vector<std::pair<int, int>> directions = {
        {0, -1}, {1, 0}, {0, 1}, {-1, 0}
    };
    
    /**
     * @brief Проверяет, является ли клетка "черной" в шахматном паттерне
     * 
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если клетка "черная", false иначе
     */
    bool isBlackCell(int x, int y) const {
        return ((x + y) % 2) == 0;
    }
    
    /**
     * @brief Проверяет, не выходит ли клетка за границы поля
     * 
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если клетка внутри поля, false иначе
     */
    bool isValidCell(int x, int y) const {
        return x >= 0 && x < boardSize && y >= 0 && y < boardSize;
    }
    
    /**
     * @brief Проверяет, не стреляли ли уже в эту клетку
     * 
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если в клетку еще не стреляли, false иначе
     */
    bool isAvailableCell(int x, int y) const {
        return isValidCell(x, y) && 
               std::find(shots.begin(), shots.end(), std::make_pair(x, y)) == shots.end() &&
               excludedCells.find(std::make_pair(x, y)) == excludedCells.end();
    }
    
    /**
     * @brief Добавляет соседние клетки в очередь для режима добивания с учетом ориентации
     * 
     * @param x X-координата попадания
     * @param y Y-координата попадания
     * @param board Текущее состояние игрового поля
     */
    void addAdjacentCellsToTargetQueue(int x, int y, const Board& board) {
        // Определяем ориентацию корабля, если есть достаточно попаданий
        bool isVertical = false, isHorizontal = false;
        
        if (hits.size() >= 2) {
            // Проверяем, все ли удары имеют один и тот же x или y
            int firstX = hits[0].first;
            int firstY = hits[0].second;
            isHorizontal = true;
            isVertical = true;
            
            for (size_t i = 1; i < hits.size(); ++i) {
                if (hits[i].first != firstX) isVertical = false;
                if (hits[i].second != firstY) isHorizontal = false;
            }
        }
        
        // Находим крайние точки для поиска
        int minX = boardSize, maxX = -1, minY = boardSize, maxY = -1;
        for (const auto& hit : hits) {
            minX = std::min(minX, hit.first);
            maxX = std::max(maxX, hit.first);
            minY = std::min(minY, hit.second);
            maxY = std::max(maxY, hit.second);
        }
        
        // Очищаем очередь, чтобы приоритезировать новые целевые клетки
        std::queue<std::pair<int, int>> empty;
        targetQueue.swap(empty);
        
        // Для вертикальной ориентации
        if (isVertical) {
            // Проверка сверху
            if (isValidCell(minX, minY - 1) && !board.wasShotAt(minX, minY - 1) &&
                excludedCells.find(std::make_pair(minX, minY - 1)) == excludedCells.end()) {
                targetQueue.push({minX, minY - 1});
            }
            // Проверка снизу
            if (isValidCell(minX, maxY + 1) && !board.wasShotAt(minX, maxY + 1) &&
                excludedCells.find(std::make_pair(minX, maxY + 1)) == excludedCells.end()) {
                targetQueue.push({minX, maxY + 1});
            }
        } 
        // Для горизонтальной ориентации
        else if (isHorizontal) {
            // Проверка слева
            if (isValidCell(minX - 1, minY) && !board.wasShotAt(minX - 1, minY) &&
                excludedCells.find(std::make_pair(minX - 1, minY)) == excludedCells.end()) {
                targetQueue.push({minX - 1, minY});
            }
            // Проверка справа
            if (isValidCell(maxX + 1, minY) && !board.wasShotAt(maxX + 1, minY) &&
                excludedCells.find(std::make_pair(maxX + 1, minY)) == excludedCells.end()) {
                targetQueue.push({maxX + 1, minY});
            }
        }
        // Если ориентация не определена или только одно попадание
        else {
            // Добавляем клетки в крест вокруг попадания с приоритетом
            for (const auto& dir : directions) {
                int newX = x + dir.first;
                int newY = y + dir.second;
                
                if (isValidCell(newX, newY) && !board.wasShotAt(newX, newY) &&
                    excludedCells.find(std::make_pair(newX, newY)) == excludedCells.end()) {
                    targetQueue.push({newX, newY});
                }
            }
        }
    }
    
    /**
     * @brief Собирает клетки с указанной четностью
     * 
     * @param board Текущее состояние игрового поля
     * @param parity Четность клеток (0 - четные, 1 - нечетные)
     * @return Вектор пар (x,y) непроверенных клеток с указанной четностью
     */
    std::vector<std::pair<int, int>> collectParity(const Board& board, bool parity) {
        std::vector<std::pair<int, int>> cells;
        for (int y = 0; y < boardSize; ++y) {
            for (int x = 0; x < boardSize; ++x) {
                if (!board.wasShotAt(x, y) && ((x + y) % 2 == parity) &&
                    excludedCells.find(std::make_pair(x, y)) == excludedCells.end()) {
                    cells.emplace_back(x, y);
                }
            }
        }
        return cells;
    }
    
    /**
     * @brief Анализирует текущую доску и определяет оптимальную четность
     * 
     * @param board Текущее состояние игрового поля
     * @return true для предпочтения черных клеток, false для белых
     */
    bool determineOptimalParity(const Board& board) {
        // Просто возвращаем текущее предпочтение четности, в реальном
        // приложении здесь может быть более сложная логика анализа
        return preferEvenParity;
    }
    
    /**
     * @brief Помечает клетки вокруг потопленного корабля как недоступные для выстрелов
     * Основан на правиле "no-touch" - корабли не могут касаться друг друга
     * 
     * @param board Текущее состояние игрового поля
     */
    void markSurroundingCellsAsUnavailable(const Board& board) {
        // Временная матрица для отслеживания посещенных клеток
        std::vector<std::vector<bool>> visited(boardSize, std::vector<bool>(boardSize, false));
        
        // Находим все потопленные корабли на доске
        for (int y = 0; y < boardSize; ++y) {
            for (int x = 0; x < boardSize; ++x) {
                if (board.isShot(x, y) && board.wasShipSunkAt(x, y) && !visited[y][x]) {
                    // Нашли потопленный корабль, определяем его размер и ориентацию
                    std::vector<std::pair<int, int>> shipCells;
                    
                    // Проверяем горизонтальную ориентацию
                    bool isHorizontal = (x + 1 < boardSize && board.isShot(x + 1, y) && 
                                         board.wasShipSunkAt(x + 1, y));
                    
                    // Проверяем вертикальную ориентацию
                    bool isVertical = (y + 1 < boardSize && board.isShot(x, y + 1) && 
                                       board.wasShipSunkAt(x, y + 1));
                    
                    if (isHorizontal) {
                        // Собираем все клетки горизонтального корабля
                        int xx = x;
                        while (xx < boardSize && board.isShot(xx, y) && board.wasShipSunkAt(xx, y)) {
                            shipCells.emplace_back(xx, y);
                            visited[y][xx] = true;
                            xx++;
                        }
                    } else if (isVertical) {
                        // Собираем все клетки вертикального корабля
                        int yy = y;
                        while (yy < boardSize && board.isShot(x, yy) && board.wasShipSunkAt(x, yy)) {
                            shipCells.emplace_back(x, yy);
                            visited[yy][x] = true;
                            yy++;
                        }
                    } else {
                        // Одноклеточный корабль
                        shipCells.emplace_back(x, y);
                        visited[y][x] = true;
                    }
                    
                    // Теперь помечаем все окружающие клетки как недоступные для выстрелов
                    for (const auto& cell : shipCells) {
                        int cellX = cell.first;
                        int cellY = cell.second;
                        
                        // Проходим по всем соседним клеткам (включая диагональные)
                        for (int dy = -1; dy <= 1; ++dy) {
                            for (int dx = -1; dx <= 1; ++dx) {
                                int nx = cellX + dx;
                                int ny = cellY + dy;
                                
                                // Проверяем, что клетка внутри границ и не является частью корабля
                                if (isValidCell(nx, ny) && 
                                    !(board.isShot(nx, ny) && board.wasShipSunkAt(nx, ny))) {
                                    excludedCells.emplace(nx, ny);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
public:
    /**
     * @brief Конструктор стратегии шахматной доски
     * 
     * @param boardSize Размер игрового поля (обычно 10)
     */
    explicit CheckerboardStrategy(int boardSize = 10) 
        : boardSize(boardSize), currentMode(Mode::HUNT), m_rng(), preferEvenParity(true) {
        reset();
    }
    
    /**
     * @brief Конструктор с передачей генератора случайных чисел
     * 
     * @param rng Генератор случайных чисел
     * @param boardSize Размер игрового поля (обычно 10)
     */
    explicit CheckerboardStrategy(RNG& rng, int boardSize = 10) 
        : boardSize(boardSize), currentMode(Mode::HUNT), m_rng(rng), preferEvenParity(true) {
        reset();
    }
    
    /**
     * @brief Получает клетку для следующего выстрела
     * 
     * @param board Текущее состояние игрового поля
     * @return Пара координат (x, y) для следующего выстрела
     */
    std::pair<int, int> getNextShot(const Board& board) override {
        // если остались только однопалубники — отключаем фильтр четности
        if (board.largestRemainingShipSize() <= 2) {
            preferEvenParity = false;
        }
        // Проверка на конец игры
        if (board.allShipsSunk()) {
            return {-1, -1};
        }
        
        std::pair<int, int> nextShot;
        
        // Если в режиме добивания и есть клетки в очереди
        if (currentMode == Mode::TARGET && !targetQueue.empty()) {
            // Берем первую клетку из очереди
            nextShot = targetQueue.front();
            targetQueue.pop();
            
            // Проверяем, доступна ли она (может быть уже обстреляна или исключена)
            while (!targetQueue.empty() && 
                   (board.wasShotAt(nextShot.first, nextShot.second) || 
                    excludedCells.find(nextShot) != excludedCells.end())) {
                nextShot = targetQueue.front();
                targetQueue.pop();
            }
            
            // Если нашли доступную клетку, используем её
            if (!board.wasShotAt(nextShot.first, nextShot.second) && 
                excludedCells.find(nextShot) == excludedCells.end()) {
                shots.push_back(nextShot);
                return nextShot;
            }
            
            // Если все клетки в очереди недоступны, переходим в режим поиска
            currentMode = Mode::HUNT;
        }
        
        // Режим поиска (или если очередь добивания пуста)
        if (currentMode == Mode::HUNT) {
            // Определяем оптимальную четность для поиска
            bool optimalParity = determineOptimalParity(board);
            
            // Используем оптимизированный метод сбора клеток с четностью
            auto preferredCells = collectParity(board, optimalParity);
            
            // Если есть доступные клетки предпочтительной четности, выбираем случайную
            if (!preferredCells.empty()) {
                int index = m_rng.uniformInt(0, preferredCells.size() - 1);
                nextShot = preferredCells[index];
                shots.push_back(nextShot);
                return nextShot;
            }
            
            // Если все клетки предпочтительной четности обстреляны, берем другие
            auto otherCells = collectParity(board, !optimalParity);
            if (!otherCells.empty()) {
                int index = m_rng.uniformInt(0, otherCells.size() - 1);
                nextShot = otherCells[index];
                shots.push_back(nextShot);
                return nextShot;
            }
        }
        
        // Если все клетки обстреляны, возвращаем невалидные координаты
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
        if (!hit) return; // Если промах, ничего не делаем
        
        // Запоминаем попадание
        hits.emplace_back(x, y);
            
        if (sunk) {
            // Корабль потоплен, помечаем клетки вокруг него как недоступные
            markSurroundingCellsAsUnavailable(board);
            
            // Очищаем очередь и переходим в режим поиска
            std::queue<std::pair<int, int>> emptyQueue;
            targetQueue.swap(emptyQueue);
            currentMode = Mode::HUNT;
                
            // Очищаем список попаданий, так как корабль потоплен
            hits.clear();
        } else {
            // Корабль не потоплен, переходим в режим добивания
            currentMode = Mode::TARGET;
                
            // Добавляем соседние клетки в очередь для добивания, используя реальную доску
            addAdjacentCellsToTargetQueue(x, y, board);
        }
        // если остались только однопалубники — отключаем фильтр четности
        if (board.largestRemainingShipSize() <= 2) {
            preferEvenParity = false;
        }
    }
    
    /**
     * @brief Сброс стратегии для новой игры
     */
    void reset() override {
        shots.clear();
        hits.clear();
        
        // Очистка очереди
        std::queue<std::pair<int, int>> emptyQueue;
        targetQueue.swap(emptyQueue);
        
        // Очистка списка исключенных клеток
        excludedCells.clear();
        
        currentMode = Mode::HUNT;
        
        // Случайно выбираем предпочтение четности для этой игры
        preferEvenParity = m_rng.uniformInt(0, 1) == 0;
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
        return "Checkerboard";
    }
    
    /**
     * @brief Устанавливает предпочтение четности
     * 
     * @param preferEven true для предпочтения четных (черных) клеток, false для нечетных (белых)
     */
    void setPreferEvenParity(bool preferEven) {
        preferEvenParity = preferEven;
    }
}; 