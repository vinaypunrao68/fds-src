/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <checker/LeveldbDiffer.h>

namespace fds {

std::ostream& operator << (std::ostream &os, const DiffResult &res) {
    static const char* TypeStr[] = {"MATCH","DELETED_KEY","ADDITIONAL_KEY","VALUE_MISMATCH"};
    os << res.key  << " " << TypeStr[res.mismatchType] << " " << res.additionalDetail;
    return os;
}

LevelDbDiffer::LevelDbDiffer(const std::string &path1, const std::string &path2,
                             LevelDbDiffAdapter *adapter) {
    this->adapter = adapter;

    db1 = openDb_(path1);
    itr1 = db1->NewIterator(leveldb::ReadOptions());
    itr1->SeekToFirst();

    db2 = openDb_(path2);
    itr2 = db2->NewIterator(leveldb::ReadOptions());
    itr2->SeekToFirst();

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
    if (adapter->getComparator()) {
        options.comparator = adapter->getComparator();
    }
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
        return {adapter->keyAsString(itr2), DiffResult::DELETED_KEY, ""};
    }
    if (!itr2->Valid()) {
        return {adapter->keyAsString(itr1), DiffResult::ADDITIONAL_KEY, ""};
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
