#pragma once
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>

// Форвард-декларации классов для избежания циклических зависимостей
class PlacementChromosome;
class DecisionChromosome;
class Fleet;

/**
 * @brief Класс для логирования процесса эволюции генетических алгоритмов
 * 
 * Реализован как синглтон для централизованной записи статистики,
 * сохранения и загрузки состояния генетического алгоритма,
 * а также записи топ-50 расстановок в текстовый файл для последующего анализа.
 */
class Logger {
public:
    /**
     * @brief Получение единственного экземпляра класса (Singleton)
     * @return Ссылка на экземпляр Logger
     */
    static Logger& instance();
    
    /**
     * @brief Инициализация логгера
     */
    void open(const std::string& runId = "");
    
    /**
     * @brief Закрытие логгера
     */
    void close();

    /**
     * @brief Записывает произвольное сообщение в лог
     * @param message Сообщение для записи
     */
    void logMessage(const std::string& message);

    /**
     * @brief Логирует информацию о поколении PlacementGA
     * 
     * @tparam ChromVec Тип вектора хромосом (должен иметь PlacementChromosome)
     * @param gen Номер поколения
     * @param bestFit Лучшая приспособленность в поколении
     * @param avgFit Средняя приспособленность поколения
     * @param mutationRate Текущий коэффициент мутации
     * @param top5 Вектор с топ-5 лучшими хромосомами
     */
    template<class ChromVec>
    void logPlacementGen(int gen,
                         double bestFit,
                         double avgFit,
                         double mutationRate,
                         const ChromVec& top5);

    /**
     * @brief Логирует информацию о поколении DecisionGA
     * 
     * @param gen Номер поколения
     * @param bestFit Лучшая приспособленность в поколении
     * @param avgFit Средняя приспособленность поколения
     * @param sigmaNow Текущее значение σ для гауссовой мутации
     */
    void logDecisionGen(int gen,
                        double bestFit,
                        double avgFit,
                        double sigmaNow);
                        
    /**
     * @brief Сохраняет текущее состояние ГА в файл для возможности продолжения эволюции
     * 
     * @tparam ChromVec Тип вектора хромосом
     * @param gen Текущее поколение
     * @param population Текущая популяция хромосом
     * @param mutationRate Текущий коэффициент мутации
     * @param filename Имя файла для сохранения (по умолчанию "ga_state.dat")
     * @return true в случае успешного сохранения, false в случае ошибки
     */
    template<class ChromVec>
    bool saveGAState(int gen, 
                     const ChromVec& population, 
                     double mutationRate,
                     const std::string& filename = "ga_state.dat");
    
    /**
     * @brief Загружает состояние ГА из файла для продолжения эволюции
     * 
     * @tparam ChromVec Тип вектора хромосом
     * @param gen Переменная для сохранения номера поколения
     * @param population Вектор для загрузки популяции
     * @param mutationRate Переменная для загрузки коэффициента мутации
     * @param filename Имя файла для загрузки (по умолчанию "ga_state.dat")
     * @return true в случае успешной загрузки, false в случае ошибки
     */
    template<class ChromVec>
    bool loadGAState(int& gen, 
                     ChromVec& population, 
                     double& mutationRate,
                     const std::string& filename = "ga_state.dat");
                     
    /**
     * @brief Сохраняет итоговые результаты эволюции
     * 
     * @tparam ChromVec Тип вектора хромосом
     * @param topChromosomes Топ-50 (или меньше) хромосом
     * @param bestPerGeneration Лучшие хромосомы по поколениям
     * @param strategyStats Статистика по стратегиям стрельбы
     * @param filename Имя файла для сохранения
     */
    template<class ChromVec>
    void saveEvolutionResults(
        const ChromVec& topChromosomes,
        const std::map<int, typename ChromVec::value_type>& bestPerGeneration,
        const std::map<std::string, double>& strategyStats,
        const std::string& filename);
    
    /**
     * @brief Проверяет наличие файла с состоянием ГА
     * 
     * @param filename Имя файла для проверки
     * @return true если файл существует, false в противном случае
     */
    bool stateFileExists(const std::string& filename = "ga_state.dat") const;
    
private:
    std::ofstream m_file;
    Logger() = default;
    
    /**
     * @brief Преобразует расстановку корабля в ASCII-представление
     * 
     * @param ch Хромосома с расстановкой кораблей
     * @return ASCII-представление доски
     */
    std::string boardAscii(const PlacementChromosome& ch) const;
    
    /**
     * @brief Создает строковое представление генов хромосомы
     * 
     * @tparam Chromosome Тип хромосомы
     * @param ch Хромосома
     * @return Строковое представление генов
     */
    template<typename Chromosome>
    std::string genesAsString(const Chromosome& ch) const;
}; 