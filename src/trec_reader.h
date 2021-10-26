#ifndef TREC_READER_H
#define TREC_READER_H

#include <iostream>
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
        TrecDoc *readDoc();

        DocMeta getInfo(size_t);
        void getDoc(size_t, std::string&);
        void writeMeta();
        void readMeta(std::vector<DocMeta>&);
    private:
        std::string fname;
        std::string buffer;
        size_t totalSize;

        TrecDoc *parseDoc(std::string);
        std::vector<DocMeta> docmeta;
};

#endif
