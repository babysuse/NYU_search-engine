#ifndef INDEX_BUILDER_H
#define INDEX_BUILDER_H

#include "data_compress.h"

#include <iostream>
#include <climits>
#include <fstream>
#include <queue>
#include <list>
#include <map>

typedef struct {
    unsigned int docid;
    unsigned int freq;
} posting;

typedef struct {
    std::string lex;
    std::list<posting> postings;
} postings_list;

typedef std::pair<int, postings_list> htype;

class HeapGreater {
    public:
        bool operator() (htype p1, htype p2) {
            return p1.second.lex.compare( p2.second.lex ) >= 1;
        }
};

class IndexBuilder {
    public:
        explicit IndexBuilder(size_t = UINT_MAX);
        void buildPList(std::string, std::string);
        void writeTempFile();
        void mergeFile();
    private:
        unsigned int currId;
        size_t maxmem;
        size_t tempFile;
        const std::string RESULT = "index.out";
        std::map<std::string, std::vector<posting> > counter;
        std::string currLex;

        void readFile(std::ifstream&, std::queue<postings_list>&);
        void heapPop(std::priority_queue<htype, std::vector<htype>, HeapGreater>&,
                int,
                std::ifstream&,
                std::queue<postings_list>&);
        void mergePList(std::list<posting>&, std::list<posting>&);
        void writeFile(postings_list&);
};

#endif
