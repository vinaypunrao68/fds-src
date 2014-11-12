#include <cmath>
#include <algorithm>
#include <iostream>
#include <assert.h>
#include <atomic>

using namespace std;

#define BITS_PER_BYTE 8
#define MAX_NFUNC 10
#define PREFETCH_STRIDE 6
typedef uint64_t hashIdx [MAX_NFUNC];

class countingBloomFilter {
  public:
    countingBloomFilter(uint64_t n, double p) {
        this->n = n;
        this->p = p;
        this->bitsPerBin = sizeof(uint16_t) * BITS_PER_BYTE;
        // Should be byte boundary per bin
        assert((bitsPerBin % 8) == 0);

        static_assert(sizeof(char16_t) == sizeof(uint16_t), "size doesn't match");

        this->m = ceil((n * log(p)) / log(1.0 / (pow(2.0, log(2.0)))));
        this->k = round(log(2.0) * m / n);
        this->totalMem = ceil((m * bitsPerBin) / BITS_PER_BYTE);
        //this->vector = (uint16_t *)malloc(sizeof(uint16_t) * m);
        this->atomicVector = new std::atomic<std::int_fast16_t> [m];
    }
    ~countingBloomFilter() {
        delete this->atomicVector;
    }

    double getFPRate() {
        return this->p;
    }

    void printConfig();
    void hashFunc(uint64_t uniqId, hashIdx& hashIdxs);
    void inline prefetchBinsReadWrite(hashIdx& hashIdxs);
    void inline prefetchBinsRead(hashIdx& hashIdxs);
    void insert(uint64_t uniqId);
    void undoInsert(uint32_t k, hashIdx &hashIdxs);
    void remove(uint64_t uniqId);
    void undoRemove(uint32_t k, hashIdx &hashIdxs);
    bool mayContain(uint64_t uniqId);
    void timeDecayFunc();
    void reset();

  private:
    uint64_t n; // number of items in the filter
    double   p; // false positive probability double (0.00 .. 1.00)
    uint32_t k; // number of hash functions
    uint64_t m; // number of bins in the filter
    uint32_t bitsPerBin;
    uint64_t totalMem;
    int16_t *vector; // TODO(sean)... this should be generic.  hardcoded to
    std::atomic<std::int_fast16_t> *atomicVector;
};


void
countingBloomFilter::printConfig()
{
    std::cout << "Counting Bloom Filter Config" << std::endl;
    std::cout << "n (number of items) = " << n << std::endl;
    std::cout << "p (false positive rate) = " << p << std::endl;
    std::cout << "k (number of hash functions) = " << k << std::endl;
    std::cout << "m (number of bin) = " << m << std::endl;
    std::cout << "bitsPerBin = " << bitsPerBin << std::endl;
    std::cout << "totalMem = " << totalMem << " bytes" << std::endl;
}


void
countingBloomFilter::reset()
{
    assert(atomicVector != NULL);

    // TODO
    // This needs to be atomic. But, was fighting c++ for 30 mins before giving up.
    for (uint32_t i = 0; i < m; i++) {
        atomicVector[i] = 0;
    }
}

void
countingBloomFilter::hashFunc(uint64_t const uniqId, hashIdx &hashIdxs)
{
    uint32_t *hash = (uint32_t *)&uniqId;
    uint64_t bitPos;

    // double-hashing to simulate k independent hashes
    for ( uint32_t k = 0; k < this->k; ++k ) {
        // bit for k-th hash
        bitPos = (hash[0] + (k * hash[1])) % this->m;
        hashIdxs[k] = bitPos;
    }
}

void inline
countingBloomFilter::prefetchBinsReadWrite(hashIdx& hashIdxs)
{
    for ( uint32_t k = 0; k < this->k; ++k ) {
        __builtin_prefetch(&atomicVector[hashIdxs[k]], 1, 1);
    }
}

void inline
countingBloomFilter::prefetchBinsRead(hashIdx& hashIdxs)
{
    for ( uint32_t k = 0; k < this->k; ++k ) {
        __builtin_prefetch(&atomicVector[hashIdxs[k]], 0, 1);
    }
}

void
countingBloomFilter::undoInsert(uint32_t numUndo, hashIdx &hashIdxs)
{
    int_fast16_t oldVal, newVal;

    while (numUndo) {
        std::cout << "undo pos(k=" << numUndo - 1 << ") = " << std::dec << hashIdxs[numUndo - 1] << std::endl;
        do {
            oldVal = atomicVector[hashIdxs[numUndo - 1]];
            newVal = oldVal - 1;
        } while (!atomic_compare_exchange_strong(&atomicVector[hashIdxs[numUndo - 1]], &oldVal, newVal));
        --numUndo;
    }
}

void
countingBloomFilter::insert(uint64_t uniqId)
{
    hashIdx hashIdxs;
    int_fast16_t oldVal, newVal;
    uint32_t k;  // scope has to be scope wide.
    uint32_t numInserted = 0;

    hashFunc(uniqId, hashIdxs);

    prefetchBinsReadWrite(hashIdxs);

    for (k = 0; k < this->k; ++k ) {
        do {
            oldVal = atomicVector[hashIdxs[k]];
            if (oldVal == UINT16_MAX) {
                std::cout << "XXX: overflow" << std::endl;
                undoInsert(numInserted, hashIdxs);
                return;
            }
            newVal = oldVal + 1;
        } while (!atomic_compare_exchange_strong(&atomicVector[hashIdxs[k]], &oldVal, newVal));

        ++numInserted;
    }
}

void
countingBloomFilter::remove(uint64_t uniqId)
{
    hashIdx hashIdxs;
    int_fast16_t oldVal, newVal;
    uint32_t k;  // scope has to be scope wide.
    uint32_t numRemoved = 0;

    hashFunc(uniqId, hashIdxs);

    prefetchBinsReadWrite(hashIdxs);

    for (k = 0; k < this->k; ++k ) {
        do {
            oldVal = atomicVector[hashIdxs[k]];
            newVal = oldVal - 1;
            if (newVal == UINT16_MAX) {
                std::cout << "XXX: underflow" << std::endl;
                undoRemove(numRemoved, hashIdxs);
                return;
            }
        } while (!atomic_compare_exchange_strong(&atomicVector[hashIdxs[k]], &oldVal, newVal));

        ++numRemoved;
    }

}

void
countingBloomFilter::undoRemove(uint32_t numUndo, hashIdx &hashIdxs)
{
    int_fast16_t oldVal, newVal;
    while (numUndo) {
        std::cout << "undo pos(k=" << numUndo - 1 << ") = " << std::dec << hashIdxs[numUndo - 1] << std::endl;
        do {
            oldVal = atomicVector[hashIdxs[numUndo - 1]];
            newVal = oldVal + 1;
        } while (!atomic_compare_exchange_strong(&atomicVector[hashIdxs[numUndo - 1]], &oldVal, newVal));
        --numUndo;
    }
}

bool
countingBloomFilter::mayContain(uint64_t uniqId)
{
    hashIdx hashIdxs;
    uint32_t hits = 0;

    hashFunc(uniqId, hashIdxs);

    prefetchBinsRead(hashIdxs);

    for ( uint32_t k = 0; k < this->k; ++k ) {
        if (atomicVector[hashIdxs[k]] > 0) {
            ++hits;
        }
    }
    return (hits == this->k);
}

// TODO (Sean)
// This can potentially be *VERY* expensive call.
// We may need to do some sort of sliding window to minimize memory bus saturation on the system.
void
countingBloomFilter::timeDecayFunc()
{



}

#if 0
int
main(int argc, char *argv[])
{
    countingBloomFilter *cbf = new countingBloomFilter(100, 0.01);

    cbf->printConfig();
    cbf->insert(0x0123456789abcdef);
    cbf->insert(0x0123456789abcdef);
    bool found = cbf->mayContain(0x0123456789abcdef);

    std::cout << "found = " << found << std::endl;

    return 0;
}

#endif


