HEADERs = tinyxml2.h utility.h trec_reader.h index_builder.h data_compress.h
CPPs = tinyxml2.cpp utility.cpp trec_reader.cpp index_builder.cpp data_compress.cpp
ARGs = -o main -ltidy -lcurl -std=c++17 -g -O3

main: main.cpp $(HEADERs) $(CPPs)
	g++ main.cpp $(CPPs) $(ARGs)
clean:
	rm main DOCNO-ID temp/temp* index.out
