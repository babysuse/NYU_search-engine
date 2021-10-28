#include "query_processor.h"
#include "trec_reader.h"
#include "data_compress.h"

#include <iostream>
#include <string>
#include <vector>
#include <set>

using namespace std;

int main() {
    QueryProcessor pr;  // the default K = 10
    string userInput;
    set<string> endcmd { "exit", "quit" };
    while (endcmd.find(userInput) == endcmd.end()) {
        cout << "~$ ";
        getline(cin, userInput);

        // find top-k result from all the queries
        auto groups = pr.parseQuery(userInput);
        Candidates candidates;
        for (auto& g : groups) {
            pr.findCandidates(g, candidates);
        }
        auto result = pr.findTopK(candidates);
        sort_heap(result.begin(), result.end(), [](auto r1, auto r2) {
                    return r1.score < r2.score;
                });

        // output result
        cout << "searching result:" << endl;
        for (auto r = result.rbegin(); r != result.rend(); ++r) {
            pr.printInfo(r->doc);
            cout << "score: " << r->score << endl;
            // TODO: snippet generation
            cout << endl;
        }

        result.clear();
    }
}
