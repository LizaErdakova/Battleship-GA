#pragma once

#include <string>
#include <vector>
#include "../ga/placement_chromosome.h"
#include "../ga/placement_pool.h"

/**
 * @brief Сохраняет пул расстановок в бинарный файл.
 *        Формат: K подряд идущих хромосом, каждая 30 байт (uint8_t x,y,o)
 */
void savePlacements(const std::string& path,
                    const std::vector<PlacementChromosome>& placements);

/**
 * @brief Загружает пул расстановок из бинарного файла.
 */
std::vector<PlacementChromosome> loadPlacements(const std::string& path);

/**
 * @brief Загружает расстановки в указанный пул
 * 
 * @param path Путь к файлу с расстановками
 * @param pool Пул для загрузки
 * @return true если загрузка выполнена успешно, false иначе
 */
bool loadPlacementsFromFile(const std::string& path, PlacementPool& pool);

/**
 * @brief Сохраняет лучшие веса DecisionGA (20 double) в файл.
 */
void saveWeights(const std::string& path,
                 const std::vector<double>& weights);

/**
 * @brief Загружает веса DecisionGA из файла.
 */
std::vector<double> loadWeights(const std::string& path); 