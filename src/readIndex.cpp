#include "readIndex.h"

#include <fstream>
#include <string>
#include <fstream>

using namespace std;
using namespace IndexReader;
using std::string;

void IndexReader::readMeta(string fname, InvListMeta& invlist, bool& compress) {
    std::ifstream metafile (fname);
    string readbuf;

    std::getline(metafile, readbuf);
    compress = atoi(readbuf.c_str()) ? true : false;

    while (metafile) {
        string metadata;
        // read until empty line
        std::getline(metafile, readbuf);
        while (readbuf.size()) {
            metadata += readbuf + "\n";
            std::getline(metafile, readbuf);
        }
        cout << metadata.size() << endl;
        if (metadata.size()) {
            metadata.erase(metadata.end() - 1);
            extractData(metadata, invlist);
        }
    }
}

void IndexReader::extractData(string& metadata, InvListMeta& invlist) {
    string lexicon = metadata.substr(0, metadata.find(" "));
    char metalist[1024];
    cout << lexicon << ": " << metadata.substr(lexicon.size() + 1) << endl;
    sscanf(metadata.substr(lexicon.size() + 1).c_str(), "%u %u %lu %lu %lu %lu %lu %s",
            &invlist[lexicon].startdoc,
            &invlist[lexicon].enddoc,
            &invlist[lexicon].size,
            &invlist[lexicon].docOffset,
            &invlist[lexicon].docSize,
            &invlist[lexicon].freqOffset,
            &invlist[lexicon].freqSize,
            metalist);
    invlist[lexicon].metalist = string(metalist);
    cout << lexicon << endl;
}

void IndexReader::readSeq(string fname, size_t offset, size_t length, string& rawdata, vector<unsigned>* seq) {
    char readbuf[length];
    ifstream docfile (fname);
    docfile.seekg(offset);
    docfile.read(readbuf, length);
    rawdata = string(readbuf, length);
    if (seq) {
        size_t start = 0, end;
        while (start < rawdata.size() && (end = rawdata.find(",", start)) != -1) {
            seq->push_back(stoul(rawdata.substr(start, end)));
            start = end + 1;
        }
    }
    docfile.close();
}
