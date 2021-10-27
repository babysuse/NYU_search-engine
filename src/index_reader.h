#ifndef INDEX_READER_H
#define INDEX_READER_H

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

    // read metadata of inv list from file
    void readMeta(std::string, InvListMeta&, bool&);
    void extractData(std::string&, InvListMeta&);
    
    // read inv list from file
    // pass NULL to or ignore std::vector<unsigned>* if the index file is in compress form 
    void readSeq(std::string fname, size_t offset, size_t nbytes, std::string& data, std::vector<unsigned>* seq = nullptr);
}

#endif
