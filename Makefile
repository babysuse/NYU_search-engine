HEADER = tinyxml2.h utility.h trec_reader.h index_builder.h
CFILE = tinyxml2.cpp utility.cpp trec_reader.cpp index_builder.cpp
main: main.cpp $(HEADER) $(CFILE)
	g++ main.cpp $(CFILE) -o main -ltidy -lcurl -std=c++17
clean:
	rm main DOCNO-ID temp/temp* index.out
