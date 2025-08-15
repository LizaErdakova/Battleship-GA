#include "rng.h"
#include <chrono>
#include <iostream>

// Инициализация статических членов класса
std::mt19937 RNG::engine;
bool RNG::initialized = false;

void RNG::initialize(uint32_t seed) {
    if (seed == 0) {
        // Используем текущее время в качестве сида, если не указан явно
        seed = static_cast<uint32_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
    }
    
    engine.seed(seed);
    initialized = true;
    std::cout << "RNG initialized with seed: " << seed << std::endl;
}

int RNG::getInt(int min, int max) {
    if (!initialized) {
        initialize();
    }
    
    std::uniform_int_distribution<int> dist(min, max);
    return dist(engine);
}

double RNG::getDouble(double min, double max) {
    if (!initialized) {
        initialize();
    }
    
    std::uniform_real_distribution<double> dist(min, max);
    return dist(engine);
}

double RNG::getNormal(double mean, double std) {
    if (!initialized) {
        initialize();
    }
    
    std::normal_distribution<double> dist(mean, std);
    return dist(engine);
}

bool RNG::getBool(double probability) {
    if (!initialized) {
        initialize();
    }
    
    std::bernoulli_distribution dist(probability);
    return dist(engine);
} 