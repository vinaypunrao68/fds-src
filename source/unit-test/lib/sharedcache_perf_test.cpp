/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#include <cstdint>
#include <chrono>
#include <random>
#include <string>
#include <utility>
#include <list>

#include <google/profiler.h>
#include "boost/smart_ptr/make_shared.hpp"

#include <fds_types.h>
#include <blob/BlobTypes.h>
#include <cache/SharedKvCache.h>
#include <concurrency/ThreadPool.h>

using namespace fds;    // NOLINT

static const size_t TEN =                   10;
static const size_t HUNDRED =               TEN * TEN;
static const size_t THOUSAND =              TEN * HUNDRED;
static const size_t MILLION =               THOUSAND * THOUSAND;

static const size_t cacheline_min =         HUNDRED;
static const size_t cacheline_max =         TEN * THOUSAND;
static const size_t cacheline_step =        TEN;

static const size_t entries_max =           MILLION;

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
{ return sp<std::string>(new std::string(std::to_string(twister_32()))); }

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

template<typename K, typename V, typename _Hash, typename StrongAssociation>
struct CacheTest {
    typedef K key_type;
    typedef sp<V> value_type;
    typedef std::pair<key_type, value_type> entry_type;
    typedef std::list<entry_type> list_type;
    typedef SharedKvCache<key_type, V, _Hash, StrongAssociation> cache_type;
    typedef std::chrono::high_resolution_clock clock_type;

    explicit CacheTest(size_t const number_elements) :
        entry_count(number_elements),
        test_objects()
    {
        std::cout << "Generating random data..." << std::flush;
        create_objects(test_objects, entry_count);
        std::cout << "done." << std::endl;
    }

    void fill_test() {
        for (size_t cacheline_size = cacheline_min;
             cacheline_size <= cacheline_max;
             cacheline_size *= cacheline_step) {
            std::cout   << cacheline_size << ",\t"
                        << "n/a,\t" << std::flush;

            double t = fill_cache(cacheline_size);
            uint32_t iops = entry_count / t;
            std::cout << iops << ",\tn/a" << std::endl;
        }
    }

    void read_test(float const hit_percentage) {
        for (size_t cacheline_size = cacheline_min;
             cacheline_size <= cacheline_max;
             cacheline_size *= cacheline_step) {
            uint32_t hit_rate = hit_percentage > 0 ? 1.00 / hit_percentage : 0;

            cache_type cache("test_cache", cacheline_size);
            cache_type phony_cache("phony_cache", cacheline_size);
            auto it = test_objects.begin();
            for (size_t i = 0; i < cacheline_size; ++i) {
                if (hit_rate == 0 || i % hit_rate > 0) {
                    cache.add(*gen_random<key_type>(), gen_nonrandom<V>());
                } else {
                    cache.add(it->first, it->second);
                    ++it;
                }
            }

            std::cout   << cacheline_size << ",\t"
                        << hit_percentage << ",\t" << std::flush;

            value_type v;
            it = test_objects.begin();
            size_t actual_cache_hits = 0;

            clock_type::time_point start = clock_type::now();
            for (size_t i = 0; i < cacheline_size; ++i)
                if (cache.get((it++)->first, v) == ERR_OK)
                    ++actual_cache_hits;
                else
                    phony_cache.add((it++)->first, v);

            double t = 1e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - start).count();
            uint32_t iops = cacheline_size / t;
            std::cout << iops << ",\t" << actual_cache_hits << std::endl;
        }
    }

 private:
    size_t entry_count;
    list_type test_objects;

    static void create_objects(list_type& objs, size_t elements_to_insert, bool random = true) {
        // Create all the elements (don't want to profile the allocation itself)
        for (; elements_to_insert > 0; --elements_to_insert) {
            if (random)
                objs.push_front(
                    std::make_pair(*(gen_random<key_type>()), gen_nonrandom<V>()));
            else
                objs.push_front(
                    std::make_pair(*(gen_nonrandom<key_type>()), gen_nonrandom<V>()));
        }
    }

    static void add_to_cache(sp<cache_type> cache, key_type& k, value_type& v) {
        cache->add(k, v);
    }

    double fill_cache(size_t const cache_size) {
        // Create a cache of the right size
        sp<cache_type> cache = sp<cache_type>(new cache_type("test_cache", cache_size));

        size_t evictions = 0;
        clock_type::time_point start = clock_type::now();
//        ProfilerStart(("cache_" + std::to_string(cache_size) + ".perf").c_str());
        {
//            fds_threadpool pool;
            for (auto& elem : test_objects)
//                pool.schedule(add_to_cache, cache, elem.first, elem.second);
                add_to_cache(cache, elem.first, elem.second);
        }
//        ProfilerStop();
        return 1e-9*std::chrono::duration_cast<std::chrono::nanoseconds>(clock_type::now() - start).count();
    }
};

template<typename K, typename _Hash = std::hash<K>, typename StrongAssociation = std::false_type>
void run_test(size_t const entries) {
    auto test = CacheTest<K, uint32_t, _Hash, StrongAssociation>(entries);
    test.fill_test();
    test.read_test(1.00);
    test.read_test(0.50);
    test.read_test(0.25);
    test.read_test(0.10);
    test.read_test(0.00);
}

int main(int argc, char** argv) {
//    std::cout << "uint32_t ---" << std::endl;
//    run_test<uint32_t>(entries_max);
//    std::cout << "fds_volid_t ---" << std::endl;
//    run_test<uint64_t>(entries_max);
//    std::cout << "std::string ---" << std::endl;
//    run_test<std::string>(entries_max);
    std::cout << "ObjectID ---" << std::endl;
    run_test<fds::ObjectID, fds::ObjectHash, std::true_type>(entries_max);
//    std::cout << "BlobOffsetPair ---" << std::endl;
//    run_test<fds::BlobOffsetPair, fds::BlobOffsetPairHash>(entries_max);
    return 0;
}
