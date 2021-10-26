#include "utility.h"
#include "trec_reader.h"
#include "index_builder.h"
#include "thread_pool.h"
#include "data_compress.h"

#include <iomanip>
#include <iostream>
#include <fstream>
#include <array>
#include <getopt.h>
#include <string>

using namespace std;
using namespace tinyxml2;

typedef struct {
    bool compress;
    double maxmem;
    unsigned short workers;
    std::string errmsg;
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
                                    "\tAt least 256 (bytes), default is 2GB (shared by all the workers).\n"
                              "-c | --compress <compress>\n"
                                    "\tBoolean value, possible values are 1, 0, true, false.\n\n"
                                    "\tThis option indicates if the data compression is applied. (Disable for easier debugging).\n"
                                    "\tDefault is true.\n"
                              "-w | --worker <workers>\n"
                                    "\tNumeric value, possible values are 1, 0, true, false.\n\n"
                                    "\tThis option determines the number of workers/threads to be used.\n"
                                    "\tEach worker (index builder) generate an result file (index.out), so it can be used to determine\n"
                                    "\tthe size of the result file (index<worker-id>.out), basically each of which would be similar size.\n"
                                    "\tDefault is 64.\n";
                break;
            default:
                args.errmsg = "Usage: %s [-m maxmem] [-c compress]\n"
                              "More info with -h or --help option\n";
        }
    }
}

typedef struct {
    unsigned docid;
    unsigned freq;
} InvNode;

// helper of mergeFile()
void writeFile(std::ofstream& metafile, std::ofstream& docfile, std::ofstream& freqfile, char *lexicon, std::vector<InvNode> invlist, size_t& docOffset, size_t& pairOffset, bool compress) {
    std::vector<unsigned> docIDs, freqs;
    string doclist, metalist, freqlist;
    for (const auto& n : invlist) {
        docIDs.push_back(n.docid);
        freqs.push_back(n.freq);
        if (!compress) {
            doclist += to_string(n.docid) + ",";
            freqlist += to_string(n.freq) + ",";
        }
    }
    unsigned enddoc = docIDs.back();

    if (compress) {
        auto vbDocIdsPair = DataCompress::toVarBytes(docIDs, nullptr, true);
        auto vbFreqsPair = DataCompress::toVarBytes(freqs);
        doclist = vbDocIdsPair.first;
        metalist = vbDocIdsPair.second;
        freqlist = vbFreqsPair.first;
    }

    // use empty line as delimiter for two metalists in case there is 10 (the ascii for '\n') existed
    metafile << lexicon << " " << docIDs.front() << " " << enddoc << " " << docIDs.size() << " " \
        << docOffset << " " << doclist.size() << " " \
        << pairOffset << " " << freqlist.size() << " " \
        << metalist << endl << endl;
    docfile << doclist;
    freqfile << freqlist;

    docOffset += doclist.size();
    pairOffset += freqlist.size();
}

void mergeFile(size_t maxmem, bool compress) {
    std::ofstream metafile ("index/meta.out");
    std::ofstream docfile ("index/doc.out");
    std::ofstream freqfile ("index/freq.out");
    size_t docOffset = 0;
    size_t freqOffset = 0;

    char cmd[256];
    sprintf(cmd, "sort -S %ld -t: -k1,1 --merge temp/temp*", maxmem);
    // -s: --buffer-size
    // -t: --field-separator
    // -k: --key
    std::unique_ptr<FILE, decltype(&pclose)> result(popen(cmd, "r"), pclose);

    metafile << (compress ? "1" : "0") << endl;
    char prev[256] = {};
    std::vector<InvNode> invlist;
    std::array<char, 1024> readbuf;
    // read result from linux sort line by line
    while (fgets(readbuf.data(), readbuf.size(), result.get())) {
        char *input = readbuf.data();
        char *lexicon = strtok(input, ":");

        // write files
        if (strlen(prev) && strcmp(lexicon, prev) != 0) {
            cout << setw(150) << left << prev << '\r' << flush;
            sort(invlist.begin(), invlist.end(), [](InvNode node1, InvNode node2) {
                        return node1.docid < node2.docid;
                    });
            writeFile(metafile, docfile, freqfile, prev, invlist, docOffset, freqOffset, compress);
            invlist.clear();
        }

        // extract docIDs & freqs
        char *data = strtok(NULL, ":");
        while (data) {
            unsigned doc, freq;
            sscanf(data, "(%d,%d)", &doc, &freq);
            invlist.push_back({ doc, freq });
            data = strtok(NULL, ":");
        }
        strcpy(prev, lexicon);
    }
    cout << setw(150) << left << prev << '\r' << endl;
    writeFile(metafile, docfile, freqfile, prev, invlist, docOffset, freqOffset, compress);
    metafile.close();
    docfile.close();
    freqfile.close();
}

int main(int argc, char **argv) {
    // compress, maxmem, workers, errmsg
    Args args { true, 2'000'000'000, 64, "" };
    parseOpt(argc, argv, args);
    if (!args.errmsg.empty()) {
        cerr << args.errmsg << endl;
        exit(1);
    }

    // modify testing file here
    TRECReader trec ("dataset/msmarco-docs.trec.sub");

    // initialize thread pool
    size_t workers = args.workers;
    ThreadPool pool(workers);
    queue<future<void>> handlers;
    vector<IndexBuilder> builders;
    bool done[workers];
    for (int i = 0; i < workers; ++i) {
        builders.emplace_back(i, (size_t)args.maxmem / workers, args.compress);
        done[i] = true;
    }

    // read documents
    int i = 0;
    for (TrecDoc *trecdoc = trec.readDoc(); trecdoc; trecdoc = trec.readDoc(), ++i) {
        handlers.push(pool.enqueue([&builders, &done, i, workers] (TrecDoc *trecdoc) {
            if (trecdoc) {
                cout << setw(10) << i << '\r' << flush;
                int id = i % workers;
                // in order to make sure the order of the docID in temp/result file
                // no race condition since a task is assigned only to one thread
                while (!done[id]);
                done[id] = false;
                builders[id].buildPList(trecdoc->docid, trecdoc->text);
                done[id] = true;
                delete trecdoc;
            }}, trecdoc));
        // only save the task handlers in the last round to save the memory
        if (handlers.size() > workers)
            handlers.pop();
    }
    cout << endl;
    
    // wait for the workers in the thread pool
    while (!handlers.empty()) {
        handlers.front().get();
        handlers.pop();
    }
    // flush cache to tempfile
    for (int i = 0; i < workers; ++i)
        builders[i].writeTempFile();

    //mergeIndex(builders, workers);
    mergeFile(args.maxmem, args.compress);
    
    trec.writeMeta();
}
