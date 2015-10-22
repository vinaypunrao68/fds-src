/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_OBJECTREFSCANNER_H_
#define SOURCE_DATA_MGR_INCLUDE_OBJECTREFSCANNER_H_

#include <list>
#include <mutex>
#include <string>
#include <fds_module.h>
#include <fds_module_provider.h>
#include <dmhandler.h>
#include <fds_timer.h>

namespace fds {
/* Forward declarations */
struct ObjectRefMgr;
struct DataMgr;


/**
* @brief Manages bloomfilters.
* -Bloomfilters are stored in filesystem.
* -Based on cache size, recently used bloomfilters are kept in memory
*/
struct BloomFilterStore {
    BloomFilterStore(const std::string &path, uint32_t cacheSize);
    virtual ~BloomFilterStore();
    BloomFilterPtr get(const std::string &key, bool create = true);

 protected:
    BloomFilterPtr load(const std::string &key);
    void save(const std::string &key, BloomFilterPtr bloomfilter);
    void addToCache(const std::string &key, util::BloomFilterPtr bloomfilter);
    util::BloomFilterPtr getFromCache(const std::string &key);

    struct BFNode {
        uint32_t                accessCnt;
        std::string             key;
        util::BloomFilterPtr    bloomfilter;
    };
    /* Path where all bloomfilters managed by this store are stored */
    std::string                             basePath;
    /* Maximum bloomfilters to keep in cache */
    uint32_t                                maxCacheSize;
    /* Size of each bloomfilter in bits */
    uint32_t                                bloomfilterBits;
    /* set of bloomfilter names/keys being managed */
    std::set<std::string>                   index;
    /* bloomfilter cache */
    std::list<BFNode>                       cache;
    /* access counter used by the cache.  Updated on every cache acesss. */
    uint32_t                                accessCnt;
};

/**
* @brief Object reference scanner interface
*/
struct ObjectRefScanner {
    explicit ObjectRefScanner(ObjectRefMgr* m)
    : objRefMgr(m) {
    }
    virtual ~ObjectRefScanner() {}
    virtual Error scan() = 0;
    virtual Error postScan() = 0;
    virtual bool isComplete() const = 0;
    virtual std::string logString() const = 0;

 protected:
    ObjectRefMgr        *objRefMgr;
};
using ObjectRefScannerPtr = boost::shared_ptr<ObjectRefScanner>;

/**
* @brief Manages collection of ObjectRefScanner.  Each ObjectRefScanner represents
* either a volume  or a snapshot
*/
struct VolumeRefScannerContext {
    VolumeRefScannerContext(ObjectRefMgr* m, fds_volid_t vId);
    Error scan();
    Error postScan();
    bool isComplete() const;
    std::string logString() const;

    std::string                                     logStr;
    std::list<ObjectRefScannerPtr>                  scanners;
    std::list<ObjectRefScannerPtr>::iterator        itr;
};

/**
* @brief Manages collection of VolumeRefScannerContext one for each volume.
* 1. Scan of all of the volumes is scheduled on timer at configured interval
* 2. During each scan cycle all the volumes are scanned.  Scan cycle is broken up
* into small scan steps.
* 3. Each scan step is executed on qos threadpool.
* 4. During each scan step configured number of level db entries are scanned and 
* appropriate bloom filter for the volume is update.
*/
struct ObjectRefMgr : HasModuleProvider, Module {
    ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm);
    virtual ~ObjectRefMgr() = default;
    virtual void mod_startup();
    virtual void mod_shutdown();

    /**
    * @brief Runs the scanner for scanning volumes and snaps.  Currently we scan
    * one at a time.
    */
    void scan();

    inline DataMgr* getDataMgr() const { return dataMgr; }
    inline uint32_t getMaxEntriesToScan() const { return maxEntriesToScan; }
    inline DLT* getCurrentDlt() const { return currentDlt; }
    inline BloomFilterStore* getBloomFiltersStore() const {return bfStore.get(); }

 protected:
    /**
    * @brief Builds up list of volumes and snapshots to scan
    */
    void buildScanList();

    enum State {
        DISABLED,
        SCHEDULED,
        RUNNING
    };

    DataMgr                                     *dataMgr;
    DLT                                         *currentDlt;
    std::unique_ptr<BloomFilterStore>           bfStore;
    dm::Handler                                 qosHelper;
    uint32_t                                    maxEntriesToScan;
    std::chrono::seconds                        scanIntervalSec;
    FdsTimerTaskPtr                             scanTask;
    std::mutex                                  scannerLock;
    State                                       state;
    std::list<VolumeRefScannerContext>          scanList;
    std::list<VolumeRefScannerContext>::iterator currentItr;
};

}  // namespace fds

#endif      // SOURCE_DATA_MGR_INCLUDE_OBJECTREFSCANNER_H_
