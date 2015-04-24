/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <iostream>
#include <cstring>

#include <fds_assert.h>
#include <BitSet.h>

namespace fds {

BitSet::BitSet(size_t n)
{
    fds_verify(n > 0);
    nBits = n;

    // Need to convert the nbits to qword size.
    nQWords = ceil((double)nBits / (double)bitsPerQWord);

    atomicBitVector = new std::atomic<std::uint_fast64_t> [nQWords];

    memset(atomicBitVector, 0, (sizeof(std::uint_fast64_t) * nQWords));

}

BitSet::~BitSet()
{
    delete atomicBitVector;
}

size_t
BitSet::getSize()
{
    return nBits;
}

// Test if the bit is the set on position pos.
bool
BitSet::test(size_t pos) const
{
    size_t offset = (pos >> bitsShift);  // pos / 64
    size_t bitpos = (pos & (bitsPerQWord - 1));  // pos % 64
    uint_fast64_t mask = 1UL << bitpos;

    return (atomicBitVector[offset] & mask);
}

// set a bit in position pos.
bool
BitSet::set(size_t pos)
{
    if (pos >= nBits) {
        return false;
    }

    uint_fast64_t oldVal, newVal;
    size_t offset = (pos >> bitsShift);  // pos / 64
    size_t bitpos = (pos & (bitsPerQWord - 1));  // pos % 64
    uint_fast64_t mask = 1UL << bitpos;

    do {
        oldVal =  atomicBitVector[offset];
        newVal =  atomicBitVector[offset] | mask;
    } while (!std::atomic_compare_exchange_strong(&atomicBitVector[offset], &oldVal, newVal));

    return true;
}

// reset a bit in position pos.
bool
BitSet::reset(size_t pos)
{
    if (pos >= nBits) {
        return false;
    }

    uint_fast64_t oldVal, newVal;
    size_t offset = (pos >> bitsShift);  // pos / 64
    size_t bitpos = (pos & (bitsPerQWord - 1));  // pos % 64
    uint_fast64_t mask = 1UL << bitpos;

    do {
        oldVal =  atomicBitVector[offset];
        newVal =  atomicBitVector[offset] & ~mask;
    } while (!std::atomic_compare_exchange_strong(&atomicBitVector[offset], &oldVal, newVal));

    return true;
}

// Set all bits in the bitset
void
BitSet::setAll()
{
    memset(atomicBitVector, 1, (sizeof(std::uint_fast64_t) * nQWords));
}

// unset all bits in the bitset
void
BitSet::resetAll()
{
    memset(atomicBitVector, 0, (sizeof(std::uint_fast64_t) * nQWords));
}

// output bitset
std::ostream& operator<< (std::ostream &out, const BitSet& bitSet)
{
    for (size_t i = 0; i < bitSet.nQWords; ++i) {
        std::bitset<64>  bits(bitSet.atomicBitVector[i]);
        out << bits << " ";
    }
    out << "\n";

    return out;
}

}  // namespace fds


