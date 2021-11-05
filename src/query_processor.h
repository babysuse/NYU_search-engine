#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "trec_reader.h"
#include "index_reader.h"
#include <iostream>
#include <string>
#include <tuple>
#include <vector>
#include <map>

// map the terms to the scores
typedef std::map<std::string, double> QueryScores;
// map the doc to the terms
typedef std::map<unsigned, QueryScores> Candidates;

typedef struct SearchRecord {
    unsigned doc;
    double score;
    SearchRecord(unsigned doc, double score) : doc(doc), score(score) {}
} SearchRecord;
typedef std::vector<SearchRecord> SearchResults;


enum class Scoring {
    BM25,
    COSINE,
    PAGERANK
};

class QueryProcessor {
    public:
        typedef bool (* Compare)(const SearchRecord, const SearchRecord);
        constexpr static const Compare comp = [](SearchRecord r1, SearchRecord r2) { return r1.score < r2.score; };

        QueryProcessor(unsigned topK = 10);
        void printInfo(unsigned doc);
        void getDocs(const std::string& term, std::vector<unsigned>& doclist);
        void getDocs(const std::string& term, std::string& doclist);
        void getFreqs(const std::string& term, std::vector<unsigned>);
        void getFreqs(const std::string& term, std::string& freqlist);

        std::vector<std::vector<std::string>> parseQuery(std::string);
        // find candidates of subquery
        void findCandidates(std::vector<std::string>& query, Candidates& candidates);
        // merge results of subqueries into the final one (using SRF (simple ranking function) BM25)
        SearchResults findTopK(const Candidates& candidates);
    private:
        unsigned topK;
        std::string metafile;
        std::string docfile;
        std::string freqfile;
        TRECReader trec;
        IndexReader::InvListMeta invlistmeta;
        std::vector<DocMeta> docmeta;

        std::string findNextTerm(std::string&, size_t&, size_t&);
        // check existence of a document with compress format
        bool docExist(unsigned docid, size_t i, const std::vector<std::string>& terms, Candidates& candidates);

        template <typename ...T>
        double getScore(std::tuple<T...> args, Scoring score = Scoring::BM25);
        /*
         * ft: for each term, the frequency in the document
         * Nt: for each term, the number of documents that contain it
         */
        double BM25(unsigned doc, double ft, double Nt);
};

#endif
