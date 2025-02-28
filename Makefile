CXX = g++
CXXFLAGS = -Wall -Wextra -pedantic -std=c++20 -O1 -Iinclude 
SOURCE = ./Logger/Logger.cpp ./Server/Server.cpp main.cpp 
TARGET = lab-iserver

server: $(SOURCE)
	mkdir -p transport
	$(CXX) $(CXXFLAGS) $(SOURCE) -o $(TARGET) 
	make -C ./modules info
	make -C ./modules segfaulter
	make -C ./modules sleeper
	make -C ./modules serial
	make -C ./modules tcp
	touch ./transport/manifest.conf
	echo "info.so=1\nsleeper.so=2\nsegfaulter.so=3\nserial.so=4\ntcp.so=5" >> ./transport/manifest.conf

clear: 
	rm -r transport