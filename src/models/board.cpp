#include "board.h"
#include "fleet.h" // Для использования в placeFleet
#include <iostream>
#include <iomanip>
#include <algorithm> // для std::all_of

Board::Board() : m_sunkShipCells(0), m_totalShipCells(0) {
    clear();
}

void Board::clear() {
    for (auto& row : m_grid) {
        row.fill(CellState::SEA);
    }
    m_ships.clear();
    m_sunkShipCells = 0;
    m_totalShipCells = 0;
}

// Устаревшая версия, помечает одну клетку как корабль. 
// Не обновляет m_ships или m_totalShipCells.
// Используется для обратной совместимости или очень специфичных тестов.
bool Board::placeShip(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false;
    }
    if (m_grid[y][x] != CellState::SEA) { // Можно ставить только на море
        return false;
    }
    m_grid[y][x] = CellState::SHIP;
    // В этой упрощенной версии не отслеживаем m_totalShipCells и m_ships
    return true;
}

bool Board::placeShip(const Ship& ship) {
    // 1. Проверка, что корабль в границах
    if (!ship.isWithinBounds(BOARD_SIZE)) {
        return false;
    }

    // 2. Проверка, что клетки не заняты и нет касаний (по правилам)
    //    Для этого обойдем все клетки корабля и их соседей.
    for (const auto& cell : ship.getCells()) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dx = -1; dx <= 1; ++dx) {
                int curX = cell.first + dx;
                int curY = cell.second + dy;

                if (curX >= 0 && curX < BOARD_SIZE && curY >= 0 && curY < BOARD_SIZE) {
                    // Если это не клетка самого корабля (которую мы собираемся поставить)
                    // и она не является морем, то это конфликт.
                    bool isPartOfCurrentShip = false;
                    for(const auto& ownCell : ship.getCells()){
                        if(ownCell.first == curX && ownCell.second == curY){
                            isPartOfCurrentShip = true;
                            break;
                        }
                    }
                    if (!isPartOfCurrentShip && m_grid[curY][curX] != CellState::SEA) {
                        return false; // Касание или пересечение с другим кораблем
                    }
                }
            }
        }
    }
    
    // 3. Размещение корабля
    for (const auto& cell : ship.getCells()) {
        m_grid[cell.second][cell.first] = CellState::SHIP;
    }
    m_ships.push_back(ship);
    m_totalShipCells += ship.getLength();
    return true;
}

bool Board::placeFleet(const Fleet& fleet) {
    clear(); // Очищаем доску перед размещением нового флота
    for (const auto& ship : fleet.getShips()) {
        if (!placeShip(ship)) {
            // Если хотя бы один корабль не удалось разместить, очищаем доску и возвращаем false
            // Это гарантирует, что на доске либо полный валидный флот, либо ничего.
            clear(); 
            return false;
        }
    }
    return true;
}


bool Board::shoot(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false; // Выстрел за пределы поля
    }

    CellState& currentCell = m_grid[y][x];

    if (currentCell == CellState::HIT || currentCell == CellState::MISS || currentCell == CellState::SUNK) {
        return false; // Повторный выстрел
    }

    if (currentCell == CellState::SHIP) {
        currentCell = CellState::HIT;
        m_sunkShipCells++; // Считаем каждую подбитую палубу
        
        // Проверить, потоплен ли корабль, которому принадлежит эта клетка
        for (const auto& ship : m_ships) {
            bool hitThisShip = false;
            for (const auto& shipCell : ship.getCells()) {
                if (shipCell.first == x && shipCell.second == y) {
                    hitThisShip = true;
                    break;
                }
            }
            if (hitThisShip) {
                if (isShipSunk(ship)) {
                    markShipAsSunk(ship);
                }
                break; 
            }
        }
        return true; // Попадание
    } else { // currentCell == CellState::SEA
        currentCell = CellState::MISS;
        return false; // Промах
    }
}

CellState Board::getCell(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        // Можно выбросить исключение или вернуть специальное значение
        return CellState::SEA; // Для простоты вернем SEA
    }
    return m_grid[y][x];
}

bool Board::isShot(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false;
    }
    CellState state = m_grid[y][x];
    return state == CellState::HIT || state == CellState::MISS || state == CellState::SUNK;
}

bool Board::allShipsSunk() const {
    if (m_totalShipCells == 0 && m_ships.empty()) return false; // Нет кораблей для потопления
    return m_sunkShipCells >= m_totalShipCells;
}

void Board::print(bool showShips) const {
    std::cout << "  ";
    for (int i = 0; i < BOARD_SIZE; ++i) {
        std::cout << i << " ";
    }
    std::cout << std::endl;

    for (int y = 0; y < BOARD_SIZE; ++y) {
        std::cout << std::setw(2) << y << " ";
        for (int x = 0; x < BOARD_SIZE; ++x) {
            char symbol = ' ';
            switch (m_grid[y][x]) {
                case CellState::SEA:
                    symbol = '.';
                    break;
                case CellState::SHIP:
                    symbol = showShips ? 'S' : '.';
                    break;
                case CellState::HIT:
                    symbol = 'X';
                    break;
                case CellState::MISS:
                    symbol = 'O';
                    break;
                case CellState::SUNK:
                    symbol = '#'; // Символ для потопленного корабля
                    break;
            }
            std::cout << symbol << " ";
        }
        std::cout << std::endl;
    }
}

bool Board::wasShotAt(int x, int y) const {
    return isShot(x,y); // Просто алиас для isShot
}

bool Board::wasShipSunkAt(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false;
    }
    return m_grid[y][x] == CellState::SUNK;
}

bool Board::isShipSunk(const Ship& ship) const {
    for (const auto& cell : ship.getCells()) {
        if (m_grid[cell.second][cell.first] == CellState::SHIP) {
            return false; // Найдена неповрежденная часть корабля
        }
    }
    return true; // Все части корабля либо HIT, либо уже SUNK
}

void Board::markShipAsSunk(const Ship& ship) {
    for (const auto& cell : ship.getCells()) {
        m_grid[cell.second][cell.first] = CellState::SUNK;
        // Можно также обвести потопленный корабль клетками MISS, если правила это требуют
        // Это здесь не реализовано для простоты
    }
}

bool Board::isCellFree(int x, int y) const {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false; // За пределами поля - не свободно
    }
    return m_grid[y][x] == CellState::SEA;
}

bool Board::isCellFree(const Cell& cell) const {
    return isCellFree(cell.x, cell.y);
}

void Board::markCell(int x, int y) {
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        // Можно добавить проверку, что клетка SEA, если нужно
        m_grid[y][x] = CellState::SHIP; // Или другое состояние, если это общий метод
    }
}

void Board::markCell(const Cell& cell) {
    markCell(cell.x, cell.y);
}

void Board::clearCell(int x, int y) {
    if (x >= 0 && x < BOARD_SIZE && y >= 0 && y < BOARD_SIZE) {
        m_grid[y][x] = CellState::SEA;
    }
}

void Board::clearCell(const Cell& cell) {
    clearCell(cell.x, cell.y);
}

bool Board::markShot(int x, int y) {
    if (x < 0 || x >= BOARD_SIZE || y < 0 || y >= BOARD_SIZE) {
        return false; // Клетка за пределами поля
    }
    
    // Если клетка уже отмечена как выстрел, возвращаем false
    if (m_grid[y][x] == CellState::HIT || m_grid[y][x] == CellState::MISS || m_grid[y][x] == CellState::SUNK) {
        return false;
    }
    
    // Отмечаем клетку как промах (для целей проверки достаточно)
    m_grid[y][x] = CellState::MISS;
    return true;
}

// Добавляю метод для определения наибольшего оставшегося корабля
int Board::largestRemainingShipSize() const {
    int maxLen = 0;
    for (const auto& ship : m_ships) {
        if (!isShipSunk(ship)) {
            maxLen = std::max(maxLen, ship.getLength());
        }
    }
    return maxLen;
} 