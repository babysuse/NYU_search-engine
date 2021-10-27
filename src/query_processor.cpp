#include "query_processor.h"
#include "data_compress.h"
#include "index_reader.h"
#include <algorithm>
#include <utility>

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
    cout << "\t" << docmeta[doc].docno << " " << docmeta[doc].url << endl;
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

// TODO: invlist caching using LFU
void QueryProcessor::findTopK(const vector<string>& terms, vector<QueryRecord>& result) {
    // from terms according to length of invlist (from shortest to longest)
    sort(terms.begin(), terms.end(), [&] (string s1, string s2) {
                return invlistmeta[s1].size < invlistmeta[s2].size;
            });

    // read invlist & skiplist (compressed)
    vector<string> invlists (terms.size());     // invlists that contain the terms
    vector<string> skiplists (terms.size());    // skiplists for the invlists
    vector<unsigned> numOfDocs (terms.size());  // for each term, the number of documents that contain it (for calculating BM25)
    size_t i = 0;
    for (const auto& t : terms) {
        getDocs(t, invlists[i]);
        skiplists[i] = invlistmeta[t].metalist;
        numOfDocs[i] = invlistmeta[t].size;
        ++i;
    }

    // find top-K result (DAAT - Document-At-A-Time)
    vector<unsigned> candidates;
    vector<unsigned> freqs;
    DataCompress::fromVarBytes(invlists[0], candidates, true);
    for (const auto& doc : candidates) {
        if (docExist(1, invlists, skiplists, doc, terms, freqs)) {
            result.emplace_back(
                make_pair(doc, trec.BM25(freqs, numOfDocs, docmeta[doc].size)),
                terms
            );
            // maintain top-K result based on BM25 score
            push_heap(result.begin(), result.end(), [](QueryRecord r1, QueryRecord r2) {
                        return r1.first.second < r2.first.second;
                    });
            if (result.size() == topK) {
                pop_heap(result.begin(), result.end());
                result.pop_back();
            }
        }
    }
}

bool QueryProcessor::docExist(size_t i, vector<string>& tdoclist, vector<string>& skiplist, unsigned doc, const vector<string>& terms, vector<unsigned>& freqs) {
    if (i == tdoclist.size())
        return true;
    int pos = DataCompress::numExisted(doc, tdoclist[i], skiplist[i]);
    if (pos == -1) {
        freqs.clear();
        return false;
    }
    string freqlist;
    getFreqs(terms[i], freqlist);
    freqs.push_back(DataCompress::getNumUnsorted(freqlist, pos));
    return docExist(i + 1, tdoclist, skiplist, doc, terms, freqs);
}
