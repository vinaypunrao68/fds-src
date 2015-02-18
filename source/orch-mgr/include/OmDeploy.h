/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_

#include <vector>
#include <string>
#include <boost/msm/back/state_machine.hpp>
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

class ClusterMap;
struct DltDplyFSM;
typedef boost::msm::back::state_machine<DltDplyFSM> FSM_DplyDLT;

/**
 * OM DLT deployment events.
 */
class DltComputeEvt
{
  public:
    DltComputeEvt() {}
    std::string logString() const {
        return "DltComputeEvt";
    }
};

class DltTimeoutEvt
{
  public:
    DltTimeoutEvt() {}
    std::string logString() const {
        return "DltTimeoutEvt";
    }
};

class DltLoadedDbEvt
{
  public:
    DltLoadedDbEvt() {}
    std::string logString() const {
        return "DltLoadedDbEvt";
    }
};

class DltRebalOkEvt
{
  public:
    explicit DltRebalOkEvt(NodeUuid smUuid)
            : smAcked(smUuid) {}
    std::string logString() const {
        return "DltRebalOkEvt";
    }

    NodeUuid               smAcked;
};

class DltCommitOkEvt
{
  public:
    DltCommitOkEvt(fds_uint64_t dlt_ver,
                   const NodeUuid& uuid)
            : dlt_version(dlt_ver),
            sm_uuid(uuid) {}
    std::string logString() const {
        return "DltCommitOkEvt";
    }

    fds_uint64_t    dlt_version;
    NodeUuid        sm_uuid;
};

class DltCloseOkEvt
{
  public:
    DltCloseOkEvt() {}
    std::string logString() const {
        return "DltCloseOkEvt";
    }
};

struct DltErrorFoundEvt
{
    DltErrorFoundEvt(const NodeUuid& svc_uuid,
                     const Error& err)
            : svcUuid(svc_uuid), error(err) {
    }

    std::string logString() const {
        return "DltErrorFoundEvt";
    }

    NodeUuid svcUuid;  // service uuid where error happened
    Error error;
};

struct DltEndErrorEvt
{
    DltEndErrorEvt() {}
    std::string logString() const {
        return "DltEndErrorEvt";
    }
};

struct DltAbortMigrationAckEvt
{
    DltAbortMigrationAckEvt() {}
    std::string logString() const {
        return "DltAbortMigrationAckEvt";
    }
};

struct DltRecoverAckEvt
{
    DltRecoverAckEvt(fds_bool_t abortAck,
                     const NodeUuid& uuid,
                     const Error& err)
    : ackForAbort(abortAck), svcUuid(uuid), ackError(err) {}

    std::string logString() const {
        return "DltRecoverAckEvt";
    }

    fds_bool_t ackForAbort;  // otherwise dlt commit ack
    NodeUuid svcUuid;
    Error ackError;
};

/**
 * Main vector to initialize the DLT module.
 */
class OM_DLTMod : public Module
{
  public:
    explicit OM_DLTMod(char const *const name);
    ~OM_DLTMod();

    /**
     * Return the current state of the DLT deployment FSM.
     */
    char const *const dlt_deploy_curr_state();

    /**
     * Apply an event to DLT deploy state machine.
     */
    void dlt_deploy_event(DltComputeEvt const &evt);
    void dlt_deploy_event(DltRebalOkEvt const &evt);
    void dlt_deploy_event(DltCommitOkEvt const &evt);
    void dlt_deploy_event(DltCloseOkEvt const &evt);
    void dlt_deploy_event(DltLoadedDbEvt const &evt);
    void dlt_deploy_event(DltTimeoutEvt const &evt);
    void dlt_deploy_event(DltErrorFoundEvt const &evt);
    void dlt_deploy_event(DltRecoverAckEvt const &evt);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    FSM_DplyDLT             *dlt_dply_fsm;
    // to protect access to msm process_event
    fds_mutex               fsm_lock;
};

extern OM_DLTMod             gl_OMDltMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
