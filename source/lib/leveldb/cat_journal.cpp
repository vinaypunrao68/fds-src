/*
 * copyright 2014 Formation Data Systems, Inc.
 */

// Standard includes.
#include <string>
#include <iostream>
#include <vector>

// Internal includes.
#include "catalogKeys/CatalogKeyType.h"
#include "catalogKeys/JournalTimestampKey.h"
#include "db/dbformat.h"
#include "db/filename.h"
#include "db/log_reader.h"
#include "db/version_edit.h"
#include "db/write_batch_internal.h"
#include "leveldb/cat_journal.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"
#include "leveldb/options.h"
#include "leveldb/status.h"
#include "leveldb/table.h"
#include "leveldb/write_batch.h"
#include "port/port.h"
#include "util/logging.h"
#include "util/Log.h"

using namespace fds;  // NOLINT
using namespace leveldb;  // NOLINT

namespace {

class WriteBatchGetLogTime : public WriteBatch::Handler {
  public:
    WriteBatchGetLogTime() : journal_timestamp_(0) {}

    inline void Put(const Slice& key, const Slice& value) {
        if (journal_timestamp_) return;

        auto keyType = *reinterpret_cast<fds::CatalogKeyType const*>(key.data());
        if (keyType == fds::CatalogKeyType::JOURNAL_TIMESTAMP)
        {
            journal_timestamp_ = *reinterpret_cast<fds_uint64_t const*>(value.data());
        }
    }

    inline void Delete(const Slice& key) {}

    inline fds_uint64_t getJournalTimeStamp() {
        return journal_timestamp_;
    }

  private:
    fds_uint64_t journal_timestamp_;
};

}  // namespace

namespace leveldb {

fds_uint64_t getWriteBatchTimestamp(const leveldb::WriteBatch & batch) {
    WriteBatchGetLogTime handler;
    leveldb::Status s = batch.Iterate(&handler);
    if (!s.ok()) {
        LOGDEBUG << "No timestamp found for the batch";
        return 0;
    }
    return handler.getJournalTimeStamp();
}

CatJournalIterator::CatJournalIterator(const std::string & file) : file_(file),
        valid_(false), env_(leveldb::Env::Default()), sfile_(0) {
    fds_verify(!file_.empty());
    if (!env_->FileExists(file_)) {
        LOGWARN << "File '" << file_ << "' not found!";
        return;
    }

    Status s = env_->NewSequentialFile(file_, &sfile_);
    if (!s.ok()) {
        LOGWARN << "Failed to open file '" << file_ << "'";
        return;
    }

    reader_.reset(new log::Reader(sfile_, &reporter_, true, 0));
    batch_.Clear();
    valid_ = true;

    Next();
}

CatJournalIterator::~CatJournalIterator() {
    delete sfile_;
}

void CatJournalIterator::Next() {
    if (!valid_) return;

    Slice record;
    std::string scratch;
    valid_ = (0 != reader_->ReadRecord(&record, &scratch));
    if (valid_) {
        LOGDEBUG << "WriteBatch offset: " << static_cast<fds_uint64_t>(
                reader_->LastRecordOffset());
        if (record.size() < 12) {
            LOGWARN << "Catalog journal record length '" <<
                    static_cast<fds_int32_t>(record.size()) << "' is too small!";
            valid_ = false;
            return;
        }

        WriteBatchInternal::SetContents(&batch_, record);
    }
}
}  // namespace leveldb
