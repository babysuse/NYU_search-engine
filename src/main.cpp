#include "query_processor.h"
#include "trec_reader.h"
#include "data_compress.h"

#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <getopt.h>

using namespace std;

typedef struct {
    unsigned topk;
    unsigned snippetSize;
    std::string errmsg;
} Args;

void parseOpt(int argc, char **argv, Args& args) {
    const struct option options[] = {
        {"topk", required_argument, 0, 'k'},
        {"snippet-size", required_argument, 0, 'S'},
        {NULL, 0, 0, '\0'},
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "k:S:h", options, NULL)) != -1) {
        int pos;
        switch (opt) {
            case 'k':
                args.topk = atoi(optarg);
                if (args.topk < 1) {
                    args.errmsg = "Invalid argument for option topk (at least 1)\n"
                                  "More info with -h or --help option\n";
                }
                break;
            case 'S':
                args.snippetSize = atoi(optarg);
                if (args.snippetSize < 0) {
                    args.errmsg = "Invalid argument for option snippet-size (must be nonnegative)\n"
                                  "More info with -h or --help option\n";;
                }
                break;
            case 'h':
                args.errmsg = "Usage: ./main [-m <maxmem>] [-c <compress>] [-w <workers>]\n"
                              "-k | --topk <topK>\n"
                                    "\tNumeric value, must be at least 1 if specified.\n\n"
                                    "\tThis option indicates number of output ordered by the ranking.\n"
                                    "\tDefault is 10.\n"
                              "-S | --compress <compress>\n"
                                    "\tNumeric value, must be at least 0 if specified\n\n"
                                    "\tThis option indicates the approximate size of the output snippet for each documents returned.\n"
                                    "\tMay be a bit larger in order to display the complete sentences.\n"
                                    "\tDefault is 32.\n";
                break;
            default:
                args.errmsg = "Usage: %s [-k <topK>] [-S <snippet size>]\n"
                              "More info with -h or --help option\n";
        }
    }
}

int main(int argc, char** argv) {
    Args args { 10, 32 };
    parseOpt(argc, argv, args);
    if (!args.errmsg.empty()) {
        cerr << args.errmsg << endl;
        exit(1);
    }

    QueryProcessor pr (args.topk, args.snippetSize);
    string userInput;
    set<string> endcmd { "EXIT", "QUIT" };
    while (true) {
        cout << "~$ ";
        getline(cin, userInput);
        if (endcmd.find(userInput) != endcmd.end())
            break;

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
            pr.printTitle(r->doc);
            cout << pr.generateSnippet(r->doc, candidates[r->doc]) << endl;
            pr.printUrl(r->doc);
            cout << "score: " << -(r->score) << endl;
            cout << endl;
        }

        result.clear();
    }
}
