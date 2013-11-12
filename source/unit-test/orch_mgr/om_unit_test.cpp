/*
 * Copyright 2013 Formation Data Systems, Inc.
 */

/*
 * Unit tests for orchestration manager. These
 * test should only require the orchestration
 * manager to be up and running.
 */

#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <IceUtil/IceUtil.h>

#include <iostream>  // NOLINT(*)
#include <string>
#include <list>

#include <fds_types.h>
#include <fds_err.h>
#include <fds_volume.h>
#include <fdsp/FDSP.h>
#include <util/Log.h>
#include <concurrency/Mutex.h>

namespace fds {

class ControlPathReq : public FDS_ProtocolInterface::FDSP_ControlPathReq {
   public:
  ControlPathReq() { }
  ControlPathReq(fds_log* plog) {
    test_log = plog;
  }

  ~ControlPathReq() { }

  void SetLog(fds_log* plog) {
    assert(test_log == NULL);
    test_log = plog;
  }

  void PrintVolumeDesc(const std::string& prefix, const VolumeDesc* voldesc) {
    if (test_log) {
      assert(voldesc);
      FDS_PLOG(test_log) << prefix << ": volume " << voldesc->name
			 << " id " << voldesc->volUUID
			 << "; capacity" << voldesc->capacity
			 << "; policy id " << voldesc->volPolicyId
			 << " iops_min " << voldesc->iops_min
			 << " iops_max " << voldesc->iops_max
			 << " rel_prio " << voldesc->relativePrio;
    }
  }

  void NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                    const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
                    const Ice::Current&) { 
    assert(vol_msg->type == FDS_ProtocolInterface::FDSP_NOTIFY_ADD_VOL);
    VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
    PrintVolumeDesc("NotifyAddVol", vdb);  
    delete vdb;
  }

  void NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                   const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
                   const Ice::Current&) { }

  void AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                 const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
                 const Ice::Current&) { 

    VolumeDesc *vdb = new VolumeDesc(vol_msg->vol_desc);
    PrintVolumeDesc("NotifyVolAttach", vdb);
    delete vdb;
  }

  void DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                 const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
                 const Ice::Current&) { }

  void NotifyNodeAdd(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_Node_Info_TypePtr&
                     node_info,
                     const Ice::Current&) { }

  void NotifyNodeRmv(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                     const FDS_ProtocolInterface::FDSP_Node_Info_TypePtr&
                     node_info,
                     const Ice::Current&) { }

  void NotifyDLTUpdate(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       const FDS_ProtocolInterface::FDSP_DLT_TypePtr& dlt_info,
                       const Ice::Current&) {
    std::cout << "Received a DLT update" << std::endl;
    for (fds_uint32_t i = 0; i < dlt_info->DLT.size(); i++) {
      for (fds_uint32_t j = 0; j < dlt_info->DLT[i].size(); j++) {
        std::cout << "Bucket " << i << " entry " << j
                  << " value " << dlt_info->DLT[i][j] << std::endl;
      }
    }
  }

  void NotifyDMTUpdate(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                       const FDS_ProtocolInterface::FDSP_DMT_TypePtr& dmt_info,
                       const Ice::Current&) {
    std::cout << "Received a DMT update" << std::endl;
    for (fds_uint32_t i = 0; i < dmt_info->DMT.size(); i++) {
      for (fds_uint32_t j = 0; j < dmt_info->DMT[i].size(); j++) {
        std::cout << "Bucket " << i << " entry " << j
                  << " value " << dmt_info->DMT[i][j] << std::endl;
      }
    }
  }

  void SetThrottleLevel(const FDSP_MsgHdrTypePtr& msg_hdr, 
			const FDSP_ThrottleMsgTypePtr& throttle_req, 
			const Ice::Current&) {
    std::cout << "Received SetThrottleLevel with level = "
	      << throttle_req->throttle_level;
  }

  void TierPolicy(const FDSP_TierPolicyPtr &, const Ice::Current &)
  {
  }

  void TierPolicyAudit(const FDSP_TierPolicyAuditPtr &, const Ice::Current &)
  {
  }

private:
  fds_log* test_log;
};

class TestResp : public FDS_ProtocolInterface::FDSP_ConfigPathResp {
 private:
  fds_log *test_log;

 public:
  TestResp()
      : test_log(NULL) {
  }

  explicit TestResp(fds_log *log)
      : test_log(log) {
  }
  ~TestResp() {
  }

  void SetLog(fds_log *log) {
    assert(test_log == NULL);
    test_log = log;
  }

  void CreateVolResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                     fdsp_msg,
                     const FDS_ProtocolInterface::FDSP_CreateVolTypePtr&
                     crt_vol_resp,
                     const Ice::Current&) {}
  void DeleteVolResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                     fdsp_msg,
                     const FDS_ProtocolInterface::FDSP_DeleteVolTypePtr&
                     del_vol_resp,
                     const Ice::Current&) {}
  void ModifyVolResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                     fdsp_msg,
                     const FDS_ProtocolInterface::FDSP_ModifyVolTypePtr&
                     mod_vol_resp,
                     const Ice::Current&) {}
  void CreatePolicyResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                        fdsp_msg,
                        const FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr&
                        crt_pol_resp,
                        const Ice::Current&) {}
  void DeletePolicyResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                        fdsp_msg,
                        const FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr&
                        del_pol_resp,
                        const Ice::Current&) {}
  void ModifyPolicyResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                        fdsp_msg,
                        const FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr&
                        mod_pol_resp,
                        const Ice::Current&) {}
  void AttachVolResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                     fdsp_msg,
                     const FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr&
                     atc_vol_req,
                     const Ice::Current&) {}
  void DetachVolResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                     fdsp_msg,
                     const FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr&
                     dtc_vol_req,
                     const Ice::Current&) {}
  void RegisterNodeResp(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr&
                        fdsp_msg,
                        const FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr&
                        reg_node_resp,
                        const Ice::Current&) {}
};

class OmUnitTest {
 private:
  std::list<std::string>  unit_tests;

  fds_log *test_log;

  fds_uint32_t om_port_num;
  fds_uint32_t cp_port_num;
  fds_uint32_t num_updates;

  /*
   * Ice communication ptr. Should be
   * provided during class construction.
   */
  Ice::CommunicatorPtr ice_comm;

  std::list<Ice::ObjectAdapterPtr> adapterList;

  /*
   * Helper function to construct msg hdr
   */
  void initOMMsgHdr(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr) {
    msg_hdr->minor_ver = 0;
    msg_hdr->msg_code = FDS_ProtocolInterface::FDSP_MSG_PUT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
    msg_hdr->dst_id = FDS_ProtocolInterface::FDSP_ORCH_MGR;

    msg_hdr->err_code = FDS_ProtocolInterface::FDSP_ERR_SM_NO_SPACE;
    msg_hdr->result   = FDS_ProtocolInterface::FDSP_ERR_OK;
  }

  /*
   * Helper function to create and remember endpoints.
   */
  void createCpEndpoint(fds_uint32_t port) {
    std::string contPathStr = std::string("tcp -p ") + std::to_string(port);
    /*
     * Add our port to the endpoint name to make it unique.
     * The OM will need to be aware we're using this nameing
     * convention.
     */
    Ice::ObjectAdapterPtr rpc_adapter =
        ice_comm->createObjectAdapterWithEndpoints(
            "OrchMgrClient" + std::to_string(port), contPathStr);
    ControlPathReq *cpr = new ControlPathReq(test_log);
    rpc_adapter->add(cpr,
                     ice_comm->stringToIdentity(
                         "OrchMgrClient" + std::to_string(port)));
    rpc_adapter->activate();

    adapterList.push_back(rpc_adapter);
    FDS_PLOG(test_log) << "Create endpoint at " << contPathStr;
  }

  void clearCpEndpoints() {
    while (!adapterList.empty())
      {
	Ice::ObjectAdapterPtr rpc_adapter = adapterList.back();
	rpc_adapter->destroy();
	adapterList.pop_back();
      }
    adapterList.clear();
    FDS_PLOG(test_log) << "Cleared control point endpoints list";
  }

  FDS_ProtocolInterface::FDSP_ConfigPathReqPrx createOmComm(fds_uint32_t port) {
    std::string ConfigPathStr;
    /*
     * We expect the test to communicate over localhost
     */
    ConfigPathStr = "OrchMgr:tcp -h localhost -p " +
        std::to_string(om_port_num);
    Ice::ObjectPrx obj = ice_comm->stringToProxy(ConfigPathStr);
    return FDS_ProtocolInterface::FDSP_ConfigPathReqPrx::checkedCast(obj);
  }

  /*
   * Unit test funtions
   */
  fds_int32_t node_reg() {
    FDS_PLOG(test_log) << "Starting test: node_reg()";

    FDS_ProtocolInterface::FDSP_ConfigPathReqPrx fdspConfigPathAPI =
        createOmComm(om_port_num);
    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    initOMMsgHdr(msg_hdr);

    FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr reg_node_msg =
        new FDS_ProtocolInterface::FDSP_RegisterNodeType();

    reg_node_msg->domain_id  = 0;
    reg_node_msg->ip_hi_addr = 0;
    /*
     * Add some zeroed out disk info
     */
    reg_node_msg->disk_info = new FDS_ProtocolInterface::FDSP_AnnounceDiskCapability();

    /*
     * We're expecting to contact OM on localhost.
     * Set hex 127.0.0.1 value.
     * TODO: Make this cmdline configurable.
     */
    reg_node_msg->ip_lo_addr = 0x7f000001;
    /*
     * This unit test isn't using data path requests
     * so we leave the port as 0.
     */
    reg_node_msg->data_port = 0;

    for (fds_uint32_t i = 0; i < num_updates; i++) {
      /*
       * Create and endpoint to reflect the "node" that
       * we're registering, so that "node" can recieve
       * updates from the OM.
       */
      reg_node_msg->control_port = cp_port_num + i;
      createCpEndpoint(reg_node_msg->control_port);

      /*
       * TODO: Make the name just an int since the OM turns this into
       * a UUID to address the node. Fix this by adding a UUID int to
       * the FDSP.
       */
      reg_node_msg->node_name = std::to_string(i);

      /*
       * TODO: Change this to a service type since nodes registering
       * and services registering should be two different things
       * eventually.
       */
      if ((i % 3) == 0) {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_MGR;
      } else if ((i % 2) == 0) {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_DATA_MGR;
      } else {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      }

      fdspConfigPathAPI->RegisterNode(msg_hdr, reg_node_msg);

      FDS_PLOG(test_log) << "Completed node registration " << i << " at IP "
                         << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr)
                         << " control port " << reg_node_msg->control_port;
    }

    clearCpEndpoints();

    FDS_PLOG(test_log) << "Ending test: node_reg()";
    return 0;
  }

  fds_int32_t vol_reg() {
    FDS_PLOG(test_log) << "Starting test: vol_reg()";

    FDS_ProtocolInterface::FDSP_ConfigPathReqPrx fdspConfigPathAPI =
        createOmComm(om_port_num);

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr =
        new FDS_ProtocolInterface::FDSP_MsgHdrType;
    initOMMsgHdr(msg_hdr);

    FDS_ProtocolInterface::FDSP_CreateVolTypePtr crt_vol =
        new FDS_ProtocolInterface::FDSP_CreateVolType();
    crt_vol->vol_info =
        new FDS_ProtocolInterface::FDSP_VolumeInfoType();

    initOMMsgHdr(msg_hdr);
    for (fds_uint32_t i = 0; i < num_updates; i++) {
      crt_vol->vol_name = std::string("Volume ") + std::to_string(i+1);
      crt_vol->vol_info->vol_name = crt_vol->vol_name;
      crt_vol->vol_info->volUUID = i+1;
      // crt_vol->vol_info->capacity = 1024 * 1024 * 1024;  // 1 Gig
      // Currently set capacity to 0 size no one has register a storage
      // node with OM to increase/create initial capacity.
      crt_vol->vol_info->capacity = 0;
      crt_vol->vol_info->volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
      crt_vol->vol_info->consisProtocol =
          FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
      crt_vol->vol_info->appWorkload =
          FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
      crt_vol->vol_info->volPolicyId = 0;

      FDS_PLOG(test_log) << "OM unit test client creating volume "
                         << crt_vol->vol_info->vol_name
                         << " with UUID " << crt_vol->vol_info->volUUID
                         << " and capacity " << crt_vol->vol_info->capacity;

      fdspConfigPathAPI->CreateVol(msg_hdr, crt_vol);

      FDS_PLOG(test_log) << "OM unit test client completed creating volume.";
    }

    FDS_PLOG(test_log) << "Ending test: vol_reg()";
    return 0;
  }

  fds_int32_t policy_test() {
    fds_int32_t result = 0;
    FDS_PLOG(test_log) << "Starting test: volume policy";

    if (num_updates == 0) {
      FDS_PLOG(test_log) << "Nothing to do for test volume policy, num_updates == 0";
      return result;
    }

    FDS_ProtocolInterface::FDSP_ConfigPathReqPrx fdspConfigPathAPI = 
      createOmComm(om_port_num);

    FDS_ProtocolInterface::FDSP_MsgHdrTypePtr msg_hdr = 
      new FDS_ProtocolInterface::FDSP_MsgHdrType;
    initOMMsgHdr(msg_hdr);

    FDS_ProtocolInterface::FDSP_CreatePolicyTypePtr crt_pol = 
      new FDS_ProtocolInterface::FDSP_CreatePolicyType();
     crt_pol->policy_info = 
       new FDS_ProtocolInterface::FDSP_PolicyInfoType(); 

     /* first define policies we will create */
     FDS_VolumePolicy pol_tab[num_updates];
     for (fds_uint32_t i = 0; i < num_updates; ++i)
       {
	 pol_tab[i].volPolicyName = std::string("Policy ") + std::to_string(i+1);
	 pol_tab[i].volPolicyId = i+1;
	 pol_tab[i].iops_min = 100;
	 pol_tab[i].iops_max = 200;
	 pol_tab[i].relativePrio = (i+1)%8 + 1;
       }

     /* create policies */
     for (fds_uint32_t i = 0; i < num_updates; ++i)
       {
	 crt_pol->policy_name = pol_tab[i].volPolicyName;
	 crt_pol->policy_info->policy_name = crt_pol->policy_name;
	 crt_pol->policy_info->policy_id = pol_tab[i].volPolicyId;
	 crt_pol->policy_info->iops_min = pol_tab[i].iops_min;
	 crt_pol->policy_info->iops_max = pol_tab[i].iops_max;
	 crt_pol->policy_info->rel_prio = pol_tab[i].relativePrio;

	 FDS_PLOG(test_log) << "OM unit test client creating policy "
			    << crt_pol->policy_name
			    << "; id " << crt_pol->policy_info->policy_id
			    << "; iops_min " << crt_pol->policy_info->iops_min
			    << "; iops_max " << crt_pol->policy_info->iops_max
			    << "; rel_prio " << crt_pol->policy_info->rel_prio;

	 fdspConfigPathAPI->CreatePolicy(msg_hdr, crt_pol);

	 FDS_PLOG(test_log) << "OM unit test client completed creating policy";

       }

     /* register SH, DM, and SM nodes */
     FDS_ProtocolInterface::FDSP_RegisterNodeTypePtr reg_node_msg =
        new FDS_ProtocolInterface::FDSP_RegisterNodeType;

     reg_node_msg->domain_id  = 0;
     reg_node_msg->ip_hi_addr = 0;
     reg_node_msg->ip_lo_addr = 0x7f000001; /* 127.0.0.1 */
     reg_node_msg->data_port  = 0;
     /*
      * Add some zeroed out disk info
      */
     reg_node_msg->disk_info = new FDS_ProtocolInterface::FDSP_AnnounceDiskCapability();

     for (fds_uint32_t i = 0; i < 3; i++) {
      /*
       * Create and endpoint to reflect the "node" that
       * we're registering, so that "node" can recieve
       * updates from the OM.
       */
      reg_node_msg->control_port = cp_port_num + i;
      createCpEndpoint(reg_node_msg->control_port);

      /*
       * TODO: Make the name just an int since the OM turns this into
       * a UUID to address the node. Fix this by adding a UUID int to
       * the FDSP.
       */
      reg_node_msg->node_name = std::to_string(i);

      /*
       * TODO: Change this to a service type since nodes registering
       * and services registering should be two different things
       * eventually.
       */
      if (i == 0) {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_MGR;
      } else if (i == 1) {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_DATA_MGR;
      } else {
        reg_node_msg->node_type = FDS_ProtocolInterface::FDSP_STOR_HVISOR;
      }

      fdspConfigPathAPI->RegisterNode(msg_hdr, reg_node_msg);

      FDS_PLOG(test_log) << "Completed node registration " << i << " at IP "
                         << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr)
                         << " control port " << reg_node_msg->control_port;
    }



     /* create volumes, one for each policy */
     /* SM and DM should receive 'volume add' notifications that include volume policy details */
     FDS_ProtocolInterface::FDSP_CreateVolTypePtr crt_vol =
       new FDS_ProtocolInterface::FDSP_CreateVolType();
     crt_vol->vol_info =
       new FDS_ProtocolInterface::FDSP_VolumeInfoType();

     FDS_PLOG(test_log) << "OM unit test client will create volumes, one for each policy";
     const int vol_start_uuid = 900;
     for (fds_uint32_t i = 0; i < num_updates; ++i)
       {
	 crt_vol->vol_name = std::string("Volume ") + std::to_string(vol_start_uuid + i);
	 crt_vol->vol_info->vol_name = crt_vol->vol_name;
	 crt_vol->vol_info->volUUID = vol_start_uuid + i;
	 // crt_vol->vol_info->capacity = 1024 * 1024 * 1024;  // 1 Gig
         // Currently set capacity to 0 size no one has register a storage
         // node with OM to increase/create initial capacity.
         crt_vol->vol_info->capacity = 0;
	 crt_vol->vol_info->volType = FDS_ProtocolInterface::FDSP_VOL_BLKDEV_TYPE;
         crt_vol->vol_info->consisProtocol =
	   FDS_ProtocolInterface::FDSP_CONS_PROTO_STRONG;
         crt_vol->vol_info->appWorkload =
	   FDS_ProtocolInterface::FDSP_APP_WKLD_TRANSACTION;
	 crt_vol->vol_info->volPolicyId = pol_tab[i].volPolicyId;

	 FDS_PLOG(test_log) << "OM unit test client creating volume "
			    << crt_vol->vol_info->vol_name
			    << " with UUID " << crt_vol->vol_info->volUUID
			    << " and policy " << crt_vol->vol_info->volPolicyId;

	 fdspConfigPathAPI->CreateVol(msg_hdr, crt_vol);

	 FDS_PLOG(test_log) << "OM unit test client completed creating volume.";
       }

     /* attach volume 0 to SH node (node id = 2) */
     /* SH should receive first volume policy details */
     FDS_ProtocolInterface::FDSP_AttachVolCmdTypePtr att_vol =
       new FDS_ProtocolInterface::FDSP_AttachVolCmdType();
     att_vol->vol_name = std::string("Volume ") + std::to_string(vol_start_uuid);
     att_vol->vol_uuid = vol_start_uuid;
     att_vol->node_id = std::to_string(2);
     FDS_PLOG(test_log) << "OM unit test client attaching volume "
     			<< att_vol->vol_name << " UUID " << att_vol->vol_uuid 
			<< " to node " << att_vol->node_id; 
     fdspConfigPathAPI->AttachVol(msg_hdr, att_vol);
     FDS_PLOG(test_log) << "OM unit test client completed attaching volume.";

     /* modify policy with id 0 */
     /* NOTE: for now modify policy will not change anything for existing volumes 
      * need to come back to this unit test when modify policy is implemented end-to-end */
     if (num_updates > 0)
       {

         FDS_ProtocolInterface::FDSP_ModifyPolicyTypePtr mod_pol = 
            new FDS_ProtocolInterface::FDSP_ModifyPolicyType();
         mod_pol->policy_info = 
            new FDS_ProtocolInterface::FDSP_PolicyInfoType();

	 mod_pol->policy_name = pol_tab[0].volPolicyName;
	 mod_pol->policy_id = pol_tab[0].volPolicyId;
	 mod_pol->policy_info->policy_name = mod_pol->policy_name;
	 mod_pol->policy_info->policy_id = mod_pol->policy_id;
	 /* new values */
	 mod_pol->policy_info->iops_min = 333;
	 mod_pol->policy_info->iops_max = 1000;
	 mod_pol->policy_info->rel_prio = 3;

	 FDS_PLOG(test_log) << "OM unit test modifying policy "
			    << mod_pol->policy_name
			    << "; id " << mod_pol->policy_info->policy_id;

	 fdspConfigPathAPI->ModifyPolicy(msg_hdr, mod_pol);

	 FDS_PLOG(test_log) << "OM unit test client completed modifying policy";
       }

     /* delete all policies */
     FDS_ProtocolInterface::FDSP_DeletePolicyTypePtr del_pol = 
       new FDS_ProtocolInterface::FDSP_DeletePolicyType();

     for (fds_uint32_t i = 0; i < num_updates; ++i)
       {
	 del_pol->policy_name = pol_tab[i].volPolicyName;
	 del_pol->policy_id = pol_tab[i].volPolicyId; 

	 FDS_PLOG(test_log) << "OM unit test client deleting policy "
			    << del_pol->policy_name
			    << "; id" << del_pol->policy_id;

	 fdspConfigPathAPI->DeletePolicy(msg_hdr, del_pol);

	 FDS_PLOG(test_log) << "OM unit test client completed deleting policy"; 

       }
     FDS_PLOG(test_log) << " Finished test: policy";

    clearCpEndpoints();

 return result;
  }

 public:
  OmUnitTest() {
    test_log = new fds_log("om_test", "logs");

    unit_tests.push_back("node_reg");
    unit_tests.push_back("vol_reg");
    unit_tests.push_back("policy");

    num_updates = 10;
  }

  explicit OmUnitTest(fds_uint32_t om_port_arg,
                      fds_uint32_t cp_port_arg,
                      fds_uint32_t num_up_arg,
                      Ice::CommunicatorPtr ice_comm_arg)
      : OmUnitTest() {
    om_port_num = om_port_arg;
    cp_port_num = cp_port_arg;
    num_updates = num_up_arg;
    ice_comm = ice_comm_arg;
  }

  ~OmUnitTest() {
    delete test_log;
  }

  fds_log* GetLogPtr() {
    return test_log;
  }

  fds_int32_t Run(const std::string& testname) {
    int result = 0;
    std::cout << "Running unit test \"" << testname << "\"" << std::endl;

    if (testname == "node_reg") {
      result = node_reg();
    } else if (testname == "vol_reg") {
      result = vol_reg();
    } else if (testname == "policy") {
      result = policy_test();
    } else {
      std::cout << "Unknown unit test " << testname << std::endl;
    }

    if (result == 0) {
      std::cout << "Unit test \"" << testname << "\" PASSED"  << std::endl;
    } else {
      std::cout << "Unit test \"" << testname << "\" FAILED" << std::endl;
    }
    std::cout << std::endl;

    return result;
  }

  void Run() {
    fds_int32_t result = 0;
    for (std::list<std::string>::iterator it = unit_tests.begin();
         it != unit_tests.end();
         ++it) {
      result = Run(*it);
      if (result != 0) {
        std::cout << "Unit test FAILED" << std::endl;
        break;
      }
    }

    if (result == 0) {
      std::cout << "Unit test PASSED" << std::endl;
    }
  }
};
}  // namespace fds

int main(int argc, char* argv[]) {
  /*
   * Grab cmdline params
   */
  fds_uint32_t om_port_num = 0;
  fds_uint32_t cp_port_num = 0;
  fds_uint32_t num_updates = 10;
  std::string testname;
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--testname=", 11) == 0) {
      testname = argv[i] + 11;
    } else if (strncmp(argv[i], "--port=", 7) == 0) {
      om_port_num = strtoul(argv[i] + 7, NULL, 0);
    } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
      cp_port_num = strtoul(argv[i] + 10, NULL, 0);
    } else if (strncmp(argv[i], "--num_updates=", 14) == 0) {
        num_updates = atoi(argv[i] + 14);
      } else {
      std::cerr << "Invalid command line: Unknown argument "
                << argv[i] << std::endl;
      return -1;
    }
  }

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  Ice::CommunicatorPtr comm;
  comm = Ice::initialize(argc, argv, initData);

  Ice::PropertiesPtr om_props = comm->getProperties();

  if (om_port_num == 0) {
    /*
     * Read from config where to contact OM
     */
    om_port_num = om_props->getPropertyAsInt("OMgr.ConfigPort");
  }

  /*
   * Create control path server to listen for messages from OM.
   * TODO: We should fork this off into a separate function so
   * that we can create multiple clients with different ports
   * that they're listening on.
   */
  if (cp_port_num == 0) {
    /*
     * Read from config which port to listen for control path.
     * We overload the OMgrClient since this is the port the
     * OM thinks it will be talking to.
     */
    cp_port_num = om_props->getPropertyAsInt("OMgrClient.PortNumber");
  }

  fds::OmUnitTest unittest(om_port_num,
                           cp_port_num,
                           num_updates,
                           comm);
  if (testname.empty()) {
    unittest.Run();
  } else {
    unittest.Run(testname);
  }

  return 0;
}
