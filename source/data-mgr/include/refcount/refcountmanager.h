/*
 * Copyright 2015 Formation Data Systems, Inc.
 */
#ifndef SOURCE_DATA_MGR_INCLUDE_REFCOUNT_REFCOUNTMANAGER_H_
#define SOURCE_DATA_MGR_INCLUDE_REFCOUNT_REFCOUNTMANAGER_H_

#include <list>
#include <fds_module.h>
#include <net/filetransferservice.h>
namespace fds { 
struct DataMgr;
namespace refcount {
struct ObjectRefScanMgr;
struct RefCountManager : Module {
    TYPE_SHAREDPTR(RefCountManager);
    using VolumeList = SHPTR<std::list<fds_volid_t> >;
    explicit RefCountManager(DataMgr* dm);

    virtual ~RefCountManager() = default;
    virtual void mod_startup();
    virtual void mod_shutdown();
    
    void scanDoneCb(ObjectRefScanMgr*);
    void objectFileTransferredCb(fds::net::FileTransferService::Handle::ptr,
                                 const Error& err,
                                 fds_token_id token,
                                 VolumeList volumeList);

    void handleActiveObjectsResponse(fds::net::FileTransferService::Handle::ptr handle,
                                     fds_token_id token,
                                     EPSvcRequest* request,
                                     const Error& error,
                                     SHPTR<std::string> payload);
  protected:
    SHPTR<ObjectRefScanMgr> scanner;
    DataMgr* dm;
};

}  // namespace refcount
}  // namespace fds

#endif // SOURCE_DATA_MGR_INCLUDE_REFCOUNT_REFCOUNTMANAGER_H_
