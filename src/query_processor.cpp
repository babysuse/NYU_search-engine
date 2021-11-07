#include "query_processor.h"
#include "data_compress.h"
#include "index_reader.h"
#include <algorithm>
#include <limits>
#include <cctype>
#include <cstddef>
#include <set>

using std::string;
using std::vector;
using std::cout;
using std::endl;

QueryProcessor::QueryProcessor(unsigned topK, unsigned maxSnippet, bool debug):
        topK(topK),
        maxSnippet(maxSnippet),
        metafile(debug ? "index_test/meta.out" : "index/meta.out"),
        invlistCache(100),
        freqCache(100),
        docCache(100),
        docfile(debug ? "index_test/doc.out" : "index/doc.out"),
        freqfile(debug ? "index_test/freq.out" : "index/freq.out"),
        trec(debug ? "dataset/msmarco-docs.trec.sub" : "dataset/msmarco-docs.trec") {
    cout << "Loading..." << endl;
    // read metadata of invlist (skiplists)
    bool compress;
    readMeta(metafile, invlistmeta, compress);

    // read metadata of documents
    trec.readMeta(debug ? "index_test/DOCMETA" :"index/DOCMETA", docmeta);
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

vector<vector<string>> QueryProcessor::removeStopWords(const vector<vector<string>>& queries) {
    vector<vector<string>> result;
    double threshold = 0.6;
    while (result.empty()) {
        for (const auto& subquery : queries) {
            vector<string> subresult = _removeStopWords(subquery, threshold);
            if (!subresult.empty())
                result.push_back(subresult);
        }
        threshold += 0.1;
    }
    return result;
}

// helper of QueryProcessor::removeStopWords()
vector<string> QueryProcessor::_removeStopWords(const vector<string>& query, double rate) {
    vector<string> result;
    double threshold = docmeta.size() * rate;
    while (result.empty()) {
        for (auto term : query) {
            if (invlistmeta[term].size < threshold)
                result.push_back(term);
        }
    }
    return result;
}


string QueryProcessor::getTitle(unsigned doc) {
    return docmeta[doc].docno;
}

string QueryProcessor::getUrl(unsigned doc) {
    return docmeta[doc].url;
}

void QueryProcessor::getDocs(const string& term, vector<unsigned>& docIDs) {
    string rawdata;
    getDocs(term, rawdata);
    DataCompress::fromVarBytes(rawdata, docIDs, true);
}

void QueryProcessor::getDocs(const string& term, string& doclist) {
    if (invlistCache.exist(term)) {
        doclist = invlistCache.get(term);
    } else {
        IndexReader::readSeq(docfile, invlistmeta[term].docOffset, invlistmeta[term].docSize, doclist, nullptr);
        invlistCache.set(term, doclist);
    }
}

void QueryProcessor::getFreqs(const string& term, vector<unsigned int>& freqs) {
    string rawdata;
    getFreqs(term, rawdata);
    DataCompress::fromVarBytes(rawdata, freqs);
}

void QueryProcessor::getFreqs(const string& term, string& freqlist) {
    if (freqCache.exist(term)) {
        freqlist = freqCache.get(term);
    } else {
        IndexReader::readSeq(freqfile, invlistmeta[term].freqOffset, invlistmeta[term].freqSize, freqlist, nullptr);;
        freqCache.set(term, freqlist);
    }
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
        candidates[doc][terms[i]] = getScore(std::make_tuple(doc, ft, Nt));
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
double QueryProcessor::getScore(std::tuple<T...> args, Scoring scoring) {
    switch (scoring) {
    case Scoring::BM25:
        unsigned doc;
        double ft, Nt;
        std::tie (doc, ft, Nt) = args;
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

string QueryProcessor::generateSnippet(const unsigned doc, QueryScores scores) {
    string doctext;
    if (docCache.exist(doc)) {
        doctext = docCache.get(doc);
    } else {
        string document;
        trec.getDoc(doc, document, docmeta);
        auto docNode = trec.parseDoc(document);
        doctext = docNode->text;
        docCache.set(doc, doctext);
        delete docNode;
    }
    bool exist = true;
    for (auto s : scores) exist &= bool(doctext.find(s.first) != string::npos);
    if (!exist)
        return "";

    // shift the scores to guarantee positive values
    double minScore = std::numeric_limits<double>::infinity();
    for (const auto s : scores)
        minScore = s.second < minScore ? s.second : minScore;
    if (minScore < 0)
        std::for_each(scores.begin(), scores.end(), [=](auto& s) { s.second += -minScore + 0.01; });

    size_t wordId = 0;
    typedef std::pair<string::iterator, string::iterator> Word;
    std::map<size_t, Word> window;  // queried terms within the distance of maxSnippet
    double currScore = 0;

    string::iterator snippetBegin, snippetEnd;
    snippetBegin = snippetEnd = doctext.begin();
    double maxScore = 0;
    size_t wcount = 0;              // # of queried word contained

    // sliding window algorithm
    for (string::iterator curr = find_if(doctext.begin(), doctext.end(), isalpha), end = find_if_not(curr, doctext.end(), isalpha);
            curr != doctext.end();
            ++wordId, curr = find_if(end, doctext.end(), isalpha), end = std::find_if_not(curr, doctext.end(), isalpha)) {
        string token (curr, end);
        if (scores.find(token) != scores.end()) {
            currScore += scores[token];
            window[wordId] = make_pair(curr, end);
            wcount += 1;
            // check window size
            size_t frontId = window.begin()->first;
            while (wordId - frontId > maxSnippet) {
                auto [begin, end] = window.begin()->second;
                currScore -= scores[string(begin, end)];
                window.erase(frontId);
                wcount -= 1;
                frontId = window.begin()->first;
            }
            if (currScore > maxScore) {
                maxScore = currScore;
                snippetBegin = window.begin()->second.first;
                snippetEnd = window.rbegin()->second.second;
            }
        }
    }

    expandSnippet(doctext, snippetBegin, snippetEnd);
    size_t summarySize = std::distance(doctext.begin(), std::find_if_not(doctext.begin() + 100, doctext.end(), isalpha));
    string summary = doctext.substr(0, summarySize);
    string snippet = string(snippetBegin, snippetEnd);
    while (snippet[0] == ' ') snippet.erase(0, 1);
    snippet = summary + " ... " + snippet + " ...";
    replace(snippet.begin(), snippet.end(), '\n', ' ');
    replace(snippet.begin(), snippet.end(), '\t', ' ');
    return snippet;
}

void QueryProcessor::expandSnippet(const std::string& doctext, std::string::iterator& snippetBegin, std::string::iterator& snippetEnd) {
    size_t extend = 0;
    std::set<char> delimiters {'\n', '\t', '.', ',', '/', ']'};  // delimiters of sentences
    while (snippetBegin >= doctext.begin() && (extend < 10 || delimiters.find(*(snippetBegin - 1)) == delimiters.end())) {
        extend += 1;
        snippetBegin -= 1;
    }
    extend = 0;
    while (snippetEnd < doctext.end() && (extend < 10 || delimiters.find(*(snippetEnd)) == delimiters.end())) {
        extend += 1;
        snippetEnd += 1;
    }
}
