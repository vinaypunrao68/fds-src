#include <iostream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <array>
#include <algorithm>
#include <random>
#include "CountingBloomFilter.hpp"

using namespace std;

static const char alphaNum[] =
"0123456789"
"!@#$%^&*"
"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
"abcdefghijklmnopqrstuvwxyz";

static const int stringLength = sizeof(alphaNum) - 1;
static const uint32_t numElems = 100;
static const double FPRate = 0.01;

// stick with lamda = 1.5
// higher the value, steeper the curve...
static double const exp_dist_lamda = 1.5;

class randomWorkload {
  public:
    randomWorkload() :
        uniformDist(0, stringLength),
        expDist(exp_dist_lamda)
    {
        cbf = new countingBloomFilter(numElems, FPRate);
    };

    ~randomWorkload()
    {
    };

    void shuffleHashSet();
    void genHashSet();
    void dumpHashSet();
    void genExpLoad();
    void dumpHashHits();
    void genSimpleLoad();

  private:
    std::string genRandomStr();

    std::random_device randDev;
    std::mt19937 hashGen;
    std::mt19937 expGen;
    std::uniform_int_distribution<uint64_t> uniformDist;
    std::exponential_distribution<double> expDist;

    std::array <uint64_t, numElems> hashSet;
    std::array <uint64_t, numElems> negativeHashSet;
    std::array <uint64_t, numElems> hitCnt;
    std::array <uint64_t, numElems> insertCnt;
    countingBloomFilter *cbf;
};

std::string
randomWorkload::genRandomStr()
{
    std::string str;
    for(unsigned int i = 0; i < 256; ++i)
    {
        str += uniformDist(hashGen);
    }

    // don't want to return a copy, but this is not production code.
    return str;
}

void
randomWorkload::genHashSet()
{
    std::hash<std::string> strHash;

    for (size_t i = 0; i  < hashSet.size(); ++i) {
        hashSet[i] = strHash(genRandomStr());
        negativeHashSet[i] = strHash(genRandomStr());
    }
}

void
randomWorkload::dumpHashSet()
{
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cout << hashSet[i] << endl;
    }
}

void
randomWorkload::dumpHashHits()
{
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cout << hashSet[i] << " : " << insertCnt[i] << endl;
    }
}

void
randomWorkload::shuffleHashSet()
{
    std::random_shuffle(std::begin(hashSet), std::end(hashSet));
}

void
randomWorkload::genExpLoad()
{
    uint32_t foundCnt = 0;
    for (size_t i = 0; i  < 100000; ++i) {
        uint64_t expNum = (uint64_t)(expDist(expGen) * 10);
        if (expNum < numElems) {
            ++insertCnt[expNum];
            cbf->insert(hashSet[expNum]);
        }
    }

    foundCnt = 0;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(hashSet[i]);
        if (found) {
            foundCnt++;
        }
    }
}

void
randomWorkload::genSimpleLoad()
{
    uint32_t foundCnt = 0;
    cbf->reset();

    std::cout << "inserting set of hash values" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cbf->insert(hashSet[i]);
    }

    std::cout << "checking set of hash values: expect hits" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(hashSet[i]);
        if (!found) {
            std::cout << "ERROR: uhuh....  !found" << std::endl;
        }
    }

    std::cout << "checking set of negative hash values: expect no hits (maybe some false positives)" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(negativeHashSet[i]);
        if (found) {
            std::cout << "ERROR: uhuh....  !found" << std::endl;
            foundCnt++;
        }
    }
    std::cout << "found " << foundCnt << " items.  expect " << hashSet.size() * cbf->getFPRate() << std::endl;

    std::cout << "removing set of hash values" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cbf->remove(hashSet[i]);
    }

    std::cout << "checking set of hash values.  expect no hits" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(hashSet[i]);
        if (found) {
            std::cout << "ERROR: uhuh....  found" << std::endl;
        }
    }

    std::cout << "two inserts and one remove on same set of hash values" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cbf->insert(hashSet[i]);
        cbf->insert(hashSet[i]);
        cbf->remove(hashSet[i]);
    }

    std::cout << "expect to find all hash values" << std::endl;
    foundCnt = 0;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(hashSet[i]);
        if (found) {
            foundCnt++;
        }
    }
    if (foundCnt == hashSet.size()) {
        std::cout << "found them all" << std::endl;
    } else {
        std::cout << "uh uh....  take a deep breath..." << std::endl;
    }
    assert(foundCnt == hashSet.size());

    std::cout << "remove the rest" << std::endl;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        cbf->remove(hashSet[i]);
    }

    foundCnt = 0;
    for (size_t i = 0; i < hashSet.size(); ++i) {
        bool found = cbf->mayContain(hashSet[i]);
        if (found) {
            foundCnt++;
        }
    }
    assert(foundCnt == 0);
}

int main()
{

    randomWorkload *hashSet = new randomWorkload();

    cout << "generating" << endl;
    hashSet->genHashSet();
/*
    cout << "shuffling" << endl;
    hashSet->shuffleHashSet();

    hashSet->dumpHashHits();
*/

    hashSet->genSimpleLoad();
    hashSet->genExpLoad();

    return 0;

}

#if 0
bool approximatelyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool essentiallyEqual(float a, float b, float epsilon)
{
    return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyGreaterThan(float a, float b, float epsilon)
{
    return (a - b) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}

bool definitelyLessThan(float a, float b, float epsilon)
{
    return (b - a) > ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
}
#include <cmath>
#include <limits>

bool AreSame(double a, double b) {
    return std::fabs(a - b) < std::numeric_limits<double>::epsilon();
}
#endif
