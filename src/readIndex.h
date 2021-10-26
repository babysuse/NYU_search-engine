#ifndef READ_INDEX_H
#define READ_INDEX_H

#include <iostream>
#include <map>

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

    void readFile(InvListMeta&, bool&);
    void extractData(std::string&, InvListMeta&);
}

#endif
