#pragma once

#include "protobuf_mutator/mutator.h"
#include "proto/proto_setting.h"
using namespace std;
using namespace protobuf_mutator;
uint32_t dataNum = 0;
/**
 * @brief limit value into [range[0], range[1]]
 */
template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, T>::type 
clampToRange(const T value, const vector<T>& range) {
    // assert(range[0] <= range[1]);
    // TEST: do not use assert to avoid stopping fuzzing
    if(range[0] > range[1]) return range[0];
    if(range[0] <= value && value <= range[1])
        return value;
    T len = range[1] - range[0] + 1;
    // constrain value to [0, len-1] and then shift this interval by range[0].
    return NotNegMod(value, len) + range[0];
}

template<typename T>
inline typename std::enable_if<std::is_floating_point<T>::value, T>::type
clampToRange(const T value, const vector<T>& range) {
    // TEST: do not use assert to avoid stopping fuzzing
    if(range[0] > range[1]) return range[0];
    if(range[0] <= value && value <= range[1])
        return value;
    T len = range[1] - range[0];
    T integer = floor(value);
    T decimal = value - integer;
    return fmod(fmod(integer, len) + len, len) + range[0] + decimal;
}

/**
 * @brief Reduce a uint32_t value to the largest power of 2 that is less than or equal to it
 * @param value the value to be reduced
 */
inline uint32_t reduceToPowerOfTwo(uint32_t value) {
    if((value & (value - 1)) == 0) return value;
    uint32_t res = 1;
    while (res <= value) {
        auto temp = res;
        res <<= 1;
        // If an overflow occurs, the previous value is the desired maximum power of 2.
        if(res < temp) return temp;
    }
    return res >> 1;
}

/**
 * @brief Increase a uint32_t value to the smallest power of 2 that is greater than or equal to it
 * @param value the exponent value to be increased
 */
inline uint32_t levelUpToPowerOfTwo(uint32_t value) {
    uint32_t cnt = 0, res = 1;
    while(value > res) {
        res <<= 1;
        cnt ++;
    }
    return cnt;
}

/**
 * @brief wrapper for clampToRange(), limit value into [0, dataNum - 1]
 */
template<typename T>
inline typename std::enable_if<std::is_integral<T>::value, T>::type 
apiSrcAndDstClampToRange(const T value) {
    // If there is no data to be computed, the API will not be invoked.
    if(dataNum == 0) return value;
    return clampToRange(value, {0, dataNum - 1});
}

/**
 * @brief A function to do modular exponentiation in O(log(power))
 * @return (base ^ power) % mod
 */
inline uint64_t qPow(uint64_t base, uint64_t power, uint64_t mod){
    uint64_t ans = 1;
    while(power){
        if(power & 1) ans = ans * base % mod;
        base = base * base % mod;
        power >>= 1;
    }
    return ans % mod;
}

/**
 * @brief A function to test whether a number is a strong probable prime in O(klog^3 n)
 * @return true if the number is a strong probable prime number, false otherwise
 */
bool Miller_Rabin(uint64_t num){
    if(num == 2) return true;
    if(!(num & 1) || num < 2) return false;
    uint64_t s = 0, t = num - 1;
    while(!(t & 1)){
        s ++;
        t >>= 1;
    }
    for (int i = 1; i <= 10; i++) {
        uint64_t a = GetRandomIndex((int)1e9) % (num - 1) + 1;
        uint64_t x = qPow(a, t, num);
        for(int j = 1;j <= s;j++){
            uint64_t test = x * x % num;
            if(test == 1 && x != 1 && x != num - 1) return false;
            x = test;
        }
        if(x != 1) return false;
    }
    return true;
}