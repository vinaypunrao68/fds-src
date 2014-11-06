/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdint>
#include <cstdio>
#include <ctime>
#include <bitset>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include <google/profiler.h>
#include "boost/smart_ptr/make_shared.hpp"

#include <fds_types.h>
#include <blob/BlobTypes.h>
#include <cache/SharedKvCache.h>

using namespace fds;    // NOLINT

static const size_t TEN =                   10;
static const size_t HUNDRED =               TEN * TEN;
static const size_t THOUSAND =              TEN * HUNDRED;
static const size_t MILLION =               THOUSAND * THOUSAND;

static const size_t cacheline_min =         TEN;
static const size_t cacheline_max =         TEN * THOUSAND;
static const size_t cacheline_step =        TEN;

static const size_t entries_min =           TEN;
static const size_t entries_max =           HUNDRED * THOUSAND;
static const size_t entries_step_mag =      TEN;

static const size_t num_samples =           HUNDRED;

static std::mt19937 twister_32;
static std::mt19937_64 twister_64;

template<class T> using sp = boost::shared_ptr<T>;

template<typename T> sp<T> gen_random();
template<typename T> sp<T> gen_nonrandom();

template<>
sp<uint32_t> gen_random<uint32_t>()
{ return sp<uint32_t>(new uint32_t(twister_32())); }

template<>
sp<uint32_t> gen_nonrandom<uint32_t>()
{ return sp<uint32_t>(new uint32_t(0xCAFEBABE)); }

template<>
sp<uint64_t> gen_random<uint64_t>()
{ return sp<uint64_t>(new uint64_t(twister_64())); }

template<>
sp<uint64_t> gen_nonrandom<uint64_t>()
{ return sp<uint64_t>(new uint64_t(0xCAFEBABEDEADBEEF)); }

template<>
sp<std::string> gen_random<std::string>()
{ return sp<std::string>(new std::string(tmpnam(nullptr))); }

template<>
sp<std::string> gen_nonrandom<std::string>()
{ return sp<std::string>(new std::string("FormationDS")); }

template<>
sp<fds::ObjectID> gen_random<fds::ObjectID>()
{
    std::stringstream digest;
    digest << std::setw(20) << std::to_string(twister_64());
    return sp<fds::ObjectID>(new fds::ObjectID(digest.str()));
}

template<>
sp<fds::ObjectID> gen_nonrandom<fds::ObjectID>()
{ return sp<fds::ObjectID>(new fds::ObjectID(static_cast<uint32_t>(0xCAFEBABE))); }

template<>
sp<fds::BlobOffsetPair> gen_random<fds::BlobOffsetPair>()
{ return sp<fds::BlobOffsetPair>(
        new fds::BlobOffsetPair(*gen_random<std::string>(), *gen_random<uint64_t>())); }

template<>
sp<fds::BlobOffsetPair> gen_nonrandom<fds::BlobOffsetPair>()
{
    return sp<fds::BlobOffsetPair>(
        new fds::BlobOffsetPair("FormationDS", static_cast<uint64_t>(0xCAFEBABEDEADBEEF)));
}

template<>
sp<fds::BlobDescriptor> gen_random<fds::BlobDescriptor>()
{ return sp<fds::BlobDescriptor>(new fds::BlobDescriptor()); }

template<>
sp<fds::BlobDescriptor> gen_nonrandom<fds::BlobDescriptor>()
{ return sp<fds::BlobDescriptor>(new fds::BlobDescriptor()); }

template<typename K, typename V, typename H>
std::pair<clock_t, size_t>
fill_cache(size_t const cache_size, size_t elements_to_insert, bool const rnd = true) {
    // Create a cache of the right size
    SharedKvCache<K, V, H> cache("test_cache", cache_size);

    // Create all the elements (don't want to profile the allocation itself)
    std::vector<std::pair<K, sp<V>>> test_objects;
    for (; elements_to_insert > 0; --elements_to_insert) {
        if (rnd)
            test_objects.push_back(std::make_pair(*(gen_random<K>()), gen_random<V>()));
        else
            test_objects.push_back(std::make_pair(*(gen_nonrandom<K>()), gen_nonrandom<V>()));
    }

    size_t evictions = 0;
    clock_t start = clock();
//    ProfilerStart("cache.perf");
    {
        auto vec_end = test_objects.end();
        for (auto it = test_objects.begin(); vec_end != it; ++it) {
            if (cache.add(it->first, it->second)) ++evictions;
        }
    }
//    ProfilerStop();
    return std::make_pair((clock() - start), evictions);
}

template<typename K, typename V, typename H = std::hash<K>>
void fill_test() {
    std::cout.precision(3);
    std::cout.setf(std::ios::fixed, std::ios::floatfield);
    for (size_t cacheline_size = cacheline_min;
         cacheline_size <= cacheline_max;
         cacheline_size *= cacheline_step) {
        for (size_t entry_size = entries_min;
             entry_size <= entries_max;
             entry_size *= entries_step_mag) {
            uint64_t total_time = 0;
            size_t num_evictions = 0;
            for (size_t sample_no = num_samples; sample_no > 0; --sample_no) {
                auto p = fill_cache<K, V, H>(cacheline_size, entry_size);
               total_time += p.first;
               num_evictions += p.second;
            }
            std::cout   << cacheline_size << ",\t"
                        << entry_size << ",\t"
                        << static_cast<double>(total_time) / num_samples / entry_size << ",\t"
                        << num_evictions / num_samples << std::endl;
        }
    }
}


int main(int argc, char** argv) {
    std::cout << "Testing <uint32_t, uint32_t>" << std::endl;
    fill_test<uint32_t, uint32_t>();
    std::cout << "Testing <uint32_t, string>" << std::endl;
    fill_test<uint32_t, std::string>();
    std::cout << "Testing <string, BlobDescriptor>" << std::endl;
    fill_test<std::string, fds::BlobDescriptor>();
    std::cout << "Testing <ObjectID, string>" << std::endl;
    fill_test<fds::ObjectID, std::string, fds::ObjectHash>();
    std::cout << "Testing <BlobOffsetPair, ObjectID>" << std::endl;
    fill_test<fds::BlobOffsetPair, fds::ObjectID, fds::BlobOffsetPairHash>();
    return 0;
}
