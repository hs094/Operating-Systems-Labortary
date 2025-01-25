CXX = g++
CXXFLAGS = -std=c++11

.PHONY: all clean

all: producer consumer main

consumer: consumer.cpp 
	$(CXX) $(CXXFLAGS) -o $@ consumer.cpp -lrt

main: main.cpp 
	$(CXX) $(CXXFLAGS) -o $@ main.cpp -lrt

producer: producer.cpp 
	$(CXX) $(CXXFLAGS) -o $@ producer.cpp -lrt

clean:
	rm -f producer consumer main