#include "trec_reader.h"
#include "tinyxml2.h"
#include <cstddef>
#include <iostream>
#include <regex>

using namespace std;
using namespace tinyxml2;

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
    int start = nextDoc;
    while (trec && (nextDoc = buffer.find("<DOC>", start)) == string::npos) {
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
    string patID = R"(<DOCNO>([\s\S]*?)</DOCNO>)";
    smatch matchID, matchText;
    regex_search(content, matchID, regex(patID));

    string patText = R"(<TEXT>([\s\S]*?)</TEXT>)";
    trecdoc->docid += matchID[1];
    regex_search(content, matchText, regex(patText));
    trecdoc->text += matchText[1];
    return trecdoc;
}
