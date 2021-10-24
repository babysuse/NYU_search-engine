HEADERs = tinyxml2.h utility.h trec_reader.h index_builder.h data_compress.h
CPPs = tinyxml2.cpp utility.cpp trec_reader.cpp index_builder.cpp data_compress.cpp
ARGs = -o main -ltidy -lcurl -std=c++17 -g -O3
DEBUG_ARGs = -o main -ltidy -lcurl -std=c++17 -g

main: main.cpp $(HEADERs) $(CPPs)
	g++ main.cpp $(CPPs) $(ARGs)
test: main.cpp $(HEADERs) $(CPPs)
	g++ main.cpp $(CPPs) $(DEBUG_ARGs)
clean:
	rm main DOCMETA temp/temp* index/index*.out index/index*.meta
init:
	mkdir temp index
