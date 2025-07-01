CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++20 -Ofast -Iinclude -ggdb -ldl
SOURCE = ./Logger/Logger.cpp ./Server/Server.cpp ./Loader/Loader.cpp main.cpp 
TARGET = lab-iserver

server: $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) 
