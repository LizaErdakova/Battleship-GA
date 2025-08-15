CXX = g++
CXXFLAGS = -std=c++17 -Wall -O2 -I./src
LDFLAGS = -pthread

# Исходные файлы
SRCS = src/main.cpp \
       src/utils/rng.cpp \
       src/models/ship.cpp \
       src/models/board.cpp \
       src/models/fleet.cpp

# Объектные файлы
OBJS = $(SRCS:.cpp=.o)

# Имя исполняемого файла
TARGET = battleship_ga.exe

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	del /Q /F $(OBJS) $(TARGET)

.PHONY: all clean 