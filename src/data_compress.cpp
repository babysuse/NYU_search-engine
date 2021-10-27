#include "data_compress.h"
#include <algorithm>
#include <cstddef>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

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

string DataCompress::tovbyte(unsigned num) {
    string vbyte;
    while (num > 127) {
        vbyte = (char)(num & 127) + vbyte;
        num >>= 7;
    }
    vbyte = (char)num + vbyte;
    *vbyte.rbegin() += 128;
    return vbyte;
}

/*
 * skiplist stores
 * * the fist number in the block
 * * the number of encoded number in the block
 * * the size of encoded number in the block
 *
 * "block" is the unit to be skipped in the search algorithm,
 * blocksize grows exponentially in order to achieve O(log N) time for forward search.
 * 
 * If the input sequence is unsorted, then the skiplist is pointless.
 */
std::pair<string, string> DataCompress::toVarBytes(vector<unsigned>& nums, std::vector<Skiplist>* skiplist, bool sorted) {
    string output;              // encoded nums

    string metalist;            // encoded skiplist
    unsigned prev = 0;          // used to take differences out of sorted sequence
    unsigned first = 0;
    unsigned blocksize = 2;     // number to be encoded in this block
    unsigned blocklength = 0;   // accumulated encoded size
    unsigned count = 0;

    for (auto& v : nums) {
        if (sorted && skiplist) {
            ++count;
            if (count == 1)
                first = v;
            v -= prev;
            prev = v + prev;
        }

        output += tovbyte(v);

        // put it after the update of the output in order to correctly calculate the current blocksize
        if (count == blocksize) {
            size_t length = output.size() - blocklength;
            blocklength += length;
            skiplist->push_back({ first, count, length });

            metalist += tovbyte(first);
            metalist += tovbyte(count);
            metalist += tovbyte(length);
            blocksize *= 2;
            count = 0;
        }
    }

    // write the rest to metalist
    if (sorted && count) {
        size_t length = output.size() - blocklength;
        skiplist->push_back({ first, count, length });
        metalist += tovbyte(first);
        metalist += tovbyte(count);
        metalist += tovbyte(length);
    }
    return make_pair(output, metalist);
}

void DataCompress::fromVarBytes(string vbytes, vector<unsigned>& result, bool sorted, unsigned first) {
    size_t i = 0;
    unsigned char b;
    while (i < vbytes.size()) {
        int n = 0;
        while ((b = vbytes[i++]) < 128) {
            n = (n << 7) + b;
        }
        n = (n << 7) + b - 128;
        result.push_back(n);
    }
    if (sorted) {
        unsigned prev = 0;
        // if first is specified, which indicates only block/vbytes in the middle is retreived, use the first instead
        if (first > 0)
            result[0] = first;
        for (auto& v : result) {
            v += prev;
            prev = v;
        }
    }
}

void DataCompress::readSkiplist(string& metalist, vector<Skiplist>& skiplist) {
    vector<unsigned> nums;
    fromVarBytes(metalist, nums);
    for (int i = 0; i < nums.size(); i += 3) {
        skiplist.push_back({ nums[i], nums[i + 1], nums[i + 2] });
    }
}

int DataCompress::numExisted(unsigned target, string& vbytes, std::vector<Skiplist>& skiplist) {
    unsigned first = 0;     // first number in the previous block
    size_t num = 0;         // # of numbers encoded in the previous block
    size_t size = 0;        // size of the previous block
    size_t numSkipped = 0;
    size_t sizeSkipped = 0;
    for (const auto& b : skiplist) {
        // if first number is larger, our target locates in the previous block
        if (target < b.first)
            break;
        numSkipped += num;
        sizeSkipped += size;
        first = b.first;
        num = b.count;
        size = b.length;
    }

    // find number in the block
    vector<unsigned> nums;
    fromVarBytes(vbytes.substr(numSkipped, size), nums, true, first);
    for (auto v : nums)
        cout << v << " ";
    cout << endl;
    auto it = lower_bound(nums.begin(), nums.end(), target);
    if (it == nums.end() || *it != target)
        return -1;
    // return the overall position/index (0-based)
    return numSkipped + distance(nums.begin(), it);
}

int DataCompress::numExisted(unsigned target, std::string& vbytes, std::string& skiplist) {
    vector<Skiplist> sklist;
    readSkiplist(skiplist, sklist);
    return numExisted(target, vbytes, sklist);
}

unsigned DataCompress::getNumUnsorted(std::string& vbytes, int pos) {
    size_t i = 0;
    unsigned char byte;
    while (pos > 0) {
        byte = vbytes[i];
        if (byte > 128)
            pos -= 1;
        ++i;
    }

    // decode and return the number
    unsigned n = 0;
    byte = vbytes[i];
    while ((byte = vbytes[i]) < 128) {
        n = (n << 7) + byte;
    }
    n = (n << 7) + byte - 128;
    return n;
}

unsigned DataCompress::getNumSorted(std::string &vbytes, int pos, std::vector<Skiplist> &skiplist) {
    size_t i = 0;
    size_t blocksize = 2;
    size_t sizeSkipped = 0;
    while (pos >= blocksize) {
        sizeSkipped += skiplist[i].length;
        pos -= blocksize;
        blocksize *= 2;
        i += 1;
    }
    
    vector<unsigned> nums;
    fromVarBytes(vbytes.substr(sizeSkipped, skiplist[i].length), nums, true, skiplist[i].first);
    return nums[pos];
}

unsigned DataCompress::getNumSorted(std::string& vbytes, int pos, std::string& skiplist) {
    vector<Skiplist> sklist;
    readSkiplist(skiplist, sklist);
    return getNumSorted(vbytes, pos, sklist);
}
