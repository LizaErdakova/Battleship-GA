cmake_minimum_required(VERSION 3.10)
project(battleship_ga VERSION 1.0)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем опции компиляции для отладки и оптимизации
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -Wall")
else()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2")
    else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
    endif()
endif()

# Включаем поддержку многопоточности
find_package(Threads REQUIRED)

# Проверяем необходимость явного подключения библиотеки filesystem
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    link_libraries(stdc++fs)
endif()

# Исходные файлы проекта
set(SOURCES
    src/main.cpp
    src/models/ship.cpp
    src/models/board.cpp
    src/models/fleet.cpp
    src/utils/io.cpp
    src/utils/rng.cpp
    src/utils/logger.cpp
    src/simulator/game.cpp
    src/strategies/features.cpp
    src/strategies/feature_based_strategy.cpp
    src/strategies/monte_carlo_strategy.cpp
    src/simulator/evaluator.cpp
    src/ga/placement_chromosome.cpp
    src/ga/placement_ga.cpp
    src/ga/decision_ga.cpp
    src/ga/fitness.cpp
    src/ga/placement_generator.cpp
    src/ga/decision_chromosome.cpp
    src/ga/placement_pool.cpp
)

# Заголовочные файлы
include_directories(src)

# Создаем исполняемый файл
add_executable(battleship_ga ${SOURCES})

# Привязываем библиотеку многопоточности
target_link_libraries(battleship_ga PRIVATE Threads::Threads) 

# Для GCC < 9.0 нужно подключить библиотеку filesystem явно
if(CMAKE_COMPILER_IS_GNUCXX AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9.0)
    target_link_libraries(battleship_ga PRIVATE stdc++fs)
endif()

# Создаем директорию для логов во время установки
install(DIRECTORY DESTINATION ${CMAKE_INSTALL_PREFIX}/logs)

# Создаем директорию для логов в рабочей директории
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/logs) 