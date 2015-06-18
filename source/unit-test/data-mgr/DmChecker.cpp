/* Copyright 2015 Formation Data Systems, Inc.
 */
#include <list>
#include <vector>
#include <string>
#include <boost/lexical_cast.hpp>
#include <fds_process.h>
#include <LeveldbDiffer.h>
#include <dm-vol-cat/DmPersistVolDB.h>

namespace fds {

/**
* @brief Interface for providing the environment necessary for running DM checker
*/
struct DMCheckerEnv {
    virtual std::list<fds_volid_t> getVolumeIds() const = 0;
    virtual uint32_t getReplicaCount(const fds_volid_t &volId) const = 0;
    virtual std::string getCatalogPath(const fds_volid_t &volId,
                                       const uint32_t &replicaIdx) const = 0;
};

/**
* @brief Checker environment implmementation for running checker in a offline mode
* At the moment offline mode requires OM to be up and all DM catalogs to be hosted
* on same node
*/
struct DMOfflineCheckerEnv : DMCheckerEnv {
    virtual std::list<fds_volid_t> getVolumeIds() const {
        // TODO(Rao):
        return std::list<fds_volid_t>();
    }
    virtual uint32_t getReplicaCount(const fds_volid_t &volId) const {
        // TODO(Rao):
        return 0;
    }
    virtual std::string getCatalogPath(const fds_volid_t &volId,
                                       const uint32_t &replicaIdx) const {
        // TODO(Rao):
        return "";
    }
};

/**
* @brief Level db adpater specific to DmPersistVolDB used in diffing.
* @see LevelDbDiffAdapter
*/
struct DmPersistVolDBDiffAdapter : LevelDbDiffAdapter {
    int compareKeys(leveldb::Iterator *itr1, leveldb::Iterator *itr2) const override {
        return comparator.Compare(itr1->key(), itr2->key());
    }

    bool compareValues(leveldb::Iterator *itr1,
                      leveldb::Iterator *itr2,
                      std::string &mismatchDetails) const override {
        /* Note this call assumes keys are equal */

        /* For now physical diff should be enough */
        return itr1->value() == itr2->value();

#if 0       // Possible logical diff implementation
        if (isVolumeDescriptor(itr1)) {
            VolumeMetaDesc volDesc1;
            auto status = volDesc1.loadSerialized(itr1->value().ToString());
            fds_verify(status == ERR_OK);

            VolumeMetaDesc volDesc2;
            auto status = volDesc2.loadSerialized(itr2->value().ToString());
            fds_verify(status == ERR_OK);
            return volDesc1 == volDesc2;
        } else if (isBlobDescriptor(itr1)) {
            /* For now physical diff should be enough */
            return itr1->value() == itr2->value();
        } else {
            /* For now physical diff should be enough */
            return itr1->value() == itr2->value();
        }
#endif
    }

    std::string keyAsString(leveldb::Iterator *itr) const override { 
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        std::stringstream ss;
        ss << key->blobId << ":" << key->objIndex;
        return ss.str();
    }

    static bool isVolumeDescriptor(leveldb::Iterator *itr) {
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        return key->blobId == VOL_META_ID;
    }

    static bool isBlobDescriptor(leveldb::Iterator *itr) {
        const BlobObjKey *key = reinterpret_cast<const BlobObjKey *>(itr->key().data());
        return key->objIndex == BLOB_META_INDEX;
    }

    BlobObjKeyComparator comparator;
};

/**
* @brief DM Checker
*/
struct DMChecker {
    explicit DMChecker(DMCheckerEnv *env) {
        this->env = env;
        totalMismatches = 0;
    }
    void logMisMatch(const fds_volid_t &volId, const std::vector<DiffResult> &mismatches) {
        totalMismatches += mismatches.size();
    }
    uint64_t run() {
        auto volumeList  = env->getVolumeIds();
        for (const auto &volId : volumeList) {
            auto replicaCnt = env->getReplicaCount(volId);

            auto primaryCatPath = env->getCatalogPath(volId, 0);
            if (primaryCatPath.empty()) {
                logMisMatch(volId,
                            {{boost::lexical_cast<std::string>(volId),
                            DiffResult::DELETED_KEY,
                            "Primary db doesn't exist"}});
                continue;
            }

            for (uint32_t i = 1; i < replicaCnt; i++) {
                auto backupCatPath = env->getCatalogPath(volId, i);
                if (backupCatPath.empty()) {
                    logMisMatch(volId,
                                {{boost::lexical_cast<std::string>(volId),
                                DiffResult::DELETED_KEY,
                                "Replica db doesn't exist"}});
                    continue;
                }

                LevelDbDiffer  differ(primaryCatPath, backupCatPath, &diffAdapter);
                std::vector<DiffResult> diffs;
                while (differ.diff(2048, diffs)) {
                    if (diffs.size() > 0) {
                        logMisMatch(volId, diffs);
                        diffs.clear();
                    }
                }
            }
        }
        return totalMismatches;
    }
    DMCheckerEnv                                                *env;
    DmPersistVolDBDiffAdapter                                   diffAdapter;
    uint64_t                                                    totalMismatches;
};
} // namespace fds

int main() {
    DMOfflineCheckerEnv env;
    fds::DMChecker checker(&env);
    auto ret = checker.run();
    return ret == 0 ? 0 : -1;
}
