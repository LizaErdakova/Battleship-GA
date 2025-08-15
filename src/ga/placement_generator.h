#pragma once
#include "placement_chromosome.h"
#include <unordered_set>

enum class Bias { EDGE, CORNER, CENTER, RANDOM };

class PlacementGenerator {
public:
    explicit PlacementGenerator(int maxTries = 50);
    PlacementChromosome generate(Bias bias, RNG& rng) const;
    std::vector<PlacementChromosome> generatePopulation(
            size_t n, RNG& rng) const;
private:
    bool placeShip(int len, bool vertical,
                   std::vector<std::vector<int>>& grid,
                   int& outX, int& outY, RNG& rng,
                   Bias bias, int shipIdx) const;
    bool fits(int x, int y, int len, bool vertical,
              const std::vector<std::vector<int>>& grid) const;
    int maxTries;
}; 