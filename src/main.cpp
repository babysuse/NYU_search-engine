#include "trec_reader.h"
#include "data_compress.h"
#include "readIndex.h"

#include <iostream>
#include <vector>
#include <set>

using namespace std;
using namespace DataCompress;
using namespace IndexReader;

int main() {
    InvListMeta invlist;
    bool compress;
    readMeta("index_test/meta.out", invlist, compress);

    string userInput;
    set<string> endcmd { "exit", "quit" };
    while (endcmd.find(userInput) != endcmd.end()) {
        cout << "~$ ";
        cin >> userInput;
        cout << invlist[userInput].startdoc << "-" << invlist[userInput].enddoc << "(" << invlist[userInput].size << ")" << endl;

        // read from docfile
        string rawdata;
        vector<unsigned> docIDs;
        readSeq("index/doc.out", invlist[userInput].docOffset, invlist[userInput].docSize, rawdata, nullptr);
        fromVarBytes(rawdata, docIDs, true);
        cout << "document id: ";
        for (auto id : docIDs)
            cout << id << " ";
        cout << endl;
        
        // read from freqfile
        vector<unsigned> freqs;
        readSeq("index/freq.out", invlist[userInput].freqOffset, invlist[userInput].freqSize, rawdata, nullptr);
        fromVarBytes(rawdata, freqs);
        cout << "frequencies id: ";
        for (auto f : freqs)
            cout << f << " ";
        cout << endl;
    }
}
