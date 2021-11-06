#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "trec_reader.h"
#include "index_reader.h"
#include "LFU.h"
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

        QueryProcessor(unsigned topK, unsigned maxSnippet, bool debug);
        void printTitle(unsigned doc);
        void printUrl(unsigned doc);
        void getDocs(const std::string& term, std::vector<unsigned>& doclist);
        void getDocs(const std::string& term, std::string& doclist);
        void getFreqs(const std::string& term, std::vector<unsigned>&);
        void getFreqs(const std::string& term, std::string& freqlist);

        std::vector<std::vector<std::string>> parseQuery(std::string);
        std::vector<std::vector<std::string>> removeStopWords(const std::vector<std::vector<std::string>>& queries);
        // find candidates of subquery
        void findCandidates(std::vector<std::string>& query, Candidates& candidates);
        // merge results of subqueries into the final one (using SRF (simple ranking function) BM25)
        SearchResults findTopK(const Candidates& candidates);

        std::string generateSnippet(const unsigned doc, QueryScores scores);
    private:
        unsigned topK;
        unsigned maxSnippet;
        std::string metafile;
        std::string docfile;
        LFU<std::string, std::string> invlistCache;
        std::string freqfile;
        LFU<std::string, std::string> freqCache;
        TRECReader trec;
        IndexReader::InvListMeta invlistmeta;
        std::vector<DocMeta> docmeta;
        LFU<unsigned, std::string> docCache;

        std::string findNextTerm(std::string&, size_t&, size_t&);
        std::vector<std::string> _removeStopWords(const std::vector<std::string>& queries, double threshold);
        // check existence of a document with compress format
        bool docExist(unsigned docid, size_t i, const std::vector<std::string>& terms, Candidates& candidates);

        template <typename ...T>
        double getScore(std::tuple<T...> args, Scoring score = Scoring::BM25);
        /*
         * ft: for each term, the frequency in the document
         * Nt: for each term, the number of documents that contain it
         */
        double BM25(unsigned doc, double ft, double Nt);

        void expandSnippet(const std::string& doctext, std::string::iterator& snippetBegin, std::string::iterator& snippetEnd);
};

#endif
