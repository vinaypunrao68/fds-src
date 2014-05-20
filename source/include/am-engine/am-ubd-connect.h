/*
 * Copyright 2014 Formation Data Systems, Inc.
 */
#ifndef SOURCE_INCLUDE_AM_ENGINE_AM_UBD_CONNECT_H_
#define SOURCE_INCLUDE_AM_ENGINE_AM_UBD_CONNECT_H_

#include <string>
#include <fds_module.h>
#include <native_api.h>
#include <stor-hvisor/StorHvisorCPP.h>

namespace fds {

/**
 * Class that provides interface mapping between
 * UBD, the user space block device, and AM.
 */
class AmUbdConnect : public Module {
  private:
    FDS_NativeAPI::ptr       am_api;

  public:
    explicit AmUbdConnect(const std::string &name);
    virtual ~AmUbdConnect();

    typedef boost::shared_ptr<AmUbdConnect> ptr;

    int  mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();
    void init_server(FDS_NativeAPI::ptr api);

    Error amUbdPutBlob(fbd_request_t *blkReq);
    Error amUbdGetBlob(fbd_request_t *blkReq);
};

extern AmUbdConnect gl_AmUbdConnect;

}  // namespace fds

#endif  // SOURCE_INCLUDE_AM_ENGINE_AM_UBD_CONNECT_H_
