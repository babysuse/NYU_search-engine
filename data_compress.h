#ifndef DATA_COMPRESS_H
#define DATA_COMPRESS_H

#include <iostream>
#include <list>

class DataCompress {
    public:
        static std::string compressText(const std::string&, const std::string&);
        static std::string decompressText(const std::string&, const std::string&);
        static std::string toVarBytes(std::list<unsigned int>&, bool = false);
        static std::list<unsigned int> *fromVarBytes(std::string, bool = false);
};

#endif
