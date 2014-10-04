/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#include <iostream>
#include <assert.h>
#include <list>
#include <string>

#include <leveldb/db.h>
#include <leveldb/cache.h>
#include <leveldb/write_batch.h>

#include <util/timeutils.h>

// const unsigned NUM_OBJ = 1048576;     // 20
// const unsigned NUM_OBJ = 209715;   // 100
// const unsigned NUM_OBJ = 40960;    // 512
// const unsigned NUM_OBJ = 20480;    // 1024
// const unsigned NUM_OBJ = 2048;     // 10K
// const unsigned NUM_OBJ = 1024;     // 20K
const unsigned NUM_OBJ = 512;      // 40K

class MyBatchHandler : public leveldb::WriteBatch::Handler {
  public:
    virtual void Put(const leveldb::Slice& key, const leveldb::Slice & value) override{
        std::cout << key.ToString() << std::endl;
    }

    virtual void Delete(const leveldb::Slice& key) override {
        std::cout << "Deleted " << key.ToString() << std::endl;
    }
};

int main(int argc, char *argv[]) {
    leveldb::DB * db;
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(8 * 1024 * 1024);
    options.create_if_missing = true;
    options.write_buffer_size = 2 * 1024 * 1024;

    leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);
    assert(status.ok());

    leveldb::WriteBatch batch;
    leveldb::WriteOptions writeOpts;
    writeOpts.sync = false;
    fds_uint64_t startTs = fds::util::getTimeStampNanos();
    for (unsigned i = 0; i < NUM_OBJ; ++i) {
//        size_t sz = 20;      // 1048576
//        size_t sz = 100;        // 209715
//        size_t sz = 512;     // 40960
//        size_t sz = 1024;    // 20480
//        size_t sz = 10240;   // 2048
//        size_t sz = 20480;   // 1024
        size_t sz = 40960;   // 512
        char * ptr = static_cast<char *>(malloc(sz));
        std::string val(ptr, sz);

        std::string key = "key_" + std::to_string(i);

// /*
        batch.Put(key, val);
        if (!(i+1) % 1024) {
            db->Write(writeOpts, &batch);
            batch.Clear();
        }
// */
//        db->Put(leveldb::WriteOptions(), key, val);
        delete ptr;
    }

    // std::cout << "Writing to DB..." << std::endl;
    db->Write(writeOpts, &batch);
    fds_uint64_t endTs = fds::util::getTimeStampNanos();
    std::cout << "Time to write: " << (endTs - startTs) / (1024 * 1024) << std::endl;

    // MyBatchHandler handler;
    // status = batch.Iterate(&handler);
    // assert(status.ok());

    delete db;

    return 0;
}

