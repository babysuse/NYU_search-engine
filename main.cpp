#include "tinyxml2.h"
#include "utility.h"
#include "trec_reader.h"
#include "index_builder.h"
#include "thread_pool.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
using namespace tinyxml2;

void flushCache(vector<IndexBuilder>& builders, size_t workers) {
    for (int i = 0; i < workers; ++i)
        builders[i].writeTempFile();
}

void mergeIndex(vector<IndexBuilder>& builders, size_t workers) {
    for (int i = 0; i < workers; ++i)
        builders[i].mergeFile();
}

int main(int argc, char **argv) {
    TRECReader trec ("dataset/msmarco-docs.trec");

    size_t workers = 3;
    ThreadPool pool(workers);
    queue<future<void>> handlers;
    vector<IndexBuilder> builders;
    bool done[workers];
    for (int i = 0; i < workers; ++i) {
        builders.emplace_back(i);
        done[i] = true;
    }

    int i = 0;
    for (TrecDoc *trecdoc = trec.readDoc(); trecdoc; trecdoc = trec.readDoc(), ++i) {
        cout << i << endl;
        // builders[0].buildPList(trecdoc->docid, trecdoc->text);
        //*
        handlers.push(pool.enqueue([&builders, &done, i, workers] (TrecDoc *trecdoc) {
                if (trecdoc) {
                    // cout << "assign " << trecdoc->docid << " to " << i % workers << endl;
                    int id = i % workers;
                    while (!done[id]);
                    done[id] = false;
                    builders[id].buildPList(trecdoc->docid, trecdoc->text);
                    done[id] = true;
                    delete trecdoc;
                    trecdoc = nullptr;
                }}, trecdoc));
        if (handlers.size() > workers)
            handlers.pop();
        // */
    }
    
    while (!handlers.empty()) {
        handlers.front().get();
        handlers.pop();
    }
    flushCache(builders, workers);
    mergeIndex(builders, workers);
    trec.writeMeta();
}
