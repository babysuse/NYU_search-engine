#include "query_processor.h"
#include "data_compress.h"
#include <algorithm>
#include <cctype>
#include <cstddef>

using namespace std;

QueryProcessor::QueryProcessor() {
    // read metadata of invlist (skiplists)
    bool compress;
    readMeta("index_test/meta.out", invlist, compress);

    // read metadata of documents
    TRECReader trec ("dataset/msmarco-docs.trec.sub");
    trec.readMeta("index_test/DOCMETA", docmeta);
}

vector<vector<string>> QueryProcessor::process(string query) {
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

void QueryProcessor::printInfo(string term) {
    cout << invlist[term].startdoc << "-" << invlist[term].enddoc << "(" << invlist[term].size << ")" << endl;
}

void QueryProcessor::getDocs(string term, vector<unsigned> docIDs) {
    string rawdata;
    IndexReader::readSeq("index/doc.out", invlist[term].docOffset, invlist[term].docSize, rawdata, nullptr);
    DataCompress::fromVarBytes(rawdata, docIDs, true);
    cout << "document id: ";
    for (auto id : docIDs)
        cout << id << " ";
    cout << endl;

}

void QueryProcessor::getFreqs(string term, vector<unsigned int> freqs) {
    string rawdata;
    IndexReader::readSeq("index/freq.out", invlist[term].freqOffset, invlist[term].freqSize, rawdata, nullptr);
    DataCompress::fromVarBytes(rawdata, freqs);
    cout << "frequencies id: ";
    for (auto f : freqs)
        cout << f << " ";
    cout << endl;
}
