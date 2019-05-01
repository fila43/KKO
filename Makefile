all:kko.h kko.cc main.cpp
	g++ -Wall -Wextra  -std=c++17 -g kko.cc main.cpp -o huff_codec
