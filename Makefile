CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++20 -O2 -Iinclude -ggdb -ldl
SOURCE = ./Logger/Logger.cpp ./Server/Server.cpp main.cpp 
TARGET = lab-iserver

server: $(SOURCE)
	mkdir -p transport
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) 
	make -C ./modules info
	make -C ./modules segfaulter
	make -C ./modules sleeper
	make -C ./modules tcp
	touch ./transport/manifest.conf
	echo "info.so=1,get_info" >> ./transport/manifest.conf

clear: 
	rm -r transport


server_only: $(SOURCE)
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) 