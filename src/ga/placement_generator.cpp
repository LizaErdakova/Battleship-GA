#include "placement_generator.h"
#include <iostream>

PlacementGenerator::PlacementGenerator(int maxTries)
        : maxTries(maxTries) {}

// Вспомогательная функция для генерации уникального строкового ключа хромосомы
static std::string generateChromosomeKey(const PlacementChromosome& chrom) {
    std::string key;
    for (int gene : chrom.getGenes()) {
        key += std::to_string(gene) + ",";
    }
    return key;
}

static std::pair<int,int> randomXY(int len, bool vert,
                                   RNG& rng, Bias bias, int shipIdx) {
    int x,y;
    switch (bias) {
        case Bias::EDGE: {
            // Выбираем случайный край (0-верх, 1-право, 2-низ, 3-лево)
            int edge = rng.uniformInt(0, 3);
            switch (edge) {
                case 0: // Верхний край
                    y = 0;
                    x = rng.uniformInt(0, 10-(vert?0:len));
                    break;
                case 1: // Правый край
                    x = 10-(vert?0:len);
                    y = rng.uniformInt(0, 10-(vert?len:0));
                    break;
                case 2: // Нижний край
                    y = 10-(vert?len:0);
                    x = rng.uniformInt(0, 10-(vert?0:len));
                    break;
                case 3: // Левый край
                default:
                    x = 0;
                    y = rng.uniformInt(0, 10-(vert?len:0));
                    break;
            }
            break;
        }
        case Bias::CORNER: {
            // Выбираем случайный угол (0-левый верхний, 1-правый верхний, 2-правый нижний, 3-левый нижний)
            int corner = rng.uniformInt(0, 3);
            switch (corner) {
                case 0: // Левый верхний
                    x = rng.uniformInt(0, 1);
                    y = rng.uniformInt(0, 1);
                    break;
                case 1: // Правый верхний
                    x = rng.uniformInt(8, 9);
                    y = rng.uniformInt(0, 1);
                    break;
                case 2: // Правый нижний
                    x = rng.uniformInt(8, 9);
                    y = rng.uniformInt(8, 9);
                    break;
                case 3: // Левый нижний
                default:
                    x = rng.uniformInt(0, 1);
                    y = rng.uniformInt(8, 9);
                    break;
            }
            // Корректируем, если корабль выходит за поле
            if (vert && y+len > 10) y = 10-len;
            if (!vert && x+len > 10) x = 10-len;
            break;
        }
        case Bias::CENTER: {
            // Размещаем в центральной области
            if (shipIdx < 3) { // Самые большие корабли - в центре
                x = rng.uniformInt(3, 6-(vert?0:len-1));
                y = rng.uniformInt(3, 6-(vert?len-1:0));
            } else {
                // Остальные - вокруг центра с небольшим разбросом
                x = rng.uniformInt(2, 7-(vert?0:len-1));
                y = rng.uniformInt(2, 7-(vert?len-1:0));
            }
            break;
        }
        default: { // RANDOM
            // Полностью случайное размещение в пределах поля
            x = rng.uniformInt(0, (vert?9:10-len));
            y = rng.uniformInt(0, (vert?10-len:9));
        }
    }
    return {x,y};
}

bool PlacementGenerator::fits(int x, int y, int len, bool vert,
    const std::vector<std::vector<int>>& g) const {
    // Проверка границ поля
    if (x < 0 || (vert ? x > 9 : x + len - 1 > 9) ||
        y < 0 || (vert ? y + len - 1 > 9 : y > 9)) {
        return false; // Выход за границы
    }
    
    // Проверка каждой клетки корабля и правила no-touch
    for (int i=0; i<len; ++i){
        int xi = x + (!vert?i:0);
        int yi = y + (vert?i:0);
        if (g[yi][xi]) return false;                // занято
        // проверка no-touch по правилу Чебышева (расстояние > 1)
        for(int dy=-1; dy<=1; ++dy)
            for(int dx=-1; dx<=1; ++dx){
                int xx=xi+dx, yy=yi+dy;
                if (xx>=0 && xx<10 && yy>=0 && yy<10 && g[yy][xx]) return false;
            }
    }
    return true;
}

bool PlacementGenerator::placeShip(int len, bool vert,
    std::vector<std::vector<int>>& g, int& outX, int& outY,
    RNG& rng, Bias bias, int shipIdx) const {
    for(int t=0; t<maxTries; ++t){
        auto [x,y] = randomXY(len, vert, rng, bias, shipIdx);
        if (fits(x, y, len, vert, g)){
            // Размещаем корабль в сетке
            for(int i=0; i<len; ++i){
                g[y+(vert?i:0)][x+(!vert?i:0)]=1;
            }
            outX=x; outY=y;
            return true;
        }
        // После первых нескольких неудачных попыток снимаем жёсткий bias
        if (t > maxTries / 2) bias = Bias::RANDOM;
    }
    return false;
}

PlacementChromosome PlacementGenerator::generate(Bias bias, RNG& rng) const {
    std::vector<std::vector<int>> grid(10, std::vector<int>(10,0));
    std::vector<int> genes(PlacementChromosome::GENES_COUNT);
    const int lens[10]={4,3,3,2,2,2,1,1,1,1};
    int geneIdx=0;
    
    for(int s=0; s<10; ++s){
        int len = lens[s];
        bool vert = rng.getBool(0.5); // 50% вероятность вертикального размещения
        int x, y;
        if (!placeShip(len, vert, grid, x, y, rng, bias, s))
            return generate(Bias::RANDOM, rng); // Рестарт всей схемы с RANDOM bias
            
        genes[geneIdx++] = x;
        genes[geneIdx++] = y;
        genes[geneIdx++] = vert ? 0 : 1; // 0=вертикальный, 1=горизонтальный
        
        // Вероятность "расслабления" bias уменьшается с каждым кораблем
        double relaxProb = 0.1 + s * 0.05; // От 10% до 55%
        if (rng.uniformReal(0.0, 1.0) < relaxProb) bias = Bias::RANDOM;
    }
    
    PlacementChromosome chromosome(genes);
    return chromosome;
}

std::vector<PlacementChromosome>
PlacementGenerator::generatePopulation(size_t n, RNG& rng) const {
    std::unordered_set<std::string> seen;
    std::vector<PlacementChromosome> pop;
    pop.reserve(n);
    
    const Bias all[4]={Bias::EDGE, Bias::CORNER, Bias::CENTER, Bias::RANDOM};
    int attempts = 0;
    const int maxAttempts = n * 10; // Ограничение на количество попыток
    
    while(pop.size() < n && attempts < maxAttempts){
        attempts++;
        // Выбираем случайный bias с равномерной вероятностью
        Bias b = all[rng.uniformInt(0,3)];
        
        // Генерируем хромосому
        auto chrom = generate(b, rng);
        
        // Проверяем валидность
        if (!chrom.isValid()) {
            continue; // Пропускаем невалидные хромосомы
        }
        
        // Создаем уникальный ключ и добавляем в популяцию, если такой расстановки еще нет
        if (seen.insert(generateChromosomeKey(chrom)).second) {
            pop.push_back(chrom);
        }
    }
    
    // Если не удалось сгенерировать достаточно уникальных хромосом,
    // дополняем популяцию используя стандартный метод
    int additionalAttempts = 0;
    const int maxAdditionalAttempts = n * 10; // Еще одно ограничение для предотвращения бесконечного цикла
    
    while (pop.size() < n && additionalAttempts < maxAdditionalAttempts) {
        additionalAttempts++;
        std::vector<int> genes = PlacementChromosome::generateValidRandomGenes(rng);
        PlacementChromosome chrom(genes);
        
        // Добавляем в популяцию, только если такой расстановки еще нет
        if (seen.insert(generateChromosomeKey(chrom)).second) {
        pop.push_back(chrom);
        }
    }
    
    // Если после всех попыток у нас все еще недостаточно хромосом,
    // выводим предупреждение и возвращаем то, что есть
    if (pop.size() < n) {
        std::cerr << "Предупреждение: не удалось сгенерировать " << n 
                  << " уникальных расстановок. Сгенерировано только " 
                  << pop.size() << " уникальных расстановок." << std::endl;
    }
    
    return pop;
} 