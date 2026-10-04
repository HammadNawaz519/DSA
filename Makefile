CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
TARGET = ttdb_server
SRC = ttdb_server.cpp

all: $(TARGET)

$(TARGET): $(SRC)
$(CXX) $(CXXFLAGS) $(SRC) -o $(TARGET)

clean:
rm -f $(TARGET) resolve.bin session.tdbg *.o

.PHONY: all clean
