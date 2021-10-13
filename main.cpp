#include "tinyxml2.h"
#include "utility.h"
#include "trec_reader.h"
#include "index_builder.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
using namespace tinyxml2;

int main(int argc, char **argv) {
    TRECReader trec ("dataset/msmarco-docs.trec");
    IndexBuilder builder;
    int i = 0;
    for (TrecDoc *trecdoc = trec.readDoc(); trecdoc; trecdoc = trec.readDoc(), ++i) {
        cout << i << endl;
        builder.buildPList(trecdoc->docid, trecdoc->text);
        delete trecdoc;
    }
    builder.mergeFile();
}
