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
#include <util/stringutils.h>
#include <util/bloomfilter.h>

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
    /**
    * @brief returns a refernce to stored bloomfilter.  When create is set to true
    * if bloomfilter with key doesn't exist then a new bloomfilter is created and
    * returned.  If create set to false and bloomfilter with key doesn't exit then
    * null is returned.
    *
    * @param key
    * @param create
    *
    * @return 
    */
    util::BloomFilterPtr get(const std::string &key, bool create = true);

    /**
    * @brief Writes all the cached bloomfilters to filesystem
    */
    void sync();
    
    /**
    * @brief clears out the index and removes all bloomfilters from the filesystem
    */
    void purge();

    inline size_t getIndexSize() const { return index.size(); } 

 protected:
    util::BloomFilterPtr load(const std::string &key);
    void save(const std::string &key, util::BloomFilterPtr bloomfilter);
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
struct ObjectRefScannerIf {
    explicit ObjectRefScannerIf(ObjectRefMgr* m)
    : objRefMgr(m),
      state(INIT),
      completionError(ERR_OK)
    {
    }
    virtual ~ObjectRefScannerIf() {}
    virtual Error scanStep() = 0;
    virtual Error finishScan(const Error &e) = 0;
    virtual bool isComplete() const = 0;
    virtual std::string logString() const = 0;

 protected:
    enum State {
        INIT,
        SCANNING,
        COMPLETE
    };

    /* Reference to parent manager */
    ObjectRefMgr        *objRefMgr;
    /* Current scanner state */
    State               state;
    /* Status that scanner completed with */
    Error               completionError;
};
using ObjectRefScannerPtr = boost::shared_ptr<ObjectRefScannerIf>;

/**
* @brief Manages collection of ObjectRefScanner.  Each ObjectRefScanner represents
* either a volume  or a snapshot
*/
struct VolumeRefScannerContext : ObjectRefScannerIf {
    VolumeRefScannerContext(ObjectRefMgr* m, fds_volid_t vId);
    Error scanStep();
    Error finishScan(const Error &e);
    bool isComplete() const;
    std::string logString() const;

    fds_volid_t                                     volId;
    std::string                                     logStr;
    std::list<ObjectRefScannerPtr>                  scanners;
    std::list<ObjectRefScannerPtr>::iterator        itr;
};

/**
* @brief Scans objects in a volume and updates bloomfilter
*/
struct VolumeObjectRefScanner : ObjectRefScannerIf {
    VolumeObjectRefScanner(ObjectRefMgr* m, fds_volid_t vId);
    Error init();
    virtual Error scanStep() override;
    virtual Error finishScan(const Error &e) override;

    virtual bool isComplete()  const override { return (itr != nullptr && !(itr->Valid())); }
    virtual std::string logString() const override { return logStr; }

    fds_volid_t                                     volId;
    Catalog::MemSnap                                snap;
    std::unique_ptr<Catalog::catalog_iterator_t>    itr;
    std::string                                     logStr;
};

/**
* @brief Scans objects in a snapshot and updates bloomfilter
*/
struct SnapshotRefScanner : ObjectRefScannerIf {
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
    using ScanDoneCb = std::function<void(ObjectRefMgr*)>;
    ObjectRefMgr(CommonModuleProviderIf *moduleProvider, DataMgr* dm);
    virtual ~ObjectRefMgr() = default;
    virtual void mod_startup();
    virtual void mod_shutdown();
    /* Use this to manually start scan. Don't use it when timer based scan is enabled */
    void scanOnce(ScanDoneCb cb);

    util::BloomFilterPtr getTokenBloomFilter(const fds_token_id &tokenId);

    void dumpStats() const;

    inline DataMgr* getDataMgr() const { return dataMgr; }
    inline uint32_t getMaxEntriesToScan() const { return maxEntriesToScan; }
    inline const DLT* getCurrentDlt() const { return currentDlt; }
    inline BloomFilterStore* getBloomFiltersStore() const {return bfStore.get(); }
    std::list<fds_volid_t>& getScanSuccessVols() { return scanSuccessVols; }

    inline static std::string volTokBloomFilterKey(const fds_volid_t& volId, 
                                                   const fds_token_id &tokenId) {
        return util::strformat("vol%ld_tok%d", volId, tokenId);
    }

    inline static std::string tokenBloomFilterKey(const fds_token_id &tokenId) {
        return util::strformat("aggr_tok%d", tokenId);
    }
    inline size_t getScanSuccessVolsCnt() const {return scanSuccessVols.size();}
    inline uint64_t getObjectsScannedCntr() const { return objectsScannedCntr; }

 protected:
    /**
    * @brief Runs the scanner for scanning volumes and snaps.  Currently we scan
    * one at a time.
    */
    void scanStep();
    /**
    * @brief Initialization before a scan cycle.  Builds up list of volumes and
    * snapshots to scan
    */
    void prescanInit();

    DataMgr                                     *dataMgr;
    const DLT                                   *currentDlt;
    std::unique_ptr<BloomFilterStore>           bfStore;
    dm::Handler                                 qosHelper;
    /* Controls whether scanning based on timer is enabled or not.   NOTE it is still possible
     * to run the scanner manually
     */
    bool                                        enabled;
    uint32_t                                    maxEntriesToScan;
    std::chrono::seconds                        scanIntervalSec;
    FdsTimerTaskPtr                             scanTask;
    std::mutex                                  scannerLock;
    std::list<VolumeRefScannerContext>          scanList;
    std::list<VolumeRefScannerContext>::iterator currentItr;
    std::list<fds_volid_t>                      scanSuccessVols;
    ScanDoneCb                                  scandoneCb;
    uint32_t                                    scanCntr;
    uint64_t                                    objectsScannedCntr;
    friend class VolumeRefScannerContext;
    friend class VolumeObjectRefScanner;
};

}  // namespace fds

#endif      // SOURCE_DATA_MGR_INCLUDE_OBJECTREFSCANNER_H_
