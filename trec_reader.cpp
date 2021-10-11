#include "trec_reader.h"
#include "tinyxml2.h"
#include <cstddef>
#include <iostream>
#include <iterator>
#include <regex>
#include <sstream>
#include <algorithm>

using namespace tinyxml2;
using std::string;

TRECReader::TRECReader(string filename): nextDoc( 5 ) {
    trec.open(filename);
}

TRECReader::~TRECReader() {
    trec.close();
}

TrecDoc *TRECReader::getDoc() {
    static size_t size = 256;
    static char *readbuf = new char [size];

    memset(readbuf, 0, size);
    trec.read(readbuf, size);
    int nextDoc;
    while (trec && (nextDoc = buffer.find("<DOC>", 5)) == string::npos) {
        buffer.append(readbuf, trec.gcount());
        trec.read(readbuf, size);
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
    
    return doc.size() ? parseDoc(doc) : nullptr;
}

TrecDoc *TRECReader::parseDoc(string content) {
    TrecDoc *trecdoc = new TrecDoc;
    std::smatch matchID, matchText;
    string patID = R"(<DOCNO>([\s\S]*?)</DOCNO>)";
    regex_search(content, matchID, std::regex(patID));
    trecdoc->docid += matchID[1];

    string patText = R"(<TEXT>([\s\S]*?)</TEXT>)";
    regex_search(content, matchText, std::regex(patText));
    trecdoc->text += matchText[1];

    int i = trecdoc->text.find("http");
    if (i != string::npos) {
        int j = std::distance(trecdoc->text.begin(), find_if(trecdoc->text.begin() + i, trecdoc->text.end(), isspace));
        trecdoc->url += trecdoc->text.substr(i, j - i);
        trecdoc->text.erase(i, j + 1);
    }
    return trecdoc;
}
