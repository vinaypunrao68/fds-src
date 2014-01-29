/*
 * Copyright 2013 by Formation Data Systems, Inc.
 */
#ifndef SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
#define SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_

#include <OmResources.h>

namespace fds {

class OMDeployState
{
  public:
    explicit OMDeployState(Module *owner) {}
    ~OMDeployState() {}

    virtual void fsm_action() = 0;
  protected:
};

class OMDeploy_CompDLT : public OMDeployState
{
  public:
    void fsm_action() {}

  protected:
};

class OMDeploy_UpdateDLT : public OMDeployState
{
  public:
    void fsm_action() {}

  protected:
};

class OMDeploy_CommitDLT : public OMDeployState
{
  public:
    void fsm_action() {}

  protected:
};

/**
 * Simple state machine to deploy the new DLT
 */
class OMDeployDLT_FSM
{
  public:
  protected:
};

class NodeAgent;

/**
 * Interface class to compute the DLT.  TBD
 *
 * Input:
 *    Number of columns.
 *    Old DLT or current DLT.
 *    List of nodes in the membership.  Each node is abstracted by NodeAgent.
 * Output:
 *    New DLT
 */

/**
 * Main vector to initialize the DLT module.
 */
class OM_DLTMod : public Module
{
  public:
    explicit OM_DLTMod(char const *const name) : Module(name) {}
    ~OM_DLTMod() {}

    /**
     * Module methods
     */
    virtual int  mod_init(SysParams const *const param);
    virtual void mod_startup();
    virtual void mod_shutdown();

  protected:
};

}  // namespace fds

#endif  // SOURCE_ORCH_MGR_INCLUDE_OMDEPLOY_H_
