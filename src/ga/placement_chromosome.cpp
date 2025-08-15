#include "placement_chromosome.h"
#include "placement_generator.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

// Определение статической константы SHIP_LENGTHS
const std::array<int, PlacementChromosome::SHIP_COUNT> PlacementChromosome::SHIP_LENGTHS = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};

PlacementChromosome::PlacementChromosome() 
    : m_genes(GENES_COUNT, 0), m_fitness(0.0), m_meanShots(0.0), m_stdDevShots(0.0),
      m_meanShotsRandom(0.0), m_meanShotsCheckerboard(0.0), m_meanShotsMC(0.0)
{
    // Конструктор по умолчанию, инициализирует гены нулями
}

PlacementChromosome::PlacementChromosome(RNG& rng) 
    : m_genes(GENES_COUNT), m_fitness(0.0), m_meanShots(0.0), m_stdDevShots(0.0),
      m_meanShotsRandom(0.0), m_meanShotsCheckerboard(0.0), m_meanShotsMC(0.0)
{
    generateRandomGenes(rng);
}

PlacementChromosome::PlacementChromosome(const std::vector<int>& genes)
    : m_genes(genes), m_fitness(0.0), m_meanShots(0.0), m_stdDevShots(0.0),
      m_meanShotsRandom(0.0), m_meanShotsCheckerboard(0.0), m_meanShotsMC(0.0)
{
    if (genes.size() != GENES_COUNT) {
        throw std::invalid_argument("Incorrect number of genes");
    }
}

PlacementChromosome::PlacementChromosome(const PlacementChromosome& other)
    : m_genes(other.m_genes), m_fitness(other.m_fitness), 
      m_meanShots(other.m_meanShots), m_stdDevShots(other.m_stdDevShots),
      m_meanShotsRandom(other.m_meanShotsRandom),
      m_meanShotsCheckerboard(other.m_meanShotsCheckerboard),
      m_meanShotsMC(other.m_meanShotsMC)
{
}

PlacementChromosome& PlacementChromosome::operator=(const PlacementChromosome& other)
{
    if (this != &other) {
        m_genes = other.m_genes;
        m_fitness = other.m_fitness;
        m_meanShots = other.m_meanShots;
        m_stdDevShots = other.m_stdDevShots;
        m_meanShotsRandom = other.m_meanShotsRandom;
        m_meanShotsCheckerboard = other.m_meanShotsCheckerboard;
        m_meanShotsMC = other.m_meanShotsMC;
    }
    return *this;
}

std::shared_ptr<Fleet> PlacementChromosome::decodeFleet() const {
    // Используем конструктор по умолчанию Fleet() и добавляем корабли
    auto fleet = std::make_shared<Fleet>(); 

    // Размеры кораблей в порядке: 4,3,3,2,2,2,1,1,1,1 (из Fleet::standardShipLengths)
    // const std::vector<int> shipSizes = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1}; // Заменено на Fleet::standardShipLengths
    
    int geneIndex = 0;
    for (size_t i = 0; i < Fleet::standardShipLengths.size(); ++i) {
        int size = Fleet::standardShipLengths[i];
        // Проверка, что гены не выходят за пределы m_genes
        if (geneIndex + 2 >= m_genes.size()) {
            // Ошибка: недостаточно генов для декодирования всех кораблей
            // Можно вернуть nullptr или кинуть исключение
            std::cerr << "Ошибка декодирования флота: недостаточно генов в хромосоме." << std::endl;
            return nullptr; 
        }
        int x = m_genes[geneIndex++];
        int y = m_genes[geneIndex++];
        bool isHorizontal = m_genes[geneIndex++] == 1; // 1 для горизонтального, 0 для вертикального
        
        fleet->addShip(Ship(x, y, size, isHorizontal));
    }
    
    return fleet;
}

bool PlacementChromosome::isValid() const {
    // Декодируем хромосому в флот
        auto fleet = decodeFleet();
    
    // Проверяем, что флот создан успешно
    if (!fleet) {
        std::cout << "ОШИБКА: Не удалось декодировать флот из хромосомы" << std::endl;
        return false;
    }
    
    // Проверяем, что количество кораблей соответствует ожидаемому
    if (fleet->size() != SHIP_COUNT) {
        std::cout << "ОШИБКА: Неверное количество кораблей: " << fleet->size() << " вместо " << SHIP_COUNT << std::endl;
        return false;
    }
    
    // Проверяем валидность флота (корабли не пересекаются и не касаются)
    bool fleetValid = fleet->isValid();
    
    // Дополнительная проверка: все гены должны быть в допустимом диапазоне
    bool genesValid = true;
    for (int i = 0; i < SHIP_COUNT; ++i) {
        int x = m_genes[i * 3];
        int y = m_genes[i * 3 + 1];
        int o = m_genes[i * 3 + 2];
        int length = SHIP_LENGTHS[i];
        
        // Координаты должны быть в пределах 0-9
        if (x < 0 || x > 9 || y < 0 || y > 9) {
            std::cout << "ОШИБКА: Некорректные координаты корабля " << i << ": (" << x << "," << y << ")" << std::endl;
            genesValid = false;
            break;
        }
        
        // Ориентация должна быть 0 или 1
        if (o != 0 && o != 1) {
            std::cout << "ОШИБКА: Некорректная ориентация корабля " << i << ": " << o << std::endl;
            genesValid = false;
            break;
        }
        
        // Проверка выхода за границы поля
        if ((o == 0 && y + length - 1 > 9) || (o == 1 && x + length - 1 > 9)) {
            genesValid = false;
            break;
        }
    }
    
    return fleetValid && genesValid;
}

// Новая реализация, использующая PlacementGenerator
std::vector<int> PlacementChromosome::generateValidRandomGenes(RNG& rng) {
    // Создаем временный генератор
    PlacementGenerator generator;
    
    // Выбираем случайный bias для разнообразия
    Bias bias = static_cast<Bias>(rng.uniformInt(0, 3));
        
    // Генерируем расстановку
    auto chrom = generator.generate(bias, rng);
    
    // Возвращаем гены
    return chrom.getGenes();
}

// Реализации функций для различных стратегий размещения,
// все они делегируют работу PlacementGenerator

std::vector<int> PlacementChromosome::generateCornerPlacement(RNG& rng) {
    PlacementGenerator generator;
    auto chrom = generator.generate(Bias::CORNER, rng);
    return chrom.getGenes();
}

std::vector<int> PlacementChromosome::generateEdgePlacement(RNG& rng) {
    PlacementGenerator generator;
    auto chrom = generator.generate(Bias::EDGE, rng);
    return chrom.getGenes();
}

std::vector<int> PlacementChromosome::generateCenterPlacement(RNG& rng) {
    PlacementGenerator generator;
    auto chrom = generator.generate(Bias::CENTER, rng);
    return chrom.getGenes();
}

std::vector<int> PlacementChromosome::generateMixedPlacement(RNG& rng) {
    PlacementGenerator generator;
    auto chrom = generator.generate(Bias::RANDOM, rng);
    return chrom.getGenes();
}

void PlacementChromosome::generateRandomGenes(RNG& rng) {
    // Используем новый генератор для создания гарантированно валидных генов
    m_genes = generateValidRandomGenes(rng);
}

std::string PlacementChromosome::serialize() const {
    std::string result;
    for (int i = 0; i < m_genes.size(); ++i) {
        result += std::to_string(m_genes[i]);
        if (i < m_genes.size() - 1) {
            result += ",";
    }
    }
    return result;
} 