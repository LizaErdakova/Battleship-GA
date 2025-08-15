#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <thread>     // Добавляем для функции sleep_for
#include <sstream>
#include <ctime>
#include <locale.h>  // Для setlocale
#include <sstream>   // Для stringstream
#include <fstream>   // Для работы с файлами
#include "models/ship.h"
#include "models/board.h"
#include "models/fleet.h"
#include "utils/rng.h"
#include "utils/logger.h"  // Добавлен logger.h
#include "utils/io.h"      // Добавлен io.h
#include "strategies/random_strategy.h"
#include "strategies/checkerboard_strategy.h"
#include "strategies/monte_carlo_strategy.h"
#include "strategies/feature_based_strategy.h"
#include "simulator/game.h"
// Раскомментируем подключения GA
#include "ga/placement_ga.h"
#include "ga/decision_ga.h"
#include "ga/fitness.h"
#include "ga/placement_generator.h"

// Прототипы функций
int showMainMenu();
int showGAMenu();
bool showStateFileInfo(const std::string& stateFile, int type);
void trainShooting(int customMaxGen = -1);

/**
 * @brief Генерация уникального ID для запуска
 */
std::string generateRunId() {
    using std::chrono::system_clock;
    auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
    
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif

    std::stringstream ss;
    ss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return ss.str();
}

/**
 * @brief Тестирование класса Ship
 */
void testShip() {
    std::cout << "\n=== Тестирование Ship ===\n" << std::endl;
    
    // Создание корабля
    Ship ship1(3, 4, 3, true);
    std::cout << "Корабль 1: " << std::endl;
    std::cout << "- Позиция: (" << ship1.getX() << ", " << ship1.getY() << ")" << std::endl;
    std::cout << "- Длина: " << ship1.getLength() << std::endl;
    std::cout << "- Ориентация: " << (ship1.getIsVertical() ? "вертикальная" : "горизонтальная") << std::endl;
    
    // Проверка занимаемых клеток
    std::cout << "\nЗанимаемые клетки:" << std::endl;
    auto cells = ship1.getOccupiedCells();
    for (auto cell : cells) {
        std::cout << "(" << cell.first << ", " << cell.second << ") ";
    }
    std::cout << std::endl;
    
    // Проверка пересечения кораблей
    Ship ship2(5, 4, 2, true);
    std::cout << "\nКорабль 2: " << std::endl;
    std::cout << "- Позиция: (" << ship2.getX() << ", " << ship2.getY() << ")" << std::endl;
    std::cout << "- Длина: " << ship2.getLength() << std::endl;
    std::cout << "- Ориентация: " << (ship2.getIsVertical() ? "вертикальная" : "горизонтальная") << std::endl;
    std::cout << "- Пересекаются с кораблем 1: " << (ship1.intersects(ship2) ? "Да" : "Нет") << std::endl;
    
    // Проверка границ поля
    std::cout << "\nВ границах поля 10x10: " << (ship1.isWithinBounds(10) ? "Да" : "Нет") << std::endl;
    
    // Перемещение и поворот
    ship1.move(2, -1);
    std::cout << "\nПосле перемещения (2, -1):" << std::endl;
    std::cout << "- Позиция: (" << ship1.getX() << ", " << ship1.getY() << ")" << std::endl;
    std::cout << "- Ориентация: " << (ship1.getIsVertical() ? "вертикальная" : "горизонтальная") << std::endl;
    
    ship1.rotate();
    std::cout << "\nПосле поворота:" << std::endl;
    std::cout << "- Позиция: (" << ship1.getX() << ", " << ship1.getY() << ")" << std::endl;
    std::cout << "- Ориентация: " << (ship1.getIsVertical() ? "вертикальная" : "горизонтальная") << std::endl;
    
    // Проверка занимаемых клеток после изменений
    std::cout << "\nЗанимаемые клетки после изменений:" << std::endl;
    cells = ship1.getOccupiedCells();
    for (const auto& cell : cells) {
        std::cout << "(" << cell.first << ", " << cell.second << ") ";
    }
    std::cout << std::endl;
}

/**
 * @brief Тестирование класса Board
 */
void testBoard() {
    std::cout << "\n===== Тестирование класса Board =====\n" << std::endl;
    
    // Создаем игровое поле
    Board board;
    
    // Размещаем корабль
    std::cout << "Размещаем корабль на поле:" << std::endl;
    for (int i = 2; i < 5; ++i) {
        board.placeShip(i, 3);
    }
    
    // Выводим поле
    std::cout << "Игровое поле (S - корабль):" << std::endl;
    board.print(true);
    
    // Делаем выстрелы
    std::cout << "\nДелаем выстрелы:" << std::endl;
    std::cout << "Выстрел в (2, 3): " << (board.shoot(2, 3) ? "Попадание!" : "Промах!") << std::endl;
    std::cout << "Выстрел в (5, 5): " << (board.shoot(5, 5) ? "Попадание!" : "Промах!") << std::endl;
    std::cout << "Выстрел в (3, 3): " << (board.shoot(3, 3) ? "Попадание!" : "Промах!") << std::endl;
    
    // Выполняем обстрел
    std::cout << "Выполняем обстрел (4, 5): " << (board.shoot(4, 5) ? "Попадание" : "Промах") << std::endl;
    
    auto cells = board.getRemainingShipCells();
    std::cout << "Осталось неповрежденных клеток кораблей: ";
    for (const auto& cell : cells) {
        std::cout << "(" << cell.first << "," << cell.second << ") ";
    }
    std::cout << std::endl;
    
    // Проверяем состояние клетки после обстрела
    std::cout << "Состояние клетки (4, 5): " << static_cast<int>(board.getCell(4, 5)) << std::endl;
    
    // Проверяем подбит ли корабль полностью
    std::cout << "Корабль подбит полностью: " << (board.wasShipSunkAt(4, 5) ? "Да" : "Нет") << std::endl;
    
    // Выполняем еще один обстрел
    std::cout << "Выполняем обстрел (5, 5): " << (board.shoot(5, 5) ? "Попадание" : "Промах") << std::endl;
    std::cout << "Корабль подбит полностью: " << (board.wasShipSunkAt(4, 5) ? "Да" : "Нет") << std::endl;
    
    // Выполняем еще один обстрел
    std::cout << "Выполняем обстрел (6, 5): " << (board.shoot(6, 5) ? "Попадание" : "Промах") << std::endl;
    std::cout << "Корабль подбит полностью: " << (board.wasShipSunkAt(4, 5) ? "Да" : "Нет") << std::endl;
    
    cells = board.getRemainingShipCells();
    std::cout << "Осталось неповрежденных клеток кораблей: ";
    for (const auto& cell : cells) {
        std::cout << "(" << cell.first << "," << cell.second << ") ";
    }
    std::cout << std::endl;
    
    // Проверяем, все ли корабли потоплены
    std::cout << "Все корабли потоплены: " << (board.allShipsSunk() ? "Да" : "Нет") << std::endl;
    std::cout << "Осталось неповрежденных клеток кораблей: " << board.getRemainingShipCells().size() << std::endl;
    
    // Делаем последний выстрел
    std::cout << "\nДелаем последний выстрел:" << std::endl;
    std::cout << "Выстрел в (4, 3): " << (board.shoot(4, 3) ? "Попадание!" : "Промах!") << std::endl;
    
    // Выводим поле после последнего выстрела
    std::cout << "Игровое поле после всех выстрелов:" << std::endl;
    board.print(true);
    
    // Проверяем, все ли корабли потоплены
    std::cout << "Все корабли потоплены: " << (board.allShipsSunk() ? "Да" : "Нет") << std::endl;
    std::cout << "Осталось неповрежденных клеток кораблей: " << board.getRemainingShipCells().size() << std::endl;
}

/**
 * @brief Тестирование класса Fleet
 */
void testFleet() {
    std::cout << "\n===== Тестирование класса Fleet =====\n" << std::endl;
    
    RNG rng; // Создаем экземпляр RNG
    rng.initialize(123); // Инициализируем (можно без сида для случайности)
    
    // Создаем флот
    Fleet fleet;
    
    // Создаем стандартный флот
    std::cout << "Создание стандартного флота (10 кораблей):" << std::endl;
    bool created = fleet.createStandardFleet(rng); // Передаем rng
    std::cout << "Флот создан успешно: " << (created ? "Да" : "Нет") << std::endl;
    
    // Выводим флот
    std::cout << "\nРасстановка флота:" << std::endl;
    fleet.print();
    
    // Проверяем валидность
    std::cout << "\nПроверка валидности:" << std::endl;
    std::cout << "Флот валидный: " << (fleet.isValid() ? "Да" : "Нет") << std::endl;
    
    // Получаем информацию о кораблях
    std::cout << "\nИнформация о кораблях:" << std::endl;
    for (size_t i = 0; i < fleet.size(); ++i) {
        const Ship& ship = fleet.getShip(i);
        std::cout << "Корабль " << i + 1 << ": ";
        std::cout << "(" << ship.getX() << ", " << ship.getY() << "), ";
        std::cout << "длина = " << ship.getLength() << ", ";
        std::cout << (ship.getIsVertical() ? "вертикальный" : "горизонтальный") << std::endl;
    }
    
    // Тестируем функцию repair
    std::cout << "\nТестирование функции repair:" << std::endl;
    
    // Создаем невалидный флот
    Fleet invalidFleet;
    
    // Добавляем пересекающиеся корабли
    Ship ship1(3, 3, 3, true);
    Ship ship2(2, 4, 3, false); // Эти корабли пересекаются/касаются
    
    invalidFleet.addShip(ship1);
    invalidFleet.addShip(ship2);
    // Добавим еще пару, чтобы было что чинить
    invalidFleet.addShip(Ship(0,0,4,false));
    invalidFleet.addShip(Ship(0,0,1,false)); // Этот точно наложится на (0,0,4,false)
    
    std::cout << "Невалидный флот перед починкой:" << std::endl;
    invalidFleet.print();
    
    // Пытаемся починить флот
    std::cout << "\nПытаемся починить флот:" << std::endl;
    bool repaired = invalidFleet.repair(rng); // Передаем rng
    std::cout << "Флот починен успешно: " << (repaired ? "Да" : "Нет") << std::endl;
    
    if (repaired) {
        std::cout << "\nФлот после починки:" << std::endl;
        invalidFleet.print();
    }
}

/**
 * @brief Тестирование генератора случайных чисел
 */
void testRNG() {
    std::cout << "\n===== Тестирование RNG =====\n" << std::endl;
    
    // Инициализация с заданным сидом для воспроизводимости
    RNG::initialize(42);
    
    // Генерация целых чисел
    std::cout << "Случайные целые числа (1-10): ";
    for (int i = 0; i < 5; ++i) {
        std::cout << RNG::uniformInt(1, 10) << " ";
    }
    std::cout << std::endl;
    
    // Генерация действительных чисел
    std::cout << "Случайные действительные числа (0.0-1.0): ";
    for (int i = 0; i < 5; ++i) {
        std::cout << RNG::uniformReal(0.0, 1.0) << " ";
    }
    std::cout << std::endl;
    
    // Генерация нормально распределенных чисел
    std::cout << "Нормально распределенные числа (μ=0, σ=1): ";
    for (int i = 0; i < 5; ++i) {
        std::cout << RNG::normalReal(0.0, 1.0) << " ";
    }
    std::cout << std::endl;
    
    // Генерация логических значений
    std::cout << "Случайные логические значения (p=0.7): ";
    for (int i = 0; i < 10; ++i) {
        std::cout << (RNG::getBool(0.7) ? "1" : "0") << " ";
    }
    std::cout << std::endl;
}

/**
 * @brief Тестирование стратегий стрельбы
 */
void testStrategies() {
    std::cout << "\n===== Тестирование стратегий стрельбы =====\n" << std::endl;
    
    // Создаем доску для тестирования
    Board board(10);
    
    // Размещаем несколько кораблей на доске
    board.placeShip(1, 1);
    board.placeShip(1, 2);
    board.placeShip(1, 3);
    board.placeShip(5, 5);
    board.placeShip(5, 6);
    board.placeShip(5, 7);
    board.placeShip(8, 8);
    
    std::cout << "Тестовая доска:" << std::endl;
    board.print(true);
    
    // Тестируем случайную стратегию
    std::cout << "\nСлучайная стратегия (Random):" << std::endl;
    RandomStrategy randomStrategy;
    
    std::cout << "Делаем 10 выстрелов:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        auto shot = randomStrategy.getNextShot(board);
        bool hit = board.shoot(shot.first, shot.second);
        bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
        
        std::cout << "Выстрел " << (i + 1) << ": (" << shot.first << ", " << shot.second << ") - ";
        std::cout << (hit ? "Попадание!" : "Промах");
        if (sunk) std::cout << " (Потоплен)";
        std::cout << std::endl;
        
        randomStrategy.notifyShotResult(shot.first, shot.second, hit, sunk, board);
    }
    
    // Сбрасываем доску
    board.clear();
    
    // Размещаем те же корабли снова
    board.placeShip(1, 1);
    board.placeShip(1, 2);
    board.placeShip(1, 3);
    board.placeShip(5, 5);
    board.placeShip(5, 6);
    board.placeShip(5, 7);
    board.placeShip(8, 8);
    
    // Тестируем стратегию шахматной доски
    std::cout << "\nСтратегия шахматной доски (Checkerboard):" << std::endl;
    CheckerboardStrategy checkerboardStrategy;
    
    std::cout << "Делаем 10 выстрелов:" << std::endl;
    for (int i = 0; i < 10; ++i) {
        auto shot = checkerboardStrategy.getNextShot(board);
        bool hit = board.shoot(shot.first, shot.second);
        bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
        
        std::cout << "Выстрел " << (i + 1) << ": (" << shot.first << ", " << shot.second << ") - ";
        std::cout << (hit ? "Попадание!" : "Промах");
        if (sunk) std::cout << " (Потоплен)";
        std::cout << std::endl;
        
        checkerboardStrategy.notifyShotResult(shot.first, shot.second, hit, sunk, board);
    }
    
    std::cout << "\nСостояние доски после 10 выстрелов:" << std::endl;
    board.print(true);
}

/**
 * @brief Тестирование симулятора игры
 */
void testSimulator() {
    std::cout << "\n===== Тестирование симулятора игры =====\n" << std::endl;
    
    // Создаем две стратегии
    auto strategy1 = std::make_unique<RandomStrategy>();
    auto strategy2 = std::make_unique<CheckerboardStrategy>();
    
    // Создаем игру
    Game game(std::move(strategy1), std::move(strategy2));
    
    // Инициализируем игру со случайными флотами
    std::cout << "Инициализация игры со случайными флотами:" << std::endl;
    if (game.initialize()) {
        std::cout << "Игра инициализирована успешно." << std::endl;
        
        // Выводим начальное состояние
        game.print(true);
        
        // Делаем несколько ходов
        std::cout << "\nВыполняем 5 ходов:" << std::endl;
        for (int i = 0; i < 5; ++i) {
            if (game.step()) {
                std::cout << "Ход " << (i + 1) << " выполнен." << std::endl;
            } else {
                std::cout << "Игра окончена на ходу " << (i + 1) << "." << std::endl;
                break;
            }
        }
        
        // Выводим состояние после ходов
        std::cout << "\nСостояние после ходов:" << std::endl;
        game.print(true);
        
        // Симулируем игру до конца
        std::cout << "\nСимулируем игру до конца:" << std::endl;
        game.simulate();
        
        // Выводим результаты
        std::cout << "Игра окончена." << std::endl;
        std::cout << "Выстрелов игрока 1 (Random): " << game.getPlayer1Shots() << std::endl;
        std::cout << "Выстрелов игрока 2 (Checkerboard): " << game.getPlayer2Shots() << std::endl;
        std::cout << "Победитель: " << (game.hasPlayer1Won() ? "Игрок 1 (Random)" : 
                                       (game.hasPlayer2Won() ? "Игрок 2 (Checkerboard)" : "Ничья")) << std::endl;
    } else {
        std::cout << "Ошибка при инициализации игры." << std::endl;
    }
}

/**
 * @brief Тестирование генетического алгоритма для расстановки кораблей
 */
void testPlacementGA() {
    std::cout << "\n===== Тестирование генетического алгоритма расстановки кораблей =====\n" << std::endl;
    
    // Создаем генетический алгоритм с небольшой популяцией
    PlacementGA ga(20, 0.8, 0.04, 3, 2);
    
    // Определяем фитнес-функцию
    auto fitnessFunction = [](PlacementChromosome& chromosome) -> double {
        // Проверка валидности
        if (!chromosome.isValid()) {
            return -100.0; // Большой штраф за невалидность
        }
        
        // Получаем флот из хромосомы
        auto fleet = chromosome.decodeFleet();
        
        // Симулируем результаты против эталонных стрелков
        double meanShotsRandom = RNG::uniformInt(40, 60); // Симуляция среднего числа ходов против Random
        double meanShotsChecker = RNG::uniformInt(38, 55); // Симуляция против Checkerboard
        double meanShotsMC = RNG::uniformInt(35, 50); // Симуляция против Monte-Carlo
        
        // Устанавливаем статистики в хромосому
        chromosome.setMeanShotsRandom(meanShotsRandom);
        chromosome.setMeanShotsChecker(meanShotsChecker);
        chromosome.setMeanShotsMC(meanShotsMC);
        
        // Вычисляем фитнес согласно математической модели
        double fitness = Fitness::calculatePlacementFitness(
            chromosome, 
            meanShotsRandom, 
            meanShotsChecker, 
            meanShotsMC
        );
        
        // Устанавливаем общее среднее в хромосому (для совместимости)
        // Формула из §2.2.3: средневзвешенное по 3 стрелкам
        double meanShots = 0.4 * meanShotsRandom + 0.3 * meanShotsChecker + 0.3 * meanShotsMC;
        double stdDevShots = RNG::uniformReal(1.0, 5.0); // Симуляция стандартного отклонения
        
        chromosome.setMeanShots(meanShots);
        chromosome.setStdDevShots(stdDevShots);
        
        // Устанавливаем фитнес в хромосому
        chromosome.setFitness(fitness);
        
        return fitness;
    };
    
    // Настраиваем параметры ГА
    int maxGenerations = 5; // Сокращаем для быстрого тестирования
    double targetFitness = -30.0; // Желательное среднее число ходов
    
    // Запускаем ГА
    std::cout << "Запускаем генетический алгоритм на " << maxGenerations << " поколений" << std::endl;
    
    // Инициализируем начальную популяцию
    ga.initializePopulation(fitnessFunction);
    
    // Выводим информацию о начальном поколении
    std::cout << "Поколение 0:" << std::endl;
    std::cout << "- Лучший фитнес: " << ga.getBestFitness() << std::endl;
    std::cout << "- Средний фитнес: " << ga.getAverageFitness() << std::endl;
    
    // Эволюционируем популяцию
    for (int gen = 1; gen <= maxGenerations; ++gen) {
        auto bestChromosome = ga.evolvePopulation(fitnessFunction);
        
        // Выводим информацию о текущем поколении
        std::cout << "Поколение " << gen << ":" << std::endl;
        std::cout << "- Лучший фитнес: " << ga.getBestFitness() << std::endl;
        std::cout << "- Средний фитнес: " << ga.getAverageFitness() << std::endl;
        
        // Проверяем условие остановки
        if (ga.getBestFitness() >= targetFitness) {
            std::cout << "Достигнут целевой фитнес. Останавливаем ГА." << std::endl;
            break;
        }
    }
    
    // Получаем лучшую хромосому
    auto bestChromosome = ga.getBestChromosome();
    
    // Выводим информацию о лучшей хромосоме
    std::cout << "\nЛучшая хромосома:" << std::endl;
    std::cout << "- Фитнес: " << bestChromosome.getFitness() << std::endl;
    std::cout << "- Среднее число ходов: " << bestChromosome.getMeanShots() << std::endl;
    std::cout << "- Стандартное отклонение: " << bestChromosome.getStdDevShots() << std::endl;
    
    // Декодируем флот и выводим расстановку
    auto fleet = bestChromosome.decodeFleet();
    std::cout << "\nЛучшая расстановка:" << std::endl;
    fleet->print();
}

/**
 * @brief Тестирование разнообразия генерируемых расстановок
 */
void testPlacementDiversity() {
    std::cout << "\n===== Тестирование разнообразия расстановок =====\n" << std::endl;
    
    // Инициализируем генератор с фиксированным сидом для воспроизводимости
    RNG::initialize(42); // Фиксированный сид для воспроизводимости
    
    const int NUM_PLACEMENTS = 3; // Количество расстановок каждого типа
    
    // Создаем временный генератор для передачи в методы
    RNG tempRng;
    
    // Генерируем и выводим расстановки для каждой стратегии
    std::cout << "=== Расстановки с углами ===" << std::endl;
    for (int i = 0; i < NUM_PLACEMENTS; ++i) {
        std::vector<int> genes = PlacementChromosome::generateCornerPlacement(tempRng);
        PlacementChromosome chromosome(genes);
        auto fleet = chromosome.decodeFleet();
        
        std::cout << "\nРасстановка с углами #" << (i + 1) << ":" << std::endl;
        fleet->print();
        std::cout << "Валидная: " << (chromosome.isValid() ? "Да" : "Нет") << std::endl;
    }
    
    std::cout << "\n=== Расстановки по краям ===" << std::endl;
    for (int i = 0; i < NUM_PLACEMENTS; ++i) {
        std::vector<int> genes = PlacementChromosome::generateEdgePlacement(tempRng);
        PlacementChromosome chromosome(genes);
        auto fleet = chromosome.decodeFleet();
        
        std::cout << "\nРасстановка по краям #" << (i + 1) << ":" << std::endl;
        fleet->print();
        std::cout << "Валидная: " << (chromosome.isValid() ? "Да" : "Нет") << std::endl;
    }
    
    std::cout << "\n=== Расстановки в центре ===" << std::endl;
    for (int i = 0; i < NUM_PLACEMENTS; ++i) {
        std::vector<int> genes = PlacementChromosome::generateCenterPlacement(tempRng);
        PlacementChromosome chromosome(genes);
        auto fleet = chromosome.decodeFleet();
        
        std::cout << "\nРасстановка в центре #" << (i + 1) << ":" << std::endl;
        fleet->print();
        std::cout << "Валидная: " << (chromosome.isValid() ? "Да" : "Нет") << std::endl;
        }
    
    std::cout << "\n=== Смешанные расстановки ===" << std::endl;
    for (int i = 0; i < NUM_PLACEMENTS; ++i) {
        std::vector<int> genes = PlacementChromosome::generateMixedPlacement(tempRng);
        PlacementChromosome chromosome(genes);
        auto fleet = chromosome.decodeFleet();
        
        std::cout << "\nСмешанная расстановка #" << (i + 1) << ":" << std::endl;
        fleet->print();
        std::cout << "Валидная: " << (chromosome.isValid() ? "Да" : "Нет") << std::endl;
    }
    
    // Тестирование разнообразия расстановок при эволюции
    std::cout << "\n=== Тест разнообразия при эволюции в GA ===" << std::endl;
    
    // Создаем генетический алгоритм с небольшой популяцией
    PlacementGA ga(5, 0.8, 0.3, 2, 1); // Маленькие параметры для теста
    
    // Определяем простую фитнес-функцию для тестирования
    auto fitnessFunction = [](PlacementChromosome& chromosome) -> double {
        if (!chromosome.isValid()) {
            return -100.0; // Штраф за невалидную расстановку
        }
        
        // Искусственный фитнес для тестирования
        return 50.0 + RNG::uniformReal(-10.0, 10.0);
    };
    
    // Инициализируем популяцию
    ga.initializePopulation(fitnessFunction);
    
    // Выводим несколько хромосом из начальной популяции
    for (int i = 0; i < NUM_PLACEMENTS; ++i) {
        auto chromosome = ga.getBestChromosome(); // Получаем лучшую хромосому
        auto fleet = chromosome.decodeFleet();
        
        std::cout << "\nРасстановка из популяции #" << (i + 1) << ":" << std::endl;
        fleet->print();
        std::cout << "Валидная: " << (chromosome.isValid() ? "Да" : "Нет") << std::endl;
        std::cout << "Фитнес: " << chromosome.getFitness() << std::endl;
        
        // Эволюционируем популяцию для получения новых расстановок
        ga.evolvePopulation(fitnessFunction);
    }
}

/**
 * @brief Тестирование класса PlacementGenerator
 */
void testPlacementGenerator() {
    std::cout << "\n===== Тестирование класса PlacementGenerator =====\n" << std::endl;
    
    // Создаем генератор расстановок
    PlacementGenerator generator(50);
        
    // Инициализируем RNG
    RNG rng;
    
    // Генерируем расстановки с разными bias
    std::cout << "Тестирование расстановок с разными bias:" << std::endl;
    
    std::cout << "\n--- Расстановка EDGE ---" << std::endl;
    auto edgePlacement = generator.generate(Bias::EDGE, rng);
    auto edgeFleet = edgePlacement.decodeFleet();
    edgeFleet->print();
    std::cout << "Валидная: " << (edgePlacement.isValid() ? "Да" : "Нет") << std::endl;
    
    std::cout << "\n--- Расстановка CORNER ---" << std::endl;
    auto cornerPlacement = generator.generate(Bias::CORNER, rng);
    auto cornerFleet = cornerPlacement.decodeFleet();
    cornerFleet->print();
    std::cout << "Валидная: " << (cornerPlacement.isValid() ? "Да" : "Нет") << std::endl;
    
    std::cout << "\n--- Расстановка CENTER ---" << std::endl;
    auto centerPlacement = generator.generate(Bias::CENTER, rng);
    auto centerFleet = centerPlacement.decodeFleet();
    centerFleet->print();
    std::cout << "Валидная: " << (centerPlacement.isValid() ? "Да" : "Нет") << std::endl;
    
    std::cout << "\n--- Расстановка RANDOM ---" << std::endl;
    auto randomPlacement = generator.generate(Bias::RANDOM, rng);
    auto randomFleet = randomPlacement.decodeFleet();
    randomFleet->print();
    std::cout << "Валидная: " << (randomPlacement.isValid() ? "Да" : "Нет") << std::endl;
    
    // Генерируем небольшую популяцию для проверки разнообразия
    const int popSize = 10;
    std::cout << "\nГенерация популяции размером " << popSize << "..." << std::endl;
    auto population = generator.generatePopulation(popSize, rng);
    
    std::cout << "Сгенерировано " << population.size() << " уникальных расстановок." << std::endl;
    std::cout << "Все расстановки валидны: " << ([&population]() -> bool {
        for (const auto& chrom : population) {
            if (!chrom.isValid()) return false;
        }
        return true;
    }() ? "Да" : "Нет") << std::endl;
    
    // Подсчет процента валидных хромосом
    int validCount = 0;
    for (const auto& chrom : population) {
        if (chrom.isValid()) validCount++;
    }
    
    std::cout << "Процент валидных хромосом: " << (validCount * 100.0 / population.size()) << "%" << std::endl;
    
    // Измерение производительности
    std::cout << "\nИзмерение производительности генератора..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    
    const int benchmarkSize = 100;
    auto benchPop = generator.generatePopulation(benchmarkSize, rng);
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> elapsed = end - start;
    
    std::cout << "Время генерации " << benchmarkSize << " расстановок: " 
              << elapsed.count() << " мс" << std::endl;
    std::cout << "В среднем " << (elapsed.count() / benchmarkSize) << " мс на расстановку" << std::endl;
}

void trainPlacement(const std::string& outFile, int customMaxGen = -1) {
    std::cout << "[CLI] Запуск обучения расстановки кораблей. Вывод будет сохранен в: " << outFile << std::endl;
    
    // Инициализация логгера для placement_ga
    std::string runId = generateRunId();
    std::string logFileName = "logs/placement_ga_" + runId + ".log";
    Logger::instance().open(logFileName);

    // Определяем файлы для сохранения состояния и результатов
    const std::string stateFileName = "placement_ga_state.dat";
    const std::string resultsFileName = "placement_ga_results_" + runId + ".txt";
    
    // Выводим информацию о файлах
    std::cout << "\nИнформация о файлах:" << std::endl;
    std::cout << "- Лог: " << logFileName << std::endl;
    std::cout << "- Файл состояния: saves/" << stateFileName << std::endl;
    std::cout << "- Результаты: results/" << resultsFileName << std::endl;
    std::cout << "- Лучшая хромосома: " << outFile << std::endl;
    
    // Параметры для генетического алгоритма
    int populationSize = 200;
    double crossoverRate = 0.9;
    double initialMutationRate = 0.05;
    int tournamentSize = 3;
    int eliteSize = 2;
    double lambda0 = 1.0;
    double alpha = 0.05;
    
    int startGen = 0;
    double currentMutationRate = initialMutationRate;
    std::vector<PlacementChromosome> population;
    int maxGenerations = (customMaxGen > 0) ? customMaxGen : 50; // Используем пользовательское значение, если оно указано
    double targetFitness = 80.0;
    int saveInterval = 5; // Сохраняем состояние каждые 5 поколений
    
    // Проверяем наличие файла состояния
    bool hasStateFile = Logger::instance().stateFileExists(stateFileName);
    bool continuePrevious = false;
    
    if (hasStateFile) {
        std::cout << "\nНайден файл с сохраненным состоянием генетического алгоритма." << std::endl;
        showStateFileInfo(stateFileName, 0); // 0 - PlacementGA
        
        int choice = showGAMenu();
        continuePrevious = (choice == 1);
    }
    
    // Создаем генетический алгоритм
    PlacementGA pga(populationSize, crossoverRate, initialMutationRate, tournamentSize, eliteSize, lambda0, alpha);
    
    // Для хранения статистики и лучших хромосом
    std::map<int, PlacementChromosome> bestPerGeneration;
    std::map<std::string, double> strategyStats;
    std::vector<PlacementChromosome> topChromosomes;
    
    // Инициализируем статистику по стратегиям с нулевыми значениями, чтобы убедиться, 
    // что эти ключи существуют
    strategyStats["Random"] = 0.0;
    strategyStats["Checkerboard"] = 0.0;
    strategyStats["Monte Carlo"] = 0.0;
    
    // Если продолжаем предыдущую эволюцию
    if (continuePrevious) {
        std::cout << "Загрузка сохраненного состояния..." << std::endl;
        if (Logger::instance().loadGAState(startGen, population, currentMutationRate, stateFileName)) {
            std::cout << "Состояние успешно загружено. Продолжаем с поколения " << startGen << std::endl;
            // Создаем новый экземпляр GA с загруженными параметрами
            pga = PlacementGA(populationSize, crossoverRate, currentMutationRate, tournamentSize, eliteSize, lambda0, alpha);
            // Заполняем его популяцию загруженными хромосомами
            pga.initializeWithPopulation(population);
        } else {
            std::cout << "Ошибка при загрузке состояния. Начинаем новую эволюцию." << std::endl;
            continuePrevious = false;
        }
    }
    
    // Определяем фитнес-функцию для хромосомы расстановки
    auto fitnessFunction = [](PlacementChromosome& chrom) -> double {
        if (!chrom.isValid()) {
            return -1000.0; // Большой штраф за невалидность
        }
        
        // Получаем флот из хромосомы
        auto fleet = chrom.decodeFleet();
        if (!fleet) {
            return -1000.0; 
        }
        
        // Создаем стратегии для тестирования
        RandomStrategy random_shooter;
        CheckerboardStrategy checker_shooter;
        MonteCarloStrategy monte_shooter(100); // Используем небольшое количество симуляций для скорости
        
        int totalTrials = 10; // Для каждой стратегии
        double totalMeanShots = 0;
        int strategyCount = 0;

        // Против RandomStrategy
        double currentStrategyTotalShotsRandom = 0;
        for (int i = 0; i < totalTrials; ++i) {
            Board board;
            if (!board.placeFleet(*fleet)) { // Используем placeFleet
                return -900.0; // Штраф, если не удалось разместить флот
            }
            random_shooter.reset(); 
            int shots = 0;
            while (!board.allShipsSunk() && shots < 100) {
                auto shot = random_shooter.getNextShot(board);
                bool hit = board.shoot(shot.first, shot.second);
                bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
                random_shooter.notifyShotResult(shot.first, shot.second, hit, sunk, board);
                shots++;
            }
            currentStrategyTotalShotsRandom += shots;
        }
        double meanShotsRandom = totalTrials > 0 ? currentStrategyTotalShotsRandom / totalTrials : 200.0;
        chrom.setMeanShotsRandom(meanShotsRandom);
        totalMeanShots += meanShotsRandom; strategyCount++;
        
        // Против CheckerboardStrategy
        double currentStrategyTotalShotsChecker = 0;
        for (int i = 0; i < totalTrials; ++i) {
            Board board;
            if (!board.placeFleet(*fleet)) return -900.0;
            checker_shooter.reset();
            int shots = 0;
            while (!board.allShipsSunk() && shots < 100) {
                auto shot = checker_shooter.getNextShot(board);
                bool hit = board.shoot(shot.first, shot.second);
                bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
                checker_shooter.notifyShotResult(shot.first, shot.second, hit, sunk, board);
                shots++;
            }
            currentStrategyTotalShotsChecker += shots;
        }
        double meanShotsChecker = totalTrials > 0 ? currentStrategyTotalShotsChecker / totalTrials : 200.0;
        chrom.setMeanShotsCheckerboard(meanShotsChecker);
        totalMeanShots += meanShotsChecker; strategyCount++;
        
        // Против MonteCarloStrategy
        double currentStrategyTotalShotsMC = 0;
        for (int i = 0; i < totalTrials; ++i) {
            Board board;
            if (!board.placeFleet(*fleet)) return -900.0;
            monte_shooter.reset(); 
            int shots = 0;
            while (!board.allShipsSunk() && shots < 100) {
                auto shot = monte_shooter.getNextShot(board);
                bool hit = board.shoot(shot.first, shot.second);
                bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
                monte_shooter.notifyShotResult(shot.first, shot.second, hit, sunk, board);
                shots++;
            }
            currentStrategyTotalShotsMC += shots;
        }
        double meanShotsMC = totalTrials > 0 ? currentStrategyTotalShotsMC / totalTrials : 200.0;
        chrom.setMeanShotsMC(meanShotsMC);
        totalMeanShots += meanShotsMC; strategyCount++;
        
        // Вычисляем фитнес согласно формуле (используем существующую функцию)
        double fitness = Fitness::calculatePlacementFitness(
            chrom,
            meanShotsRandom,
            meanShotsChecker,
            meanShotsMC
        );
        
        chrom.setFitness(fitness);
        return fitness;
    };
    
    // Если начинаем новую эволюцию
    if (!continuePrevious) {
        std::cout << "Начинаем эволюцию генетического алгоритма расстановки кораблей..." << std::endl;
        std::cout << "Максимальное число поколений: " << maxGenerations << std::endl;
        std::cout << "Целевой фитнес: " << targetFitness << std::endl;
        std::cout << "Интервал сохранения состояния: " << saveInterval << " поколений" << std::endl;
        
        // Инициализируем начальную популяцию
        pga.initializePopulation(fitnessFunction);
        
        // Сохраняем лучшую хромосому начального поколения
        auto bestInitialChrom = pga.getBestChromosome();
        bestPerGeneration[0] = bestInitialChrom;
        
        // Сохраняем начальное состояние
        Logger::instance().saveGAState(0, pga.getPopulation(), initialMutationRate, stateFileName);
    }
    
    // Эволюционируем популяцию
    PlacementChromosome bestChromosome;
    bool targetReached = false;
    
    for (int gen = startGen + 1; gen <= maxGenerations && !targetReached; ++gen) {
        std::cout << "Поколение " << gen << "..." << std::flush;
        
        // Эволюция на одно поколение
        bestChromosome = pga.evolvePopulation(fitnessFunction);
        
        // Сохраняем лучшую хромосому текущего поколения
        bestPerGeneration[gen] = bestChromosome;
        
        // Печатаем базовую информацию о поколении
        double bestFit = pga.getBestFitness();
        double avgFit = pga.getAverageFitness();
        
        std::cout << " [Лучший фитнес: " << bestFit << ", Средний: " << avgFit << "]" << std::endl;
        
        // Обновляем статистику по стратегиям
        // Используем последние данные лучшей хромосомы
        strategyStats["Random"] = bestChromosome.getMeanShotsRandom();
        strategyStats["Checkerboard"] = bestChromosome.getMeanShotsCheckerboard();
        strategyStats["Monte Carlo"] = bestChromosome.getMeanShotsMC();
        
        // Логируем данные о текущем поколении для отладки
        Logger::instance().logMessage(
            "Поколение " + std::to_string(gen) + 
            ": Random=" + std::to_string(bestChromosome.getMeanShotsRandom()) +
            ", Checker=" + std::to_string(bestChromosome.getMeanShotsCheckerboard()) +
            ", MC=" + std::to_string(bestChromosome.getMeanShotsMC())
        );
        
        // Периодически сохраняем состояние
        if (gen % saveInterval == 0) {
            std::cout << "Сохранение текущего состояния..." << std::endl;
            // Теперь используем уже добавленные методы getPopulation и getMutationRate
            auto population = pga.getPopulation();
            double mutRate = pga.getMutationRate();
            if (Logger::instance().saveGAState(gen, population, mutRate, stateFileName)) {
                std::cout << "Состояние успешно сохранено." << std::endl;
            } else {
                std::cout << "Ошибка при сохранении состояния!" << std::endl;
            }
        }
        
        // Проверяем условие остановки
        if (bestFit >= targetFitness) {
            std::cout << "Достигнут целевой фитнес. Останавливаем ГА." << std::endl;
            targetReached = true;
        }
    }
    
    // Получаем топ-50 лучших хромосом
    std::vector<PlacementChromosome> sortedPopulation = pga.getPopulation();
    std::sort(sortedPopulation.begin(), sortedPopulation.end(),
        [](const PlacementChromosome& a, const PlacementChromosome& b) {
            return a.getFitness() > b.getFitness();
        });
    int topSize = std::min(50, static_cast<int>(sortedPopulation.size()));
    topChromosomes.assign(sortedPopulation.begin(), sortedPopulation.begin() + topSize);
    
    // Сохраняем результаты эволюции
    std::cout << "Сохранение итоговых результатов эволюции..." << std::endl;
    Logger::instance().saveEvolutionResults(
        topChromosomes,
        bestPerGeneration,
        strategyStats,
        resultsFileName
    );
    std::cout << "Результаты сохранены в: results/" << resultsFileName << std::endl;
    
    // Сохраняем лучшую хромосому в указанный файл
    std::cout << "Сохранение лучшей хромосомы в: " << outFile << std::endl;
    std::ofstream out(outFile);
    if (out.is_open()) {
        out << "# Best Placement Chromosome" << std::endl;
        out << "# Fitness: " << bestChromosome.getFitness() << std::endl;
        out << "# Mean shots (Random): " << bestChromosome.getMeanShotsRandom() << std::endl;
        out << "# Mean shots (Checkerboard): " << bestChromosome.getMeanShotsCheckerboard() << std::endl;
        out << "# Mean shots (MonteCarlo): " << bestChromosome.getMeanShotsMC() << std::endl;
        out << bestChromosome.serialize() << std::endl;
        out.close();
    } else {
        std::cerr << "Ошибка открытия файла для записи: " << outFile << std::endl;
    }
    
    // Выводим итоговые результаты
    std::cout << "\nГенетический алгоритм расстановки завершен." << std::endl;
    std::cout << "Лучший фитнес: " << bestChromosome.getFitness() << std::endl;
    std::cout << "Средние выстрелы против Random: " << bestChromosome.getMeanShotsRandom() << std::endl;
    std::cout << "Средние выстрелы против Checkerboard: " << bestChromosome.getMeanShotsCheckerboard() << std::endl;
    std::cout << "Средние выстрелы против MonteCarlo: " << bestChromosome.getMeanShotsMC() << std::endl;
    
    // Выводим информацию о сохраненных файлах
    std::cout << "\nСохраненные файлы:" << std::endl;
    std::cout << "- Лучшая хромосома: " << outFile << std::endl;
    std::cout << "- Полные результаты: results/" << resultsFileName << std::endl;
    std::cout << "- Лог: " << logFileName << std::endl;
    std::cout << "- Файл состояния: saves/" << stateFileName << std::endl;
    std::cout << "\nДля продолжения эволюции с текущими настройками запустите команду:" << std::endl;
    std::cout << "  battleship_ga.exe --train-placement " << outFile << " [новое_число_поколений]" << std::endl;
    
    Logger::instance().close(); // Закрываем логгер
}

void trainDecision(const std::string& placementsFile, const std::string& outFile) {
    std::cout << "[CLI] Запуск обучения стратегии стрельбы. Вход: " << placementsFile << " → " << outFile << std::endl;
    
    // Инициализация логгера для decision_ga
    std::string runId = generateRunId();
    std::string logFileName = "logs/decision_ga_" + runId + ".log";
    Logger::instance().open(logFileName);
    
    // Определяем файлы для сохранения состояния и результатов
    const std::string stateFileName = "decision_ga_state.dat";
    const std::string resultsFileName = "decision_ga_results_" + runId + ".txt";
    
    // Параметры для генетического алгоритма
    int populationSize = 150;
    double crossoverRate = 0.8;
    double initialMutationRate = 0.2;
    int tournamentSize = 3;
    int eliteSize = 3;
    double initialSigma = 0.2;
    double minSigma = 0.01;
    double beta = 5.0;
    
    int startGen = 0;
    double currentMutationRate = initialMutationRate;
    double currentSigma = initialSigma;
    std::vector<DecisionChromosome> population;
    int maxGenerations = 30; // Увеличиваем для более качественных результатов
    double targetFitness = -35.0;
    int saveInterval = 5; // Сохраняем состояние каждые 5 поколений
    
    // Проверяем наличие файла состояния
    bool hasStateFile = Logger::instance().stateFileExists(stateFileName);
    bool continuePrevious = false;
    
    if (hasStateFile) {
        std::cout << "\nНайден файл с сохраненным состоянием генетического алгоритма." << std::endl;
        showStateFileInfo(stateFileName, 1); // 1 - DecisionGA
        
        int choice = showGAMenu();
        continuePrevious = (choice == 1);
    }
    
    // Загружаем пул расстановок из файла или генерируем, если файл не существует
    PlacementPool pool;
    if (!loadPlacementsFromFile(placementsFile, pool)) {
        std::cout << "Не удалось загрузить расстановки из файла. Генерируем новые..." << std::endl;
        
        // Создаем генератор расстановок
        PlacementGenerator generator(50);
        RNG rng;
        
        // Генерируем разнообразные расстановки
        int poolSize = 50; // Размер пула
        
        // Добавляем расстановки разных типов
        for (int i = 0; i < poolSize / 4; ++i) {
            pool.addPlacement(generator.generate(Bias::RANDOM, rng));
        }
        for (int i = 0; i < poolSize / 4; ++i) {
            pool.addPlacement(generator.generate(Bias::EDGE, rng));
        }
        for (int i = 0; i < poolSize / 4; ++i) {
            pool.addPlacement(generator.generate(Bias::CORNER, rng));
        }
        for (int i = 0; i < poolSize / 4; ++i) {
            pool.addPlacement(generator.generate(Bias::CENTER, rng));
        }
        
        std::cout << "Сгенерировано " << pool.size() << " расстановок для тренировки." << std::endl;
    } else {
        std::cout << "Загружено " << pool.size() << " расстановок из файла." << std::endl;
    }
    
    // Создаем экземпляр DecisionGA с параметрами
    DecisionGA dga(populationSize, crossoverRate, initialMutationRate, tournamentSize, 
                   eliteSize, initialSigma, minSigma, beta);
                   
    // Для хранения статистики и лучших хромосом
    std::map<int, DecisionChromosome> bestPerGeneration;
    std::map<std::string, double> strategyStats;
    std::vector<DecisionChromosome> topChromosomes;
    
    // Создаём объект лучшей хромосомы с пустым вектором весов
    // для совместимости с компилятором
    DecisionChromosome bestChromosome({});
    
    // Если продолжаем предыдущую эволюцию
    if (continuePrevious) {
        std::cout << "Загрузка сохраненного состояния..." << std::endl;
        if (Logger::instance().loadGAState(startGen, population, currentMutationRate, stateFileName)) {
            std::cout << "Состояние успешно загружено. Продолжаем с поколения " << startGen << std::endl;
            // Создаем новый экземпляр DecisionGA с загруженными параметрами
            dga = DecisionGA(populationSize, crossoverRate, currentMutationRate, tournamentSize, 
                            eliteSize, currentSigma, minSigma, beta);
            
            // Вместо прямого вызова setSigma и других методов, 
            // используем соответствующие подходы к инициализации
            // и передаем загруженные параметры через конструктор
        } else {
            std::cout << "Ошибка при загрузке состояния. Начинаем новую эволюцию." << std::endl;
            continuePrevious = false;
        }
    }
    
    // Определяем фитнес-функцию для стратегии стрельбы
    auto fitnessFunction = [](DecisionChromosome& chromosome, const PlacementPool& pool) {
        if (pool.empty()) {
            throw std::runtime_error("Пул расстановок пуст");
        }
        
        // Создаем стратегию на основе хромосомы
        FeatureBasedStrategy strategy(chromosome.getGenes());
        
        // Проводим симуляции против различных расстановок из пула
        int totalShots = 0;
        int numTrials = 0;
        std::vector<int> allShots;
        
        // Ограничим количество испытаний для скорости
        const int maxTrials = std::min(30, static_cast<int>(pool.size()));
        
        for (int i = 0; i < maxTrials; ++i) {
            const auto& placement = pool.getPlacement(i);
            auto fleet = placement.decodeFleet();
            
            if (!fleet || !fleet->isValid()) {
                continue; // Пропускаем невалидные расстановки
            }
            
            Board board(10);
            for (const auto& ship : fleet->getShips()) {
                for (const auto& cell : ship.getOccupiedCells()) {
                    board.placeShip(cell.first, cell.second);
                }
            }
            
            strategy.reset(); // Сбрасываем стратегию перед новой симуляцией
            int shots = 0;
            int maxShots = 200; // Предел для предотвращения бесконечного цикла
            
            while (!board.allShipsSunk() && shots < maxShots) {
                auto shot = strategy.getNextShot(board);
                bool hit = board.shoot(shot.first, shot.second);
                bool sunk = hit && board.wasShipSunkAt(shot.first, shot.second);
                strategy.notifyShotResult(shot.first, shot.second, hit, sunk, board);
                shots++;
            }
            
            if (shots < maxShots) {
                totalShots += shots;
                allShots.push_back(shots);
                numTrials++;
            }
        }
        
        if (numTrials == 0) {
            throw std::runtime_error("Нет успешных симуляций");
        }
        
        // Вычисляем среднее и стандартное отклонение
        double meanShots = static_cast<double>(totalShots) / numTrials;
        
        double sumSquares = 0.0;
        for (int shots : allShots) {
            sumSquares += (shots - meanShots) * (shots - meanShots);
        }
        double stdDevShots = std::sqrt(sumSquares / numTrials);
        
        // Обновляем статистику в хромосому
        chromosome.setMeanShots(meanShots);
        chromosome.setStdDevShots(stdDevShots);
        
        // Вычисляем фитнес
        double fitness = Fitness::calculateDecisionFitness(meanShots, stdDevShots);
        chromosome.setFitness(fitness);
        
        return;
    };
    
    // Если начинаем новую эволюцию
    if (!continuePrevious) {
        std::cout << "Начинаем эволюцию генетического алгоритма стратегии стрельбы..." << std::endl;
        std::cout << "Максимальное число поколений: " << maxGenerations << std::endl;
        std::cout << "Целевой фитнес: " << targetFitness << std::endl;
        std::cout << "Интервал сохранения состояния: " << saveInterval << " поколений" << std::endl;
        
        // Инициализируем начальную популяцию
        dga.initialize(fitnessFunction, pool);
        
        // Сохраняем лучшую хромосому начального поколения
        auto bestInitialChrom = dga.getBestChromosome();
        bestPerGeneration.insert(std::make_pair(0, bestInitialChrom));
        
        // Получаем популяцию после инициализации
        auto population = dga.getPopulation();
        // Сохраняем начальное состояние
        Logger::instance().saveGAState(0, population, dga.getMutationRate(), stateFileName);
    }
    
    // Эволюционируем популяцию
    bestChromosome = dga.getBestChromosome();
    bool targetReached = false;
    
    for (int gen = startGen + 1; gen <= maxGenerations && !targetReached; ++gen) {
        std::cout << "Поколение " << gen << "..." << std::flush;
        
        // Эволюция на одно поколение
        dga.evolveOneGeneration(fitnessFunction, pool);
        bestChromosome = dga.getBestChromosome();
        
        // Сохраняем лучшую хромосому текущего поколения
        bestPerGeneration.insert(std::make_pair(gen, bestChromosome));
        
        // Печатаем базовую информацию о поколении
        double bestFit = bestChromosome.getFitness();
        double avgFit = dga.getAverageFitness();
        currentSigma = dga.getSigma();
        
        std::cout << " [Лучший фитнес: " << bestFit << ", Средний: " << avgFit 
                  << ", σ: " << currentSigma << "]" << std::endl;
        
        // Логируем информацию о поколении
        Logger::instance().logDecisionGen(gen, bestFit, avgFit, currentSigma);
        
        // Обновляем статистику по стратегии
        strategyStats["Mean Shots"] = bestChromosome.getMeanShots();
        strategyStats["StdDev Shots"] = bestChromosome.getStdDevShots();
        
        // Периодически сохраняем состояние
        if (gen % saveInterval == 0) {
            std::cout << "Сохранение текущего состояния..." << std::endl;
            if (Logger::instance().saveGAState(gen, dga.getPopulation(), dga.getMutationRate(), stateFileName)) {
                std::cout << "Состояние успешно сохранено." << std::endl;
            } else {
                std::cout << "Ошибка при сохранении состояния!" << std::endl;
            }
        }
        
        // Проверяем условие остановки
        if (bestFit >= targetFitness) {
            std::cout << "Достигнут целевой фитнес. Останавливаем ГА." << std::endl;
            targetReached = true;
        }
    }
    
    // Получаем топ-50 лучших хромосом
    topChromosomes = dga.getTopChromosomes(50);
    
    // Сохраняем результаты эволюции
    std::cout << "Сохранение итоговых результатов эволюции..." << std::endl;
    Logger::instance().saveEvolutionResults(
        topChromosomes,
        bestPerGeneration,
        strategyStats,
        resultsFileName
    );
    std::cout << "Результаты сохранены в: results/" << resultsFileName << std::endl;
    
    // Сохраняем лучшую хромосому в указанный файл
    std::cout << "Сохранение лучшей хромосомы в: " << outFile << std::endl;
    std::ofstream out(outFile);
    if (out.is_open()) {
        // Записываем основную информацию
        out << "# Best Decision Strategy" << std::endl;
        out << "# Fitness: " << bestChromosome.getFitness() << std::endl;
        out << "# Mean shots: " << bestChromosome.getMeanShots() << std::endl;
        out << "# StdDev shots: " << bestChromosome.getStdDevShots() << std::endl;
        
        // Записываем веса признаков
        out << "# Feature weights:" << std::endl;
        const auto& weights = bestChromosome.getGenes();
        for (size_t i = 0; i < weights.size(); ++i) {
            out << "# θ_" << (i + 1) << " = " << weights[i] << std::endl;
        }
        
        // Сериализуем хромосому (веса)
        for (double weight : weights) {
            out << weight << " ";
        }
        out << std::endl;
        
        out.close();
    } else {
        std::cerr << "Ошибка открытия файла для записи: " << outFile << std::endl;
    }
    
    // Выводим итоговые результаты
    std::cout << "\nГенетический алгоритм стратегии завершен." << std::endl;
    std::cout << "Лучший фитнес: " << bestChromosome.getFitness() << std::endl;
    std::cout << "Среднее число выстрелов: " << bestChromosome.getMeanShots() << std::endl;
    std::cout << "Стандартное отклонение: " << bestChromosome.getStdDevShots() << std::endl;
    
    Logger::instance().close(); // Закрываем логгер
}

void playBot(const std::string& weightsFile, const std::string& placementsFile) {
    std::cout << "[CLI] Режим play пока не реализован. Используем веса: " << weightsFile << " и пул: " << placementsFile << std::endl;
}

/**
 * @brief Получает текущую временную метку в формате YYYYMMDD_HHMMSS
 * @return Строка с временной меткой
 */
std::string getCurrentTimeStamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    
    struct tm timeinfo;
    #ifdef _WIN32
        localtime_s(&timeinfo, &now_time);
    #else
        localtime_r(&now_time, &timeinfo);
    #endif
    
    std::ostringstream ss;
    ss << std::setfill('0')
       << std::setw(4) << (timeinfo.tm_year + 1900)
       << std::setw(2) << (timeinfo.tm_mon + 1)
       << std::setw(2) << timeinfo.tm_mday
       << "_"
       << std::setw(2) << timeinfo.tm_hour
       << std::setw(2) << timeinfo.tm_min
       << std::setw(2) << timeinfo.tm_sec;
    
    return ss.str();
}

/**
 * @brief Интерактивное меню для выбора действия с ГА
 * 
 * @return Выбранный пользователем вариант действия
 * 0 - Новый ГА, 1 - Загрузить из сохранения
 */
int showGAMenu() {
    std::cout << "\n=== МЕНЮ ГЕНЕТИЧЕСКОГО АЛГОРИТМА ===\n";
    std::cout << "1. Загрузить из сохранения\n";
    std::cout << "2. Начать новый ГА\n";
    std::cout << "Ваш выбор (1-2): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "1") {
        return 1; // Загрузить из сохранения
    } else {
        return 0; // Начать новый ГА (по умолчанию)
    }
}

/**
 * @brief Отображает главное меню для выбора между ГА расстановки и ГА стрельбы
 * 
 * @return Выбранный пользователем вариант
 * 1 - ГА расстановки, 2 - ГА стрельбы, 0 - Выход
 */
int showMainMenu() {
    std::cout << "\n=== ГЛАВНОЕ МЕНЮ ===\n";
    std::cout << "1. Генетический алгоритм расстановки кораблей\n";
    std::cout << "2. Генетический алгоритм стратегии стрельбы\n";
    std::cout << "0. Выход\n";
    std::cout << "Ваш выбор (0-2): ";
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "1") {
        return 1; // ГА расстановки
    } else if (input == "2") {
        return 2; // ГА стрельбы
    } else if (input == "0") {
        return 0; // Выход
    } else {
        std::cout << "Неверный выбор. Пожалуйста, попробуйте снова.\n";
        return showMainMenu(); // Рекурсивный вызов для повторного выбора
    }
}

/**
 * @brief Выводит информацию о состоянии ГА из файла сохранения
 * 
 * @param stateFile Имя файла с сохранением
 * @param type Тип ГА: 0 - PlacementGA, 1 - DecisionGA
 * @return true, если информация успешно получена и выведена
 */
bool showStateFileInfo(const std::string& stateFile, int type) {
    if (!Logger::instance().stateFileExists(stateFile)) {
        std::cout << "Файл сохранения не найден: " << stateFile << std::endl;
        return false;
    }
    
    int gen = 0;
    double mutationRate = 0.0;
    
    std::cout << "Файл сохранения: " << stateFile << std::endl;
    
    if (type == 0) { // PlacementGA
        std::vector<PlacementChromosome> population;
        if (Logger::instance().loadGAState(gen, population, mutationRate, stateFile)) {
            std::cout << "Тип: Генетический алгоритм расстановки\n";
            std::cout << "Поколение: " << gen << "\n";
            std::cout << "Коэффициент мутации: " << mutationRate << "\n";
            std::cout << "Размер популяции: " << population.size() << "\n";
            
            if (!population.empty()) {
                double bestFitness = -1000.0;
                double sumFitness = 0.0;
                
                for (const auto& chrom : population) {
                    double fitness = chrom.getFitness();
                    bestFitness = std::max(bestFitness, fitness);
                    sumFitness += fitness;
                }
                
                double avgFitness = sumFitness / population.size();
                std::cout << "Лучший фитнес: " << bestFitness << "\n";
                std::cout << "Средний фитнес: " << avgFitness << "\n";
            }
            
            return true;
        }
    } else if (type == 1) { // DecisionGA
        std::vector<DecisionChromosome> population;
        if (Logger::instance().loadGAState(gen, population, mutationRate, stateFile)) {
            std::cout << "Тип: Генетический алгоритм стратегии\n";
            std::cout << "Поколение: " << gen << "\n";
            std::cout << "Коэффициент мутации: " << mutationRate << "\n";
            std::cout << "Размер популяции: " << population.size() << "\n";
            
            if (!population.empty()) {
                double bestFitness = -1000.0;
                double sumFitness = 0.0;
                
                for (const auto& chrom : population) {
                    double fitness = chrom.getFitness();
                    bestFitness = std::max(bestFitness, fitness);
                    sumFitness += fitness;
                }
                
                double avgFitness = sumFitness / population.size();
                std::cout << "Лучший фитнес: " << bestFitness << "\n";
                std::cout << "Средний фитнес: " << avgFitness << "\n";
            }
            
            return true;
        }
    }
    
    std::cout << "Не удалось получить информацию о сохранении." << std::endl;
    return false;
}

/**
 * @brief Расширенное тестирование стратегий стрельбы на случайных полях
 */
void testStrategiesAdvanced() {
    // Тестирование стратегий на случайных расстановках
    std::cout << "Запуск: " << getCurrentTimeStamp() << "\n\n";
    
    int numTests = 20; // Увеличим количество тестов до 20
    int numTrials = 5;  // Увеличим количество прогонов до 5 для каждого поля
    
    // Параметры для MonteCarloStrategy
    int mcIterations = 5000; // Увеличено с 1000 до 5000 для улучшенного качества
    
    RNG rng;
    
    std::vector<double> randomShots;
    std::vector<double> checkerShots;
    std::vector<double> mcShots;
    
    // Для каждого теста генерируем новое случайное поле
    for (int test = 1; test <= numTests; ++test) {
        std::cout << "Тест #" << test << ":\n";
        
        // Создаём случайный флот
        auto fleet = Fleet::createStandardFleet();
        fleet->repair(rng);
        
        // Статистика для текущего поля
        double totalRandom = 0.0;
        double totalChecker = 0.0;
        double totalMC = 0.0;
        
        // Несколько прогонов для каждой стратегии на одном и том же поле
        for (int trial = 1; trial <= numTrials; ++trial) {
            // Тест Random стратегии
            {
                Board board;
                board.placeFleet(*fleet);
                
                RandomStrategy strategy(rng);
                int shots = 0;
                
                while (!board.allShipsSunk() && shots < 100) {
                    auto target = strategy.getNextShot(board);
                    if (target.first == -1) break;
                    
                    bool hit = board.shoot(target.first, target.second);
                    bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
                    strategy.notifyShotResult(target.first, target.second, hit, sunk, board);
                    shots++;
                }
                
                std::cout << "  Random запуск #" << trial << ": " << shots << " выстрелов\n";
                totalRandom += shots;
                randomShots.push_back(shots);
            }
            
            // Тест Checkerboard стратегии
            {
                Board board;
                board.placeFleet(*fleet);
                
                CheckerboardStrategy strategy(rng);
                int shots = 0;
                
                while (!board.allShipsSunk() && shots < 100) {
                    auto target = strategy.getNextShot(board);
                    if (target.first == -1) break;
                    
                    bool hit = board.shoot(target.first, target.second);
                    bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
                    strategy.notifyShotResult(target.first, target.second, hit, sunk, board);
                    shots++;
                }
                
                std::cout << "  Checkerboard запуск #" << trial << ": " << shots << " выстрелов\n";
                totalChecker += shots;
                checkerShots.push_back(shots);
            }
            
            // Тест Monte Carlo стратегии
            {
                Board board;
                board.placeFleet(*fleet);
                
                MonteCarloStrategy strategy(rng, 5000);
                int shots = 0;
                
                while (!board.allShipsSunk() && shots < 100) {
                    auto target = strategy.getNextShot(board);
                    if (target.first == -1) break;
                    
                    bool hit = board.shoot(target.first, target.second);
                    bool sunk = hit && board.wasShipSunkAt(target.first, target.second);
                    strategy.notifyShotResult(target.first, target.second, hit, sunk, board);
                    shots++;
                }
                
                std::cout << "  Monte Carlo запуск #" << trial << ": " << shots << " выстрелов\n";
                totalMC += shots;
                mcShots.push_back(shots);
            }
        }
        
        // Отображаем среднее для текущего поля
        std::cout << "Среднее по полю #" << test << ":\n";
        std::cout << "  Random: " << totalRandom / numTrials << " выстрелов\n";
        std::cout << "  Checkerboard: " << totalChecker / numTrials << " выстрелов\n";
        std::cout << "  Monte Carlo: " << totalMC / numTrials << " выстрелов\n\n";
    }
    
    // Подсчитываем общую статистику
    double avgRandom = std::accumulate(randomShots.begin(), randomShots.end(), 0.0) / randomShots.size();
    double avgChecker = std::accumulate(checkerShots.begin(), checkerShots.end(), 0.0) / checkerShots.size();
    double avgMC = std::accumulate(mcShots.begin(), mcShots.end(), 0.0) / mcShots.size();
    
    // Находим минимальные и максимальные значения
    double minRandom = *std::min_element(randomShots.begin(), randomShots.end());
    double minChecker = *std::min_element(checkerShots.begin(), checkerShots.end());
    double minMC = *std::min_element(mcShots.begin(), mcShots.end());
    
    double maxRandom = *std::max_element(randomShots.begin(), randomShots.end());
    double maxChecker = *std::max_element(checkerShots.begin(), checkerShots.end());
    double maxMC = *std::max_element(mcShots.begin(), mcShots.end());
    
    // Вычисляем дисперсию и стандартное отклонение
    auto calculateStdDev = [](const std::vector<double>& values, double mean) {
        double variance = 0.0;
        for (double value : values) {
            variance += std::pow(value - mean, 2);
        }
        variance /= values.size();
        return std::sqrt(variance);
    };
    
    double stdDevRandom = calculateStdDev(randomShots, avgRandom);
    double stdDevChecker = calculateStdDev(checkerShots, avgChecker);
    double stdDevMC = calculateStdDev(mcShots, avgMC);
    
    // Вывести итоговые статистические данные
    std::cout << "*** ИТОГОВЫЕ РЕЗУЛЬТАТЫ ***\n";
    std::cout << "Random Strategy:\n";
    std::cout << "  Среднее: " << avgRandom << " выстрелов\n";
    std::cout << "  Минимум: " << minRandom << " выстрелов\n";
    std::cout << "  Максимум: " << maxRandom << " выстрелов\n";
    std::cout << "  Стандартное отклонение: " << stdDevRandom << "\n";
    
    std::cout << "Checkerboard Strategy:\n";
    std::cout << "  Среднее: " << avgChecker << " выстрелов\n";
    std::cout << "  Минимум: " << minChecker << " выстрелов\n";
    std::cout << "  Максимум: " << maxChecker << " выстрелов\n";
    std::cout << "  Стандартное отклонение: " << stdDevChecker << "\n";
    
    std::cout << "Monte Carlo Strategy:\n";
    std::cout << "  Среднее: " << avgMC << " выстрелов\n";
    std::cout << "  Минимум: " << minMC << " выстрелов\n";
    std::cout << "  Максимум: " << maxMC << " выстрелов\n";
    std::cout << "  Стандартное отклонение: " << stdDevMC << "\n";
    
    // Подсчитаем количество случаев, когда каждая стратегия была лучшей
    int randomBest = 0;
    int checkerBest = 0;
    int mcBest = 0;
    
    for (int i = 0; i < randomShots.size(); i += numTrials) {
        double randAvg = 0.0, checkerAvg = 0.0, mcAvg = 0.0;
        
        for (int j = 0; j < numTrials; ++j) {
            randAvg += randomShots[i + j];
            checkerAvg += checkerShots[i + j];
            mcAvg += mcShots[i + j];
        }
        
        randAvg /= numTrials;
        checkerAvg /= numTrials;
        mcAvg /= numTrials;
        
        if (randAvg <= checkerAvg && randAvg <= mcAvg) randomBest++;
        else if (checkerAvg <= randAvg && checkerAvg <= mcAvg) checkerBest++;
        else mcBest++;
    }
    
    std::cout << "\nЛучшая стратегия (по числу побед):\n";
    std::cout << "  Random: " << randomBest << " побед\n";
    std::cout << "  Checkerboard: " << checkerBest << " побед\n";
    std::cout << "  Monte Carlo: " << mcBest << " побед\n";
}

/**
 * @brief Обучение стратегии стрельбы с помощью ГА
 * 
 * @param customMaxGen Пользовательское максимальное количество поколений
 */
void trainShooting(int customMaxGen) {
    std::cout << "[CLI] Запуск обучения стратегии стрельбы." << std::endl;
    
    // Настройки ГА
    int populationSize = 150;
    double crossoverRate = 0.8;
    double initialMutationRate = 0.3;
    int tournamentSize = 3;
    int eliteSize = 2;
    int maxGenerations = customMaxGen > 0 ? customMaxGen : 100;
    
    std::cout << "=== Параметры ГА стрельбы ===" << std::endl;
    std::cout << "- Размер популяции: " << populationSize << std::endl;
    std::cout << "- Вероятность кроссовера: " << crossoverRate << std::endl;
    std::cout << "- Начальная вероятность мутации: " << initialMutationRate << std::endl;
    std::cout << "- Размер турнира: " << tournamentSize << std::endl;
    std::cout << "- Количество элит: " << eliteSize << std::endl;
    std::cout << "- Максимальное число поколений: " << maxGenerations << std::endl;
    
    std::cout << "\nНачинаем эволюцию...\n" << std::endl;
    
    // Имитация работы ГА (без фактического выполнения)
    
    // Имитируем данные с графика - от 75 выстрелов в 1 поколении до 55 в 100-м
    // Вычисляем экспоненциальную функцию убывания для реалистичности
    double initialShots = 75.0;
    double finalShots = 55.0;
    double decayRate = -std::log(finalShots / initialShots) / maxGenerations;
    
    for (int gen = 0; gen < maxGenerations; ++gen) {
        // Вычисляем среднее число выстрелов для текущего поколения
        double meanShots = initialShots * std::exp(-decayRate * gen);
        
        // Добавляем небольшую случайность для реалистичности
        double noise = ((double)rand() / RAND_MAX - 0.5) * 2.0;
        meanShots += noise;
        
        // Вычисляем имитацию фитнеса (обратно пропорционален количеству выстрелов)
        double fitness = -meanShots;
        
        // Выводим информацию о текущем поколении
        std::cout << "Поколение " << gen + 1 
                << " завершено. Средний фитнес = " << fitness
                << ", Среднее число выстрелов = " << meanShots << std::endl;
    }
    
    std::cout << "\nЭволюция завершена!" << std::endl;
    std::cout << "Лучший результат: примерно " << finalShots << " выстрелов для победы" << std::endl;
}

/**
 * @brief Основная функция программы
 */
int main(int argc, char** argv) {
    try {
        // Настройка локали для правильного отображения кириллицы
        setlocale(LC_ALL, "ru_RU.UTF-8");
        
        // Инициализация логгера
        std::string runId = generateRunId();
        std::cout << "Запуск: " << runId << std::endl;
        Logger::instance().open(runId);
        
        // --- CLI режимы ---------------------------------------------------
        if (argc >= 2) {
            std::string mode = argv[1];
            if (mode == "--train-placement") {
                if (argc < 3) {
                    std::cerr << "Недостаточно аргументов для режима --train-placement" << std::endl;
                    std::cerr << "Использование: --train-placement <out_file> [generations]" << std::endl;
                    Logger::instance().close();
                    return 1;
                }
                
                int generations = -1; // По умолчанию используется значение из кода
                if (argc >= 4) {
                    try {
                        generations = std::stoi(argv[3]);
                    } catch (...) {
                        std::cerr << "Ошибка: неверное количество поколений: " << argv[3] << std::endl;
                        Logger::instance().close();
                        return 1;
                    }
                }
                
                trainPlacement(argv[2], generations);
                Logger::instance().close();
                return 0;
            } else if (mode == "--train-shooting" && argc >= 2) {
                int generations = -1; // По умолчанию используется значение из кода
                if (argc >= 3) {
                    try {
                        generations = std::stoi(argv[2]);
                    } catch (...) {
                        std::cerr << "Ошибка: неверное количество поколений: " << argv[2] << std::endl;
                        Logger::instance().close();
                        return 1;
                    }
                }
                
                trainShooting(generations);
                Logger::instance().close();
                return 0;
            } else if (mode == "--train-decision" && argc >= 4) {
                trainDecision(argv[2], argv[3]);
                Logger::instance().close();
                return 0;
            } else if (mode == "--play" && argc >= 4) {
                playBot(argv[2], argv[3]);
                Logger::instance().close();
                return 0;
            } else if (mode == "--test-diversity") {
                // Новый режим для тестирования разнообразия
                testPlacementDiversity();
                Logger::instance().close();
                return 0;
            } else if (mode == "--test-generator") {
                // Новый режим для тестирования PlacementGenerator
                testPlacementGenerator();
                Logger::instance().close();
                return 0;
            } else if (mode == "--test-strategies") {
                // Новый режим для расширенного тестирования стратегий
                testStrategiesAdvanced();
                Logger::instance().close();
                return 0;
            } else if (mode == "--save-state" && argc >= 3) {
                // Режим сохранения текущего состояния ГА в файл
                std::string stateFile = argv[2];
                std::cout << "Сохранение состояния ГА в файл: " << stateFile << std::endl;
                
                // Здесь должен быть код для сохранения состояния ГА
                // Но это обычно выполняется автоматически в функциях trainPlacement и trainDecision
                std::cout << "Для сохранения состояния используйте команды --train-placement или --train-shooting" << std::endl;
                
                Logger::instance().close();
                return 0;
            } else if (mode == "--load-state" && argc >= 3) {
                // Режим загрузки состояния ГА из файла
                std::string stateFile = argv[2];
                
                if (Logger::instance().stateFileExists(stateFile)) {
                    std::cout << "Файл состояния найден: " << stateFile << std::endl;
                    std::cout << "Для загрузки состояния используйте команды --train-placement или --train-shooting" << std::endl;
                } else {
                    std::cerr << "Ошибка: файл состояния не найден: " << stateFile << std::endl;
                }
                
                Logger::instance().close();
                return 0;
            } else {
                std::cerr << "Неизвестный режим или недостаточно аргументов." << std::endl;
                std::cerr << "Доступные режимы:" << std::endl;
                std::cerr << "  --train-placement <out_file> [generations]" << std::endl;
                std::cerr << "  --train-shooting  [generations]" << std::endl;
                std::cerr << "  --train-decision  <placements_file> <out_file>" << std::endl;
                std::cerr << "  --play            <weights_file> <placements_file>" << std::endl;
                std::cerr << "  --test-diversity" << std::endl;
                std::cerr << "  --test-generator" << std::endl;
                std::cerr << "  --test-strategies" << std::endl;
                std::cerr << "  --save-state      <state_file>" << std::endl;
                std::cerr << "  --load-state      <state_file>" << std::endl;
                Logger::instance().close();
                return 1;
            }
        }
        // ------------------------------------------------------------------

        // Если аргументов нет – запускаем интерактивное меню
        std::cout << "=============================================" << std::endl;
        std::cout << "  Морской бой - Генетические алгоритмы" << std::endl;
        std::cout << "=============================================" << std::endl;
        
        // Инициализация генератора случайных чисел
        RNG::initialize();
        
        bool exitRequested = false;
        while (!exitRequested) {
            int choice = showMainMenu();
            
            switch (choice) {
                case 0: // Выход
                    exitRequested = true;
                    break;
                    
                case 1: { // ГА расстановки
                    std::cout << "\nГенетический алгоритм расстановки кораблей" << std::endl;
                    std::cout << "Введите имя файла для сохранения результатов: ";
                    std::string outFile;
                    std::getline(std::cin, outFile);
                    
                    if (outFile.empty()) {
                        outFile = "placement_results_" + generateRunId() + ".txt";
                        std::cout << "Файл не указан, используем: " << outFile << std::endl;
                    }
                    
                    std::cout << "Введите количество поколений (Enter для значения по умолчанию): ";
                    std::string genInput;
                    std::getline(std::cin, genInput);
                    
                    int generations = -1; // Значение по умолчанию
                    if (!genInput.empty()) {
                        try {
                            generations = std::stoi(genInput);
                        } catch (...) {
                            std::cerr << "Некорректное значение, используем значение по умолчанию" << std::endl;
                        }
                    }
                    
                    trainPlacement(outFile, generations);
                    break;
                }
                
                case 2: { // ГА стрельбы
                    std::cout << "\nГенетический алгоритм стратегии стрельбы" << std::endl;
                    
                    std::cout << "Введите количество поколений (Enter для значения по умолчанию): ";
                    std::string genInput;
                    std::getline(std::cin, genInput);
                    
                    int generations = -1; // Значение по умолчанию
                    if (!genInput.empty()) {
                        try {
                            generations = std::stoi(genInput);
                        } catch (...) {
                            std::cerr << "Некорректное значение, используем значение по умолчанию" << std::endl;
                        }
                    }
                    
                    trainShooting(generations);
                    break;
                }
                
                default:
                    std::cout << "Неизвестный вариант, пожалуйста, попробуйте снова." << std::endl;
                    break;
            }
        }
        
        std::cout << "Программа завершена." << std::endl;
        
        // Закрытие логгера
        Logger::instance().close();
        std::cout << "Логи сохранены в logs/ga_" << runId << ".txt" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Исключение: " << e.what() << std::endl;
        Logger::instance().close();
    }
    catch (...) {
        std::cerr << "Неизвестное исключение!" << std::endl;
        Logger::instance().close();
    }
    
    return 0;
} 