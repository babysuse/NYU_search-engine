#include "trec_reader.h"
#include "tinyxml2.h"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <algorithm>

using namespace tinyxml2;
using std::string;
using std::endl;

TRECReader::TRECReader(string filename) {
    trec.open(filename);
}

TRECReader::~TRECReader() {
    trec.close();
}

TrecDoc *TRECReader::readDoc() {
    static size_t size = 2'000'000;
    static char *readbuf = new char [size];
    memset(readbuf, 0, size);

    int nextDoc = buffer.find("<DOC>", 5);
    size_t fpos = (size_t)trec.tellg() - buffer.size() + (nextDoc == string::npos ? 0 : nextDoc);
    while (trec && nextDoc == string::npos) {
        trec.read(readbuf, size);
        buffer.append(readbuf, trec.gcount());
        nextDoc = buffer.find("<DOC>", 5);
    }
        
    string doc;
    if (trec) {
        doc += buffer.substr(0, nextDoc);
        buffer.erase(0, nextDoc);
    } else {
        doc += buffer;
        buffer.erase();
        delete[] readbuf;
    }
    
    if (doc.size()) {
        docmeta.push_back({
            "",     // DOCNO
            "",     // url
            fpos,
            doc.size(),
        });
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
        trecdoc->text.erase(i, j + 1);
    }
    return trecdoc;
}

DocMeta TRECReader::getInfo(size_t docid) {
    return docmeta[docid];
}

string TRECReader::getDoc(size_t docid) {
    trec.seekg(docmeta[docid].offset);
    size_t length = docmeta[docid].size;
    char text[length + 1];
    trec.read(text, docmeta.size());
    text[length] = '\0';
    return string(text);
}

void TRECReader::writeMeta() {
    std::ofstream trecmeta ("DOCMETA");
    for (const auto& meta : docmeta) {
        trecmeta << docmeta.size() - 1 << ":" << meta.docno << ":" << meta.url << ":" << meta.offset << ":" << meta.size << endl;
    }
    trecmeta.close();
}
