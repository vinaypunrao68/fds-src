/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_

#include <vector>
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
class DmtComputeEvt
{
  public:
    explicit DmtComputeEvt(fds_uint32_t volNum)
            : numVols(volNum) {}

    fds_uint32_t    numVols;
};

class DmtLoadedDbEvt
{
  public:
    DmtLoadedDbEvt() {}
};

class DmtCloseOkEvt
{
  public:
    DmtCloseOkEvt() {}
};

class DmtVolAckEvt
{
  public:
    DmtVolAckEvt() {}
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
    void dmt_deploy_event(DmtComputeEvt const &evt);
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
    FSM_DplyDMT             *dmt_dply_fsm;
};

extern OM_DMTMod             gl_OMDmtMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDMTDEPLOY_H_
