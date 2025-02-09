# This file will provide a meaningful makefile somewhere in the future (I hope so)

CXX = g++
CC = gcc
INCLUDE = ./include
CXXFLAGS = -std=c++23 -Wall -Wextra -pedantic -O2
TARGET = lab-exec-node
SOURCES = main.cpp ./Server/Server.cpp  ./Logger/Logger.cpp
all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) -I$(INCLUDE) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET)