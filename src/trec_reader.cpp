#include "trec_reader.h"
#include <fstream>
#include <regex>
#include <algorithm>
#include <array>
#include <cmath>

using std::string;
using std::cout;
using std::endl;

TRECReader::TRECReader(string filename): fname(filename), totalSize(0), totalDoc(0), aveSize(0) {}

TrecDoc *TRECReader::readDoc() {
    static std::array<char, 1024> readbuf;
    static size_t offset = 0;
    static char cmd[256];

    string doc;
    std::sprintf(cmd, "tail -c +%ld %s | grep -m 1 -B -1 \"</DOC>\"", offset + 1, fname.c_str());
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    while (fgets(readbuf.data(), readbuf.size(), pipe.get())) {
        doc += readbuf.data();
    }

    if (doc.size()) {
        docmeta.push_back({
                "",     // DOCNO
                "",     // url
                offset,
                doc.size(),
                });
        offset += doc.size();
        return parseDoc(doc);
    } else {
        return nullptr;
    }
}

TrecDoc *TRECReader::parseDoc(string content) {
    TrecDoc *trecdoc = new TrecDoc;
    trecdoc->docid = docmeta.size() - 1;
    std::smatch matchID, matchText;
    regex_search(content, matchID, std::regex(R"(<DOCNO>([\s\S]*?)</DOCNO>)"));
    docmeta.back().docno = matchID[1];

    regex_search(content, matchText, std::regex(R"(<TEXT>([\s\S]*?)</TEXT>)"));
    trecdoc->text += matchText[1];
    totalSize += trecdoc->text.size();

    // extract url out of text/content
    int i = trecdoc->text.find("http");
    if (i != string::npos) {
        int j = std::distance(trecdoc->text.begin(), find_if(trecdoc->text.begin() + i, trecdoc->text.end(), isspace));
        docmeta.back().url = trecdoc->text.substr(i, j - i);
        trecdoc->text.erase(i, j - i + 1);
    }
    return trecdoc;
}

DocMeta TRECReader::getInfo(size_t docid) {
    return docmeta[docid];
}

void TRECReader::getDoc(size_t docid, string& content) {
    std::ifstream trec (fname);
    trec.seekg(docmeta[docid].offset);
    size_t length = docmeta[docid].size;
    char text[length];
    trec.read(text, length);
    content = string(text, length);
}

void TRECReader::writeMeta() {
    std::ofstream trecmeta ("DOCMETA");
    int docid = 0;
    trecmeta << docmeta.size() << " " << totalSize / docmeta.size() << endl;
    for (const auto& meta : docmeta) {
        trecmeta << docid << "|" << meta.docno << "|" << meta.url << "|" << meta.offset << "|" << meta.size << endl;
        ++docid;
    }
    trecmeta.close();
}

void TRECReader::readMeta(string fname, std::vector<DocMeta>& meta) {
    static const size_t SIZE = 65535;
    static char readbuf[SIZE];

    // use sscanf
    std::ifstream trecmeta (fname);
    trecmeta.getline(readbuf, SIZE);
    std::sscanf(readbuf, "%d %d", &totalDoc, &aveSize);
    while (trecmeta) {
        trecmeta.getline(readbuf, SIZE);
        if (strlen(readbuf)) {
            int docid;
            char docno[16], url[256];
            size_t offset, size;
            std::sscanf(readbuf, "%d|%[^|]|%[^|]|%ld|%ld", &docid, docno, url, &offset, &size);
            auto& h = meta.emplace_back(string(docno), string(url), offset, size);
        }
    }
    trecmeta.close();
}

/* BM25 score for document d based on query q (containing terms t1, t2, ...)
 * BM25(q, d) = sigma(
 *          (N - Nt + 0.5)     (k1 + 1) * ft
 *      log(--------------) * (-------------)
 *            (Nt + 0.5)          (K + ft)
 * )
 *
 * where
 * N: total number of documents
 * Nt: number of documents containing term t
 * ft: frequency of term t
 * k1: constant, usually 1.2
 *                          Ld
 * K = k1 * ((1 - b) + b * -----)
 *                          Lav
 * where
 * b: constant, usually 0.75
 * Ld: length of document d
 * Lav: average length of documents
 */

double TRECReader::_BM25(std::vector<unsigned> ft, std::vector<unsigned> Nt, unsigned dsize) {
    static const double k1 = 1.2;
    static const double b = 0.75;

    double score = 0;
    for (size_t i = 0; i < ft.size(); ++i) {
        double K = k1 * ((1 - b) + b * dsize / aveSize);
        score += log((totalDoc - Nt[i] + 0.5)/(Nt[i] + 0.5)) * (k1 + 1) * ft[i] / (K + ft[i]);
    }
    return score;
}
