/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_

#include <vector>
#include <string>
#include <boost/msm/back/state_machine.hpp>
#include <OmResources.h>
#include <OmDataPlacement.h>

namespace fds {

class ClusterMap;
struct DmtDplyFSM;
typedef boost::msm::back::state_machine<DmtDplyFSM> FSM_DplyDMT;

/**
 * OM DMT deployment events.
 */
class DmtDeployEvt
{
  public:
    DmtDeployEvt() {}
    std::string logString() const {
        return "DmtDeployEvt";
    }
};

class DmtLoadedDbEvt
{
  public:
    DmtLoadedDbEvt() {}
    std::string logString() const {
        return "DmtLoadedDbEvt";
    }
};

class DmtPushMetaAckEvt
{
  public:
    explicit DmtPushMetaAckEvt(const NodeUuid& uuid)
      : dm_uuid(uuid) {}
    std::string logString() const {
        return "DmtPushMetaAckEvt";
    }

    NodeUuid dm_uuid;
};

class DmtCommitAckEvt
{
  public:
    explicit DmtCommitAckEvt(fds_uint64_t dmt_ver,
                             fpi::FDSP_MgrIdType type)
            : dmt_version(dmt_ver),
            svc_type(type) {}
    std::string logString() const {
        return "DmtCommitAckEvt";
    }

    fds_uint64_t dmt_version;
    fpi::FDSP_MgrIdType svc_type;
};

class DmtCloseOkEvt
{
  public:
    explicit DmtCloseOkEvt(fds_uint64_t dmt_ver)
            : dmt_version(dmt_ver) {}
    std::string logString() const {
        return "DmtCloseOkEvt";
    }

    fds_uint64_t dmt_version;
};

class DmtVolAckEvt
{
  public:
    explicit DmtVolAckEvt(const NodeUuid& uuid)
            : dm_uuid(uuid) {}
    std::string logString() const {
        return "DDmtVolAckEvt";
    }

    NodeUuid dm_uuid;
};
/**
 * Main vector to initialize the DMT module.
 */
class OM_DMTMod : public Module
{
  public:
    explicit OM_DMTMod(char const *const name);
    ~OM_DMTMod();

    /**
     * Return the current state of the DMT deployment FSM.
     */
    char const *const dmt_deploy_curr_state();

    /**
     * Apply an event to DMT deploy state machine.
     */
    void dmt_deploy_event(DmtDeployEvt const &evt);
    void dmt_deploy_event(DmtPushMetaAckEvt const &evt);
    void dmt_deploy_event(DmtCommitAckEvt const &evt);
    void dmt_deploy_event(DmtCloseOkEvt const &evt);
    void dmt_deploy_event(DmtVolAckEvt const &evt);
    void dmt_deploy_event(DmtLoadedDbEvt const &evt);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    FSM_DplyDMT     *dmt_dply_fsm;
    // to protect access to msm process_event
    fds_mutex       fsm_lock;
};

extern OM_DMTMod             gl_OMDmtMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
