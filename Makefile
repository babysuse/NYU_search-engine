main: main.cpp utility.h utility.cpp tinyxml2.h tinyxml2.cpp
	g++ main.cpp utility.cpp tinyxml2.cpp -o main -ltidy -lcurl -std=c++17
clean:
	rm main
