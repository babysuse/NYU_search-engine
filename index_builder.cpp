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

IndexBuilder::IndexBuilder(size_t mem): currId( 0 ), maxmem( mem ), tempFile( 0 ) {}

void IndexBuilder::buildPList(string docNo, string docStr) {
    std::fstream file ("DOCNO-ID", std::ios_base::app);
    file << docNo << ":" << currId << std::endl;
    file.close();

    for (string::iterator curr = find_if(docStr.begin(), docStr.end(), isalnum), end = std::find_if_not(curr, docStr.end(), isalnum);
            curr != docStr.end();
            curr = find_if(end, docStr.end(), isalnum), end = std::find_if_not(curr, docStr.end(), isalnum)) {
        string token (curr, end);
        std::transform(token.begin(), token.end(), token.begin(), ::tolower);
        if (counter.find(token) == counter.end() || counter[token].back().docid != currId)
            counter[token].push_back( (posting){ currId, 1 } );
        else
            counter[token].back().freq += 1;
    }
    if (currId % 2) {
        writeTempFile();
    }
    currId += 1;
}

void IndexBuilder::writeTempFile() {
    std::ofstream temp ("temp/temp" + std::to_string(tempFile));
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
    ++tempFile;
}

void IndexBuilder::mergeFile() {
    tempFile = 5;
    std::ifstream infiles[tempFile];
    std::queue<postings_list> inbuffs[tempFile];
    std::priority_queue<htype, std::vector<htype>, HeapGreater> minheap;
    // initialize input buffers and the heap
    for (int i = 0; i < tempFile; ++i) {
        infiles[i].open("temp/temp" + std::to_string(i));
        readFile(infiles[i], inbuffs[i]);
        minheap.push(std::make_pair(i, inbuffs[i].front()));
        inbuffs[i].pop();
    }
    // merge files
    while (!minheap.empty()) {
        htype next = minheap.top();
        int i = next.first;
        heapPop(minheap, i, infiles[i], inbuffs[i]);

        cout << "merging plists" << endl;
        // merge postings_lists that share the same lexicon
        while (!minheap.empty() && next.second.lex.compare( minheap.top().second.lex ) == 0) {
            htype top = minheap.top();
            next.second.postings.merge(top.second.postings, [](posting p1, posting p2) -> bool {
                        return p1.docid < p2.docid;
                    });
            int j = top.first;
            heapPop(minheap, j, infiles[j], inbuffs[j]);
        }

        // write result
        writeFile(next.second);
    }
    tempFile = 0;
}

// helper of IndexBuilder::mergeFile()
void IndexBuilder::readFile(std::ifstream& file, std::queue<postings_list>& buff) {
    const int line = 20;
    string readbuf;
    for (int i = 0; i < line && file; ++i) {
        std::getline(file, readbuf);

        size_t pos = readbuf.find(":");
        buff.push( (postings_list){ "", std::list<posting>() });
        buff.back().lex += readbuf.substr(0, pos);
        size_t prev = pos;
        unsigned int docid, freq;
        while ((pos = readbuf.find(":", prev + 1)) != string::npos) {
            std::sscanf(readbuf.substr(prev + 1, pos - prev - 1).c_str(), "(%hu,%hu)", &docid, &freq);
            buff.back().postings.push_back( (posting){ docid, freq } );
            prev = pos;
        }
        std::sscanf(readbuf.substr(prev + 1, readbuf.size() - prev - 1).c_str(), "(%hu,%hu)", &docid, &freq);
        buff.back().postings.push_back( (posting){ docid, freq } );
        readbuf.clear();
    }
}

// helper of IndexBuilder::mergeFile()
void IndexBuilder::heapPop(std::priority_queue<htype, std::vector<htype>, HeapGreater>& heap,
        int i,
        std::ifstream& file,
        std::queue<postings_list>& queue) {
    heap.pop();
    if (!queue.empty()) {
        postings_list next = queue.front();
        queue.pop();
        heap.push(std::make_pair(i, next));
        
        if (queue.empty() && !file.eof()) {
            IndexBuilder::readFile(file, queue);
        }
    }
}

// helper of IndexBuilder::mergeFile()
void IndexBuilder::writeFile(postings_list& plist) {
    std::ofstream file (RESULT, std::ios_base::app);
    std::list<unsigned int> idList;
    std::list<unsigned int> freqList;
    for (posting p : plist.postings) {
        idList.push_back(p.docid);
        freqList.push_back(p.freq);
    }
    string lexCompressed = DataCompress::compressText(currLex, plist.lex);
    currLex = plist.lex;
    string idChunks = DataCompress::toVarBytes(idList, true);
    string freqChunks = DataCompress::toVarBytes(freqList);
    file << lexCompressed << " " << idChunks << " " << freqChunks << endl;
    file.close();
}
