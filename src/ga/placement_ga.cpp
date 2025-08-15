#include "placement_ga.h"
#include "../utils/logger.h"
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <iostream>
#include <cmath>
#include <chrono> // Для ETA

PlacementGA::PlacementGA(
    int populationSize,
    double crossoverRate,
    double mutationRate,
    int tournamentSize,
    int eliteCount,
    double initialPenalty,
    double penaltyAlpha
)
    : m_populationSize(populationSize)
    , m_crossoverRate(crossoverRate)
    , m_mutationRate(mutationRate)
    , m_tournamentSize(tournamentSize)
    , m_eliteCount(eliteCount)
    , m_currentGeneration(0)
    , m_initialPenalty(initialPenalty)
    , m_currentPenalty(initialPenalty)
    , m_penaltyAlpha(penaltyAlpha)
    , m_rng()
    , m_regeneratedCount(0)
{
    // Инициализируем генератор случайных чисел, если это еще не сделано
    // Вызываем любой метод RNG, чтобы убедиться, что он инициализирован
    RNG::getInt(0, 1);
    
    if (populationSize <= 0) {
        throw std::invalid_argument("Размер популяции должен быть положительным");
    }
    
    if (crossoverRate < 0.0 || crossoverRate > 1.0) {
        throw std::invalid_argument("Вероятность кроссовера должна быть в диапазоне [0,1]");
    }
    
    if (mutationRate < 0.0 || mutationRate > 1.0) {
        throw std::invalid_argument("Вероятность мутации должна быть в диапазоне [0,1]");
    }
    
    if (tournamentSize <= 0 || tournamentSize > populationSize) {
        throw std::invalid_argument("Размер турнира должен быть в диапазоне [1,populationSize]");
    }
    
    if (eliteCount < 0 || eliteCount > populationSize) {
        throw std::invalid_argument("Количество элитных особей должно быть в диапазоне [0,populationSize]");
    }
}

PlacementChromosome PlacementGA::evolve(
    int maxGenerations,
    double targetFitness,
    const std::function<double(PlacementChromosome&)>& fitnessFunction
) {
    // Инициализируем популяцию
    initializePopulation(fitnessFunction);
    
    PlacementChromosome bestChromosome = getBestChromosome();
    
    std::cout << "Поколение 0: Лучший фитнес = " << bestChromosome.getFitness() << std::endl;
    
    int topN = std::min(5, static_cast<int>(m_population.size()));
    std::vector<PlacementChromosome> top5(m_population.begin(),
                                         m_population.begin() + topN);

    Logger::instance().logPlacementGen(
            0, 
            bestChromosome.getFitness(),
            getAverageFitness(),
            m_mutationRate,
            top5);
    
    // Основной цикл генетического алгоритма
    for (m_currentGeneration = 1; m_currentGeneration <= maxGenerations; ++m_currentGeneration) {
        auto tick = std::chrono::high_resolution_clock::now(); // Начало замера времени
        
        PlacementChromosome currentBest = evolvePopulation(fitnessFunction);
        
        auto toc = std::chrono::high_resolution_clock::now(); // Конец замера времени
        double ms = std::chrono::duration<double, std::milli>(toc - tick).count();

        std::cout << "Поколение " << m_currentGeneration 
                  << " завершено за " << ms << " мс. "
                  << "Лучший фитнес = " << currentBest.getFitness()
                  << ", Средний фитнес = " << getAverageFitness() << std::endl;
        
        topN = std::min(5, static_cast<int>(m_population.size()));
        top5 = std::vector<PlacementChromosome>(m_population.begin(),
                                              m_population.begin() + topN);

        Logger::instance().logPlacementGen(
                m_currentGeneration,
                currentBest.getFitness(),
                getAverageFitness(),
                m_mutationRate,
                top5);
        
        if (currentBest.getFitness() > bestChromosome.getFitness()) {
            bestChromosome = currentBest;
        }
        
        if (bestChromosome.getFitness() >= targetFitness) {
            std::cout << "Целевой фитнес (" << targetFitness 
                      << ") достигнут в поколении " << m_currentGeneration << std::endl;
            break;
        }
    }
    
    std::cout << "Генетический алгоритм завершен." << std::endl;
    std::cout << "Лучший фитнес: " << bestChromosome.getFitness() << std::endl;
    std::cout << "Общее количество перегенерированных (невалидных) особей за все время: " 
              << m_regeneratedCount << std::endl;
    
    return bestChromosome;
}

void PlacementGA::initializePopulation(
    const std::function<double(PlacementChromosome&)>& fitnessFunction
) {
    m_population.clear();
    m_population.reserve(m_populationSize);
    
    // С вероятностью 70% используем новый PlacementGenerator для создания разнообразных расстановок
    if (m_rng.uniformReal(0.0, 1.0) < 0.7) {
        PlacementGenerator generator(50);
        auto initialPopulation = generator.generatePopulation(m_populationSize, m_rng);
        
        // Проверяем валидность всей популяции (это должно быть избыточно, но для уверенности)
        for (auto& chromosome : initialPopulation) {
            if (!chromosome.isValid()) {
                std::cerr << "ПРЕДУПРЕЖДЕНИЕ: PlacementGenerator создал невалидную хромосому" << std::endl;
        
                // Пытаемся исправить хромосому
                repair(chromosome);
                
                // Если не удалось исправить
                if (!chromosome.isValid()) {
                    // Создаем хромосому с гарантированно валидными генами
                    std::vector<int> validGenes = PlacementChromosome::generateValidRandomGenes(m_rng);
                    chromosome = PlacementChromosome(validGenes);
                    m_regeneratedCount++;
                }
        }
        
            // Вычисляем фитнес каждой хромосомы
            fitnessFunction(chromosome);
        }
        
        // Используем сгенерированную популяцию
        m_population = std::move(initialPopulation);
    } else {
        // Используем старый метод в остальных случаях
        // Создаем начальную популяцию из случайных хромосом
        for (int i = 0; i < m_populationSize; ++i) {
            // С вероятностью 70% используем более эффективный метод размещения
            PlacementChromosome chromosome;
            if (m_rng.uniformReal(0.0, 1.0) < 0.7) {
                std::vector<int> validGenes = PlacementChromosome::generateValidRandomGenes(m_rng);
                chromosome = PlacementChromosome(validGenes);
            } else {
                chromosome = PlacementChromosome(m_rng);
            }
            
            // Проверяем и обеспечиваем валидность хромосомы
        if (!chromosome.isValid()) {
                // Пытаемся исправить хромосому
            repair(chromosome);
                
                // Дополнительная проверка валидности после ремонта
                if (!chromosome.isValid()) {
                    // Создаем новую с гарантированно валидными генами
                    std::vector<int> validGenes = PlacementChromosome::generateValidRandomGenes(m_rng);
                    chromosome = PlacementChromosome(validGenes);
                    m_regeneratedCount++;
                }
        }
        
        // Вычисляем фитнес для хромосомы
        fitnessFunction(chromosome);
        
            // Добавляем в популяцию
        m_population.push_back(chromosome);
    }
    }
    
    // Проверка валидности всей популяции
    if (!verifyPopulationValidity()) {
        throw std::runtime_error("Невалидная начальная популяция");
    }
    
    // Устанавливаем начальные значения
    m_currentGeneration = 0;
    m_currentPenalty = m_initialPenalty;
}

PlacementChromosome PlacementGA::evolvePopulation(
    const std::function<double(PlacementChromosome&)>& fitnessFunction
) {
    // Создаем новую популяцию
    std::vector<PlacementChromosome> newPopulation;
    newPopulation.reserve(m_populationSize);
    
    // Сохраняем элитных особей (лучшие хромосомы из текущей популяции)
    for (int i = 0; i < m_eliteCount; ++i) {
        newPopulation.push_back(m_population[i]);
    }
    
    // Заполняем оставшуюся часть новой популяции потомками
    while (newPopulation.size() < m_populationSize) {
        // Выбираем родителей
        PlacementChromosome parent1 = selectParent();
        PlacementChromosome parent2 = selectParent();
        
        // С вероятностью crossoverRate выполняем кроссовер
        PlacementChromosome offspring = 
            (m_rng.uniformReal(0.0, 1.0) < m_crossoverRate) ?
            crossover(parent1, parent2) : parent1;
        
        // С вероятностью mutationRate выполняем мутацию
        if (m_rng.uniformReal(0.0, 1.0) < m_mutationRate) {
            mutate(offspring);
        }
        
        // Проверяем и обеспечиваем валидность потомка
        // Сначала пытаемся исправить потомка, и если не получается - создаем новую валидную хромосому
        if (!offspring.isValid()) {
            // Метод repair теперь всегда возвращает true, так как гарантированно создает валидную хромосому
            repair(offspring);
        
            // Дополнительная проверка после ремонта (перестраховка)
        if (!offspring.isValid()) {
                // Если по какой-то причине хромосома все еще невалидна, 
                // генерируем новую с использованием гарантированно валидного метода
                std::vector<int> validGenes = PlacementChromosome::generateValidRandomGenes(m_rng);
                offspring = PlacementChromosome(validGenes);
                m_regeneratedCount++; // Увеличиваем счетчик перегенерированных особей
            }
        }
        
        // На этом этапе offspring гарантированно валидна
        
        // Вычисляем фитнес для нового потомка
        fitnessFunction(offspring);
        
        // Добавляем потомка в новую популяцию
        newPopulation.push_back(offspring);
    }
    
    // Заменяем текущую популяцию новой
    m_population = std::move(newPopulation);
    
    // Сортируем популяцию по убыванию фитнеса
    std::sort(m_population.begin(), m_population.end(),
              [](const PlacementChromosome& a, const PlacementChromosome& b) {
                  return a.getFitness() > b.getFitness();
              });
    
    // Проверяем валидность всех хромосом в новой популяции
    verifyPopulationValidity();
    
    // Возвращаем лучшую хромосому
    return m_population.front();
}

PlacementChromosome PlacementGA::getBestChromosome() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    // Находим хромосому с максимальным фитнесом
    auto it = std::max_element(m_population.begin(), m_population.end(),
                               [](const PlacementChromosome& a, const PlacementChromosome& b) {
                                   return a.getFitness() < b.getFitness();
                               });
    
    return *it;
}

double PlacementGA::getBestFitness() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    return m_population.front().getFitness();
}

double PlacementGA::getAverageFitness() const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    double sum = std::accumulate(m_population.begin(), m_population.end(), 0.0,
                                [](double sum, const PlacementChromosome& chromosome) {
                                    return sum + chromosome.getFitness();
                                });
    
    return sum / m_population.size();
}

PlacementChromosome PlacementGA::crossover(
    const PlacementChromosome& parent1,
    const PlacementChromosome& parent2
) {
    // Реализуем Ship-swap crossover
    // Этот оператор меняет местами гены, соответствующие случайно выбранным кораблям
    
    // Получаем гены родителей
    std::vector<int> offspringGenes = parent1.getGenes();
    const std::vector<int>& parent2Genes = parent2.getGenes();
    
    // Выбираем случайное количество кораблей для обмена (1-4)
    int swapCount = m_rng.uniformInt(1, 4);
    
    for (int i = 0; i < swapCount; ++i) {
        // Выбираем случайный корабль
        int shipIndex = m_rng.uniformInt(0, 9);
        
        // Индекс начала генов для выбранного корабля
        int geneIndex = shipIndex * 3;
        
        // Копируем гены корабля из родителя 2 в потомка
        offspringGenes[geneIndex]     = parent2Genes[geneIndex];     // x
        offspringGenes[geneIndex + 1] = parent2Genes[geneIndex + 1]; // y
        offspringGenes[geneIndex + 2] = parent2Genes[geneIndex + 2]; // ориентация
    }
    
    // Создаем новую хромосому с полученными генами
    // Статистические данные будут унаследованы от родителя 1, но затем обновлены
    // при вычислении фитнес-функции
    PlacementChromosome offspring(offspringGenes);
    
    // Копируем статистические данные от первого родителя, 
    // они будут пересчитаны позже при вызове fitness-функции
    offspring.setFitness(parent1.getFitness());
    offspring.setMeanShots(parent1.getMeanShots());
    offspring.setStdDevShots(parent1.getStdDevShots());
    offspring.setMeanShotsRandom(parent1.getMeanShotsRandom());
    offspring.setMeanShotsCheckerboard(parent1.getMeanShotsCheckerboard());
    offspring.setMeanShotsMC(parent1.getMeanShotsMC());
    
    return offspring;
}

void PlacementGA::mutate(PlacementChromosome& chromosome) {
    // Реализуем трёхуровневую мутацию:
    // 1. micro-shift (75%) - сдвиг x/y на ±1
    // 2. flip-orient (20%) - изменение ориентации
    // 3. teleport (5%) - полная перестановка корабля
    
    // Получаем гены для модификации
    std::vector<int> genes = chromosome.getGenes();
    
    // Выбираем случайный корабль
    int shipIndex = m_rng.uniformInt(0, 9);
    
    // Индекс начала генов для выбранного корабля
    int geneIndex = shipIndex * 3;
    
    // Длина выбранного корабля
    const int shipSizes[10] = {4, 3, 3, 2, 2, 2, 1, 1, 1, 1};
    int length = shipSizes[shipIndex];
    
    // Выбираем тип мутации по вероятности
    double mutationType = m_rng.uniformReal(0.0, 1.0);
    
    if (mutationType < 0.75) { 
        // micro-shift: сдвигаем на ±1 по x или y
        int dx = m_rng.uniformInt(-1, 1);
        int dy = m_rng.uniformInt(-1, 1);
        
        genes[geneIndex] = std::max(0, std::min(9, genes[geneIndex] + dx));
        genes[geneIndex + 1] = std::max(0, std::min(9, genes[geneIndex + 1] + dy));
            }
    else if (mutationType < 0.95) { 
        // flip-orient: меняем ориентацию корабля (0 ↔ 1)
        genes[geneIndex + 2] = 1 - genes[geneIndex + 2]; // Инвертируем бит ориентации
    } 
    else {
        // teleport: полностью случайное перемещение
        bool isVertical = (m_rng.uniformInt(0, 1) == 0);
        int maxX = isVertical ? 9 : (10 - length);
        int maxY = isVertical ? (10 - length) : 9;
        
        genes[geneIndex] = m_rng.uniformInt(0, maxX);
        genes[geneIndex + 1] = m_rng.uniformInt(0, maxY);
        genes[geneIndex + 2] = isVertical ? 0 : 1;
    }
    
    // Обновляем гены хромосомы
    chromosome.setGenes(genes);
}

PlacementChromosome PlacementGA::selectParent() {
    // Реализуем турнирную селекцию
    
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    // Выбираем m_tournamentSize случайных хромосом из популяции
    std::vector<PlacementChromosome> tournament;
    tournament.reserve(m_tournamentSize);
    
    for (int i = 0; i < m_tournamentSize; ++i) {
        int index = m_rng.uniformInt(0, m_population.size() - 1);
        tournament.push_back(m_population[index]);
    }
    
    // Находим хромосому с максимальным фитнесом в турнире
    auto it = std::max_element(tournament.begin(), tournament.end(),
                               [](const PlacementChromosome& a, const PlacementChromosome& b) {
                                   return a.getFitness() < b.getFitness();
                               });
    
    return *it;
}

bool PlacementGA::repair(PlacementChromosome& chromosome) {
    // 0. Быстрая проверка
    if (chromosome.isValid()) return true;

    // 1. Декодируем корабли, чтобы работать с координатами
    auto genes = chromosome.getGenes();                   // 30 int
    constexpr int SHIP_COUNT = 10;
    const int lens[SHIP_COUNT] = {4,3,3,2,2,2,1,1,1,1};
    
    // 2. «Каркас» поля: 0 = свободно, 1+ = занято/запрещено
    int field[10][10] = {0};

    auto markShip = [&](int idx, int v){             // v=+1 поставить, v=-1 убрать
        int len = lens[idx];
        int x   = genes[idx*3];
        int y   = genes[idx*3+1];
        bool hor= genes[idx*3+2]==1;
        for(int i=0;i<len;++i){
            int xx = x + (hor?i:0);
            int yy = y + (hor?0:i);
            if (xx < 0 || xx >= 10 || yy < 0 || yy >= 10) continue; // Защита от выхода за границы
            for(int dy=-1;dy<=1;++dy)
                for(int dx=-1;dx<=1;++dx){
                    int nx=xx+dx, ny=yy+dy;
                    if(nx>=0 && nx<10 && ny>=0 && ny<10)
                        field[ny][nx] += v;
    }
        }
    };

    // 3. Пытаемся 50 × (#конфликтных) раз «починить» конфигурацию
    for(int attempt=0; attempt<50 && !chromosome.isValid(); ++attempt){

        // 3.1 выбираем любой из 10 кораблей случайно
        int s = m_rng.uniformInt(0, SHIP_COUNT-1);

        // 3.2 снимаем его с поля
        markShip(s, -1);

        // 3.3 пробуем до 30 раз поставить заново вокруг текущей позиции
        bool placed = false;
        for(int local=0; local<30 && !placed; ++local){

            // случайный сдвиг на [-2..2] и/или поворот
            int len=lens[s];
            bool hor = m_rng.uniformInt(0,1);
            int x = std::clamp(genes[s*3] + m_rng.uniformInt(-2,2), 0, 9);
            int y = std::clamp(genes[s*3+1] + m_rng.uniformInt(-2,2), 0, 9);

            // коррекция выхода за край
            if (hor && x+len-1>9) x = 10-len;
            if (!hor && y+len-1>9) y = 10-len;
    
            // проверка
            bool ok=true;
            for(int i=0;i<len && ok;++i){
                int xx=x+(hor?i:0), yy=y+(hor?0:i);
                if(field[yy][xx]) ok=false;          // занято или запрещено
            }
            if(ok){
                genes[s*3]=x; genes[s*3+1]=y; genes[s*3+2]=hor?1:0;
                markShip(s,+1);
                placed=true;
            }
        }

        // 3.4 если за 30 локальных не вышло — полностью рандомное място
        if(!placed){
            for(bool done=false; !done && attempt<30; ){
                attempt++; // Увеличиваем счетчик, чтобы не зациклиться
                bool hor = m_rng.uniformInt(0,1);
                int len = lens[s];
                int x   = m_rng.uniformInt(0, hor?10-len:9);
                int y   = m_rng.uniformInt(0, hor?9:10-len);
                bool ok=true;
                for(int i=0;i<len && ok;++i){
                    int xx=x+(hor?i:0), yy=y+(hor?0:i);
                    if(field[yy][xx]) ok=false;
                }
                if(ok){
                    genes[s*3]=x; genes[s*3+1]=y; genes[s*3+2]=hor?1:0;
                    markShip(s,+1);
                    done=true;
                }
            }
        }
    }
        
    // 4. Обновляем гены ребёнка
    chromosome.setGenes(genes);
        
    // Если не удалось починить за отведенное число попыток, создаем полностью новую хромосому
    if (!chromosome.isValid()) {
        // Используем PlacementGenerator для создания гарантированно валидной расстановки с разнообразием
        PlacementGenerator generator;
        
        // Выбираем случайную стратегию размещения для разнообразия
        Bias bias = static_cast<Bias>(m_rng.uniformInt(0, 3));
        
        // Генерируем новую хромосому
        auto newChromosome = generator.generate(bias, m_rng);
        
        // Сохраняем статистические данные из старой хромосомы
        double oldFitness = chromosome.getFitness();
        double oldMeanShots = chromosome.getMeanShots();
        double oldStdDevShots = chromosome.getStdDevShots();
        double oldMeanShotsRandom = chromosome.getMeanShotsRandom();
        double oldMeanShotsCheckerboard = chromosome.getMeanShotsCheckerboard();
        double oldMeanShotsMC = chromosome.getMeanShotsMC();
        
        // Применяем гены к исходной хромосоме
        chromosome.setGenes(newChromosome.getGenes());
        
        // Восстанавливаем статистические данные, которые будут обновлены позже в fitnessFunction
        chromosome.setFitness(oldFitness);
        chromosome.setMeanShots(oldMeanShots);
        chromosome.setStdDevShots(oldStdDevShots);
        chromosome.setMeanShotsRandom(oldMeanShotsRandom);
        chromosome.setMeanShotsCheckerboard(oldMeanShotsCheckerboard);
        chromosome.setMeanShotsMC(oldMeanShotsMC);
        
        m_regeneratedCount++; // Увеличиваем счетчик перегенерированных особей
        
        // Финальная проверка
        if (!chromosome.isValid()) {
            std::cerr << "ОШИБКА: После repair хромосома все еще невалидна!" << std::endl;
            // Этого не должно происходить, но на всякий случай используем другой bias
            auto fallbackChromosome = generator.generate(Bias::RANDOM, m_rng);
            chromosome.setGenes(fallbackChromosome.getGenes());
            // Восстанавливаем статистические данные после повторной попытки
            chromosome.setFitness(oldFitness);
            chromosome.setMeanShots(oldMeanShots);
            chromosome.setStdDevShots(oldStdDevShots);
            chromosome.setMeanShotsRandom(oldMeanShotsRandom);
            chromosome.setMeanShotsCheckerboard(oldMeanShotsCheckerboard);
            chromosome.setMeanShotsMC(oldMeanShotsMC);
            }
        
        return true; // Возвращаем true, так как мы создали валидную хромосому
    }
    
    return chromosome.isValid(); // true => успех; false => особь можно отбросить (но это не должно случаться)
}

bool PlacementGA::verifyPopulationValidity() const {
    bool allValid = true;
    
    for (size_t i = 0; i < m_population.size(); ++i) {
        if (!m_population[i].isValid()) {
            std::cerr << "ОШИБКА: Невалидная хромосома в популяции на позиции " << i << std::endl;
            allValid = false;
        }
    }
    
    if (allValid) {
        std::cout << "Проверка валидности: Все хромосомы в популяции валидны." << std::endl;
    } else {
        std::cerr << "Проверка валидности: Обнаружены невалидные хромосомы в популяции!" << std::endl;
    }
    
    return allValid;
}

const PlacementChromosome& PlacementGA::tournamentSelection(int k) const {
    if (m_population.empty()) {
        throw std::runtime_error("Популяция пуста");
    }
    
    // Проверка количества участников турнира
    k = std::min(k, static_cast<int>(m_population.size()));
    k = std::max(1, k);
    
    // Выбираем k случайных хромосом из популяции для турнира
    std::vector<std::reference_wrapper<const PlacementChromosome>> tournament;
    tournament.reserve(k);
    
    for (int i = 0; i < k; ++i) {
        int index = m_rng.uniformInt(0, static_cast<int>(m_population.size()) - 1);
        tournament.push_back(std::ref(m_population[index]));
    }
    
    // Находим хромосому с максимальным фитнесом в турнире
    auto it = std::max_element(tournament.begin(), tournament.end(),
                               [](const auto& a, const auto& b) {
                                   return a.get().getFitness() < b.get().getFitness();
                               });
    
    return it->get();
}

PlacementChromosome PlacementGA::evolveWithParams(
    const GAParams& params,
    const std::function<double(PlacementChromosome&)>& fitnessFunction,
    int maxGenerations,
    double targetFitness
) {
    // Настраиваем начальные параметры
    m_populationSize = params.popSize;
    m_crossoverRate = params.crossoverP;
    m_mutationRate = params.mutationP;
    m_tournamentSize = params.tournamentK;
    m_eliteCount = params.eliteCount;
    
    // Инициализируем популяцию
    initializePopulation(fitnessFunction);
        
    // Получаем лучшую хромосому в начальной популяции
    PlacementChromosome bestChromosome = getBestChromosome();
    
    std::cout << "Поколение 0: Лучший фитнес = " << bestChromosome.getFitness() << std::endl;
    
    // Основной цикл генетического алгоритма
    for (m_currentGeneration = 1; m_currentGeneration <= maxGenerations; ++m_currentGeneration) {
        // Создаем новую популяцию
        std::vector<PlacementChromosome> nextPopulation;
        nextPopulation.reserve(m_populationSize);
        
        // 1. Сохраняем элитных особей
        std::partial_sort(m_population.begin(), m_population.begin() + m_eliteCount, m_population.end(),
                        [](const PlacementChromosome& a, const PlacementChromosome& b) {
                            return a.getFitness() > b.getFitness();
                        });
        
        nextPopulation.insert(nextPopulation.end(), m_population.begin(), m_population.begin() + m_eliteCount);
        
        // 2. Заполняем остаток новой популяции через операторы GA
        while (nextPopulation.size() < m_populationSize) {
            // Выбираем двух родителей турнирной селекцией
            const PlacementChromosome& parent1 = tournamentSelection(m_tournamentSize);
            const PlacementChromosome& parent2 = tournamentSelection(m_tournamentSize);
            
            // С вероятностью crossoverP выполняем кроссовер
            PlacementChromosome offspring = 
                (m_rng.uniformReal(0.0, 1.0) < m_crossoverRate) ?
                crossover(parent1, parent2) : PlacementChromosome(parent1);
            
            // С вероятностью mutationP выполняем мутацию
            if (m_rng.uniformReal(0.0, 1.0) < m_mutationRate) {
                mutate(offspring);
            }
            
            // Проверяем и обеспечиваем валидность потомка
            if (!offspring.isValid()) {
                repair(offspring);
                
                // Если по какой-то причине хромосома все еще невалидна,
                // продолжаем цикл без добавления этой особи
                if (!offspring.isValid()) {
                    continue;
                }
            }
            
            // Вычисляем фитнес для нового потомка
            fitnessFunction(offspring);
            
            // Добавляем потомка в новую популяцию
            nextPopulation.push_back(std::move(offspring));
        }
        
        // Заменяем текущую популяцию новой
        m_population = std::move(nextPopulation);
        
        // Сортируем популяцию по убыванию фитнеса
        std::sort(m_population.begin(), m_population.end(),
                [](const PlacementChromosome& a, const PlacementChromosome& b) {
                    return a.getFitness() > b.getFitness();
                });
        
        // Получаем лучшую хромосому текущего поколения
        PlacementChromosome currentBest = m_population[0];
        
        // Выводим информацию о текущем поколении
        std::cout << "Поколение " << m_currentGeneration 
                << ": Лучший фитнес = " << currentBest.getFitness()
                << ", Средний фитнес = " << getAverageFitness() << std::endl;
        
        // Обновляем лучшую хромосому, если нужно
        if (currentBest.getFitness() > bestChromosome.getFitness()) {
            bestChromosome = currentBest;
    }
    
        // Проверяем условие ранней остановки
        if (bestChromosome.getFitness() >= targetFitness) {
            std::cout << "Целевой фитнес достигнут в поколении " 
                    << m_currentGeneration << std::endl;
            break;
        }
    }
    
    std::cout << "Генетический алгоритм завершен." << std::endl;
    std::cout << "Лучший фитнес: " << bestChromosome.getFitness() << std::endl;
    std::cout << "Регенерировано невалидных хромосом: " << m_regeneratedCount << std::endl;
    
    return bestChromosome;
} 