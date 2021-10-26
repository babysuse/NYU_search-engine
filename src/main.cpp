#include "query_processor.h"
#include "trec_reader.h"
#include "data_compress.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace std;

int main() {
    QueryProcessor pr;
    vector<unsigned> docIDs, freqs;

    string userInput;
    set<string> endcmd { "exit", "quit" };
    while (endcmd.find(userInput) == endcmd.end()) {
        cout << "~$ ";
        getline(cin, userInput);

        auto groups = pr.process(userInput);
        for (auto g : groups) {
            for (auto s : g) {
                cout << s << " ";
            }
            cout << endl;
        }
        docIDs.clear();
        freqs.clear();
    }
}
