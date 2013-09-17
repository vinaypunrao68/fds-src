#include <iostream>

#include <Ice/Ice.h>
#include <IceStorm/IceStorm.h>

#include "include/fds_types.h"
#include "include/fds_err.h"
#include "include/fds_volume.h"
#include "fdsp/fdsp_types.h"
#include "fdsp/FDSP.h"
#include "include/fds_err.h"
#include "util/Log.h"
#include "util/concurrency/Mutex.h"

using namespace std;
using namespace fds;
using namespace FDSP_Types;
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

int main(int argc, char* argv[])
{

  fds_log *om_utest_log = new fds_log("om_test", "logs");

  Ice::InitializationData initData;
  initData.properties = Ice::createProperties();
  initData.properties->load("om_client.conf");

  Ice::CommunicatorPtr comm;
  comm = Ice::initialize(argc, argv, initData);

  std::string tcpProxyStr = "OrchMgr:tcp -h localhost -p 8903";
  Ice::ObjectPrx obj = comm->stringToProxy(tcpProxyStr);
  FDSP_ConfigPathReqPrx fdspConfigPathAPI = FDSP_ConfigPathReqPrx::checkedCast(obj);

  FDSP_MsgHdrTypePtr msg_hdr = new FDSP_MsgHdrType;
  initOMMsgHdr(msg_hdr);

  FDSP_RegisterNodeTypePtr reg_node_msg = new FDSP_RegisterNodeType;
  reg_node_msg->node_type = FDSP_STOR_HVISOR;
  reg_node_msg->ip_hi_addr = 0;
  reg_node_msg->ip_lo_addr = 0x0a010aca; //10.1.10.202
  reg_node_msg->control_port = 7901;
  reg_node_msg->data_port = 0; // for now

  int i = 0;

  for (i = 0; i < 3; i++) {

    reg_node_msg->node_id = std::string("Node ") + std::to_string(i);
    reg_node_msg->node_type = (i == 0)? FDSP_DATA_MGR:((i==1)?FDSP_STOR_MGR:FDSP_STOR_HVISOR);
    reg_node_msg->control_port = 7900 + i;

    FDS_PLOG(om_utest_log) << "OM unit test client registering node " << fds::ipv4_addr_to_str(reg_node_msg->ip_lo_addr) << " control port:" << reg_node_msg->control_port 
		    << " data port:" << reg_node_msg->data_port
		    << " with Orchaestration Manager at " << tcpProxyStr;

    // fdspConfigPathAPI->RegisterNode(msg_hdr, reg_node_msg);

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
  
}
