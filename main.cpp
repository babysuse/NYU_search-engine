#include "tinyxml2.h"
#include "utility.h"
#include "trec_reader.h"
#include "index_builder.h"
#include "thread_pool.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include <getopt.h>

using namespace std;
using namespace tinyxml2;

typedef struct {
    bool compress;
    double maxmem;
    unsigned short workers;
    string errmsg;
} Args;

void parseOpt(int argc, char **argv, Args& args) {
    const struct option options[] = {
        {"mem", required_argument, 0, 'm'},
        {"compress", required_argument, 0, 'c'},
        {"worker", required_argument, 0, 'w'},
        {"help", no_argument, 0, 'h'},
        {NULL, 0, 0, '\0'},
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "m:c:w:h", options, NULL)) != -1) {
        int pos;
        char *unit;
        switch (opt) {
            case 'm':
                args.maxmem = strtof(optarg, &unit);
                if (unit && (unit[0] | 32) == 'k')
                    args.maxmem *= 1'000;
                else if (unit && (unit[0] | 32) == 'm')
                    args.maxmem *= 1'000'000;
                else if (unit && (unit[0] | 32) == 'g')
                    args.maxmem *= 1'000'000'000;
                if (args.maxmem < 256) {
                    args.errmsg = "Invalid argument for option mem (at least 256 bytes)\n"
                                  "More info with -h or --help option\n";
                }
                break;
            case 'c':
                if (strcmp(optarg, "true") == 0 || strcmp(optarg, "1") == 0) {
                    args.compress = true;
                } else if (strcmp(optarg, "false") == 0 || strcmp(optarg, "1") == 0) {
                    args.compress = false;
                } else {
                    args.errmsg = "Invalid argument for option compress\n"
                                  "More info with -h or --help option\n";
                }
                break;
            case 'w':
                args.workers = atoi(optarg);
                if (args.workers == 0) {
                    args.errmsg = "Invalid argument for option worder\n"
                                  "More info with -h or --help option\n";;
                }
                break;
            case 'h':
                args.errmsg = "Usage: ./main [-m <maxmem>] [-c <compress>] [-w <workers>]\n"
                              "-m | --mem <maxmem>\n"
                                    "\tNumeric value, could have case-insensitive suffix k, m, g with no space.\n\n"
                                    "\tThis option indicates the cache size for posting lists before writing into temporary file,\n"
                                    "\tOn the other hand, it indicates the size of the temporary files.\n"
                                    "\tAt least 256 (bytes), default is 2MB.\n"
                              "-c | --compress <compress>\n"
                                    "\tBoolean value, possible values are 1, 0, true, false.\n\n"
                                    "\tThis option indicates if the data compression is applied. (Disable for easier debugging).\n"
                                    "\tDefault is true.\n"
                              "-w | --worker <workers>\n"
                                    "\tNumeric value, possible values are 1, 0, true, false.\n\n"
                                    "\tThis option determines the number of workers/threads to be used.\n"
                                    "\tEach worker (index builder) generate an result file (index.out), so it can be used to determine\n"
                                    "\tthe size of the result file (index<worker-id>.out), basically each of which would be similar size.\n"
                                    "\tDefault is 3.\n";
                break;
            default:
                args.errmsg = "Usage: %s [-m maxmem] [-c compress]\n"
                              "More info with -h or --help option\n";
        }
    }
}

void flushCache(vector<IndexBuilder>& builders, size_t workers) {
    for (int i = 0; i < workers; ++i)
        builders[i].writeTempFile();
}

void mergeIndex(vector<IndexBuilder>& builders, size_t workers) {
    for (int i = 0; i < workers; ++i)
        builders[i].mergeFile();
}

int main(int argc, char **argv) {
    Args args { true, 2'000'000, 3, "" };
    parseOpt(argc, argv, args);
    if (!args.errmsg.empty()) {
        cerr << args.errmsg << endl;
        exit(1);
    }
    // modify testing file here
    TRECReader trec ("dataset/msmarco-docs.trec.sub");
    size_t workers = args.workers;
    ThreadPool pool(workers);
    queue<future<void>> handlers;
    vector<IndexBuilder> builders;
    bool done[workers];
    for (int i = 0; i < workers; ++i) {
        builders.emplace_back(i, (size_t)args.maxmem, args.compress);
        done[i] = true;
    }

    int i = 0;
    for (TrecDoc *trecdoc = trec.readDoc(); trecdoc; trecdoc = trec.readDoc(), ++i) {
        cout << i << endl;
        handlers.push(pool.enqueue([&builders, &done, i, workers] (TrecDoc *trecdoc) {
                if (trecdoc) {
                    int id = i % workers;
                    while (!done[id]);  // in order to make sure the order of the docID in temp/result file
                    done[id] = false;
                    builders[id].buildPList(trecdoc->docid, trecdoc->text);
                    done[id] = true;
                    delete trecdoc;
                }}, trecdoc));
        // only save the task handlers in the last round to save the memory
        if (handlers.size() > workers)
            handlers.pop();
    }
    // wait for the workers in the thread pool
    while (!handlers.empty()) {
        handlers.front().get();
        handlers.pop();
    }
    flushCache(builders, workers);
    mergeIndex(builders, workers);
    trec.writeMeta();
}
