#include "data_compress.h"
#include <string>

using namespace std;

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

string DataCompress::toVarBytes(list<unsigned int>& nums, bool sorted) {
    unsigned int prev = 0;
    if (sorted) {
        for (auto& v : nums) {
            v -= prev;
            prev = v + prev;
        }
    }
    string output;
    for (auto v : nums) {
        string bytes;
        while (v > 127) {
            bytes = (char)(v & 127) + bytes;
            v >>= 7;
        }
        bytes = (char)v + bytes;
        *bytes.rbegin() += 128;
        output += bytes;
    }
    return output;
}

list<unsigned int> *DataCompress::fromVarBytes(string dataBlocks, bool sorted) {
    auto nums = new list<unsigned int>();
    size_t i = 0;
    unsigned char b;
    while (i < dataBlocks.size()) {
        int n = 0;
        while ((b = dataBlocks[i++]) < 128) {
            n = (n << 7) + b;
        }
        n = (n << 7) + b - 128;
        nums->push_back(n);
    }
    if (sorted) {
        unsigned prev = 0;
        for (auto it = nums->begin(); it != nums->end(); ++it) {
            *it += prev;
            prev = *it;
        }
    }
    return nums;
}
