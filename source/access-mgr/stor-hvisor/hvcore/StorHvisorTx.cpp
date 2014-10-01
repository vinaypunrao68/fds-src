#include "list.h"
#include "StorHvisorNet.h"
#include "fds_qos.h"
#include "qos_ctrl.h"
#include <hash/MurmurHash3.h>
#include <arpa/inet.h>
#include <fds_timestamp.h>
#include <platform/platform-lib.h>

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace fds;

extern StorHvCtrl *storHvisor;

#define SRC_IP  0x0a010a65

void StorHvCtrl::InitSmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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
    msg_hdr->dst_id = FDSP_STOR_MGR;
    
    msg_hdr->src_node_name = this->my_node_name;
    msg_hdr->src_service_uuid.uuid =
            (fds_int64_t)(PlatformProcess::plf_manager()->
                          plf_get_my_svc_uuid())->uuid_get_val();
    
    msg_hdr->origin_timestamp = fds::get_fds_timestamp_ms();

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;
    msg_hdr->proxy_count = 0;
}

void StorHvCtrl::InitDmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
{
    msg_hdr->msg_code = FDSP_MSG_UPDATE_CAT_OBJ_REQ;
    msg_hdr->msg_id =  1;

    msg_hdr->major_ver = 0xa5;
    msg_hdr->minor_ver = 0x5a;

    msg_hdr->num_objects = 1;
    msg_hdr->frag_len = 0;
    msg_hdr->frag_num = 0;

    msg_hdr->tennant_id = 0;
    msg_hdr->local_domain_id = 0;

    msg_hdr->src_id = FDSP_STOR_HVISOR;
    msg_hdr->dst_id = FDSP_DATA_MGR;

    msg_hdr->src_node_name = this->my_node_name;

    msg_hdr->err_code = ERR_OK;
    msg_hdr->result = FDSP_ERR_OK;
    msg_hdr->proxy_count = 0;
}
