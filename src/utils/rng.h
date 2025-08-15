#ifndef RNG_H
#define RNG_H

#include <random>
#include <cstdint>

/**
 * @class RNG
 * @brief Статический класс для генерации случайных чисел.
 * 
 * Обертка вокруг стандартных средств C++ для генерации случайных чисел.
 * Обеспечивает воспроизводимость результатов при фиксированных сидах.
 */
class RNG {
private:
    static std::mt19937 engine;
    static bool initialized;

public:
    /**
     * @brief Инициализация генератора случайных чисел
     * @param seed Сид для генератора (если 0, используется случайный сид)
     */
    static void initialize(uint32_t seed = 0);

    /**
     * @brief Получить случайное целое число в заданном диапазоне
     * @param min Минимальное значение (включительно)
     * @param max Максимальное значение (включительно)
     * @return Случайное целое число
     */
    static int getInt(int min, int max);

    /**
     * @brief Получить случайное действительное число в заданном диапазоне
     * @param min Минимальное значение (включительно)
     * @param max Максимальное значение (исключительно)
     * @return Случайное действительное число
     */
    static double getDouble(double min, double max);

    /**
     * @brief Получить случайное число с нормальным распределением
     * @param mean Среднее значение
     * @param std Стандартное отклонение
     * @return Случайное число с нормальным распределением
     */
    static double getNormal(double mean, double std);

    /**
     * @brief Получить случайное логическое значение
     * @param probability Вероятность true (от 0.0 до 1.0)
     * @return Случайное логическое значение
     */
    static bool getBool(double probability = 0.5);

    // Алиасы методов для совместимости с другими частями кода
    static int uniformInt(int min, int max) { return getInt(min, max); }
    static double uniformReal(double min, double max) { return getDouble(min, max); }
    static double gaussianReal(double mean, double std) { return getNormal(mean, std); }
    static double normalReal(double mean, double std) { return getNormal(mean, std); }
};

#endif // RNG_H 