#ifndef TREC_READER_H
#define TREC_READER_H

#include <iostream>
#include <fstream>

typedef struct {
    std::string docid;
    std::string text;
} TrecDoc ;

class TRECReader {
    public:
        explicit TRECReader(std::string);
        ~TRECReader();
        TrecDoc *getDoc();
    private:
        std::ifstream trec;
        int nextDoc;
        std::string buffer;
        TrecDoc *parseDoc(std::string);
};

#endif
