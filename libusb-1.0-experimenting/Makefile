CC=g++
CXXFLAGS=-std=c++17 -Wall -Wextra -Wpedantic
INCLFLAGS=-I/usr/include/ -I/usr/local/include/libusb-1.0/
LDFLAGS=-ldl -lusb-1.0
CXXNAMES=$(wildcard *.cpp)
OUT=main

all:
	$(CC) $(INCLFLAGS) $(CXXFLAGS) $(CXXNAMES) -o $(OUT) $(LDFLAGS)

clean:
	rm -f $(OUT)

optimize:
	$(CC) $(INCLFLAGS) $(CXXFLAGS) -O3 $(CXXNAMES) -o $(OUT) $(LDFLAGS)
