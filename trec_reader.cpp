#include "trec_reader.h"

using namespace std;

TRECReader::TRECReader(string filename) {
    trec.open(filename);
}

TRECReader::~TRECReader() {
    trec.close();
}

string TRECReader::getDoc() {
    if (trec) {
        static size_t size = 256;
        static char *readbuf = new char [size];
        memset(readbuf, 0, size);
        
        trec.read(readbuf, size);
        while (trec && buffer.rfind("</DOC>") == string::npos) {
            buffer.append(readbuf);
            trec.read(readbuf, size);
        }
        if (!trec)
            delete[] readbuf;
    }
        
    string doc;
    int end = buffer.rfind("</DOC>");
    if (end > 0) {
        doc += buffer.substr(0, end + 6);
        buffer.erase(0, end + 6);
    } else {
        doc += buffer;
        buffer.erase();
    }
    
    return doc;
}
