#ifndef DATA_COMPRESS_H
#define DATA_COMPRESS_H

#include <iostream>
#include <vector>

namespace DataCompress {
    struct Skiplist {
        unsigned first; // the first number in the block
        size_t count;   // the number of the encoded numbers
        size_t length;  // the size of the encoded numbers
    };
    std::string compressText(const std::string&, const std::string&);
    std::string decompressText(const std::string&, const std::string&);

    std::string tovbyte(unsigned);
    // (varbyte, varbyte_skiplist) is returned, if the sequence is unsorted, varbyte_skiplist is an empty string
    std::pair<std::string, std::string> toVarBytes(std::vector<unsigned>&, std::vector<Skiplist>* = nullptr, bool = false);
    void fromVarBytes(std::string, std::vector<unsigned>&, bool = false, unsigned first = 0);

    void readSkiplist(std::string&, std::vector<Skiplist>&);
    // find number using skiplist without decompress all the vbytes
    int numExisted(unsigned target, std::string& vbytes, std::vector<Skiplist>&);
    int numExisted(unsigned target, std::string& vbytes, std::string& skiplist);
    // get number at specific position
    unsigned getNumUnsorted(const std::string&, int pos);
    unsigned getNumSorted(const std::string&, int pos, std::vector<Skiplist>&);
    unsigned getNumSorted(const std::string&, int pos, std::string& skiplist);
};

#endif
