#include <iostream>

#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>
#include <IceUtil/IceUtil.h>

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "include/fds_volume.h"
#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"

using namespace std;
using namespace fds;
//using namespace FDSP_Types;
using namespace FDS_ProtocolInterface;

void initOMMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
	msg_hdr->minor_ver = 0;
	msg_hdr->msg_code = FDSP_MSG_PUT_OBJ_REQ;
        msg_hdr->msg_id =  1;

        msg_hdr->major_ver = 0xa5;
        msg_hdr->minor_ver = 0x5a;

        msg_hdr->num_objects = 1;
        msg_hdr->frag_len = 0;
        msg_hdr->frag_num = 0;

        msg_hdr->tennant_id = 0;
        msg_hdr->local_domain_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_ORCH_MGR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;
}

class ControlPathReq : public FDS_ProtocolInterface::FDSP_ControlPathReq {
  
 private:
  
  
 public:
  ControlPathReq() { }
  ~ControlPathReq() { }
  
  void NotifyAddVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                    const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
                    const Ice::Current&) { }
  
  void NotifyRmVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
		   const FDS_ProtocolInterface::FDSP_NotifyVolTypePtr& vol_msg,
		   const Ice::Current&) { }
  
  
  void AttachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                 const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
                 const Ice::Current&) { }
  
  void DetachVol(const FDS_ProtocolInterface::FDSP_MsgHdrTypePtr& msg_hdr,
                 const FDS_ProtocolInterface::FDSP_AttachVolTypePtr& vol_msg,
                 const Ice::Current&) { }

  void NotifyNodeAdd(const FDSP_MsgHdrTypePtr& msg_hdr, 
		     const FDSP_Node_Info_TypePtr& node_info,
		     const Ice::Current&) { }

  void NotifyNodeRmv(const FDSP_MsgHdrTypePtr& msg_hdr, 
		     const FDSP_Node_Info_TypePtr& node_info,
		     const Ice::Current&) { }

  void NotifyDLTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
		       const FDSP_DLT_TypePtr& dlt_info,
		       const Ice::Current&) { }

  void NotifyDMTUpdate(const FDSP_MsgHdrTypePtr& msg_hdr,
		       const FDSP_DMT_TypePtr& dmt_info,
		       const Ice::Current&) { }
  
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
  
  void CreateVolResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                     const FDSP_CreateVolTypePtr& crt_vol_resp,
                     const Ice::Current&) {}
  void DeleteVolResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                     const FDSP_DeleteVolTypePtr& del_vol_resp,
                     const Ice::Current&) {}
  void ModifyVolResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                     const FDSP_ModifyVolTypePtr& mod_vol_resp,
                     const Ice::Current&) {}
  void CreatePolicyResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_CreatePolicyTypePtr& crt_pol_resp,
                        const Ice::Current&) {}
  void DeletePolicyResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_DeletePolicyTypePtr& del_pol_resp,
                        const Ice::Current&) {}
  void ModifyPolicyResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_ModifyPolicyTypePtr& mod_pol_resp,
                        const Ice::Current&) {}
  void AttachVolResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                     const FDSP_AttachVolCmdTypePtr& atc_vol_req,
                     const Ice::Current&) {}
  void DetachVolResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                     const FDSP_AttachVolCmdTypePtr& dtc_vol_req,
                     const Ice::Current&) {}
  void RegisterNodeResp(const FDSP_MsgHdrTypePtr& fdsp_msg,
                        const FDSP_RegisterNodeTypePtr& reg_node_resp,
                        const Ice::Current&) {
    std::cout << "Got register node response" << std::endl;
  }
};

int main(int argc, char* argv[])
{

  /*
   * Grab cmdline params
   */
  fds_uint32_t om_port_num = 0;
  fds_uint32_t cp_port_num = 0;
  for (int i = 1; i < argc; i++) {
    if (strncmp(argv[i], "--port=", 7) == 0) {
      om_port_num = strtoul(argv[i] + 7, NULL, 0);
    } else if (strncmp(argv[i], "--cp_port=", 10) == 0) {
      cp_port_num = strtoul(argv[i] + 10, NULL, 0);
    } else {
      std::cerr << "Invalid command line: Unknown argument" << std::endl;
      std::cout << "Unit test PASSED" << std::endl;
      return -1;
    }
  }

  fds_log *om_utest_log = new fds_log("om_test", "logs");

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  Ice::CommunicatorPtr comm;
  comm = Ice::initialize(argc, argv, initData);
  
  Ice::PropertiesPtr om_props = comm->getProperties();
  std::string ConfigPathStr;
  if (om_port_num == 0) {
    /*
     * Read from config where to contact OM
     */
    om_port_num = om_props->getPropertyAsInt("OMgr.ConfigPort");
  }
  ConfigPathStr = "OrchMgr:tcp -h localhost -p " + std::to_string(om_port_num);

  Ice::ObjectPrx obj = comm->stringToProxy(ConfigPathStr);
  FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(obj);

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
  std::string ContPathStr = std::string("tcp -p ") + std::to_string(cp_port_num);
  Ice::ObjectAdapterPtr rpc_adapter = comm->createObjectAdapterWithEndpoints("OrchMgrClient", ContPathStr);
  ControlPathReq *cpr = new ControlPathReq();
  rpc_adapter->add(cpr, comm->stringToIdentity("OrchMgrClient"));
  rpc_adapter->activate();

  Ice::ObjectAdapterPtr adapter = comm->createObjectAdapter("");
  Ice::Identity ident;
  ident.name = IceUtil::generateUUID();
  ident.category = "";
  TestResp* fdspDataPathResp;
  fdspDataPathResp = new TestResp();
  if (!fdspDataPathResp) {
    throw "Invalid fdspDataPathRespCback";
  }

  adapter->add(fdspDataPathResp, ident);
  adapter->activate();
  
  fdspConfigPathAPI->ice_getConnection()->setAdapter(adapter);
  fdspConfigPathAPI->AssociateRespCallback(ident);
  
  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  initOMMsgHdr(msg_hdr);

  FDSP_RegisterNodeTypePtr reg_node_msg = new FDSP_RegisterNodeType;
  reg_node_msg->ip_hi_addr = 0;
  /*
   * We're expecting to contact OM on localhost.
   * Set hex 127.0.0.1 value.
   * TODO: Make this cmdline configurable.
   */
  reg_node_msg->ip_lo_addr = 0x7f000001;
  reg_node_msg->control_port = cp_port_num;
  /*
   * This unit test isn't using data path requests
   * so we leave the port as 0.
   */
  reg_node_msg->data_port = 0;

  int i = 0;

  for (i = 0; i < 3; i++) {
    /*
     * TODO: Change to an external entity, since HVISOR is not
     * going to be issuing config requests to OM.
     */
    reg_node_msg->node_name = std::string("Node ") + std::to_string(i);
    reg_node_msg->node_type = (i == 0)? FDSP_DATA_MGR:((i==1)?FDSP_STOR_MGR:FDSP_STOR_HVISOR);
    // reg_node_msg->control_port = 7900 + i;

    FDS_PLOG(om_utest_log) << "OM unit test client registering node " << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr) << " control port:" << reg_node_msg->control_port 
                           << " data port:" << reg_node_msg->data_port
                           << " with Orchaestration Manager at " << ConfigPathStr;

    try {
      fdspConfigPathAPI->RegisterNode(msg_hdr, reg_node_msg);
    } catch(const ::Ice::OperationNotExistException& e) {
      std::cout << "Caught op not exist exception " << std::endl;
    } catch(const ::Ice::ConnectionLostException& e) {
      std::cout << "Caught conn lost exception " << std::endl;
    } catch(const ::Ice::ConnectionRefusedException& e) {
      std::cout << "Caught conn refused exception " << std::endl;
    }catch(const ::Ice::UnknownLocalException& e) {
      std::cout << "Caught unknown local exception " << std::endl;
    } catch(...) {
      std::cout << "Caught some strange exception " << std::endl;
    }

    FDS_PLOG(om_utest_log) << "OM unit test client completed node registration.";

  }

  FDSP_CreateVolTypePtr crt_vol = new FDSP_CreateVolType();
  crt_vol->vol_info = new FDSP_VolumeInfoType();

  for (i = 0; i < 3; i++) {

    crt_vol->vol_name = std::string("Volume ") + std::to_string(i);
    crt_vol->vol_info->vol_name = crt_vol->vol_name;
    crt_vol->vol_info->volUUID = i;
    crt_vol->vol_info->capacity = 1024 * 1024 * 1024; // 1 Gig
    crt_vol->vol_info->volType = FDSP_VOL_BLKDEV_TYPE;
    crt_vol->vol_info->consisProtocol = FDSP_CONS_PROTO_STRONG;
    crt_vol->vol_info->appWorkload = FDSP_APP_WKLD_TRANSACTION;

    FDS_PLOG(om_utest_log) << "OM unit test client creating volume " << crt_vol->vol_info->vol_name
			   << " with UUID " << crt_vol->vol_info->volUUID 
			   << " and capacity " << crt_vol->vol_info->capacity;

    fdspConfigPathAPI->CreateVol(msg_hdr, crt_vol);

    FDS_PLOG(om_utest_log) << "OM unit test client completed creating volume.";

  }

  std::cout << "Unit test PASSED" << std::endl;
  
}
