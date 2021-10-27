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
    vector<QueryRecord> result;

    string userInput;
    set<string> endcmd { "exit", "quit" };
    while (endcmd.find(userInput) == endcmd.end()) {
        cout << "~$ ";
        getline(cin, userInput);

        auto groups = pr.parseQuery(userInput);
        for (const auto& g : groups) {
            pr.findTopK(g, result);
        }

        // output result
        sort_heap(result.begin(), result.end());
        for (auto r = result.rbegin(); r != result.rend(); ++r) {
            unsigned doc = r->first.first;
            double score = r->first.second;
            vector<string> query = r->second;

            cout << doc << ":" << endl;
            pr.printInfo(doc);
            // TODO: snippet generation
        }
    }
}
