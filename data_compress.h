#ifndef DATA_COMPRESS_H
#define DATA_COMPRESS_H

#include <iostream>
#include <list>

namespace DataCompress {
    std::string compressText(const std::string&, const std::string&);
    std::string decompressText(const std::string&, const std::string&);
    std::string tovbyte(unsigned int);
    std::string toVarBytes(std::list<unsigned int>&, bool = false, size_t = 200);
    std::list<unsigned int> fromvbyte(std::string, bool = false);
    std::list<unsigned int> *fromVarBytes(std::string, bool = false);
    unsigned int getFirstVbyte(std::string);
};

#endif
