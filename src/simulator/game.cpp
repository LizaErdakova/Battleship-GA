#include "game.h"
#include <iostream>
#include <memory>
#include <utility>

Game::Game(std::unique_ptr<Strategy> strategy1, 
           std::unique_ptr<Strategy> strategy2, 
           int boardSize, 
           bool strictAdjacency,
           int maxMoves)
    : boardSize(boardSize), 
      board1(boardSize), 
      board2(boardSize),
      strategy1(std::move(strategy1)),
      strategy2(std::move(strategy2)),
      strictAdjacency(strictAdjacency),
      maxMoves(maxMoves),
      player1Shots(0),
      player2Shots(0),
      player1Won(false),
      player2Won(false),
      gameOver(false) {
}

bool Game::initialize(const Fleet& fleet1, const Fleet& fleet2) {
    // Проверяем валидность флотов
    if (!fleet1.isValid() || !fleet2.isValid()) {
        return false;
    }
    
    // Сбрасываем доски и размещаем корабли
    board1.clear();
    board2.clear();
    
    if (!fleet1.placeOnBoard(board1) || !fleet2.placeOnBoard(board2)) {
        return false;
    }
    
    // Сбрасываем статистику игры
    reset();
    
    return true;
}

bool Game::initialize() {
    // Создаем случайные флоты
    Fleet fleet1; // Используем конструктор по умолчанию
    Fleet fleet2;
    
    RNG rng; // Создаем RNG
    // Генерируем случайные расстановки
    if (!fleet1.createStandardFleet(rng, boardSize) || !fleet2.createStandardFleet(rng, boardSize)) {
        return false;
    }
    
    // Инициализируем игру с созданными флотами
    return initialize(fleet1, fleet2);
}

bool Game::step() {
    if (gameOver) {
        return false;
    }
    
    // Ход первого игрока
    auto shot1 = strategy1->getNextShot(board2);
    if (shot1.first < 0 || shot1.second < 0) {
        gameOver = true;
        return false;
    }
    
    bool hit1 = board2.shoot(shot1.first, shot1.second);
    bool sunk1 = hit1 && board2.wasShipSunkAt(shot1.first, shot1.second);
    player1Shots++;
    
    strategy1->notifyShotResult(shot1.first, shot1.second, hit1, sunk1, board2);
    
    // Проверяем условие победы первого игрока
    if (board2.allShipsSunk()) {
        player1Won = true;
        gameOver = true;
        return false;
    }
    
    // Ход второго игрока
    auto shot2 = strategy2->getNextShot(board1);
    if (shot2.first < 0 || shot2.second < 0) {
        gameOver = true;
        return false;
    }
    
    bool hit2 = board1.shoot(shot2.first, shot2.second);
    bool sunk2 = hit2 && board1.wasShipSunkAt(shot2.first, shot2.second);
    player2Shots++;
    
    strategy2->notifyShotResult(shot2.first, shot2.second, hit2, sunk2, board1);
    
    // Проверяем условие победы второго игрока
    if (board1.allShipsSunk()) {
        player2Won = true;
        gameOver = true;
        return false;
    }
    
    // Проверяем максимальное число ходов
    if (player1Shots >= maxMoves || player2Shots >= maxMoves) {
        gameOver = true;
        return false;
    }
    
    return true;
}

bool Game::simulate() {
    if (!gameOver) {
        // Выполняем шаги до окончания игры
        while (step()) {
            // Продолжаем игру
        }
    }
    
    return gameOver && (player1Won || player2Won);
}

void Game::reset() {
    // Сбрасываем статистику игры
    player1Shots = 0;
    player2Shots = 0;
    player1Won = false;
    player2Won = false;
    gameOver = false;
    
    // Сбрасываем стратегии
    strategy1->reset();
    strategy2->reset();
}

void Game::print(bool showShips) const {
    std::cout << "=== Состояние игры ===" << std::endl;
    
    std::cout << "Поле игрока 1:" << std::endl;
    board1.print(showShips);
    
    std::cout << "Поле игрока 2:" << std::endl;
    board2.print(showShips);
    
    std::cout << "Статистика:" << std::endl;
    std::cout << "Ходы игрока 1: " << player1Shots << std::endl;
    std::cout << "Ходы игрока 2: " << player2Shots << std::endl;
    
    if (gameOver) {
        std::cout << "Игра окончена. ";
        if (player1Won) {
            std::cout << "Победил игрок 1." << std::endl;
        } else if (player2Won) {
            std::cout << "Победил игрок 2." << std::endl;
        } else {
            std::cout << "Ничья (превышено максимальное число ходов)." << std::endl;
        }
    } else {
        std::cout << "Игра продолжается." << std::endl;
    }
} 