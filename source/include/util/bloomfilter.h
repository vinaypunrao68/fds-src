/*
 * Copyright 2014 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_UTIL_BLOOMFILTER_H_
#define SOURCE_INCLUDE_UTIL_BLOOMFILTER_H_

#include <boost/dynamic_bitset.hpp>
#include <vector>
#include <serialize.h>
#include <fds_types.h>
#include <fds_defines.h>
#include <fds_typedefs.h>
namespace fds { namespace util {

/**
 * This is a non- thread safe bloom filter
 */
struct BloomFilter {
    explicit BloomFilter(uint32_t totalBits=1*MB, uint8_t bitsPerKey=8);
    void add(const ObjectID& ojID);
    void add(const std::string& data);
    bool lookup(const ObjectID& ojID) const;
    bool lookup(const std::string& data) const;

    void add(const std::vector<uint32_t>& positions);
    bool lookup(const std::vector<uint32_t>& positions) const;

    void merge(const BloomFilter& filter);

    std::vector<uint32_t> generatePositions(const void* data, uint32_t len) const;

    uint32_t write(serialize::Serializer*  s) const;
    uint32_t read(serialize::Deserializer* d);
    uint32_t getEstimatedSize() const;

  protected:
    uint32_t bitsPerKey = 8;
    uint32_t totalBits = 1024;
    SHPTR<boost::dynamic_bitset<> > bits;
};

using BloomFilterPtr = SHPTR<BloomFilter>;


}  // namespace util
}  // namespace fds
#endif  // SOURCE_INCLUDE_UTIL_BLOOMFILTER_H_
