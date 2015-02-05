/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_
#define SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_

#include <boost/shared_ptr.hpp>
#include <fds_config.hpp>

namespace fds {

/* Forward declarations */
class FdsTimer;
typedef boost::shared_ptr<FdsTimer> FdsTimerPtr;
class FdsCountersMgr;
typedef boost::shared_ptr<FdsCountersMgr> FdsCounterMgrPtr;
class FdsRootDir;
class fds_threadpool;
class Platform;
class TracebufferPool;
struct SvcMgr;

/* Interface for providing common process modules
 * NOTES
 * 1. Keep it abastract.  Don't add any memebers to it.
 * 2. DON'T add lot of methods for returning modules in here.  Keep this interface simple.
 * Only add things are that absolutely needed for every process.  Keep in mind adding
 * an item here impacts all implemeters of this interface.
 * 3. Return types are a bit consistent.  This because of supporting legacy code.  Ideally
      all the interfaces should return just a pointer.
*/
// TODO(Rao):
// 1. Make all the returns below implement Module interface
// 2. Have a consistent method naming
class CommonModuleProviderIf {
 public:
    virtual ~CommonModuleProviderIf() {}
    virtual FdsConfigAccessor get_conf_helper() const { return FdsConfigAccessor(); }

    virtual boost::shared_ptr<FdsConfig> get_fds_config() const { return nullptr; }

    virtual FdsTimerPtr getTimer() const { return nullptr; }

    virtual boost::shared_ptr<FdsCountersMgr> get_cntrs_mgr() const { return nullptr; }

    virtual const FdsRootDir *proc_fdsroot() const  { return nullptr; }

    virtual fds_threadpool *proc_thrpool() const { return nullptr; }

    virtual Platform* get_plf_manager() { return nullptr; }

    virtual TracebufferPool* getTracebufferPool() {return nullptr; }

    virtual SvcMgr* getSvcMgr() {return nullptr;}
};

extern CommonModuleProviderIf* gModuleProvider;

}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_
