#include "data_compress.h"
#include <string>

using namespace std;
using namespace DataCompress;

string DataCompress::compressText(const string& prev, const string& text) {
    int i = 0;
    for (; i < prev.size() && i < text.size(); ++i)
        if (prev[i] != text[i])
            break;
    if (i > 2)
        return std::to_string(i) + ":" + (i < text.size() ? text.substr(i) : "");
    return text;
}

string DataCompress::decompressText(const string& prev, const string& text) {
    int delimiter = text.find(':');
    if (delimiter == string::npos)
        return text;
    int prelen = stoi(text.substr(0, delimiter));
    return prev.substr(0, prelen) + (delimiter < text.size() - 1 ? text.substr(delimiter + 1) : "");
}

string DataCompress::tovbyte(unsigned int num) {
    string vbyte;
    while (num > 127) {
        vbyte = (char)(num & 127) + vbyte;
        num >>= 7;
    }
    vbyte = (char)num + vbyte;
    *vbyte.rbegin() += 128;
    return vbyte;
}

string DataCompress::toVarBytes(list<unsigned int>& nums, bool sorted, size_t nblocks) {
    unsigned int prev = 0;
    size_t count = 0;
    if (sorted) {
        for (auto& v : nums) {
            if (count % nblocks == 0) {
                prev = v;
            } else {
                v -= prev;
                prev = v + prev;
            }
            ++count;
        }
    }
    string output;
    count = 0;
    for (auto v : nums) {
        if (count && count % nblocks == 0)
            output += ':';
        output += tovbyte(v);
        ++count;
    }
    return output;
}

list<unsigned int> DataCompress::fromvbyte(string block, bool sorted) {
    list<unsigned int> nums;
    size_t i = 0;
    unsigned char b;
    while (i < block.size()) {
        int n = 0;
        while ((b = block[i++]) < 128) {
            n = (n << 7) + b;
        }
        n = (n << 7) + b - 128;
        nums.push_back(n);
    }
    if (sorted) {
        unsigned int prev = 0;
        for (auto& v : nums) {
            v += prev;
            prev = v;
        }
    }
    return nums;
}

list<unsigned int> *DataCompress::fromVarBytes(string dataBlocks, bool sorted) {
    auto nums = new list<unsigned int>;
    size_t prev = 0;
    for (int i = dataBlocks.find(':'); i != string::npos; prev = i + 1, i = dataBlocks.find(':', prev)) {
        for (auto v : fromvbyte(dataBlocks.substr(prev, i - prev), true))
            nums->push_back(v);
    }
    for (auto v : fromvbyte(dataBlocks.substr(prev), true))
        nums->push_back(v);
    return nums;
}

unsigned int DataCompress::getFirstVbyte(std::string dataBlock) {
    unsigned int n = 0;
    size_t i = 0;
    unsigned char b;
    while (i < dataBlock.size() && (b = dataBlock[i++]) < 128) {
        n = (n << 7) + b;
    }
    if (i == dataBlock.size())
        return n;
    return (n << 7) + b - 128;
}
