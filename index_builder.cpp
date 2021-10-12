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

IndexBuilder::IndexBuilder(size_t mem, bool compressing):
    size( 0 ), maxSize( mem / sizeof(posting) ), tempFile( 0 ), compressed( compressing ), resultId( 0 ) {}

size_t IndexBuilder::buildPList(unsigned short docid, string docStr) {
    size_t numOfTerm = 0;
    // parse doc
    for (string::iterator curr = find_if(docStr.begin(), docStr.end(), isalnum), end = std::find_if_not(curr, docStr.end(), isalnum);
            curr != docStr.end();
            ++size, ++numOfTerm, curr = find_if(end, docStr.end(), isalnum), end = std::find_if_not(curr, docStr.end(), isalnum)) {
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
    return numOfTerm;
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
    std::ifstream infiles[tempFile];
    std::queue<postings_list> inbuffs[tempFile];
    std::priority_queue<htype, std::vector<htype>, HeapGreater> minheap;
    // initialize input buffers and the heap
    for (int i = 0; i < tempFile; ++i) {
        infiles[i].open("temp/temp" + std::to_string(i));
        readTempFile(infiles[i], inbuffs[i]);
        minheap.push(std::make_pair(i, inbuffs[i].front()));
        inbuffs[i].pop();
    }
    // merge files
    while (!minheap.empty()) {
        // htype: {buff_id, postings_list}
        htype next = minheap.top();
        int i = next.first;
        heapPop(minheap, i, infiles[i], inbuffs[i]);

        // merge postings_lists that share the same lexicon
        while (!minheap.empty() && next.second.lex.compare( minheap.top().second.lex ) == 0) {
            htype top = minheap.top();
            next.second.postings.merge(top.second.postings, [](posting p1, posting p2) -> bool {
                        return p1.docid < p2.docid;
                    });
            // merge postings that share that same docid
            for (auto it = next.second.postings.begin(); std::next(it) != next.second.postings.end(); ++it) {
                auto itn = std::next(it);
                while (itn != next.second.postings.end() && it->docid == itn->docid) {
                    it->freq += itn->freq;
                    next.second.postings.erase(itn);
                    itn = std::next(it);
                }
            }
            int j = top.first;
            heapPop(minheap, j, infiles[j], inbuffs[j]);
        }

        // write result
        writeFile(next.second);
    }
    tempFile = 0;
}

// helper of IndexBuilder::mergeFile()
void IndexBuilder::readTempFile(std::ifstream& file, std::queue<postings_list>& buff) {
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
            std::sscanf(readbuf.substr(prev + 1, pos - prev - 1).c_str(), "(%u,%u)", &docid, &freq);
            buff.back().postings.push_back( (posting){ docid, freq } );
            prev = pos;
        }
        std::sscanf(readbuf.substr(prev + 1, readbuf.size() - prev - 1).c_str(), "(%u,%u)", &docid, &freq);
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
            IndexBuilder::readTempFile(file, queue);
        }
    }
}

// helper of IndexBuilder::mergeFile()
void IndexBuilder::writeFile(postings_list& plist) {
    string fname = "index-" + std::to_string(resultId) + ".out";
    std::ofstream file (fname, std::ios_base::app);
    std::list<unsigned int> idList;
    std::list<unsigned int> freqList;
    for (posting p : plist.postings) {
        idList.push_back(p.docid);
        freqList.push_back(p.freq);
    }

    size_t fpos = file.tellp();
    if (compressed) {
        string lexCompressed = DataCompress::compressText(currLex, plist.lex);
        currLex = plist.lex;
        string idChunks = DataCompress::toVarBytes(idList, true);
        string freqChunks = DataCompress::toVarBytes(freqList);
        file << lexCompressed << " " << idChunks << " " << freqChunks << endl;
    } else {
        file << plist.lex << " ";
        for (auto id : idList)
            file << id << ",";
        file << " ";
        for (auto freq : freqList)
            file << freq << ",";
        file << endl;
    }
    indexmeta[plist.lex] = {
        resultId,
        fpos,
        (size_t)file.tellp() - fpos,
        *idList.begin(),
        *idList.rbegin(),
        idList.size()
    };
    file.close();
}

ListMeta IndexBuilder::getInfo(string lexicon) {
    if (indexmeta.find(lexicon) != indexmeta.end())
        return indexmeta[lexicon];
    return {};
}
