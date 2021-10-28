#ifndef TREC_READER_H
#define TREC_READER_H

#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

typedef struct {
    size_t docid;
    std::string text;
} TrecDoc ;

typedef struct DocMeta {
    std::string docno;
    std::string url;
    size_t offset;      // start position of a document
    size_t size;        // document size including <DOC></DOC>

    DocMeta(std::string docno, std::string url, size_t offset, size_t size):
        docno(docno), url(url), offset(offset), size(size) {};
} DocMeta;

class TRECReader {
    public:
        explicit TRECReader(std::string);
        TrecDoc *readDoc();

        DocMeta getInfo(size_t);
        void getDoc(size_t, std::string&);
        void writeMeta();
        void readMeta(std::string, std::vector<DocMeta>&);
        /* 
         * ft: for each term, the frequency in the document
         * Nt: for each term, the number of documents that contain it
         */
        double _BM25(std::vector<unsigned> ft, std::vector<unsigned> Nt, unsigned docSize);
    private:
        std::string fname;
        std::string buffer;
        size_t totalSize;
        int totalDoc;
        int aveSize;

        TrecDoc *parseDoc(std::string);
        std::vector<DocMeta> docmeta;
};

#endif
