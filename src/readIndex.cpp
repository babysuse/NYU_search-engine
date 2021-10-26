#include "readIndex.h"
#include <fstream>
#include <string>

using namespace IndexReader;
using std::string;

void IndexReader::readFile(InvListMeta& invlist, bool& compress) {
    std::ifstream metafile ("index/meta.out");
    string readbuf;

    std::getline(metafile, readbuf);
    compress = atoi(readbuf.c_str()) ? true : false;

    while (metafile) {
        string metadata;
        // read until empty line
        std::getline(metafile, readbuf);
        while (readbuf.size()) {
            metadata += readbuf + "\n";
        }
        metadata.erase(metadata.end() - 1);
        extractData(metadata, invlist);
    }
}

void IndexReader::extractData(string& metadata, InvListMeta& invlist) {
    string lexicon = metadata.substr(0, metadata.find(" "));
    char metalist[1024];
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
