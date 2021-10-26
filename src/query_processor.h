#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "trec_reader.h"
#include "index_reader.h"
#include <iostream>
#include <string>
#include <vector>

class QueryProcessor {
    public:
        QueryProcessor();
        std::vector<std::vector<std::string>> process(std::string);

        void printInfo(std::string);
        void getDocs(std::string, std::vector<unsigned>);
        void getFreqs(std::string, std::vector<unsigned>);
    private:
        IndexReader::InvListMeta invlist;
        std::vector<DocMeta> docmeta;

        std::string findNextTerm(std::string&, int&);
};

#endif
