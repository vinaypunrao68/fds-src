/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Umbrella classes for the orchestration  manager component.
 */

#ifndef ORCH_MGR_H
#define ORCH_MGR_H

#include <Ice/Ice.h>
#include "fdsp/fdsp_types.h"
#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "util/Log.h"

namespace fds {

  class OrchMgr : virtual public Ice::Application {
 private:

    fds_log *om_log;
    typedef FDS_ProtocolInterface::FDSP_ConfigPathReqPtr  ReqCfgHandlerPtr;
    ReqCfgHandlerPtr   ReqCfgHandlersrv;

    /*
     * Cmdline configurables
     */
    int port_num;

 public:
    OrchMgr();
    ~OrchMgr();

    virtual int run(int argc, char* argv[]);
    void interruptCallback(int);
    fds_log* GetLog();

    class reqCfgHandler : public FDS_ProtocolInterface::FDSP_ConfigPathReq {

     private:
    	Ice::CommunicatorPtr _communicator;
     public:	
	explicit reqCfgHandler(const Ice::CommunicatorPtr& communicator);
	~reqCfgHandler();

     	void CreateVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreateVolTypePtr &crt_vol_req,
			 const Ice::Current&);
  	void DeleteVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr &del_vol_req,
			 const Ice::Current&);
  	void ModifyVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr &mod_vol_req,
			 const Ice::Current&);
 	void CreatePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr &crt_pol_req,
			 const Ice::Current&);
  	void DeletePolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr &del_pol_req,
			 const Ice::Current&);
  	void ModifyPolicy(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
			 const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr &mod_pol_req,
			 const Ice::Current&);
	void RegisterNode(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr &fdsp_msg,
				const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr &reg_node_req,
			 const Ice::Current&);

    }; 

    };
  
}  // namespace fds

#endif  // ORCH_MGR_H
