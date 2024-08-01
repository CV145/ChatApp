CXX = g++
CXXFLAGS = -Wall -std=c++11
LDFLAGS = -lncurses -lpthread

TARGET = server
SRCS = server.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS) $(LDFLAGS)

clean:
	rm -f $(TARGET)