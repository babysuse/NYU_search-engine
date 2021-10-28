#ifndef QUERY_PROCESSOR_H
#define QUERY_PROCESSOR_H

#include "trec_reader.h"
#include "index_reader.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>

typedef struct {
    std::vector<std::string> subquery;
    std::string freqlist;
    std::string numOfDocs;
} _Candidate;
typedef std::map<unsigned, _Candidate> Candidates;

typedef struct {
    unsigned doc;
    double score;
} _SearchRecord;
typedef std::vector<_SearchRecord> SearchResults;

class QueryProcessor {
    public:
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
        bool docExist(
                size_t i,
                std::vector<std::string>& tdoclist,
                std::vector<std::string>& skiplist,
                unsigned docid,
                const std::vector<std::string>& term,
                std::vector<unsigned>& freqSize
            );

        // merge two results from two subqueries that share the same documenet
        // for example:
        //      query: (dog AND cat) OR (dog AND rabbit)
        //   subquery: (dog AND cat) produces documents 79 80
        //             candidate[80] => {dog, cat}
        //   subquery: (dog AND rabbit) produces documents 80 81
        //             candidate[80] => {dog, rabbit}
        //   we want candidate[80] => {dog, cat, rabbit}, including the freqlist & numOfDocs
        void mergeCandidate(_Candidate& q1, const _Candidate& q2);
        /*
         * ft: for each term, the frequency in the document
         * Nt: for each term, the number of documents that contain it
         */
        double BM25(unsigned doc, std::string ft, std::string Nt);
};

#endif
