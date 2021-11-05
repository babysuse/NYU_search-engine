#include "query_processor.h"
#include "trec_reader.h"
#include "data_compress.h"

#include <algorithm>
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

        // separate the terms into subgroup (each of which is conjuctive query)
        auto groups = pr.parseQuery(userInput);
        
        // find & merge candidates in each subgroup
        Candidates candidates;
        for (auto& g : groups) {
            for(auto& str : g) cout << str << " "; cout << endl;
            pr.findCandidates(g, candidates);
        }

        // find top-k result out of candidates
        auto result = pr.findTopK(candidates);
        sort_heap(result.begin(), result.end(), QueryProcessor::comp);

        // output result
        cout << "searching result:" << endl;
        for (auto r = result.begin(); r != result.end(); ++r) {
            pr.printInfo(r->doc);
            cout << "score: " << -(r->score) << endl;
            // TODO: snippet generation
            cout << endl;
        }

        result.clear();
    }
}
