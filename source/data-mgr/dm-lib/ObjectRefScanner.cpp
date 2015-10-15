/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <ObjectRefScanner.h>
#include <DataMgr.h>
#include <util/bloomfilter.h>

namespace fds {

ObjectRefMgr::ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm)
: HasModuleProvider(moduleProvider),
  Module("RefcountScanner"),
  dataMgr(dm),
  handler(*dm)
{
    // TODO(Rao): assign other memebers
}

void ObjectRefMgr::mod_startup() {
    auto timer = MODULEPROVIDER()->getTimer();
    scanTask = boost::shared_ptr<FdsTimerTask>(
        new FdsTimerFunctionTask(
            *timer,
            [this] () {
            /* Immediately post to threadpool so we don't hold up timer thread */
            auto req = new DmFunctor(FdsDmSysTaskId, std::bind(&ObjectRefMgr::scan, this));
            handler.addToQueue(req);
            }));
    timer->schedule(scanTask, scanIntervalSec);
    // TODO(Rao): Set members based on config
    // TODO(Rao): schdule scanner based on config
}

void ObjectRefMgr::mod_shutdown() {
}

// TODO(Rao): Check if we need any synchronization
void ObjectRefMgr::scan() {
    if (scanList.size() == 0) {
        buildScanList();
    }

    if (currentItr != scanList.end()) {
        auto current = *currentItr;
        /* Scan step */
        Error e = current->scan();
        if (e != ERR_OK) {
            /* Report the error and move on */
            GLOGWARN<< "Failed to scan: " << current->logString() << " " << e;
            currentItr++;
        } else {
            if (current->isComplete()) {
                // TODO(Rao): necessary cleanups
                currentItr++;
            }
        }
    }

    if (currentItr != scanList.end()) {
        auto req = new DmFunctor(FdsDmSysTaskId, std::bind(&ObjectRefMgr::scan, this));
        handler.addToQueue(req);
    } else {
        auto timer = MODULEPROVIDER()->getTimer();
        timer->schedule(scanTask, scanIntervalSec);
    }
}

//TODO(Rao): Support scanning of multiple snapshots
struct VolumeObjectRefScanner : ObjectRefScanner {
    VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId)
    : ObjectRefScanner(m),
    bInited(false),
    volId(vId)
    {
        std::stringstream ss;
        ss << "VolumeObjectRefScanner:" << volId;
        logStr = ss.str();
    }

    Error init() {
        /* Create snapshot */
        auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
        auto err = volcatIf->getVolumeSnapshot(volId, snap);
        if (err != ERR_OK) {
            return err;
        }
        // TODO(Rao): Start the iterator based on snapshot
        // We should lock the volume as well to protect against deletions
        return err;
    }

    virtual Error scan() override {
        if (!bInited) {
            auto error = init();
            if (error != ERR_OK) {
                GLOGWARN << "Failed to initialize " << logString();
                return error;
            }
            bInited = true;
        }

        std::list<ObjectID> objects;
        auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
        volcatIf->getObjectIds(volId, objRefMgr->getMaxEntriesToScan(), snap, itr, objects); 
        for (const auto &id : objects) {
            bloomfilter.add(id);
        }

        return ERR_OK;
    }

    virtual bool isComplete()  override { return (itr != nullptr && !(itr->Valid())); }
    virtual std::string logString() override { return logStr; }

    bool                                            bInited;
    fds_volid_t                                     volId;
    Catalog::MemSnap                                snap;
    std::unique_ptr<Catalog::catalog_iterator_t>    itr;
    std::string                                     logStr;
    util::BloomFilter                               bloomfilter; 

};

struct SnapshotRefScanner : ObjectRefScanner {
};




}  // namespace fds
