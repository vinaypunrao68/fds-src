/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <ObjectRefScanner.h>
#include <DataMgr.h>

namespace fds {

ObjectRefMgr::ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm)
: HasModuleProvider(moduleProvider),
  Module("RefcountScanner"),
  dataMgr(dm)
{
    // TODO(Rao): assign other memebers
}

void ObjectRefMgr::mod_startup() {
    MODULEPROVIDER()->getTimer();
    // TODO(Rao): Set members based on config
    // TODO(Rao): schdule scanner based on config
}

void ObjectRefMgr::mod_shutdown() {
}

void ObjectRefMgr::scan() {
    if (scanList.size() == 0) {
        buildScanList();
        currentItr = scanList.begin();
    }

    if (currentItr != scanList.end()) {
        auto current = *currentItr;
        Error e = current->scan();
        if (e != ERR_OK) {
            /* Report the error and move on */
            GLOGWARN<< "Failed to scan: " << current->logString() << " " << e;
            currentItr++;
        } else {
            if (current->isComplete()) {
                currentItr++;
            }
        }
    }

    if (currentItr != scanList.end()) {
        // TODO: Schedule another scan
    }
}

struct VolumeObjectRefScanner : ObjectRefScanner {
    VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId)
    : ObjectRefScanner(m),
    volId(vId)
    {
        std::stringstream ss;
        ss << "VolumeObjectRefScanner:" << volId;
        logStr = ss.str();
    }

    virtual Error init() override {
        /* Create snapshot */
        auto volcatIf = refMgr->getDataMgr()->timeVolCat_->queryIface();
        auto err = volcatIf->getVolumeSnapshot(volId, snap);
        if (err != ERR_OK) {
            return err;
        }
        // TODO(Rao): Start the iterator based on snapshot
        // We should lock the volume as well to protect against deletions
        return err;
    }

    virtual Error scan() override {
        for (uint32_t scannedEntriesCnt = 0;
             scannedEntriesCnt < refMgr->getMaxEntriesToScan() && itr->Valid();
             itr->Next()) {
             // TODO(Rao): Update the bloom filter
        }
        return ERR_OK;
    }

    virtual bool isComplete()  override { return !(itr->Valid()); }
    virtual std::string logString() override { return logStr; }

    fds_volid_t                                     volId;
    Catalog::MemSnap                                snap;
    std::unique_ptr<Catalog::catalog_iterator_t>    itr;
    std::string                                     logStr;
};

struct SnapshotRefScanner : ObjectRefScanner {
};




}  // namespace fds
