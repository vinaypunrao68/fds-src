/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_UNIT_TEST_DATA_MGR_LEVELDBDIFFER_H_
#define SOURCE_UNIT_TEST_DATA_MGR_LEVELDBDIFFER_H_

#include <fds_assert.h>
#include <string>
#include <array>
#include <vector>
#include <leveldb/db.h>

namespace fds {

/**
* @brief Diff result
*/
struct DiffResult {
    enum Type {MATCH, DELETED_KEY, ADDITIONAL_KEY, VALUE_MISMATCH};
    // static const char* TypeStr[] = {"MATCH","DELETED_KEY","ADDITIONAL_KEY","VALUE_MISMATCH"};

    std::string             key;
    Type                    mismatchType;
    std::string             additionalDetail;
};
// const char* DiffResult::TypeStr[];

/**
* @brief Interfaces needed for diffing leveldb key/values by adapting the raw ondisk content to 
* datastructures specific to the db being diffed.
*/
struct LevelDbDiffAdapter {
    virtual int compareKeys(leveldb::Iterator *itr1, leveldb::Iterator *itr2) const  {
        auto k1 = itr1->key().ToString();
        auto k2 = itr2->key().ToString();
        if (k1 < k2) {
            return -1;
        } else if (k1 > k2) {
            return 1;
        }
        return 0;
    }
    virtual bool compareValues(leveldb::Iterator *itr1,
                              leveldb::Iterator *itr2,
                              std::string &mismatchDetails) const {
        return itr1->value().ToString() == itr2->value().ToString();
    }
    virtual std::string keyAsString(leveldb::Iterator *itr) const {
        return itr->key().ToString();
   } 
};

/**
* @brief Diffs two instance of level dbs
*/
struct LevelDbDiffer {
    LevelDbDiffer(const std::string &path1, const std::string &path2,
                  LevelDbDiffAdapter *adapter);
    virtual ~LevelDbDiffer();
    bool diff(const uint64_t &maxDiffs, std::vector<DiffResult> &results);
    DiffResult compare(leveldb::Iterator *itr1, leveldb::Iterator *itr2);
    bool isDiffDone() const {
        return diffDone;
    }

 protected:
    leveldb::DB* openDb_(const std::string &path);
    leveldb::DB*                db1;
    leveldb::DB*                db2;
    leveldb::Iterator*          itr1;
    leveldb::Iterator*          itr2;
    LevelDbDiffAdapter*         adapter;
    bool                        diffDone;
};

LevelDbDiffer::LevelDbDiffer(const std::string &path1, const std::string &path2,
                             LevelDbDiffAdapter *adapter) {
    db1 = openDb_(path1);
    itr1 = db1->NewIterator(leveldb::ReadOptions());
    itr1->SeekToFirst();

    db2 = openDb_(path2);
    itr2 = db2->NewIterator(leveldb::ReadOptions());
    itr2->SeekToFirst();

    this->adapter = adapter;
    diffDone = false;
}

LevelDbDiffer::~LevelDbDiffer() {
    if (db1) {
        delete db1;
        db1 = nullptr;
    }
    if (db2) {
        delete db2;
        db2 = nullptr;
    }
}

leveldb::DB* LevelDbDiffer::openDb_(const std::string &path) {
    leveldb::DB* db;
    leveldb::Options options;
    leveldb::Status status = leveldb::DB::Open(options, path, &db);
    assert(status.ok());
    return db;
}

bool LevelDbDiffer::diff(const uint64_t &maxDiffs, std::vector<DiffResult> &results) {
    if (diffDone) {
        return false;
    }

    while ((itr1->Valid() || itr2->Valid()) &&
           results.size() < maxDiffs) {
        auto res = compare(itr1, itr2);
        if (res.mismatchType == DiffResult::MATCH) {
            itr1->Next();
            itr2->Next();
        } else if (res.mismatchType == DiffResult::VALUE_MISMATCH) {
            results.push_back(res);
            itr1->Next();
            itr2->Next();
        } else if (res.mismatchType == DiffResult::ADDITIONAL_KEY) {
            results.push_back(res);
            itr1->Next();
        } else if (res.mismatchType == DiffResult::DELETED_KEY) {
            results.push_back(res);
            itr2->Next();
        }
    }

    if (!itr1->Valid() && !itr2->Valid()) {
        diffDone = true;
        return false;
    }
    return true;
}

DiffResult LevelDbDiffer::compare(leveldb::Iterator *itr1, leveldb::Iterator *itr2)
{
    fds_verify(itr1->Valid() || itr2->Valid());

    if (!itr1->Valid()) {
        return {itr2->key().ToString(), DiffResult::DELETED_KEY, ""};
    }
    if (!itr2->Valid()) {
        return {itr1->key().ToString(), DiffResult::ADDITIONAL_KEY, ""};
    }

    int res = adapter->compareKeys(itr1, itr2);
    if (res == 0) {                     // itr1.key == itr2.key
        std::string valueMismatchDetails;
        if (adapter->compareValues(itr1, itr2, valueMismatchDetails)) {
            return {adapter->keyAsString(itr1), DiffResult::MATCH, ""};
        } else {
            return {adapter->keyAsString(itr1), DiffResult::VALUE_MISMATCH, valueMismatchDetails};
        }
    } else if (res < 0) {               // itr1.key < itr2.key
        return {adapter->keyAsString(itr1), DiffResult::ADDITIONAL_KEY, ""};
    }
    /* itr1.key > itr2.key */
    return {adapter->keyAsString(itr2), DiffResult::DELETED_KEY, ""};
}

}  // namespace fds
#endif   // SOURCE_UNIT_TEST_DATA_MGR_LEVELDBDIFFER_H_
