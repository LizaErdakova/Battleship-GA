#include "decision_chromosome.h"
#include <algorithm>
#include <stdexcept>

// Реализация конструктора по умолчанию не требуется, так как он объявлен = default в заголовочном файле

DecisionChromosome::DecisionChromosome(RNG& rng) {
    generateRandomGenes(rng);
}

DecisionChromosome::DecisionChromosome(const std::vector<double>& genes) {
    if (genes.size() != GENES_COUNT) {
        throw std::invalid_argument("Неверное количество генов стратегии");
    }
    
    m_genes = genes;
}

DecisionChromosome::DecisionChromosome(const DecisionChromosome& other)
    : m_genes(other.m_genes)
    , m_fitness(other.m_fitness)
    , m_meanShots(other.m_meanShots)
    , m_stdDevShots(other.m_stdDevShots)
{
}

DecisionChromosome& DecisionChromosome::operator=(const DecisionChromosome& other) {
    if (this != &other) {
        m_genes = other.m_genes;
        m_fitness = other.m_fitness;
        m_meanShots = other.m_meanShots;
        m_stdDevShots = other.m_stdDevShots;
    }
    return *this;
}

double DecisionChromosome::getWeight(int featureIndex) const {
    if (featureIndex < 0 || featureIndex >= GENES_COUNT) {
        throw std::out_of_range("Индекс признака вне допустимого диапазона");
    }
    
    return m_genes[featureIndex];
}

void DecisionChromosome::generateRandomGenes(RNG& rng) {
    m_genes.resize(GENES_COUNT);
    
    // Генерируем случайные веса от 0.0 до 1.0
    for (int i = 0; i < GENES_COUNT; ++i) {
        m_genes[i] = rng.uniformReal(0.0, 1.0);
    }
} 