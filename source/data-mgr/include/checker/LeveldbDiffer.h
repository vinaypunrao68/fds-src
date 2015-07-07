/* Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_CHECKER_LEVELDBDIFFER_H_
#define SOURCE_DATA_MGR_INCLUDE_CHECKER_LEVELDBDIFFER_H_

#include <fds_assert.h>
#include <string>
#include <array>
#include <vector>
#include <ostream>
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
std::ostream& operator << (std::ostream &os, const DiffResult &res);
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
    virtual leveldb::Comparator* getComparator() {
        return nullptr;
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
}  // namespace fds
#endif   // SOURCE_DATA_MGR_INCLUDE_CHECKER_LEVELDBDIFFER_H_
