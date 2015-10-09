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

namespace fds {
/* Forward declarations */
struct ObjectRefMgr;
struct DataMgr;

struct ObjectRefScanner {
    explicit ObjectRefScanner(ObjectRefMgr* m)
    : objRefMgr(m) {
    }
    virtual ~ObjectRefScanner() {}
    virtual Error init() = 0;
    virtual Error scan() = 0;
    virtual bool isComplete() = 0;
    virtual std::string logString() = 0;

 protected:
    ObjectRefMgr        *objRefMgr;
};
using ObjectRefScannerPtr = boost::shared_ptr<ObjectRefScanner>;

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
    inline DataMgr* getDataMgr() { return dataMgr; }
    inline uint32_t getMaxEntriesToScan() { return maxEntriesToScan; }

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
    uint32_t                                    maxEntriesToScan;
    std::mutex                                  scannerLock;
    State                                       state;
    std::list<ObjectRefScannerPtr>              scanList;
    std::list<ObjectRefScannerPtr>::iterator    currentItr;
};

}  // namespace fds

#endif      // SOURCE_DATA_MGR_INCLUDE_OBJECTREFSCANNER_H_
