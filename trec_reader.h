#ifndef TREC_READER_H
#define TREC_READER_H

#include <iostream>
#include <fstream>

class TRECReader {
    public:
        explicit TRECReader(std::string);
        ~TRECReader();
        std::string getDoc();
    private:
        std::ifstream trec;
        std::string buffer;
};

#endif
