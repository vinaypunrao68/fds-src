
/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_LEVELDB_CAT_JOURNAL_H_
#define SOURCE_INCLUDE_LEVELDB_CAT_JOURNAL_H_

/**
 * XXX: This code is copied (with a few changes) from the leveldb forum post:
 * https://code.google.com/p/leveldb/issues/detail?id=184
 *
 * The solution best documented on the mentioned web-page.
 */

#include <string>
#include <leveldb/env.h>
#include <leveldb/write_batch.h>
#include <db/log_reader.h>
#include <concurrency/Mutex.h>
#include <fds_types.h>
#include <dm-vol-cat/DmPersistVolCat.h>

namespace leveldb {

class CorruptionReporter : public log::Reader::Reporter {
  public:
    virtual void Corruption(size_t bytes, const Status& status) {
          LOGWARN << "Catalog Journal Corruption: " << bytes << " bytes; " << status.ToString();
    }
};

extern fds_uint64_t getWriteBatchTimestamp(const leveldb::WriteBatch & batch);

class CatJournalIterator {
  public:
    // ctor and dtor
    explicit CatJournalIterator(const std::string & file);
    virtual ~CatJournalIterator();

    // Iterator
    void Next();

    inline bool isValid() const {
        return valid_;
    }

    inline leveldb::WriteBatch & GetBatch() {
        return batch_;
    }

  private:
    std::string file_;
    bool valid_;
    leveldb::Env * env_;

    SequentialFile *sfile_;
    CorruptionReporter reporter_;
    boost::shared_ptr<log::Reader> reader_;
    leveldb::WriteBatch batch_;
};
}  // namespace leveldb
#endif  // SOURCE_INCLUDE_LEVELDB_CAT_JOURNAL_H_
