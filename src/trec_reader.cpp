#include "trec_reader.h"
#include "tinyxml2.h"
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <memory>
#include <regex>
#include <sstream>
#include <algorithm>
#include <array>

using namespace tinyxml2;
using std::string;
using std::cout;
using std::endl;

TRECReader::TRECReader(string filename): fname(filename) {}

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
    cout << docid << " " << docmeta[docid].offset << " " << docmeta[docid].size << endl;
    char text[length];
    trec.read(text, length);
    content = string(text, length);
}

void TRECReader::writeMeta() {
    std::ofstream trecmeta ("DOCMETA");
    int docid = 0;
    for (const auto& meta : docmeta) {
        trecmeta << docid << "|" << meta.docno << "|" << meta.url << "|" << meta.offset << "|" << meta.size << endl;
        ++docid;
    }
    trecmeta.close();
}

void TRECReader::readMeta(std::vector<DocMeta> &meta) {
    static const size_t SIZE = 65535;
    static char readbuf[SIZE];

    // use sscanf
    std::ifstream trecmeta ("DOCMETA");
    while (trecmeta) {
        trecmeta.getline(readbuf, SIZE);
        int docid;
        char docno[16], url[256];
        size_t offset, length;
        std::sscanf(readbuf, "%d|%[^|]|%[^|]|%ld|%ld", &docid, docno, url, &offset, &length);
        cout << docid << " " << docno << " " << url << " " << offset << " " << length << endl;
    }
    trecmeta.close();
}
