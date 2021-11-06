#include "index_reader.h"

#include <fstream>
#include <string>
#include <fstream>

using namespace std;
using namespace IndexReader;
using std::string;

// read metadata of inv list from file
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
        if (metadata.size()) {
            metadata.erase(metadata.end() - 1);
            extractData(metadata, invlist);
        }
    }
}

// helper of IndexReader::readMeta()
void IndexReader::extractData(string& metadata, InvListMeta& invlist) {
    if (!isalpha(metadata[0]))
        return;

    string lexicon = metadata.substr(0, metadata.find(" "));
    static char metalist[65536];
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
}

// read inv list from file
void IndexReader::readSeq(string fname, size_t offset, size_t nbytes, string& data, vector<unsigned>* seq) {
    char readbuf[nbytes];
    ifstream docfile (fname);
    docfile.seekg(offset);
    docfile.read(readbuf, nbytes);
    data = string(readbuf, nbytes);
    if (seq) {
        size_t start = 0, end;
        while (start < data.size() && (end = data.find(",", start)) != -1) {
            seq->push_back(stoul(data.substr(start, end)));
            start = end + 1;
        }
    }
    docfile.close();
}
