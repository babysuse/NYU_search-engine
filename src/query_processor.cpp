#include "query_processor.h"
#include "data_compress.h"
#include "index_reader.h"
#include <algorithm>
#include <cstddef>

using namespace std;

QueryProcessor::QueryProcessor(unsigned topK):
        topK(topK),
        metafile("index_test/meta.out"),
        docfile("index_test/doc.out"),
        freqfile("index_test/freq.out"),
        trec("dataset/msmarco-docs.trec.sub") {
    // read metadata of invlist (skiplists)
    bool compress;
    readMeta("index_test/meta.out", invlistmeta, compress);

    // read metadata of documents
    trec.readMeta("index_test/DOCMETA", docmeta);
}

vector<vector<string>> QueryProcessor::parseQuery(string query) {
    vector<vector<string>> groups;
    groups.push_back({});

    size_t start = 0;
    size_t end = 0;
    string term = findNextTerm(query, start, end);
    // TODO: add support for parenthesis in query
    while (term.size() > 0) {
        if (term.compare("OR") == 0) {
            start = end;
            term = findNextTerm(query, start, end);
            if (term.size() == 0) {
                cout << "Invalid query" << endl;
                return {};
            }

            // add to new group
            transform(term.begin(), term.end(), term.begin(), ::tolower);
            groups.push_back({ term });
        } else {
            if (term.compare("AND") == 0) {
                start = end;
                term = findNextTerm(query, start, end);
                if (term.size() == 0) {
                    cout << "Invalid query" << endl;
                    return {};
                }
            }
            transform(term.begin(), term.end(), term.begin(), ::tolower);
            if (find(groups[0].begin(), groups[0].end(), term) == groups[0].end())
                groups[0].push_back(term);
        }
        start = end;
        term = findNextTerm(query, start, end);
    }
    return groups;
}

// helper of QueryProcessor::process()
string QueryProcessor::findNextTerm(string& str, size_t& start, size_t& end) {
    if (start == string::npos || (start = str.find_first_not_of(" ", start)) == string::npos)
        return "";

    end = str.find_first_of(" ", start);
    if (end == string::npos) {
        return str.substr(start);
    } else {
        return str.substr(start, end - start);
    }
}

void QueryProcessor::printInfo(unsigned doc) {
    cout << "\t" << doc << " (" << docmeta[doc].docno << ")" << endl
        << "url: " << docmeta[doc].url << endl;
}

void QueryProcessor::getDocs(const string& term, vector<unsigned>& docIDs) {
    string rawdata;
    IndexReader::readSeq(docfile, invlistmeta[term].docOffset, invlistmeta[term].docSize, rawdata, nullptr);
    DataCompress::fromVarBytes(rawdata, docIDs, true);
}

void QueryProcessor::getDocs(const string& term, string& doclist) {
    IndexReader::readSeq(docfile, invlistmeta[term].docOffset, invlistmeta[term].docSize, doclist, nullptr);
}

void QueryProcessor::getFreqs(const string& term, vector<unsigned int> freqs) {
    string rawdata;
    IndexReader::readSeq(freqfile, invlistmeta[term].freqOffset, invlistmeta[term].freqSize, rawdata, nullptr);
    DataCompress::fromVarBytes(rawdata, freqs);
}

void QueryProcessor::getFreqs(const string& term, string& freqlist) {
    IndexReader::readSeq(freqfile, invlistmeta[term].freqOffset, invlistmeta[term].freqSize, freqlist, nullptr);;
}

// for AND/conjunctive terms/query including term1, term2, term3
// term1: doc2, doc3, doc5, doc7, doc11, doc13
// term2: doc1, doc2, doc3, doc5, doc8, doc13, doc21
// term3: doc1, doc3, doc5, doc7, doc9, doc11, doc13
// our candidates are
//      doc3, doc5, doc13
void QueryProcessor::findCandidates(vector<string>& terms, Candidates& candidates) {
    // sort terms according to length of invlist (from shortest to longest)
    sort(terms.begin(), terms.end(), [this] (const string& s1, const string& s2) -> bool {
                return invlistmeta[s1].size < invlistmeta[s2].size;
            });

    // find top-K result (DAAT - Document-At-A-Time)
    // loop through the documents in the first/shortest invlist
    string invlist;
    getDocs(terms[0], invlist);
    vector<unsigned> documents;
    DataCompress::fromVarBytes(invlist, documents, true);
    for (const auto& doc : documents) {
        // check if the doc exists for each queried term (in each invlist)
        docExist(doc, 0, terms, candidates);
    }
}

// TODO: invlist caching using LFU => getDoc(), getFreq()
// helper of findCandidates()
bool QueryProcessor::docExist(unsigned doc, size_t i, const vector<string>& terms, Candidates& candidates) {
    if (i == terms.size())
        return true;

    string& skiplist = invlistmeta[terms[i]].metalist;  // skiplist of the invlist
    string invlist;                                     // invlist of the querying term
    getDocs(terms[i], invlist);
    int pos = DataCompress::numExisted(doc, invlist, skiplist);
    if (pos == -1)
        return false;

    bool result = docExist(doc, i + 1, terms, candidates);
    // add score if
    //      the doc does exist for each queried term (in each invlist)
    //      the score is not calculated/added
    if (result && candidates[doc].find(terms[i]) == candidates[doc].end()) {
        string freqlist;
        getFreqs(terms[i], freqlist);
        unsigned ft = DataCompress::getNumUnsorted(freqlist, pos);
        unsigned Nt = invlistmeta[terms[i]].size;
        candidates[doc][terms[i]] = getScore(make_tuple(doc, ft, Nt));
    }
    return result;
}

SearchResults QueryProcessor::findTopK(const Candidates& candidates) {
    SearchResults result;
    for (auto const& cdoc : candidates) {
        // sum up the score
        double score = 0;
        for (auto const& t : cdoc.second) {
            score += t.second;
        }

        // use min heap to maintain top-k result
        result.emplace_back(cdoc.first, score);
        push_heap(result.begin(), result.end(), QueryProcessor::comp);
        if (result.size() > topK) {
            pop_heap(result.begin(), result.end(), QueryProcessor::comp);
            result.pop_back();
        }
    }
    return result;
}

template <typename ...T>
double QueryProcessor::getScore(tuple<T...> args, Scoring scoring) {
    switch (scoring) {
    case Scoring::BM25:
        unsigned doc;
        double ft, Nt;
        tie (doc, ft, Nt) = args;
        return BM25(doc, ft, Nt);
    case Scoring::COSINE:
        return 0;
    case Scoring::PAGERANK:
        return 0;
    }
}

double QueryProcessor::BM25(unsigned doc, double ft, double Nt) {
    return trec._BM25(ft, Nt, docmeta[doc].size);
}
