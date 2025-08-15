#pragma once

#include <vector>
#include <string> 
#include <array>
#include <memory>

#include "ship.h"
#include "../utils/rng.h" 

class Board; // Forward declaration

/**
 * @class Fleet
 * @brief Класс, представляющий флот кораблей в игре "Морской бой".
 * 
 * Флот состоит из 10 кораблей разной длины:
 * - 1 линкор (4 клетки)
 * - 2 крейсера (3 клетки)
 * - 3 эсминца (2 клетки)
 * - 4 подводные лодки (1 клетка)
 * 
 * Класс обеспечивает проверку правил размещения кораблей и
 * возможность ремонта недопустимых расстановок.
 */
class Fleet {
public:
    /**
     * @brief Конструктор флота
     */
    Fleet();
    
    /**
     * @brief Добавить корабль во флот
     * @param ship Корабль
     */
    void addShip(const Ship& ship);
    
    /**
     * @brief Получить все корабли флота
     * @return Вектор кораблей
     */
    const std::vector<Ship>& getShips() const { return m_ships; }
    
    /**
     * @brief Получить корабль по индексу
     * @param index Индекс корабля
     * @return Ссылка на корабль
     * @throw std::out_of_range если индекс недопустим
     */
    const Ship& getShip(size_t index) const;
    
    /**
     * @brief Получить корабль по индексу
     * @param index Индекс корабля
     * @return Ссылка на корабль
     * @throw std::out_of_range если индекс недопустим
     */
    Ship& getShip(size_t index);
    
    /**
     * @brief Получить размер флота (количество кораблей)
     * @return Количество кораблей
     */
    size_t size() const { return m_ships.size(); }
    
    /**
     * @brief Проверить, все ли правила размещения выполнены
     * @return true, если все правила выполнены, false иначе
     */
    bool isValid(int boardSize = 10) const;
    
    /**
     * @brief Проверить, все ли корабли находятся в границах поля
     * @return true, если все корабли в границах, false иначе
     */
    bool allShipsWithinBounds() const;
    
    /**
     * @brief Починить расстановку кораблей, если она недопустима
     * 
     * Метод пытается исправить недопустимые расстановки, сдвигая
     * и поворачивая корабли, пока не будут выполнены все правила.
     * @param maxAttempts Максимальное количество попыток починки (50 по умолчанию)
     * @return true, если удалось починить расстановку, false иначе
     */
    bool repair(RNG& rng, int boardSize = 10, int maxAttemptsPerShip = 100);
    
    /**
     * @brief Разместить флот на игровом поле
     * @param board Игровое поле
     * @return true, если флот успешно размещен, false иначе
     */
    bool placeOnBoard(Board& board) const;
    
    /**
     * @brief Получить все клетки, занимаемые кораблями флота
     * @return Вектор пар координат (x, y)
     */
    std::vector<std::pair<int, int>> getAllOccupiedCells() const;
    
    /**
     * @brief Вывести флот в консоль
     */
    void print() const;

    /**
     * @brief Проверить, есть ли корабль в указанной клетке
     * @param x X-координата клетки
     * @param y Y-координата клетки
     * @return true, если в клетке есть корабль, false иначе
     */
    bool hasShipAt(int x, int y) const;
    
    /**
     * @brief Очистить флот (удалить все корабли)
     */
    void clear() { m_ships.clear(); }
    
    /**
     * @brief Стандартные длины кораблей для флота
     * 
     * Массив из 10 элементов, содержащий длины кораблей:
     * [4, 3, 3, 2, 2, 2, 1, 1, 1, 1]
     */
    static const std::array<int, 10> standardShipLengths;

    /**
     * @brief Проверяет, пуст ли флот
     * @return true, если флот пуст, иначе false
     */
    bool isEmpty() const { return m_ships.empty(); }

    /**
     * @brief Сохраняет флот в файл
     * @param filename Имя файла
     * @return true, если сохранение успешно, иначе false
     */
    bool saveToFile(const std::string& filename) const;

    /**
     * @brief Загружает флот из файла
     * @param filename Имя файла
     * @return true, если загрузка успешна, иначе false
     */
    bool loadFromFile(const std::string& filename);

    /**
     * @brief Создать стандартный флот из 10 кораблей (статический метод)
     * 
     * Создает и возвращает новый флот со стандартными кораблями
     * @return Указатель на новый флот
     */
    static std::unique_ptr<Fleet> createStandardFleet();

    /**
     * @brief Создать стандартный флот из 10 кораблей
     * 
     * Корабли размещаются случайно с учетом всех правил.
     * @return true, если удалось создать флот, false иначе
     */
    bool createStandardFleet(RNG& rng, int boardSize = 10);

    /**
     * @brief Сериализует флот в строку
     * @return Строка, представляющая флот
     */
    std::string serialize() const;

    /**
     * @brief Десериализует флот из строки
     * @param data Строка, представляющая флот
     * @return true, если десериализация успешна, false иначе
     */
    bool deserialize(const std::string& data);

private:
    std::vector<Ship> m_ships;          ///< Корабли флота
    
    // Вспомогательные методы, должны быть private, если используются только внутри класса
    bool tryPlaceShip(Ship& ship, RNG& rng, int boardSize, int maxAttempts);
    void randomlyPlaceShip(Ship& ship, RNG& rng, int boardSize);
};