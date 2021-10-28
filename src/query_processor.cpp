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

// TODO: invlist caching using LFU
void QueryProcessor::findCandidates(vector<string>& terms, Candidates& candidates) {
    // sort terms according to length of invlist (from shortest to longest)
    sort(terms.begin(), terms.end(), [this] (const string& s1, const string& s2) -> bool {
                return invlistmeta[s1].size < invlistmeta[s2].size;
            });

    // read invlist & skiplist (compressed)
    vector<string> invlists (terms.size());     // invlists of the querying terms
    vector<string> skiplists (terms.size());    // skiplists of the invlists
    vector<unsigned> numOfDocs (terms.size());  // # of documents containing each terms (for calculating BM25)
    size_t i = 0;
    for (const auto& t : terms) {
        getDocs(t, invlists[i]);
        skiplists[i] = invlistmeta[t].metalist;
        numOfDocs[i] = invlistmeta[t].size;
        ++i;
    }
    string numlist = DataCompress::toVarBytes(numOfDocs).first;

    // find top-K result (DAAT - Document-At-A-Time)
    // loop through the documents in the first/shortest invlist
    i = 0;
    vector<unsigned> documents;
    DataCompress::fromVarBytes(invlists[0], documents, true);
    for (const auto& doc : documents) {
        string freqlist;
        getFreqs(terms[0], freqlist);
        unsigned f = DataCompress::getNumUnsorted(freqlist, i++);
        vector<unsigned> freqs {f};
        if (docExist(1, invlists, skiplists, doc, terms, freqs)) {
            // we cannot filter out any document here, since each of it may have much higher score in other subqueries or in the end
            if (candidates.find(doc) == candidates.end()) {
                candidates[doc] = {
                    terms,
                    DataCompress::toVarBytes(freqs).first,
                    numlist
                };
            } else {
                mergeCandidate(candidates[doc], {
                    terms,
                    DataCompress::toVarBytes(freqs).first,
                    numlist
                });
            }
        }
        freqs.clear();
    }
}

// helper of findCandidates()
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

// helper of findCandidates()
void QueryProcessor::mergeCandidate(_Candidate& cand, const _Candidate& newCand) {
    vector<unsigned> freqs;
    DataCompress::fromVarBytes(cand.freqlist, freqs);
    vector<unsigned> numOfDocs;
    DataCompress::fromVarBytes(cand.numOfDocs, numOfDocs);
    for (auto i = 0; i < newCand.subquery.size(); ++i) {
        if (find(cand.subquery.begin(), cand.subquery.end(), newCand.subquery[i]) == cand.subquery.end()) {
            cand.subquery.push_back(newCand.subquery[i]);
            freqs.push_back(DataCompress::getNumUnsorted(newCand.freqlist, i));
            numOfDocs.push_back(DataCompress::getNumUnsorted(newCand.numOfDocs, i));
        }
    }
    cand.freqlist = DataCompress::toVarBytes(freqs).first;
    cand.numOfDocs = DataCompress::toVarBytes(numOfDocs).first;
}

SearchResults QueryProcessor::findTopK(const Candidates& candidates) {
    SearchResults result;
    for (auto const& r : candidates) {
        // use max heap to maintain top-k result
        result.push_back({ r.first, BM25(r.first, r.second.freqlist, r.second.numOfDocs)});
        push_heap(result.begin(), result.end(), [](auto r1, auto r2) {
                    return r1.score < r2.score;
                });
        if (result.size() > topK) {
            pop_heap(result.begin(), result.end(), [](auto r1, auto r2) {
                    return r1.score < r2.score;
                });
            result.pop_back();
        }
    }
    return result;
}

double QueryProcessor::BM25(unsigned int doc, std::string ft, std::string Nt) {
    vector<unsigned> freqlist;  // for each term, the occurrence of it in the document
    vector<unsigned> numOfDocs; // for each term, the number of documents that contain it
    DataCompress::fromVarBytes(ft, freqlist);
    DataCompress::fromVarBytes(Nt, numOfDocs);
    return trec._BM25(freqlist, numOfDocs, docmeta[doc].size);
}
