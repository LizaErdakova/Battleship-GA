#include "io.h"
#include <fstream>
#include <stdexcept>

using std::uint8_t;

static void writeUint8(std::ofstream& ofs, uint8_t value) {
    ofs.write(reinterpret_cast<const char*>(&value), sizeof(uint8_t));
}

static uint8_t readUint8(std::ifstream& ifs) {
    uint8_t v{};
    ifs.read(reinterpret_cast<char*>(&v), sizeof(uint8_t));
    return v;
}

void savePlacements(const std::string& path,
                    const std::vector<PlacementChromosome>& placements) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Cannot open file for writing: " + path);

    for (const auto& chrom : placements) {
        const auto& genes = chrom.getGenes();
        if (genes.size() != PlacementChromosome::GENES_COUNT)
            throw std::runtime_error("Invalid gene count in chromosome");
        for (int g : genes) {
            uint8_t byte = static_cast<uint8_t>(g & 0xFF);
            writeUint8(ofs, byte);
        }
    }
}

std::vector<PlacementChromosome> loadPlacements(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open file for reading: " + path);

    std::vector<PlacementChromosome> out;
    std::vector<int> genes(PlacementChromosome::GENES_COUNT);

    while (ifs.peek() != EOF) {
        for (int i = 0; i < PlacementChromosome::GENES_COUNT; ++i) {
            uint8_t b = readUint8(ifs);
            genes[i] = static_cast<int>(b);
        }
        out.emplace_back(genes);
    }
    return out;
}

bool loadPlacementsFromFile(const std::string& path, PlacementPool& pool) {
    try {
        auto placements = loadPlacements(path);
        
        if (placements.empty()) {
            return false;
        }
        
        // Добавляем все загруженные расстановки в пул
        for (const auto& placement : placements) {
            pool.addPlacement(placement);
        }
        
        return true;
    } catch(const std::exception& e) {
        return false; // В случае ошибки возвращаем false
    }
}

void saveWeights(const std::string& path,
                 const std::vector<double>& weights) {
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("Cannot open file for writing: " + path);

    for (double w : weights) {
        ofs.write(reinterpret_cast<const char*>(&w), sizeof(double));
    }
}

std::vector<double> loadWeights(const std::string& path) {
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs) throw std::runtime_error("Cannot open file for reading: " + path);

    std::vector<double> weights;
    double w;
    while (ifs.read(reinterpret_cast<char*>(&w), sizeof(double))) {
        weights.push_back(w);
    }
    
    return weights;
} 