#include "monte_carlo_strategy.h"
#include "random_strategy.h"
#include <algorithm>
#include <map>
#include <cmath>

bool MonteCarloStrategy::fits(int x, int y, int len, bool hor, 
                              const MCPlacement& p, const Board& board,
                              const std::vector<std::pair<int, int>>& hits) const {
    // Флаг для проверки, покрывает ли корабль хотя бы одно попадание
    bool coversHit = false;
    int hitsCovered = 0;
    
    // Проверяем все клетки корабля и окружающие их клетки
    for (int i = 0; i < len; ++i) {
        int xx = x + (hor ? i : 0);
        int yy = y + (hor ? 0 : i);
        
        // Проверка границ и занятости
        if (!inside(xx, yy) || p.occ[yy][xx])
            return false;
        
        // Клетка не должна быть промахом
        if (board.isShot(xx, yy) && board.getCell(xx, yy) == CellState::MISS)
            return false;
        
        // Проверяем, покрывает ли корабль попадание
        if (!hits.empty()) {
            auto it = std::find(hits.begin(), hits.end(), std::make_pair(xx, yy));
            if (it != hits.end()) {
                coversHit = true;
                hitsCovered++;
            }
        }
        
        // Проверка окружающих клеток (правило "не касаться")
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                // Пропускаем текущую клетку
                if (dx == 0 && dy == 0) continue;
                
                int nx = xx + dx;
                int ny = yy + dy;
                
                if (inside(nx, ny) && p.occ[ny][nx])
                    return false;
            }
        }
    }
    
    // Если на доске есть попадания:
    // 1) Корабль должен покрывать хотя бы одно попадание
    // 2) Все попадания, которые он покрывает, должны быть в линию (соответствовать ориентации)
    if (!hits.empty()) {
        // Если ни одно попадание не покрыто, не подходит
        if (!coversHit)
            return false;
            
        // Если покрыто больше одного попадания, проверяем, соответствуют ли они ориентации
        if (hitsCovered > 1) {
            // Для горизонтального корабля все попадания должны быть на одной y-координате
            // Для вертикального корабля все попадания должны быть на одной x-координате
            if (hor) {
                int firstY = -1;
                for (const auto& hit : hits) {
                    // Пропускаем попадания, не покрытые этим кораблем
                    if (hit.first < x || hit.first >= x + len || hit.second != y)
                        continue;
                        
                    if (firstY == -1)
                        firstY = hit.second;
                    else if (hit.second != firstY)
                        return false; // Попадания не в одну линию по горизонтали
                }
            } else {
                int firstX = -1;
                for (const auto& hit : hits) {
                    // Пропускаем попадания, не покрытые этим кораблем
                    if (hit.second < y || hit.second >= y + len || hit.first != x)
                        continue;
                        
                    if (firstX == -1)
                        firstX = hit.first;
                    else if (hit.first != firstX)
                        return false; // Попадания не в одну линию по вертикали
                }
            }
        }
    }
    
    return true;
}

void MonteCarloStrategy::place(int x, int y, int len, bool hor, MCPlacement& p) const {
    for (int i = 0; i < len; ++i) {
        int xx = x + (hor ? i : 0);
        int yy = y + (hor ? 0 : i);
        p.occ[yy][xx] = 1;
    }
}

std::vector<int> MonteCarloStrategy::getRemainingShips(const Board& board) const {
    // Стандартный набор кораблей: 4,3,3,2,2,2,1,1,1,1
    std::vector<int> ships = { 4, 3, 3, 2, 2, 2, 1, 1, 1, 1 };
    
    // Для монте-карло симуляции нам нужны только те корабли, которые еще не потоплены
    // Идея: проверим доску на количество уже потопленных кораблей каждого размера
    
    // Временная карта для отслеживания посещенных клеток SUNK
    std::array<std::array<bool, 10>, 10> visited = {};
    for (auto& row : visited) row.fill(false);
    
    // Корабли, которые мы уже определили как потопленные (их размеры)
    std::vector<int> sunkShips;
    
    // Обнаружение потопленных кораблей
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            if (board.isShot(x, y) && board.wasShipSunkAt(x, y) && !visited[y][x]) {
                // Нашли потопленный корабль, определяем его размер
                int size = 0;
                
                // Проверяем горизонтальное направление
                bool isHorizontal = (x + 1 < 10 && board.isShot(x + 1, y) && board.wasShipSunkAt(x + 1, y));
                
                // Проверяем вертикальное направление
                bool isVertical = (y + 1 < 10 && board.isShot(x, y + 1) && board.wasShipSunkAt(x, y + 1));
                
                if (isHorizontal) {
                    // Подсчитываем горизонтальную длину
                    int xx = x;
                    while (xx < 10 && board.isShot(xx, y) && board.wasShipSunkAt(xx, y)) {
                        visited[y][xx] = true;
                        size++;
                        xx++;
                    }
                } else if (isVertical) {
                    // Подсчитываем вертикальную длину
                    int yy = y;
                    while (yy < 10 && board.isShot(x, yy) && board.wasShipSunkAt(x, yy)) {
                        visited[yy][x] = true;
                        size++;
                        yy++;
                    }
                } else {
                    // Одноклеточный корабль
                    visited[y][x] = true;
                    size = 1;
                }
                
                // Добавляем размер потопленного корабля
                sunkShips.push_back(size);
            }
        }
    }
    
    // Удаляем найденные потопленные корабли из списка оставшихся
    for (int size : sunkShips) {
        auto it = std::find(ships.begin(), ships.end(), size);
        if (it != ships.end()) {
            ships.erase(it);
        }
    }
    
    return ships;
}

void MonteCarloStrategy::init_prob_board() {
    for (auto& row : prob_board) {
        row.fill(0);
    }
}

void MonteCarloStrategy::updateHitsList(const Board& board) {
    // Очищаем текущий список попаданий
    m_hits.clear();
    
    // Собираем все клетки с попаданиями, но не потопленными кораблями
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            if (board.isShot(x, y) && 
                board.getCell(x, y) == CellState::HIT && 
                !board.wasShipSunkAt(x, y)) {
                m_hits.emplace_back(x, y);
            }
        }
    }
}

void MonteCarloStrategy::build_probability(const Board& board) {
    if (m_prob_board_valid) return;
    init_prob_board();
    updateHitsList(board);

    const std::vector<int> baseShips = getRemainingShips(board);
    if (baseShips.empty()) return;

    int successful = 0;
    const int NEED = m_samples;
    for (int tries = 0; successful < NEED; ++tries) {
        MCPlacement p{};
        auto ships = baseShips;
        bool ok = true;
        
        // 1. если есть попадания, расставляем самый длинный корабль на попадания
        if (!m_hits.empty()) {
            // перемешиваем ships вручную (Fisher-Yates)
            for (int idx = static_cast<int>(ships.size()) - 1; idx > 0; --idx) {
                int j = m_rng.uniformInt(0, idx);
                std::swap(ships[idx], ships[j]);
            }
            std::sort(ships.begin(), ships.end(), std::greater<int>());
            int longest = ships.front();

            bool placed = false;
            for (int k = 0; k < 200 && !placed; ++k) {
                bool hor = m_rng.uniformInt(0,1);
                auto [hx, hy] = m_hits[m_rng.uniformInt(0, m_hits.size()-1)];
                int x0 = hor ? hx - m_rng.uniformInt(0, longest-1) : hx;
                int y0 = hor ? hy : hy - m_rng.uniformInt(0, longest-1);
                if (fits(x0, y0, longest, hor, p, board, m_hits)) {
                    place(x0, y0, longest, hor, p);
                    ships.erase(std::find(ships.begin(), ships.end(), longest));
                    placed = true;
                }
            }
            if (!placed) ok = false;
        }

        // 2. расставляем остальные корабли
        for (int len : ships) {
            bool placed = false;
            for (int k = 0; k < 200 && !placed; ++k) {
                bool hor = m_rng.uniformInt(0,1);
                int x = m_rng.uniformInt(0, 10 - (hor ? len : 1));
                int y = m_rng.uniformInt(0, 10 - (hor ? 1 : len));
                if (fits(x, y, len, hor, p, board, m_hits)) {
                    place(x, y, len, hor, p);
                    placed = true;
                }
            }
            if (!placed) { ok = false; break; }
        }

        if (!ok) continue;

        // 3. учитываем образец
        ++successful;
        for (int yy = 0; yy < 10; ++yy) {
            for (int xx = 0; xx < 10; ++xx) {
                if (p.occ[yy][xx]) {
                    prob_board[yy][xx] += 1;
                }
            }
        }
    }

    m_prob_board_valid = true;
}

// Константы для направлений обхода (вверх, вправо, вниз, влево)
const int dx[4] = {0, 1, 0, -1};
const int dy[4] = {-1, 0, 1, 0};

void MonteCarloStrategy::addTargetsAroundHit(int x, int y) {
    // Если уже есть несколько попаданий, определяем их ориентацию
    bool isVertical = false;
    bool isHorizontal = false;
    
    if (m_hits.size() > 1) {
        // Проверяем вертикальную ориентацию
        bool allSameX = true;
        int firstX = m_hits[0].first;
        
        // Проверяем горизонтальную ориентацию
        bool allSameY = true;
        int firstY = m_hits[0].second;
        
        for (size_t i = 1; i < m_hits.size(); ++i) {
            if (m_hits[i].first != firstX)
                allSameX = false;
            if (m_hits[i].second != firstY)
                allSameY = false;
        }
        
        isVertical = allSameX && !allSameY;
        isHorizontal = allSameY && !allSameX;
    }
    
    // Если ориентация определена
    if (isVertical || isHorizontal) {
        // Находим крайние точки
        int minX = 10, maxX = -1, minY = 10, maxY = -1;
        for (const auto& hit : m_hits) {
            minX = std::min(minX, hit.first);
            maxX = std::max(maxX, hit.first);
            minY = std::min(minY, hit.second);
            maxY = std::max(maxY, hit.second);
        }
        
        // Для вертикальной ориентации проверяем клетки сверху и снизу
        if (isVertical) {
            // Клетка сверху от верхней точки
            int nx = minX;
            int ny = minY - 1;
            
            if (inside(nx, ny) && 
                std::find(shots.begin(), shots.end(), std::make_pair(nx, ny)) == shots.end() &&
                m_excluded_cells.find(std::make_pair(nx, ny)) == m_excluded_cells.end()) {
                m_targets.emplace(nx, ny);
            }
            
            // Клетка снизу от нижней точки
            nx = minX;
            ny = maxY + 1;
            
            if (inside(nx, ny) && 
                std::find(shots.begin(), shots.end(), std::make_pair(nx, ny)) == shots.end() &&
                m_excluded_cells.find(std::make_pair(nx, ny)) == m_excluded_cells.end()) {
                m_targets.emplace(nx, ny);
            }
        } 
        // Для горизонтальной ориентации проверяем клетки слева и справа
        else if (isHorizontal) {
            // Клетка слева от левой точки
            int nx = minX - 1;
            int ny = minY;
            
            if (inside(nx, ny) && 
                std::find(shots.begin(), shots.end(), std::make_pair(nx, ny)) == shots.end() &&
                m_excluded_cells.find(std::make_pair(nx, ny)) == m_excluded_cells.end()) {
                m_targets.emplace(nx, ny);
            }
            
            // Клетка справа от правой точки
            nx = maxX + 1;
            ny = minY;
            
            if (inside(nx, ny) && 
                std::find(shots.begin(), shots.end(), std::make_pair(nx, ny)) == shots.end() &&
                m_excluded_cells.find(std::make_pair(nx, ny)) == m_excluded_cells.end()) {
                m_targets.emplace(nx, ny);
            }
        }
    } 
    else {
        // Если ориентация неизвестна, добавляем все 4 направления
        for (int i = 0; i < 4; ++i) {
            int nx = x + dx[i];
            int ny = y + dy[i];
            
            // Если клетка в границах, не была обстреляна и не исключена
            if (inside(nx, ny) && 
                std::find(shots.begin(), shots.end(), std::make_pair(nx, ny)) == shots.end() &&
                m_excluded_cells.find(std::make_pair(nx, ny)) == m_excluded_cells.end()) {
                m_targets.emplace(nx, ny);
            }
        }
    }
}

void MonteCarloStrategy::removeFromProbBoard(int x, int y) {
    if (inside(x, y)) {
        prob_board[y][x] = 0;
    }
}

void MonteCarloStrategy::markSurroundingCellsAsUnavailable(const Board& board) {
    // Временная карта для отслеживания посещенных клеток с потопленными кораблями
    std::array<std::array<bool, 10>, 10> visited = {};
    for (auto& row : visited) row.fill(false);
    
    // Находим все потопленные корабли на доске
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            if (board.isShot(x, y) && board.wasShipSunkAt(x, y) && !visited[y][x]) {
                // Нашли потопленный корабль, определяем его размер и ориентацию
                std::vector<std::pair<int, int>> shipCells;
                
                // Проверяем горизонтальную ориентацию
                bool isHorizontal = (x + 1 < 10 && board.isShot(x + 1, y) && 
                                     board.wasShipSunkAt(x + 1, y));
                
                // Проверяем вертикальную ориентацию
                bool isVertical = (y + 1 < 10 && board.isShot(x, y + 1) && 
                                  board.wasShipSunkAt(x, y + 1));
                
                if (isHorizontal) {
                    // Собираем все клетки горизонтального корабля
                    int xx = x;
                    while (xx < 10 && board.isShot(xx, y) && board.wasShipSunkAt(xx, y)) {
                        shipCells.emplace_back(xx, y);
                        visited[y][xx] = true;
                        xx++;
                    }
                } else if (isVertical) {
                    // Собираем все клетки вертикального корабля
                    int yy = y;
                    while (yy < 10 && board.isShot(x, yy) && board.wasShipSunkAt(x, yy)) {
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
                            if (inside(nx, ny) && 
                                !(board.isShot(nx, ny) && board.wasShipSunkAt(nx, ny))) {
                                m_excluded_cells.emplace(nx, ny);
                                // Также обнуляем вероятность для этой клетки
                                removeFromProbBoard(nx, ny);
                            }
                        }
                    }
                }
            }
        }
    }
}

std::pair<int, int> MonteCarloStrategy::getNextShot(const Board& board) {
    // при первом ходе партии сбрасываем флаг валидности вероятностной карты
    if (shots.empty()) {
        m_prob_board_valid = false;
    }
    // Если мы в режиме добивания и очередь не пуста
    if (m_targeting_mode && !m_targets.empty()) {
        // Берем клетку из очереди
        std::pair<int, int> target = m_targets.front();
        m_targets.pop();
        
        // Проверяем, что клетка еще не была обстреляна и не исключена
        while (!m_targets.empty() && 
               (std::find(shots.begin(), shots.end(), target) != shots.end() ||
                m_excluded_cells.find(target) != m_excluded_cells.end())) {
            target = m_targets.front();
            m_targets.pop();
        }
        
        // Если нашли подходящую клетку, стреляем туда
        if (std::find(shots.begin(), shots.end(), target) == shots.end() &&
            m_excluded_cells.find(target) == m_excluded_cells.end()) {
            shots.push_back(target);
            return target;
        }
    }
    
    // Если режим добивания не активен или очередь пуста, используем вероятностный подход
    
    // Строим или используем вероятностную карту
    if (!m_prob_board_valid) {
        build_probability(board);
    }
    
    // Находим клетку с максимальной "температурой"
    int bestHeat = -1;
    int bestX = -1;
    int bestY = -1;
    
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            // Проверяем, что клетка не исключена и не была обстреляна
            if (!board.wasShotAt(x, y) && 
                m_excluded_cells.find(std::make_pair(x, y)) == m_excluded_cells.end() && 
                prob_board[y][x] > bestHeat) {
                bestHeat = prob_board[y][x];
                bestX = x;
                bestY = y;
                }
            }
        }
        
    // Если нашли хотя бы одну подходящую клетку
    if (bestHeat >= 0) {
        auto nextShot = std::make_pair(bestX, bestY);
        shots.push_back(nextShot);
        return nextShot;
    }
    
    // В крайне маловероятном случае, если не нашли подходящую клетку,
    // используем случайную стратегию как запасной вариант
    RandomStrategy fallbackStrategy(m_rng);
    auto nextShot = fallbackStrategy.getNextShot(board);
    
    // Пропускаем исключенные клетки
    while (m_excluded_cells.find(nextShot) != m_excluded_cells.end()) {
        nextShot = fallbackStrategy.getNextShot(board);
    }
    
    shots.push_back(nextShot);
    return nextShot;
}

void MonteCarloStrategy::notifyShotResult(int x, int y, bool hit, bool sunk, const Board& board) {
    // Если попали, добавляем соседние клетки в очередь целей и переходим в режим добивания
    if (hit) {
        m_targeting_mode = true;
        
        // Добавляем координаты попадания в список и в очередь добивания
        m_hits.emplace_back(x, y);
        addTargetsAroundHit(x, y);
        
        // Если есть несколько попаданий, проверяем, не образуют ли они линию
        if (m_hits.size() > 1) {
            bool horizontal_line = true;
            bool vertical_line = true;
            int base_y = m_hits[0].second;
            int base_x = m_hits[0].first;
            
            for (size_t i = 1; i < m_hits.size(); ++i) {
                if (m_hits[i].second != base_y)
                    horizontal_line = false;
                if (m_hits[i].first != base_x)
                    vertical_line = false;
            }
            
            // Если попадания образуют линию, приоритизируем клетки в этом направлении
            if (horizontal_line && !vertical_line) {
                // Находим крайние точки по X
                int min_x = base_x;
                int max_x = base_x;
                for (const auto& hit : m_hits) {
                    min_x = std::min(min_x, hit.first);
                    max_x = std::max(max_x, hit.first);
                }
                
                // Очищаем текущую очередь
                std::queue<std::pair<int, int>> new_targets;
                
                // Добавляем только клетки слева и справа от крайних попаданий
                if (min_x > 0 && !std::any_of(shots.begin(), shots.end(), 
                                              [min_x, base_y](const auto& s) { 
                                                  return s.first == min_x - 1 && s.second == base_y; 
                                              }) &&
                   m_excluded_cells.find(std::make_pair(min_x - 1, base_y)) == m_excluded_cells.end()) {
                    new_targets.emplace(min_x - 1, base_y);
                }
                
                if (max_x < 9 && !std::any_of(shots.begin(), shots.end(), 
                                             [max_x, base_y](const auto& s) { 
                                                 return s.first == max_x + 1 && s.second == base_y; 
                                             }) &&
                   m_excluded_cells.find(std::make_pair(max_x + 1, base_y)) == m_excluded_cells.end()) {
                    new_targets.emplace(max_x + 1, base_y);
                }
                
                // Заменяем очередь целей
                m_targets = new_targets;
            }
            else if (!horizontal_line && vertical_line) {
                // Находим крайние точки по Y
                int min_y = base_y;
                int max_y = base_y;
                for (const auto& hit : m_hits) {
                    min_y = std::min(min_y, hit.second);
                    max_y = std::max(max_y, hit.second);
                }
                
                // Очищаем текущую очередь
                std::queue<std::pair<int, int>> new_targets;
                
                // Добавляем только клетки сверху и снизу от крайних попаданий
                if (min_y > 0 && !std::any_of(shots.begin(), shots.end(), 
                                              [base_x, min_y](const auto& s) { 
                                                  return s.first == base_x && s.second == min_y - 1; 
                                              }) &&
                   m_excluded_cells.find(std::make_pair(base_x, min_y - 1)) == m_excluded_cells.end()) {
                    new_targets.emplace(base_x, min_y - 1);
                }
                
                if (max_y < 9 && !std::any_of(shots.begin(), shots.end(), 
                                             [base_x, max_y](const auto& s) { 
                                                 return s.first == base_x && s.second == max_y + 1; 
                                             }) &&
                   m_excluded_cells.find(std::make_pair(base_x, max_y + 1)) == m_excluded_cells.end()) {
                    new_targets.emplace(base_x, max_y + 1);
                }
                
                // Заменяем очередь целей
                m_targets = new_targets;
            }
        }
        
        // Инвалидируем вероятностную доску, т.к. изменился список попаданий
        m_prob_board_valid = false;
    } 
    else {
        // Если промах, просто убираем эту клетку из вероятностной карты
        removeFromProbBoard(x, y);
        // Инвалидируем вероятностную доску после промаха
        m_prob_board_valid = false;
    }
    
    // Если потопили корабль
    if (sunk) {
        // Помечаем клетки вокруг потопленного корабля как недоступные
        markSurroundingCellsAsUnavailable(board);
        
        // Очищаем список попаданий, так как корабль уже потоплен
        m_hits.clear();
        
        // Очищаем очередь целей
        while (!m_targets.empty()) m_targets.pop();
        
        // Выходим из режима добивания
        m_targeting_mode = false;
        
        // Инвалидируем вероятностную доску
        m_prob_board_valid = false;
    }
}

void MonteCarloStrategy::reset() {
    shots.clear();
    init_prob_board();
    m_targeting_mode = false;
    
    // Очищаем очередь целей и список попаданий
    while (!m_targets.empty()) m_targets.pop();
    m_hits.clear();
    // Очищаем список исключенных клеток
    m_excluded_cells.clear();
    // Сбрасываем флаг валидности вероятностной карты для новой игры
    m_prob_board_valid = false;
}

std::vector<std::pair<int, int>> MonteCarloStrategy::getAllShots() const {
    return shots;
}

std::string MonteCarloStrategy::getName() const {
    return "Monte-Carlo-" + std::to_string(m_samples);
} 