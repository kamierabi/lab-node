CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++20 -O2 -Iinclude -ggdb -ldl -lpthread
SOURCE = ./Logger/Logger.cpp ./Server/Server.cpp ./ThreadPool/ThreadPool.cpp ./Loader/Loader.cpp main.cpp 
TARGET = build/lab-iserver

server: $(SOURCE)
	mkdir build/
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) 
