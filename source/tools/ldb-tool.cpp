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

#include <boost/program_options.hpp>

#include <util/timeutils.h>

namespace po = boost::program_options;

static unsigned NUM_KEYS = 512;
static unsigned VAL_SIZE = 40960;   // 40K
static bool USE_SYNC = false;
static bool USE_BATCH = false;
static unsigned BATCH_SIZE = 1024;

class MyBatchHandler : public leveldb::WriteBatch::Handler {
  public:
    virtual void Put(const leveldb::Slice& key, const leveldb::Slice & value) override{
        std::cout << key.ToString() << std::endl;
    }

    virtual void Delete(const leveldb::Slice& key) override {
        std::cout << "Deleted " << key.ToString() << std::endl;
    }
};

int run() {
    leveldb::DB * db;
    leveldb::Options options;
    options.block_cache = leveldb::NewLRUCache(32 * 1024 * 1024);
    options.create_if_missing = true;
    options.write_buffer_size = 32 * 1024 * 1024;

    leveldb::Status status = leveldb::DB::Open(options, "./testdb", &db);
    assert(status.ok());

    leveldb::WriteBatch batch;
    leveldb::WriteOptions writeOpts;
    writeOpts.sync = USE_SYNC;
    fds_uint64_t startTs = fds::util::getTimeStampNanos();

    for (unsigned i = 0; i < NUM_KEYS; ++i) {
        char * ptr = static_cast<char *>(malloc(VAL_SIZE));
        std::string val(ptr, VAL_SIZE);

        std::string key = "key_" + std::to_string(i);

        if (USE_BATCH) {
            batch.Put(key, val);
            if (!(i + 1) % BATCH_SIZE) {
                db->Write(writeOpts, &batch);
                batch.Clear();
            }
        } else {
            db->Put(leveldb::WriteOptions(), key, val);
        }

        delete ptr;
    }

    if (USE_BATCH) {
        db->Write(writeOpts, &batch);
    }

    fds_uint64_t endTs = fds::util::getTimeStampNanos();
    std::cout << "Time to write: " << (endTs - startTs) / (1024 * 1024) << "ms" << std::endl;

//    MyBatchHandler handler;
//    status = batch.Iterate(&handler);
//    assert(status.ok());

    delete db;

    return 0;
}

int main(int argc, char *argv[]) {
    // process command line options
    po::options_description desc("\nldb-tool command line options:");
    desc.add_options()
            ("help,h", "help/ usage message")
            ("num-keys,n",
            po::value<unsigned>(&NUM_KEYS)->default_value(NUM_KEYS),
            "number of keys")
            ("val-size,v",
            po::value<unsigned>(&VAL_SIZE)->default_value(VAL_SIZE),
            "value size in bytes")
            ("use-sync,s", "use sync after each write")
            ("use-batch,b", "batch writes")
            ("batch-size",
            po::value<unsigned>(&BATCH_SIZE)->default_value(BATCH_SIZE),
            "write batch size");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << std::endl;
        return 1;
    }

    USE_SYNC = 0 != vm.count("use-sync");
    USE_BATCH = 0 != vm.count("use-batch");

    return run();
}

