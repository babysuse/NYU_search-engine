#include "query_processor.h"
#include "data_compress.h"

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

    int start = 0;
    string term = findNextTerm(query, start);
    while (term.size() > 0) {
        if (term.compare("OR") == 0) {
            term = findNextTerm(query, start);
            if (term.size() == 0) {
                cout << "Invalid query" << endl;
                return {};
            }

            // add to new group
            groups.push_back({ term });
        } else {
            if (term.compare("AND") == 0) {
                term = findNextTerm(query, start);
                if (term.size() == 0) {
                    cout << "Invalid query" << endl;
                    return {};
                }
            }
            groups[0].push_back(term);
        }
        term = findNextTerm(query, start);
    }
    return groups;
}

// helper of QueryProcessor::process()
string QueryProcessor::findNextTerm(string& str, int& start) {
    if (start >= str.size())
        return "";

    string term;
    int end = str.find(" ", start);
    if (end == -1) {
        term = str.substr(start);
        start = str.size();
    } else {
        term = str.substr(start, end - start);
        start = end + 1;
    }
    return term;
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
