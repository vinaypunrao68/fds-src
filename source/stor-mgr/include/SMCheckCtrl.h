/*
 * Copyright 2015 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_STOR_MGR_INCLUDE_SMCHECKCTRL_H_
#define SOURCE_STOR_MGR_INCLUDE_SMCHECKCTRL_H_

#include <fds_module.h>
#include <fds_error.h>
#include <fdsp/sm_types_types.h>

namespace fds {

// Forward declaration.
class DLT;
class SMCheckOnline;
class SmIoReqHandler;


// Controller for online SM checker.
class SMCheckControl : public Module{
  public:
    SMCheckControl(const std::string &moduleName,
                   SmDiskMap::ptr diskmap,
                   SmIoReqHandler *datastore);
    ~SMCheckControl();

    typedef std::unique_ptr<SMCheckControl> unique_ptr;

    // Start online SM check.
    // TODO(Sean):  For now, once it starts, there is no way to stop.
    // Need to implement a way to stop it.
    Error startSMCheck(std::set<fds_token_id> tgtTokens);

    // TODO(Sean):  not yet implemented.
    Error stopSMCheck();

    // Update the DLT.  This will be udpated from DLT close event.
    // When updated, it will be cloned.
    void updateSMCheckDLT(const DLT *latestDLT);

    // Get status of the SM checker.
    Error getStatus(fpi::CtrlNotifySMCheckStatusRespPtr status);

    int mod_init(SysParams const *const param);
    void mod_startup();
    void mod_shutdown();

  private:
    // Pointer to online SM checker.
    SMCheckOnline *SMChk;
};  // SMCheckControl

}  // namespace fds

#endif  // SOURCE_STOR_MGR_INCLUDE_SMCHECKCTRL_H_

