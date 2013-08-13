#include <Ice/Ice.h>
#include "FDSP.h"
#include <ObjectFactory.h>
#include <Ice/ObjectFactory.h>
#include <Ice/BasicStream.h>
#include <Ice/Object.h>
#include <IceUtil/Iterator.h>
#include "list.h"
#include "StorHvisorNet.h"
#include "StorHvisorCPP.h"
#include "hvisor_lib.h"
#include "MurmurHash3.h"
//#include "tapdisk.h"

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;

extern FDSP_NetworkCon  NETPtr;
extern struct fbd_device *fbd_dev;
extern int vvc_entry_get(vvc_vhdl_t vhdl, const char *blk_name, int *num_segments, doid_t **doid_list);


BEGIN_C_DECLS
int StorHvisorProcIoRd(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	int rc, result = 0;
	struct fbd_device *fbd;

  	int n_segments = 0;
//	int flag;
//	FDSP_MsgHdrType   *sm_msg;
	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req = new FDSP_GetObjType;
	unsigned int      trans_id = 0;
	//	struct timer_list *p_ti;
	

	struct  DOID {
	  uint64_t	doid;
	  uint64_t	doid1;
	}doid;

	fds_doid_t	*doid_list;

	int      data_size    = req->secs * HVISOR_SECTOR_SIZE;
//	uint64_t data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
//	char *data_buf = req->buf;

	// fbd = req->rq_disk->private_data;
	fbd = fbd_dev;

	rc = vvc_entry_get(fbd->vhdl, "BlockName", &n_segments,(doid_t **)&doid_list);
	if (rc)
	  {
	    printf("Error reading the VVC catalog  Error code : %d req:%p\n", rc,req);
	    /* for now just return the  and ack the read. we will have to come up with logic to handle these cases */
	     //__blk_end_request_all(req, 0);
	    result=rc;
	    return result;
	  }


       	NETPtr.procIo.InitSmMsgHdr(fdsp_msg_hdr);
	fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
	fdsp_msg_hdr->msg_id =  1;
	memcpy((void *)&(get_obj_req->data_obj_id), (void *)&(doid_list[0].bytes[0]), sizeof(doid));
	get_obj_req->data_obj_len = data_size;
/*
	printf(" read buf addr: %p \n", data_buf);
	printf("Read Req len: %d  offset: %d  flag:%d sm_msg:%p \
				doid:%lx:%lx \n",
	       data_size, data_offset, flag, sm_msg, doid_list[0].obj_id.hash_high, doid_list[0].obj_id.hash_low);
*/
			/* open transaction  */
	trans_id = NETPtr.procIo.procJ.get_trans_id();

	NETPtr.procIo.rwlog_tbl[trans_id].trans_state = FDS_TRANS_OPEN;
	NETPtr.procIo.rwlog_tbl[trans_id].fbd_ptr = (void *)fbd;
	NETPtr.procIo.rwlog_tbl[trans_id].write_ctx = (void *)req;
	NETPtr.procIo.rwlog_tbl[trans_id].comp_req = comp_req;
	NETPtr.procIo.rwlog_tbl[trans_id].comp_arg1 = arg1;
	NETPtr.procIo.rwlog_tbl[trans_id].comp_arg2 = arg2;
	NETPtr.procIo.rwlog_tbl[trans_id].sm_msg = fdsp_msg_hdr; 
	NETPtr.procIo.rwlog_tbl[trans_id].dm_msg = NULL;
	NETPtr.procIo.rwlog_tbl[trans_id].sm_ack_cnt = 0;
	NETPtr.procIo.rwlog_tbl[trans_id].dm_ack_cnt = 0;

	fdsp_msg_hdr->req_cookie = trans_id;


       	NETPtr.fdspDPAPI->GetObject(fdsp_msg_hdr, get_obj_req);
	if ( result < 0)
	  {
	    printf("  READ-SM:Error %d: Error  sending the data %p \n ",result,req);
	    pthread_mutex_unlock(&fbd->tx_lock);
	    // if(req)
	    //  __blk_end_request_all(req, -EIO);
	    return result;
	  }
#if 0
	p_ti = (struct timer_list *)kzalloc(sizeof(struct timer_list), GFP_KERNEL);
	init_timer(p_ti);
	p_ti->function = fbd_process_req_timeout;
	p_ti->data = (unsigned long)trans_id;
	p_ti->expires = jiffies + HZ*5;
	add_timer(p_ti);
	rwlog_tbl[trans_id].p_ti = p_ti;
#endif

	return 0;
}

int StorHvisorProcIoWr(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)

{
	int   trans_id;
	int   data_size    = req->secs * HVISOR_SECTOR_SIZE;
	double data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
	char *tmpbuf = (char*)req->buf;
 	struct  DOID {
        int64_t        doid;
        int64_t        doid1;
        }doid;
	unsigned int doid_dlt_key;
        FDSP_DmNode *dm_nodes;
        FDSP_SmNode *sm_nodes;
        FDSP_DmNode *tmp_dm_node;
        FDSP_SmNode *tmp_sm_node;
        int num_nodes;


	printf(" Inside  integration stub \n");
	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_PutObjTypePtr put_obj_req = new FDSP_PutObjType;
	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;

	MurmurHash3_x64_128(tmpbuf, data_size, 0, &doid );
//	printf("Write Req len: %d  doid:%lx:%lx \n", data_size, doid.doid, doid.doid1);
	memcpy((void *)&(put_obj_req->data_obj_id), (void *)&doid, sizeof(doid));

	put_obj_req->data_obj = std::string((const char *)tmpbuf, (size_t )data_size);
	put_obj_req->data_obj_len = data_size;



	printf(" sending   catalog update to DM \n");
	memcpy((void *)&(upd_obj_req->data_obj_id), (void *)&doid, sizeof(doid));
	upd_obj_req->volume_offset = data_offset;
	upd_obj_req->volume_offset = data_offset;
	upd_obj_req->dm_transaction_id = 1;
	upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_OPEN;

	doid_dlt_key = doid.doid >> 56;
	trans_id = NETPtr.procIo.procJ.get_trans_id();

		NETPtr.procIo.rwlog_tbl[trans_id].fbd_ptr = (void *)fbd_dev;
        NETPtr.procIo.rwlog_tbl[trans_id].trans_state = FDS_DMGR_TXN_STATUS_OPEN;
        NETPtr.procIo.rwlog_tbl[trans_id].write_ctx = (void *)req;
        NETPtr.procIo.rwlog_tbl[trans_id].comp_req = comp_req;
        NETPtr.procIo.rwlog_tbl[trans_id].comp_arg1 = arg1;
        NETPtr.procIo.rwlog_tbl[trans_id].comp_arg2 = arg2;
        NETPtr.procIo.rwlog_tbl[trans_id].sm_msg = fdsp_msg_hdr;
        NETPtr.procIo.rwlog_tbl[trans_id].dm_msg = fdsp_msg_hdr_dm;
        NETPtr.procIo.rwlog_tbl[trans_id].sm_ack_cnt = 0;
        NETPtr.procIo.rwlog_tbl[trans_id].dm_ack_cnt = 0;
        NETPtr.procIo.rwlog_tbl[trans_id].dm_commit_cnt = 0;


// 		fdsp_msg_hdr->src_ip_lo_addr = trans_id;
//		fdsp_msg_hdr_dm->src_ip_lo_addr = trans_id;
 		fdsp_msg_hdr->req_cookie = trans_id;
        fdsp_msg_hdr_dm->req_cookie = trans_id;
       	NETPtr.procIo.InitSmMsgHdr(fdsp_msg_hdr);

        sm_nodes = NETPtr.procIo.procT.get_sm_nodes_for_doid_key(doid_dlt_key);
        num_nodes = 0;
        list_for_each_entry(tmp_sm_node,& sm_nodes->list, list) {

        	NETPtr.procIo.rwlog_tbl[trans_id].sm_ack[num_nodes].ipAddr = tmp_sm_node->node_ipaddr;
 			fdsp_msg_hdr->dst_ip_lo_addr = tmp_sm_node->node_ipaddr;
        	NETPtr.procIo.rwlog_tbl[trans_id].sm_ack[num_nodes].ack_status = FDS_CLS_ACK;
        	num_nodes++;

        	NETPtr.fdspDPAPI->PutObject(fdsp_msg_hdr, put_obj_req);
	}
        NETPtr.procIo.rwlog_tbl[trans_id].num_sm_nodes = num_nodes;


        dm_nodes = NETPtr.procIo.procT.get_dm_nodes_for_volume(fbd_dev->vol_id);
        num_nodes = 0;

        list_for_each_entry(tmp_dm_node, & dm_nodes->list, list) {

          NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[num_nodes].ipAddr = tmp_dm_node->node_ipaddr;
 		  fdsp_msg_hdr->dst_ip_lo_addr = tmp_dm_node->node_ipaddr;
          NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[num_nodes].ack_status = FDS_CLS_ACK;
	  	  NETPtr.procIo.rwlog_tbl[trans_id].dm_ack[num_nodes].commit_status = FDS_CLS_ACK;
          num_nodes++;
          NETPtr.procIo.InitDmMsgHdr(fdsp_msg_hdr_dm);
          NETPtr.fdspDPAPI->UpdateCatalogObject(fdsp_msg_hdr_dm, upd_obj_req);
        }
        NETPtr.procIo.rwlog_tbl[trans_id].num_dm_nodes = num_nodes;

	return 0;
}
END_C_DECLS


void FDSP_Proc_Io::InitSmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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
        msg_hdr->glob_volume_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_STOR_MGR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


}

void FDSP_Proc_Io::InitDmMsgHdr(const FDSP_MsgHdrTypePtr& msg_hdr)
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
        msg_hdr->glob_volume_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_DATA_MGR;

   		msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;

}


int FDSP_procJourn::fds_init_trans_log(void)
{
	int i =0;

        for(i=0; i<= FDS_READ_WRITE_LOG_ENTRIES; i++)
        {
                NETPtr.procIo.rwlog_tbl[i].trans_state = FDS_TRANS_EMPTY;
                NETPtr.procIo.rwlog_tbl[i].replc_cnt = 0;
                NETPtr.procIo.rwlog_tbl[i].sm_ack_cnt = 0;
                NETPtr.procIo.rwlog_tbl[i].dm_ack_cnt = 0;
                NETPtr.procIo.rwlog_tbl[i].st_flag = 0;
                NETPtr.procIo.rwlog_tbl[i].lt_flag = 0;
        }

	return 0;
}

int FDSP_procJourn::get_trans_id(void)
{

  static int t_id = 1;

        return t_id++;

}
