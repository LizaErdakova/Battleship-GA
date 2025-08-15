#pragma once

#include <memory>
#include <vector>
#include <utility>
#include "../models/board.h"
#include "../models/fleet.h"
#include "../strategies/strategy.h"

/**
 * @brief Класс для симуляции одиночной игры
 * 
 * Этот класс позволяет смоделировать игру между двумя игроками (стратегиями),
 * отслеживая количество ходов и исход игры.
 */
class Game {
private:
    int boardSize;                   ///< Размер игрового поля
    Board board1;                    ///< Игровое поле первого игрока
    Board board2;                    ///< Игровое поле второго игрока
    std::unique_ptr<Strategy> strategy1; ///< Стратегия первого игрока
    std::unique_ptr<Strategy> strategy2; ///< Стратегия второго игрока
    bool strictAdjacency;            ///< Строгое правило о недопустимости касания кораблей
    int maxMoves;                    ///< Максимальное число ходов (для предотвращения зацикливания)
    
    int player1Shots;                ///< Число выстрелов первого игрока
    int player2Shots;                ///< Число выстрелов второго игрока
    bool player1Won;                 ///< Флаг победы первого игрока
    bool player2Won;                 ///< Флаг победы второго игрока
    bool gameOver;                   ///< Флаг окончания игры
    
public:
    /**
     * @brief Конструктор игры
     * 
     * @param strategy1 Стратегия первого игрока
     * @param strategy2 Стратегия второго игрока
     * @param boardSize Размер игрового поля (обычно 10)
     * @param strictAdjacency Строгое правило о недопустимости касания кораблей
     * @param maxMoves Максимальное число ходов для предотвращения зацикливания
     */
    Game(std::unique_ptr<Strategy> strategy1, 
         std::unique_ptr<Strategy> strategy2, 
         int boardSize = 10, 
         bool strictAdjacency = true,
         int maxMoves = 200);
    
    /**
     * @brief Инициализация игры с заданными флотами
     * 
     * @param fleet1 Флот первого игрока
     * @param fleet2 Флот второго игрока
     * @return true, если инициализация успешна, false иначе
     */
    bool initialize(const Fleet& fleet1, const Fleet& fleet2);
    
    /**
     * @brief Инициализация игры со случайными флотами
     * 
     * @return true, если инициализация успешна, false иначе
     */
    bool initialize();
    
    /**
     * @brief Выполнение хода игры
     * 
     * @return true, если игра продолжается, false если игра окончена
     */
    bool step();
    
    /**
     * @brief Полная симуляция игры до окончания
     * 
     * @return true, если игра завершилась успешно, false иначе
     */
    bool simulate();
    
    /**
     * @brief Проверка окончания игры
     * 
     * @return true, если игра окончена, false иначе
     */
    bool isGameOver() const { return gameOver; }
    
    /**
     * @brief Получение числа выстрелов первого игрока
     * 
     * @return Число выстрелов первого игрока
     */
    int getPlayer1Shots() const { return player1Shots; }
    
    /**
     * @brief Получение числа выстрелов второго игрока
     * 
     * @return Число выстрелов второго игрока
     */
    int getPlayer2Shots() const { return player2Shots; }
    
    /**
     * @brief Проверка победы первого игрока
     * 
     * @return true, если первый игрок победил, false иначе
     */
    bool hasPlayer1Won() const { return player1Won; }
    
    /**
     * @brief Проверка победы второго игрока
     * 
     * @return true, если второй игрок победил, false иначе
     */
    bool hasPlayer2Won() const { return player2Won; }
    
    /**
     * @brief Сброс игры для новой партии
     */
    void reset();
    
    /**
     * @brief Вывод текущего состояния игры
     * 
     * @param showShips true, если нужно показывать корабли, false иначе
     */
    void print(bool showShips = false) const;
}; 