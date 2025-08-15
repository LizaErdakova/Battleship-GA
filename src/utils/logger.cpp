#include "logger.h"
#include "../ga/placement_chromosome.h"
#include "../ga/decision_chromosome.h"
#include "../models/fleet.h"
#include <chrono>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <algorithm>

// ---------- helpers ----------
static std::string nowIso()
{
    using std::chrono::system_clock;
    auto t = system_clock::to_time_t(system_clock::now());
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream ss;
    ss << std::put_time(&tm, "%Y-%m-%d_%H-%M-%S");
    return ss.str();
}

static std::string boardAscii(const PlacementChromosome& ch)   // 10×10 . #
{
    char board[10][10];
    std::fill(&board[0][0], &board[0][0] + 100, '.');

    auto fleet = ch.decodeFleet();
    if (fleet) {
        for (const auto& ship : fleet->getShips()) {
            for (const auto& cell : ship.getAllCells()) {
                if (cell.x >= 0 && cell.x < 10 && cell.y >= 0 && cell.y < 10) {
                    board[cell.y][cell.x] = '#';
                }
            }
        }
    }

    std::ostringstream out;
    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            out << board[y][x];
        }
        out << '\n';
    }
    return out.str();
}

// ---------- Logger impl ----------
Logger& Logger::instance() { 
    static Logger inst; 
    return inst; 
}

void Logger::open(const std::string& runId)
{
    namespace fs = std::filesystem;
    fs::create_directories("logs");
    std::string fname = "logs/ga_" + runId + ".txt";
    m_file.open(fname, std::ios::out | std::ios::trunc);
    
    if (m_file.is_open()) {
        m_file << "==================================================\n";
        m_file << "  BoтTLEsHіP-HA: GA Evolution Log " << runId << "\n";
        m_file << "  Started: " << nowIso() << "\n";
        m_file << "==================================================\n\n";
        m_file.flush();
    }
}

void Logger::close() { 
    if (m_file.is_open()) {
        m_file << "\n==== Log closed: " << nowIso() << " ====\n";
        m_file.close(); 
    }
}

void Logger::logMessage(const std::string& message) {
    if (!m_file.is_open()) return;
    
    m_file << message << std::endl;
    m_file.flush();
}

template<typename Chromosome>
std::string Logger::genesAsString(const Chromosome& ch) const
{
    std::ostringstream ss;
    const auto& genes = ch.getGenes();
    
    for (size_t i = 0; i < genes.size(); ++i) {
        if (i > 0) ss << " ";
        ss << genes[i];
    }
    
    return ss.str();
}

template<class ChromVec>
void Logger::logPlacementGen(int g, double best, double avg, double mut, const ChromVec& top)
{
    if (!m_file) return;
    m_file << "=== PlacementGA Gen " << g << " ===\n";
    m_file << "best=" << best << " avg=" << avg
           << " mutationRate=" << mut << '\n';
    
    int rank = 1;
    for (const auto& ch : top) {
        m_file << "-- rank " << rank++ << " fitness=" << ch.getFitness() << '\n';
        m_file << "genes:" << genesAsString(ch) << "\n";
        m_file << boardAscii(ch) << '\n';
    }
    m_file.flush();
}

void Logger::logDecisionGen(int g, double best, double avg, double sigma)
{
    if (!m_file) return;
    m_file << "=== DecisionGA Gen " << g << " ===\n";
    m_file << "best=" << best << " avg=" << avg
           << " sigma=" << sigma << '\n';
    m_file.flush();
}

template<class ChromVec>
bool Logger::saveGAState(int gen, const ChromVec& population, double mutationRate, const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::create_directories("saves");
    std::string filepath = "saves/" + filename;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Записываем текущее поколение и коэффициент мутации
    file.write(reinterpret_cast<const char*>(&gen), sizeof(gen));
    file.write(reinterpret_cast<const char*>(&mutationRate), sizeof(mutationRate));
    
    // Записываем размер популяции
    size_t popSize = population.size();
    file.write(reinterpret_cast<const char*>(&popSize), sizeof(popSize));
    
    // Записываем каждую хромосому
    for (const auto& chrom : population) {
        // Получаем гены хромосомы
        const auto& genes = chrom.getGenes();
        
        // Записываем размер вектора генов
        size_t genesSize = genes.size();
        file.write(reinterpret_cast<const char*>(&genesSize), sizeof(genesSize));
        
        // Записываем сами гены
        for (const auto& gene : genes) {
            file.write(reinterpret_cast<const char*>(&gene), sizeof(gene));
        }
        
        // Записываем значение фитнеса
        double fitness = chrom.getFitness();
        file.write(reinterpret_cast<const char*>(&fitness), sizeof(fitness));
        
        // Для PlacementChromosome записываем информацию о средних результатах по стратегиям
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            double meanRandom = chrom.getMeanShotsRandom();
            double meanChecker = chrom.getMeanShotsCheckerboard();
            double meanMC = chrom.getMeanShotsMC();
            
            file.write(reinterpret_cast<const char*>(&meanRandom), sizeof(meanRandom));
            file.write(reinterpret_cast<const char*>(&meanChecker), sizeof(meanChecker));
            file.write(reinterpret_cast<const char*>(&meanMC), sizeof(meanMC));
        }
    }
    
    return file.good();
}

template<class ChromVec>
bool Logger::loadGAState(int& gen, ChromVec& population, double& mutationRate, const std::string& filename)
{
    std::string filepath = "saves/" + filename;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Читаем текущее поколение и коэффициент мутации
    file.read(reinterpret_cast<char*>(&gen), sizeof(gen));
    file.read(reinterpret_cast<char*>(&mutationRate), sizeof(mutationRate));
    
    // Читаем размер популяции
    size_t popSize;
    file.read(reinterpret_cast<char*>(&popSize), sizeof(popSize));
    
    // Очищаем переданный вектор и подготавливаем его к заполнению
    population.clear();
    
    // Читаем каждую хромосому
    for (size_t i = 0; i < popSize && file.good(); ++i) {
        // Читаем размер вектора генов
        size_t genesSize;
        file.read(reinterpret_cast<char*>(&genesSize), sizeof(genesSize));
        
        // Создаем вектор для генов
        std::vector<int> genes(genesSize);
        
        // Читаем гены
        for (size_t j = 0; j < genesSize; ++j) {
            file.read(reinterpret_cast<char*>(&genes[j]), sizeof(int));
        }
        
        // Создаем новую хромосому с загруженными генами
        typename ChromVec::value_type chrom(genes);
        
        // Читаем значение фитнеса
        double fitness;
        file.read(reinterpret_cast<char*>(&fitness), sizeof(fitness));
        chrom.setFitness(fitness);
        
        // Для PlacementChromosome читаем информацию о средних результатах по стратегиям
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            double meanRandom, meanChecker, meanMC;
            
            file.read(reinterpret_cast<char*>(&meanRandom), sizeof(meanRandom));
            file.read(reinterpret_cast<char*>(&meanChecker), sizeof(meanChecker));
            file.read(reinterpret_cast<char*>(&meanMC), sizeof(meanMC));
            
            chrom.setMeanShotsRandom(meanRandom);
            chrom.setMeanShotsCheckerboard(meanChecker);
            chrom.setMeanShotsMC(meanMC);
        }
        
        // Добавляем хромосому в популяцию
        population.push_back(std::move(chrom));
    }
    
    return file.good();
}

template<class ChromVec>
void Logger::saveEvolutionResults(
    const ChromVec& topChromosomes,
    const std::map<int, typename ChromVec::value_type>& bestPerGeneration,
    const std::map<std::string, double>& strategyStats,
    const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::create_directories("results");
    std::string filepath = "results/" + filename;
    
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Не удалось открыть файл для записи результатов: " << filepath << std::endl;
        return;
    }
    
    // Заголовок
    file << "===================================================\n";
    file << "     РЕЗУЛЬТАТЫ ЭВОЛЮЦИИ ГЕНЕТИЧЕСКОГО АЛГОРИТМА      \n";
    file << "                   " << nowIso() << "\n";
    file << "===================================================\n\n";
    
    // --- СТАТИСТИКА ПО СТРАТЕГИЯМ ---
    file << "СТАТИСТИКА ПО СТРАТЕГИЯМ СТРЕЛЬБЫ\n";
    file << "===================================================\n";
    
    // Форматируем вывод с фиксированной точностью
    file << std::fixed << std::setprecision(2);
    
    // Выводим статистику по стратегиям
    for (const auto& [strategy, shots] : strategyStats) {
        file << strategy << ": " << shots << " выстрелов в среднем\n";
    }
    file << "\n";
    
    // --- ЛУЧШИЕ ХРОМОСОМЫ ПО ПОКОЛЕНИЯМ ---
    file << "ЛУЧШИЕ РАССТАНОВКИ ПО ПОКОЛЕНИЯМ\n";
    file << "===================================================\n";
    
    for (const auto& [gen, chrom] : bestPerGeneration) {
        file << "Поколение " << gen << " (фитнес: " << chrom.getFitness() << ")\n";
        
        // Для PlacementChromosome выводим средние результаты по стратегиям
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << "Средние выстрелы - Random: " << chrom.getMeanShotsRandom()
                 << ", Checkerboard: " << chrom.getMeanShotsCheckerboard()
                 << ", Monte Carlo: " << chrom.getMeanShotsMC() << "\n";
        }
        
        file << "Гены: " << genesAsString(chrom) << "\n";
        
        // Для PlacementChromosome выводим ASCII-представление расстановки
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << boardAscii(chrom);
        }
        
        file << "---------------------------------------------------\n\n";
    }
    
    // --- ТОП-50 ЛУЧШИХ РАССТАНОВОК ---
    file << "ТОП-" << topChromosomes.size() << " ЛУЧШИХ РАССТАНОВОК\n";
    file << "===================================================\n";
    
    int rank = 1;
    for (const auto& chrom : topChromosomes) {
        file << "Позиция #" << rank++ << " (фитнес: " << chrom.getFitness() << ")\n";
        
        // Для PlacementChromosome выводим средние результаты по стратегиям
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << "Средние выстрелы - Random: " << chrom.getMeanShotsRandom()
                 << ", Checkerboard: " << chrom.getMeanShotsCheckerboard()
                 << ", Monte Carlo: " << chrom.getMeanShotsMC() << "\n";
        }
        
        file << "Гены: " << genesAsString(chrom) << "\n";
        
        // Для PlacementChromosome выводим ASCII-представление расстановки
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << boardAscii(chrom);
        }
        
        file << "---------------------------------------------------\n\n";
    }
    
    // --- ЛУЧШАЯ РАССТАНОВКА ---
    if (!topChromosomes.empty()) {
        file << "ЛУЧШАЯ РАССТАНОВКА\n";
        file << "===================================================\n";
        
        const auto& bestChrom = topChromosomes.front();
        file << "Фитнес: " << bestChrom.getFitness() << "\n";
        
        // Для PlacementChromosome выводим средние результаты по стратегиям
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << "Средние выстрелы - Random: " << bestChrom.getMeanShotsRandom()
                 << ", Checkerboard: " << bestChrom.getMeanShotsCheckerboard()
                 << ", Monte Carlo: " << bestChrom.getMeanShotsMC() << "\n";
        }
        
        file << "Гены: " << genesAsString(bestChrom) << "\n";
        
        // Для PlacementChromosome выводим ASCII-представление расстановки
        if constexpr (std::is_same_v<typename ChromVec::value_type, PlacementChromosome>) {
            file << boardAscii(bestChrom);
        }
    }
    
    file.close();
}

bool Logger::stateFileExists(const std::string& filename) const
{
    namespace fs = std::filesystem;
    return fs::exists("saves/" + filename);
}

// Добавляем реализацию метода boardAscii как метода класса Logger
std::string Logger::boardAscii(const PlacementChromosome& ch) const 
{
    // Используем реализацию из статической функции
    return ::boardAscii(ch);
}

// *** instantiate templates ***
template void Logger::logPlacementGen<std::vector<PlacementChromosome>>
(int, double, double, double, const std::vector<PlacementChromosome>&); 

template bool Logger::saveGAState<std::vector<PlacementChromosome>>
(int, const std::vector<PlacementChromosome>&, double, const std::string&);

template bool Logger::loadGAState<std::vector<PlacementChromosome>>
(int&, std::vector<PlacementChromosome>&, double&, const std::string&);

template void Logger::saveEvolutionResults<std::vector<PlacementChromosome>>
(const std::vector<PlacementChromosome>&, 
 const std::map<int, PlacementChromosome>&, 
 const std::map<std::string, double>&, 
 const std::string&);

template std::string Logger::genesAsString<PlacementChromosome>
(const PlacementChromosome&) const;

// Специализации для DecisionChromosome
template std::string Logger::genesAsString<DecisionChromosome>
(const DecisionChromosome&) const;

template void Logger::saveEvolutionResults<std::vector<DecisionChromosome>>
(const std::vector<DecisionChromosome>&, 
 const std::map<int, DecisionChromosome>&, 
 const std::map<std::string, double>&, 
 const std::string&);

// Специализация для сохранения DecisionChromosome
template<>
bool Logger::saveGAState<std::vector<DecisionChromosome>>
(int gen, const std::vector<DecisionChromosome>& population, double mutationRate, const std::string& filename)
{
    namespace fs = std::filesystem;
    fs::create_directories("saves");
    std::string filepath = "saves/" + filename;
    
    std::ofstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Записываем текущее поколение и коэффициент мутации
    file.write(reinterpret_cast<const char*>(&gen), sizeof(gen));
    file.write(reinterpret_cast<const char*>(&mutationRate), sizeof(mutationRate));
    
    // Записываем размер популяции
    size_t popSize = population.size();
    file.write(reinterpret_cast<const char*>(&popSize), sizeof(popSize));
    
    // Записываем каждую хромосому
    for (const auto& chrom : population) {
        // Получаем гены хромосомы
        const auto& genes = chrom.getGenes();
        
        // Записываем размер вектора генов
        size_t genesSize = genes.size();
        file.write(reinterpret_cast<const char*>(&genesSize), sizeof(genesSize));
        
        // Записываем сами гены (с преобразованием double -> int для совместимости)
        for (const auto& gene : genes) {
            int geneAsInt = static_cast<int>(gene);
            file.write(reinterpret_cast<const char*>(&geneAsInt), sizeof(geneAsInt));
        }
        
        // Записываем значение фитнеса
        double fitness = chrom.getFitness();
        file.write(reinterpret_cast<const char*>(&fitness), sizeof(fitness));
    }
    
    return file.good();
}

// Специализация для загрузки DecisionChromosome
template<>
bool Logger::loadGAState<std::vector<DecisionChromosome>>
(int& gen, std::vector<DecisionChromosome>& population, double& mutationRate, const std::string& filename)
{
    std::string filepath = "saves/" + filename;
    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Читаем текущее поколение и коэффициент мутации
    file.read(reinterpret_cast<char*>(&gen), sizeof(gen));
    file.read(reinterpret_cast<char*>(&mutationRate), sizeof(mutationRate));
    
    // Читаем размер популяции
    size_t popSize;
    file.read(reinterpret_cast<char*>(&popSize), sizeof(popSize));
    
    // Очищаем переданный вектор и подготавливаем его к заполнению
    population.clear();
    
    // Читаем каждую хромосому
    for (size_t i = 0; i < popSize && file.good(); ++i) {
        // Читаем размер вектора генов
        size_t genesSize;
        file.read(reinterpret_cast<char*>(&genesSize), sizeof(genesSize));
        
        // Создаем вектор для генов (для DecisionChromosome это должен быть вектор double)
        std::vector<double> genes(genesSize);
        
        // Читаем гены и конвертируем из int в double, если необходимо
        for (size_t j = 0; j < genesSize; ++j) {
            int geneAsInt;
            file.read(reinterpret_cast<char*>(&geneAsInt), sizeof(int));
            genes[j] = static_cast<double>(geneAsInt);
        }
        
        // Создаем новую хромосому с загруженными генами
        DecisionChromosome chrom(genes);
        
        // Читаем значение фитнеса
        double fitness;
        file.read(reinterpret_cast<char*>(&fitness), sizeof(fitness));
        chrom.setFitness(fitness);
        
        // Добавляем хромосому в популяцию
        population.push_back(std::move(chrom));
    }
    
    return file.good();
} 