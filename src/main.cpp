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
    bool debug;
    std::string errmsg;
} Args;

void parseOpt(int argc, char **argv, Args& args) {
    const struct option options[] = {
        {"topk", required_argument, 0, 'k'},
        {"snippet-size", required_argument, 0, 'S'},
        {"debug", no_argument, 0, 'd'},
        {"help", no_argument, 0, 'h'},
        {NULL, 0, 0, '\0'},
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "k:S:dh", options, NULL)) != -1) {
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
             case 'd':
                args.debug = true;
                break;
            case 'h':
                args.errmsg = "Usage: ./main [-m <maxmem>] [-c <compress>] [-w <workers>]\n"
                              "-k | --topk <topK>\n"
                                    "\tNumeric value, must be at least 1 if specified.\n\n"
                                    "\tThis option indicates number of output ordered by the ranking.\n"
                                    "\tDefault is 10.\n"
                              "-S | --snippet-size <compress>\n"
                                    "\tNumeric value, must be at least 0 if specified\n\n"
                                    "\tThis option indicates the approximate size of the output snippet for each documents returned.\n"
                                    "\tMay be a bit larger in order to display the complete sentences.\n"
                                    "\tDefault is 32.\n"
                              "-b | --debug\n"
                                    "\tIf this option is given, the program use the dataset for test/debug in the directory index_test.\n"
                                    "\tThe orignal dataset contains about 3M documents, while the testing dataset is the subset of it,\n"
                                    "\twhich is 30K documents.\n"
                              "-h | --help\n"
                                    "\tShow this page to get an idea of the how program works.";
                break;
            default:
                args.errmsg = "Usage: %s [-k <topK>] [-S <snippet size>]\n"
                              "More info with -h or --help option\n";
        }
    }
}

int main(int argc, char** argv) {
    Args args { 10, 32, false };
    parseOpt(argc, argv, args);
    if (!args.errmsg.empty()) {
        cerr << args.errmsg << endl;
        exit(1);
    }

    QueryProcessor pr (args.topk * 2, args.snippetSize, args.debug);
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
            pr.findCandidates(g, candidates);
        }

        // find top-k result out of candidates
        auto result = pr.findTopK(candidates);
        sort_heap(result.begin(), result.end(), QueryProcessor::comp);

        // output result
        cout << "searching result:" << endl;
        int count = 0;
        for (auto r = result.begin(); count < args.topk && r != result.end(); ++r) {
            string snippet = pr.generateSnippet(r->doc, candidates[r->doc]);
            if (snippet.empty()) continue;
            ++count;
            cout << r->doc << " (" << pr.getTitle(r->doc) << ")" << endl;
            cout << "Url: " << pr.getUrl(r->doc) << endl;
            cout << "Score: " << -(r->score) << endl;
            if (args.snippetSize) cout << "\t" << snippet << endl;
            cout << endl;
        }

        cout << endl;
        result.clear();
    }
}
