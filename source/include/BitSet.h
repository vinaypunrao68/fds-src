/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_BITSET_H_
#define SOURCE_INCLUDE_BITSET_H_

#include <cstddef>
#include <cstdint>
#include <cmath>
#include <ostream>
#include <bitset>
#include <atomic>
#include <memory>

/**
 * This is a simple atomic bitset operation
 */

namespace fds {

class BitSet {
  public:
    explicit BitSet(size_t n);
    ~BitSet();

    // Get size of the bitset
    size_t getSize() const;

    // Test if bit on pos is set or not.
    bool test(size_t pos) const;

    // Set and reset bits on pos
    bool set(size_t pos);
    bool reset(size_t pos);

    // set or unset all bits
    void setAll();
    void resetAll();

    friend std::ostream& operator<< (std::ostream&out, const BitSet& bitSet);

  private:
    // shift value for 64bit vector.
    const uint64_t bitsShift = 6;   // 2^6 = 64
    // number of bits for qword (64bits)
    const uint64_t bitsPerQWord = 64;
    // number of bits in bitset.
    size_t nBits;
    // number of QWords corresponding to number of bits in bitset.
    size_t nQWords;
    // atomic vector.
    std::unique_ptr<std::atomic<std::uint_fast64_t> []> atomicBitVector;
}; //  class BitSet

}  // namespace fds

#endif  // SOURCE_INCLUDE_BITSET_H_
