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

typedef struct {
    size_t offset;          // start position of the postings_list in the file
    size_t blocksize;       // block size of the postings_list
    size_t startdoc;        // first docid of the postings_list
    size_t enddoc;          // last docid of the postings_list
    size_t numOfDoc;
} ListMeta;

class IndexBuilder {
    public:
        std::map<std::string, ListMeta> indexmeta;

        IndexBuilder(unsigned short = 0, size_t = 2'000'000, bool = true);
        void buildPList(unsigned int, std::string&);
        void writeTempFile();
        void writeMeta();
        void mergeFile();
        ListMeta getInfo(std::string);
    private:
        unsigned short workerId;
        bool compressed;
        size_t size;
        size_t maxSize;
        size_t tempCount;
        std::string tempfname;
        std::string resultfname;
        std::map<std::string, std::vector<posting>> counter;

        void readTempFile(std::ifstream&, std::queue<postings_list>&);
        void heapPop(std::priority_queue<htype, std::vector<htype>, HeapGreater>&,
            int,
            std::ifstream&,
            std::queue<postings_list>&);
        void writeFile(postings_list&, std::string&);
};

#endif
