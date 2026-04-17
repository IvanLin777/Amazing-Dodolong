CXX ?= g++
CXXFLAGS ?= -std=c++11 -O2 -Wall -Wextra
TARGET := snake_game_cpp
SRC := snake_game.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)
