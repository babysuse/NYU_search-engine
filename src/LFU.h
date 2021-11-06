#ifndef LFU_H
#define LFU_H

#include <map>
#include <vector>
#include <queue>
#include <iostream>

template<typename KType, typename VType>
class LFU {
    public:

        explicit LFU(int maxSize);

        void set(KType key, VType value);
        VType get(KType);
        bool exist(KType);
    private:
        int maxSize;
        int minFreq;
        std::vector<std::map<KType, VType>> cache;
        std::map<KType, int> keyFreq;
};

template<typename KType, typename VType>
LFU<KType, VType>::LFU(int maxSize)
        : maxSize(maxSize), minFreq(0) {
    cache.push_back({});
}

template<typename KType, typename VType>
void LFU<KType, VType>::set(KType key, VType value) {
    if (keyFreq.find(key) != keyFreq.end()) {
        int freq = keyFreq[key]++;
        // remove from the cache of old freq
        cache[freq].erase(key);
        if (cache[freq].empty())
            ++minFreq; // append to the cache of new freq
        if (freq + 1 == cache.size())
            cache.push_back({});
        cache[freq + 1][key] = value;
        return;
    }
        
    minFreq = 0;
    keyFreq[key] = 0;
    cache[0][key] = value;

    // remove the least frequently used key
    if (keyFreq.size() > maxSize) {
        KType key = cache[minFreq].begin()->first;
        keyFreq.erase(key);
        cache[minFreq].erase(key);
        while (cache[minFreq].empty()) {
            ++minFreq;
        }
    }
}

template<typename KType, typename VType>
VType LFU<KType, VType>::get(KType key) {
    if (keyFreq.find(key) == keyFreq.end())
        return {};

    int freq = keyFreq[key]++;
    auto value = cache[freq][key];
    // remove from the cache of old freq
    cache[freq].erase(key);
    if (cache[freq].empty())
        ++minFreq;
    // append to the cache of new freq
    if (freq + 1 == cache.size())
        cache.push_back({});
    cache[freq + 1][key] = value;
    return value;
}

template<typename KType, typename VType>
bool LFU<KType, VType>::exist(KType key) {
    return keyFreq.find(key) != keyFreq.end();
}

#endif
