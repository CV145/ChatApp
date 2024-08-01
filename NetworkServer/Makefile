#compiler and compiler flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2

#executable
TARGET = server

# Define the source files and object files
SRC = server.cpp
OBJ = $(SRC:.cpp=.o)

# Default target to build the executable
all: $(TARGET)

# Rule to build the executable
$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)