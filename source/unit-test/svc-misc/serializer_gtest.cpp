
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */

#define GTEST_USE_OWN_TR1_TUPLE 0

#include <string>
#include <fds_assert.h>
#include <fds_types.h>
#include <concurrency/HashedLocks.hpp>
#include <vector>
#include <unordered_map>
#include <thread>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <iostream>
#include <util/timeutils.h>
#include <testlib/DataGen.hpp>
#include <testlib/TestUtils.h>
#include <testlib/TestFixtures.h>
#include <serialize.h>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::AtLeast;
using ::testing::Return;
using namespace fds;  // NOLINT

namespace fds {

/* Test fixture */
struct SerializationTest : BaseTestFixture
{
};

/* class that stores hashtable of object ids */
struct Bloblist : public serialize::Serializable {
    void put(const int& key, ObjectID val) {
        tbl_[key] = val;
    }
    /* Overrides from Serializable */
    virtual uint32_t write(serialize::Serializer* serializer) const override
    {
        uint32_t written = 0;
        written += serializer->writeI32(tbl_.size());
        for (auto &kv : tbl_) {
            // written += serializer->writeI32(kv.first);
            written += 4;
            written += kv.second.write(serializer);
        }
        return written;
    }
    virtual uint32_t read(serialize::Deserializer* deserializer) override
    {
        return 0;
    }
    virtual uint32_t getEstimatedSize() const override
    {
        uint32_t sz = sizeof(int32_t);
        for (auto &kv : tbl_) {
            sz += sizeof(uint32_t);
            sz += 20;
        }
        return sz;
    }

    uint32_t writeMemcpy(char* buf) {
        uint32_t idx = 0;
        int32_t sz = static_cast<int32_t>(tbl_.size());
        memcpy(&buf[idx], &sz, sizeof(int32_t));
        idx += sizeof(int32_t);
        for (auto &kv : tbl_) {
            memcpy(&buf[idx], &(kv.first), sizeof(int32_t));
            idx += sizeof(int32_t);
            memcpy(&buf[idx], kv.second.GetId(), 20);
            idx += 20;
        }
        return idx;
    }

    std::unordered_map<int, ObjectID>& tbl() {
        return tbl_;
    }

 protected:
    std::unordered_map<int, ObjectID> tbl_;
};

/* Class that stores hashtable of strings */
struct StringTbl : public serialize::Serializable {
    void put(const int& key, StringPtr val) {
        tbl_[key] = val;
    }
    /* Overrides from Serializable */
    virtual uint32_t write(serialize::Serializer* serializer) const override
    {
        uint32_t written = 0;
        written += serializer->writeI32(tbl_.size());
        for (auto &kv : tbl_) {
            written += serializer->writeString(*kv.second);
        }
        return written;
    }
    virtual uint32_t read(serialize::Deserializer* deserializer) override
    {
        uint32_t readSz = 0;
        int32_t sz;
        readSz += deserializer->readI32(sz);
        for (int i = 0; i < sz; i++) {
            StringPtr s(new std::string());
            readSz += deserializer->readString(*s);
            tbl_[i] = s;
        }
        return readSz;
    }
    virtual uint32_t getEstimatedSize() const override
    {
        uint32_t sz = sizeof(int32_t);
        for (auto &kv : tbl_) {
            sz += sizeof(uint32_t);
            sz += kv.second->size();
        }
        return sz;
    }

    uint32_t writeMemcpy(char* buf) {
        uint32_t idx = 0;
        int32_t sz = static_cast<int32_t>(tbl_.size());
        memcpy(&buf[idx], &sz, sizeof(int32_t));
        idx += sizeof(int32_t);
        for (auto &kv : tbl_) {
            sz = static_cast<int32_t>(kv.second->size());
            memcpy(&buf[idx], &sz, sizeof(int32_t));
            idx += sizeof(int32_t);
            memcpy(&buf[idx], kv.second->data(), kv.second->size());
            idx += kv.second->size();
        }
        return idx;
    }

    std::unordered_map<int, StringPtr>& tbl() {
        return tbl_;
    }

 protected:
    std::unordered_map<int, StringPtr> tbl_;
};

/* Test to time memcpy timings */
TEST_F(SerializationTest, memcpy)
{
    uint32_t bufSz = this->getArg<uint32_t>("buf-size");
    uint32_t nBufs = this->getArg<uint32_t>("cnt");
    uint64_t startTime;
    uint64_t endTime;

    DataGenIf *g = new RandDataGenerator<>(bufSz * nBufs, bufSz * nBufs);
    /* Time single buffer copy */
    auto d = g->nextItem();
    std::string serialized(d->size(), '\0');
    startTime = util::getTimeStampNanos();
    memcpy(&serialized[0], &(*d)[0], d->size());
    endTime = util::getTimeStampNanos();
    std::cout << "Single buffer copy time: " << endTime-startTime
        << "written: " << d->size() << std::endl;
    serialized.clear();
    delete g;

    /* Time hashtable copy */
    g = new RandDataGenerator<>(bufSz, bufSz);
    uint32_t totalSz = 0;
    uint32_t copyIdx = 0;
    std::unordered_map<int, StringPtr> bufMap;
    for (uint32_t i = 0; i < nBufs; i++) {
        bufMap[i] = g->nextItem();
        totalSz += bufMap[i]->size();
    }

    startTime = util::getTimeStampNanos();
    serialized.resize(totalSz);
    for (uint32_t i = 0; i < nBufs; i++) {
        auto s = bufMap[i];
        memcpy(&serialized[copyIdx], &(*s)[0], s->size());
        copyIdx += s->size();
    }
    endTime = util::getTimeStampNanos();
    std::cout << "Hashtable memcpy time: " << endTime-startTime
        << " written: " << copyIdx << std::endl;
    delete g;
}

/* Test to time serialization of StringTbl object via memcpy vs thrift */
TEST_F(SerializationTest, stringTbl)
{
    uint32_t bufSz = this->getArg<uint32_t>("buf-size");
    uint32_t nBufs = this->getArg<uint32_t>("cnt");
    uint64_t startTime;
    uint64_t endTime;

    auto g = new RandDataGenerator<>(bufSz, bufSz);
    StringTbl bufMap;
    for (uint32_t i = 0; i < nBufs; i++) {
        bufMap.put(i, g->nextItem());
    }

    serialize::Serializer *s;
    char *writeBuffer;
    uint32_t written;
    uint32_t bufMapSz = bufMap.getEstimatedSize();

    startTime = util::getTimeStampNanos();
    writeBuffer = new char[bufMapSz];
    written = bufMap.writeMemcpy(writeBuffer);
    endTime = util::getTimeStampNanos();
    ASSERT_TRUE(written == bufMapSz);
    std::cout << "memcpy serilization time: " << endTime - startTime
        << " written: " << written << std::endl;

    startTime = util::getTimeStampNanos();
    s = serialize::getMemSerializer(bufMapSz);
    written = bufMap.write(s);
    endTime = util::getTimeStampNanos();
    ASSERT_TRUE(written == bufMapSz);
    std::cout << "Thrift serilization time: " << endTime - startTime
        << " written: " << written << std::endl;

    delete s;
    delete writeBuffer;
    delete g;
}

/* Test to time serialization of Bloblist object via memcpy vs thrift */
TEST_F(SerializationTest, bloblist)
{
    uint32_t nBufs = this->getArg<uint32_t>("cnt");
    uint64_t startTime;
    uint64_t endTime;

    Bloblist blobMap;
    for (uint32_t i = 0; i < nBufs; i++) {
        blobMap.put(i, ObjectID());
    }

    serialize::Serializer *s;
    char *writeBuffer;
    uint32_t written;
    uint32_t blobMapSz = blobMap.getEstimatedSize();

    startTime = util::getTimeStampNanos();
    writeBuffer = new char[blobMapSz];
    written = blobMap.writeMemcpy(writeBuffer);
    endTime = util::getTimeStampNanos();
    ASSERT_TRUE(written == blobMapSz);
    std::cout << "memcpy serilization time: " << endTime - startTime
        << " written: " << written << std::endl;

    startTime = util::getTimeStampNanos();
    s = serialize::getMemSerializer(blobMapSz);
    written = blobMap.write(s);
    endTime = util::getTimeStampNanos();
    ASSERT_TRUE(written == blobMapSz) << "written: " << written << " blobMapSz: " << blobMapSz;
    std::cout << "Thrift serilization time: " << endTime - startTime
        << " written: " << written << std::endl;

    delete s;
    delete writeBuffer;
}

}  // namespace fds

int main(int argc, char** argv) {
    // The following line must be executed to initialize Google Mock
    // (and Google Test) before running the tests.
    ::testing::InitGoogleMock(&argc, argv);
    po::options_description opts("Allowed options");
    opts.add_options()
        ("help", "produce help message")
        ("buf-size", po::value<uint32_t>()->default_value(150 << 10), "buffer size")
        ("cnt", po::value<uint32_t>()->default_value(10), "cnt");
    SerializationTest::init(argc, argv, opts);
    std::cout << std::fixed;
    return RUN_ALL_TESTS();
}
