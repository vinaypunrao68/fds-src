/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_

#include <vector>
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
class DltCompRebalEvt
{
  public:
    DltCompRebalEvt(ClusterMap *c,
                DataPlacement *d,
                OM_SmContainer::pointer sm_nodes)
           : ode_cm(c),
            ode_dp(d),
            ode_sm_nodes(sm_nodes) {}

    ClusterMap               *ode_cm;
    DataPlacement            *ode_dp;
    OM_SmContainer::pointer  ode_sm_nodes;
};

class DltNoRebalEvt
{
  public:
    DltNoRebalEvt() {}
};

class DltRebalOkEvt
{
  public:
    DltRebalOkEvt(ClusterMap *cm, DataPlacement *d)
        : ode_clusmap(cm), ode_dp(d) {}

    ClusterMap            *ode_clusmap;
    DataPlacement         *ode_dp;
};

class DltCommitOkEvt
{
  public:
    explicit DltCommitOkEvt(fds_uint64_t dlt_ver)
            : cur_dlt_version(dlt_ver) {}

    fds_uint64_t    cur_dlt_version;
    NodeAgent::pointer        ode_done_node;
};

class DltCloseOkEvt
{
  public:
    DltCloseOkEvt() {}
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
    void dlt_deploy_event(DltCompRebalEvt const &evt);
    void dlt_deploy_event(DltNoRebalEvt const &evt);
    void dlt_deploy_event(DltRebalOkEvt const &evt);
    void dlt_deploy_event(DltCommitOkEvt const &evt);
    void dlt_deploy_event(DltCloseOkEvt const &evt);

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  private:
    FSM_DplyDLT             *dlt_dply_fsm;
};

extern OM_DLTMod             gl_OMDltMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
