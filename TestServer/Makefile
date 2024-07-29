# Variables
CXX = g++
CXXFLAGS = -Wall -pthread
TARGET = server
SRCS = server.cpp
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(TARGET)

# Link the target
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS)

# Compile .cpp files to .o files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
