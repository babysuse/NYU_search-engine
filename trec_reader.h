#ifndef TREC_READER_H
#define TREC_READER_H

#include <iostream>
#include <fstream>
#include <vector>

typedef struct {
    size_t docid;
    std::string text;
} TrecDoc ;

typedef struct {
    std::string docno;
    std::string url;
    size_t offset;      // start position of a document
    size_t size;        // document size including <DOC></DOC>
} DocMeta;

class TRECReader {
    public:
        explicit TRECReader(std::string);
        ~TRECReader();
        TrecDoc *readDoc();

        DocMeta getInfo(size_t);
        std::string getDoc(size_t);
        void writeMeta();
    private:
        std::ifstream trec;
        std::string buffer;
        // size_t docid;   // we don't need this since it is represented with the index of the docmeta

        TrecDoc *parseDoc(std::string);
        std::vector<DocMeta> docmeta;
};

#endif
