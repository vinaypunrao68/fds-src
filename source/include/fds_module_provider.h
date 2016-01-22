/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_
#define SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_

#include <boost/shared_ptr.hpp>
#include <fds_config.hpp>
#include <util/properties.h>
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
 * 1. Keep it abstract.  Don't add any members to it.
 * 2. DON'T add lot of methods for returning modules in here.  Keep this interface simple.
 * Only add things are that absolutely needed for every process.  Keep in mind adding
 * an item here impacts all implementers of this interface.
 * 3. Return types are a bit inconsistent.  This because of supporting legacy code.  Ideally
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

    virtual util::Properties* getProperties() { return nullptr;}
};

#define MODULEPROVIDER() getModuleProvider()
#define CONFIG_BOOL(name, defvalue) MODULEPROVIDER()->get_fds_config()->get<fds_bool_t>(name, defvalue)
#define CONFIG_STR(name, defvalue) MODULEPROVIDER()->get_fds_config()->get<std::string>(name, defvalue)
#define CONFIG_UINT32(name, defvalue) MODULEPROVIDER()->get_fds_config()->get<fds_uint32_t>(name, defvalue)
#define CONFIG_UINT64(name, defvalue) MODULEPROVIDER()->get_fds_config()->get<fds_uint64_t>(name, defvalue)

/**
* @brief Derive from this class to have access to module provider.
* IMPORTANT - FOR ANY NEW CODE, DON'T USE GLOBALS.  INSTEAD DERIVE THIS CLASS.
*/
struct HasModuleProvider {
    explicit HasModuleProvider(CommonModuleProviderIf* provider) {
        setModuleProvider(provider);
    }
    void setModuleProvider(CommonModuleProviderIf* provider) {
        moduleProvider_ = provider;
    }
    CommonModuleProviderIf* getModuleProvider() const {
        return moduleProvider_;
    }
    CommonModuleProviderIf* moduleProvider_;
};

/*
 * IMPORTANT - Don't use getModuleProvider() directly.  Instead use the MODULEPROVIDER() macro
 */
extern CommonModuleProviderIf* getModuleProvider();


}  // namespace fds
#endif  // SOURCE_INCLUDE_FDS_MODULE_PROVIDER_H_
