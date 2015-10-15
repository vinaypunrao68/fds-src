/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#include <sstream>
#include <lib/Catalog.h>
#include <ObjectRefScanner.h>
#include <DataMgr.h>
#include <util/bloomfilter.h>

namespace fds {

/**
* @brief Scans objects in a volume and updates bloomfilter
*/
struct VolumeObjectRefScanner : ObjectRefScanner {
    VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId);
    Error init();
    virtual Error scan() override;
    virtual Error postScan() override;

    virtual bool isComplete()  const override { return (itr != nullptr && !(itr->Valid())); }
    virtual std::string logString() const override { return logStr; }

    bool                                            bInited;
    fds_volid_t                                     volId;
    Catalog::MemSnap                                snap;
    std::unique_ptr<Catalog::catalog_iterator_t>    itr;
    std::string                                     logStr;
    util::BloomFilter                               bloomfilter; 

};

/**
* @brief Scans objects in a snapshot and updates bloomfilter
*/
struct SnapshotRefScanner : ObjectRefScanner {
};

ObjectRefMgr::ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm)
: HasModuleProvider(moduleProvider),
  Module("RefcountScanner"),
  dataMgr(dm),
  qosHelper(*dm)
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
            qosHelper.addToQueue(req);
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
        /* Scan step */
        Error e = currentItr->scan();
        if (e != ERR_OK) {
            /* Report the error and move on */
            GLOGWARN<< "Failed to scan: " << currentItr->logString() << " " << e;
            currentItr->postScan();
            currentItr++;
        } else {
            if (currentItr->isComplete()) {
                currentItr->postScan();
                currentItr++;
            }
        }
    }

    if (currentItr != scanList.end()) {
        /* Schedule next scan step */
        auto req = new DmFunctor(FdsDmSysTaskId, std::bind(&ObjectRefMgr::scan, this));
        qosHelper.addToQueue(req);
    } else {
        /* Schedule another scan cycle */
        auto timer = MODULEPROVIDER()->getTimer();
        timer->schedule(scanTask, scanIntervalSec);
    }
}

void ObjectRefMgr::buildScanList() {
    // TODO(Rao): Get list of volumes and create volume contexts for each volume
}

VolumeRefScannerContext::VolumeRefScannerContext(ObjectRefMgr* m, fds_volid_t vId)
{
    std::stringstream ss;
    ss << "VolumeRefScannerContext:" << vId;
    logStr = ss.str();

    scanners.push_back(ObjectRefScannerPtr(new VolumeObjectRefScanner(m, vId)));
    // TODO(Rao): Add snapshots as well
    itr = scanners.begin();
}

Error VolumeRefScannerContext::scan() {
    Error e = ERR_OK;
    if (itr != scanners.end()) {
        auto scanner = *itr;
        e = scanner->scan();
        if (e != ERR_OK) {
            GLOGWARN<< "Failed to scan: " << scanner->logString() << " " << e;
            scanner->postScan();
            itr++;
        } else {
            if (scanner->isComplete()) {
                scanner->postScan();
                itr++;
            }
        }
    }
    return e;
}

Error VolumeRefScannerContext::postScan() {
    // TODO(Rao): Either write the bloom filter to  a file or schedule a task to send the
    // bloom filters to SMs
    return ERR_OK;
}

bool VolumeRefScannerContext::isComplete() const {
    return itr == scanners.end();
}

std::string VolumeRefScannerContext::logString() const {
    return logStr;
}

VolumeObjectRefScanner::VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId)
: ObjectRefScanner(m),
    bInited(false),
    volId(vId)
{
    std::stringstream ss;
    ss << "VolumeObjectRefScanner:" << volId;
    logStr = ss.str();
}

Error VolumeObjectRefScanner::init() {
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

Error VolumeObjectRefScanner::scan() {
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

Error VolumeObjectRefScanner::postScan() {
    auto volcatIf = objRefMgr->getDataMgr()->timeVolCat_->queryIface();
    auto err = volcatIf->freeVolumeSnapshot(volId, snap);
    // TODO(Rao): Update the aggregate bloomfilter
    return err;
}

}  // namespace fds
