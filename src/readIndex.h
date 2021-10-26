#ifndef READ_INDEX_H
#define READ_INDEX_H

#include <iostream>
#include <map>
#include <vector>

namespace IndexReader {
    typedef struct {
        unsigned startdoc;
        unsigned enddoc;
        size_t size;
        size_t docOffset;
        size_t docSize;
        size_t freqOffset;
        size_t freqSize;
        std::string metalist;
    } MetaNode;
    typedef std::map<std::string, MetaNode> InvListMeta;

    void readMeta(std::string, InvListMeta&, bool&);
    void extractData(std::string&, InvListMeta&);
    
    // if the index file is in compress form, pass NULL to std::vector<unsigned>*
    void readSeq(std::string, size_t, size_t, std::string&, std::vector<unsigned>* = nullptr);
}

#endif
