/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_

#include <OmResources.h>
#include <boost/msm/back/state_machine.hpp>

namespace fds {

class ClusterMap;
class DataPlacement;
struct DltDplyFSM;
typedef boost::msm::back::state_machine<DltDplyFSM> FSM_DplyDLT;

/**
 * OM DLT deployment events.
 */
struct DltCompEvt
{
    explicit DltCompEvt(DataPlacement *d)
        : ode_dp(d) {}

    DataPlacement            *ode_dp;
};

struct DltUpdateEvt
{
    explicit DltUpdateEvt(ClusterMap *m)
        : ode_clusmap(m) {}

    ClusterMap               *ode_clusmap;
};

struct DltUpdateOkEvt
{
    DltUpdateOkEvt() {}

    ClusterMap               *ode_clusmap;
    NodeAgent::pointer        ode_done_node;
};

struct DltCommitEvt
{
    DltCommitEvt() {}

    ClusterMap               *ode_clusmap;
};

struct DltCommitOkEvt
{
    DltCommitOkEvt() {}

    ClusterMap               *ode_clusmap;
    NodeAgent::pointer        ode_done_node;
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
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
    FSM_DplyDLT             *dlt_dply_fsm;
};

extern OM_DLTMod             gl_OMDltMod;

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
