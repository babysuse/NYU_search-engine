#include "index_builder.h"

#include <cstdio>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <utility>
#include <string>

using std::string;
using std::cout;
using std::endl;

IndexBuilder::IndexBuilder(unsigned short workerId, size_t mem, bool compressing):
        workerId( workerId ), compressed( compressing), tempCount( 0 ), size( 0 ), maxSize( mem / sizeof(posting) ) {
    tempfname = "temp/temp" + std::to_string(workerId);
}

void IndexBuilder::buildPList(unsigned int docid, string& docStr) {
    // parse doc
    for (string::iterator curr = find_if(docStr.begin(), docStr.end(), isalpha), end = std::find_if_not(curr, docStr.end(), isalpha);
            curr != docStr.end();
            ++size, curr = find_if(end, docStr.end(), isalpha), end = std::find_if_not(curr, docStr.end(), isalpha)) {
        string token (curr, end);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (counter.find(token) == counter.end() || counter[token].back().docid != docid)
            counter[token].push_back( (posting){ docid, 1 } );
        else
            counter[token].back().freq += 1;
        // write temp file to release memory
        if (size >= maxSize) {
            writeTempFile();
            size = 0;
        }
    }
}

void IndexBuilder::writeTempFile() {
    if (counter.empty())
        return;
    std::ofstream temp (tempfname + "-" + std::to_string(tempCount));
    for (auto& lex : counter) {
        if (lex.second.size()) {
            char wdata[20];
            string postings;
            for (posting& p : lex.second) {
                sprintf(wdata, ":(%d,%d)", p.docid, p.freq);
                postings.append(wdata);
            }
            temp << lex.first << postings << std::endl;
            lex.second.clear();
        }
    }
    temp.close();
    ++tempCount;
}
