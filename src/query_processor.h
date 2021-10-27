#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "trec_reader.h"
#include "index_reader.h"
#include <iostream>
#include <string>
#include <utility>
#include <vector>

typedef std::pair<unsigned, double> DocRecord;
typedef std::pair<DocRecord, std::vector<std::string>> QueryRecord;

class QueryProcessor {
    public:
        QueryProcessor(unsigned topK = 10);
        std::vector<std::vector<std::string>> parseQuery(std::string);
        void printInfo(unsigned doc);
        void getDocs(const std::string& term, std::vector<unsigned>& doclist);
        void getDocs(const std::string& term, std::string& doclist);
        void getFreqs(const std::string& term, std::vector<unsigned>);
        void getFreqs(const std::string& term, std::string& freqlist);

        void findTopK(const std::vector<std::string>& query, std::vector<QueryRecord>& result);
    private:
        unsigned topK;
        std::string metafile;
        std::string docfile;
        std::string freqfile;
        TRECReader trec;
        IndexReader::InvListMeta invlistmeta;
        std::vector<DocMeta> docmeta;

        std::string findNextTerm(std::string&, size_t&, size_t&);
        bool docExist(
                size_t i,
                std::vector<std::string>& tdoclist,
                std::vector<std::string>& skiplist,
                unsigned docid,
                const std::vector<std::string>& term,
                std::vector<unsigned>& freqSize
            );
};

#endif
