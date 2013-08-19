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
#include <arpa/inet.h>
//#include "tapdisk.h"

using namespace std;
using namespace FDS_ProtocolInterface;
using namespace Ice;

extern StorHvCtrl *storHvisor;
extern struct fbd_device *fbd_dev;
extern int vvc_entry_get(vvc_vhdl_t vhdl, const char *blk_name, int *num_segments, doid_t **doid_list);


BEGIN_C_DECLS
int StorHvisorProcIoRd(void *dev_hdl, fbd_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	int rc=0, result = 0;
	struct fbd_device *fbd;
        FDS_RPC_EndPoint *endPoint = NULL;
        unsigned int node_ip = 0;
	unsigned int doid_dlt_key=0;
        int num_nodes;
        int node_ids[256];
        int node_state = -1;

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

	fds_doid_t	*doid_list=NULL;

	int      data_size    = req->secs * HVISOR_SECTOR_SIZE;
//	uint64_t data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
//	char *data_buf = req->buf;

	// fbd = req->rq_disk->private_data;
	fbd = fbd_dev;

	//rc = vvc_entry_get(fbd->vhdl, "BlockName", &n_segments,(doid_t **)&doid_list);
	if (rc)
	  {
	    printf("Error reading the VVC catalog  Error code : %d req:%p\n", rc,req);
	    /* for now just return the  and ack the read. we will have to come up with logic to handle these cases */
	     //__blk_end_request_all(req, 0);
	    result=rc;
	    return result;
	  }


       	storHvisor->InitSmMsgHdr(fdsp_msg_hdr);
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
	trans_id = storHvisor->journalTbl->get_trans_id();
        StorHvJournalEntry *journEntry = storHvisor->journalTbl->get_journal_entry(trans_id);

	journEntry->trans_state = FDS_TRANS_OPEN;
	journEntry->fbd_ptr = (void *)fbd;
	journEntry->write_ctx = (void *)req;
	journEntry->comp_req = comp_req;
	journEntry->comp_arg1 = arg1;
	journEntry->comp_arg2 = arg2;
	journEntry->sm_msg = fdsp_msg_hdr; 
	journEntry->dm_msg = NULL;
	journEntry->sm_ack_cnt = 0;
	journEntry->dm_ack_cnt = 0;

	fdsp_msg_hdr->req_cookie = trans_id;


        // Lookup the Primary SM node-id/ip-address to send the GetObject to
        storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
        storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[0], &node_ip, &node_state);

        // *****CAVEAT: Modification reqd
        // ******  Need to find out which is the primary SM and send this out to that SM. ********
        char ip_address[64];
        struct sockaddr_in sockaddr;
        sockaddr.sin_addr.s_addr = node_ip;
        inet_ntop(AF_INET, &(sockaddr.sin_addr), ip_address, INET_ADDRSTRLEN);
        storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(ip_address, FDSP_STOR_MGR, endPoint);

        // RPC Call GetObject to StorMgr
        if (endPoint) { 
       	   endPoint->fdspDPAPI->GetObject(fdsp_msg_hdr, get_obj_req);
        }

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

int StorHvisorProcIoWr(void *dev_hdl, fbd_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	int   trans_id, i=0;
	int   data_size    = req->secs * HVISOR_SECTOR_SIZE;
	double data_offset  = req->sec * (uint64_t)HVISOR_SECTOR_SIZE;
	char *tmpbuf = (char*)req->buf;
 	struct  DOID {
        int64_t        doid;
        int64_t        doid1;
        }doid;
	unsigned int doid_dlt_key;
        int num_nodes;
        FDS_RPC_EndPoint *endPoint = NULL;
        int node_ids[256];
        unsigned int node_ip = 0;
        int node_state = -1;
        char ip_address[64];
        struct sockaddr_in sockaddr;

	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_PutObjTypePtr put_obj_req = new FDSP_PutObjType;
	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;

        // Obtain MurmurHash on the data object
	MurmurHash3_x64_128(tmpbuf, data_size, 0, &doid );
//	printf("Write Req len: %d  doid:%lx:%lx \n", data_size, doid.doid, doid.doid1);
	memcpy((void *)&(put_obj_req->data_obj_id), (void *)&doid, sizeof(doid));

	put_obj_req->data_obj = std::string((const char *)tmpbuf, (size_t )data_size);
	put_obj_req->data_obj_len = data_size;

	memcpy((void *)&(upd_obj_req->data_obj_id), (void *)&doid, sizeof(doid));
	upd_obj_req->volume_offset = data_offset;
	upd_obj_req->volume_offset = data_offset;
	upd_obj_req->dm_transaction_id = 1;
	upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_OPEN;

	doid_dlt_key = doid.doid >> 56;

        //  *** Get a new Journal Entry in xaction-log journalTbl
	trans_id = storHvisor->journalTbl->get_trans_id();
        StorHvJournalEntry *journEntry = storHvisor->journalTbl->get_journal_entry(trans_id);

        // *** Initialize the journEntry with a open txn
	journEntry->fbd_ptr = (void *)fbd_dev;
        journEntry->trans_state = FDS_DMGR_TXN_STATUS_OPEN;
        journEntry->write_ctx = (void *)req;
        journEntry->comp_req = comp_req;
        journEntry->comp_arg1 = arg1;
        journEntry->comp_arg2 = arg2;
        journEntry->sm_msg = fdsp_msg_hdr;
        journEntry->dm_msg = fdsp_msg_hdr_dm;
        journEntry->sm_ack_cnt = 0;
        journEntry->dm_ack_cnt = 0;
        journEntry->dm_commit_cnt = 0;


// 		fdsp_msg_hdr->src_ip_lo_addr = trans_id;
//		fdsp_msg_hdr_dm->src_ip_lo_addr = trans_id;
 	fdsp_msg_hdr->req_cookie = trans_id;
        fdsp_msg_hdr_dm->req_cookie = trans_id;
       	storHvisor->InitSmMsgHdr(fdsp_msg_hdr);


        // DLT lookup from the dataplacement object
        num_nodes = 0;
        storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
        for (i = 0; i < num_nodes; i++) {
           node_ip = 0;
           node_state = -1;
           storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[i], &node_ip, &node_state);
           journEntry->sm_ack[num_nodes].ipAddr = node_ip;
 	   fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
           journEntry->sm_ack[num_nodes].ack_status = FDS_CLS_ACK;

           // Call Put object RPC to SM
            sockaddr.sin_addr.s_addr = node_ip;
            inet_ntop(AF_INET, &(sockaddr.sin_addr), ip_address, INET_ADDRSTRLEN);
	    printf(" PutObject req RPC  to SM @%s \n", ip_address);
            storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(ip_address, FDSP_STOR_MGR, endPoint);
            if (endPoint) { 
                endPoint->fdspDPAPI->PutObject(fdsp_msg_hdr, put_obj_req);
            }
	}
        journEntry->num_sm_nodes = num_nodes;

        // DMT lookup from the data placement object
        num_nodes = 0;
        storHvisor->dataPlacementTbl->getDMTNodesForVolume(fbd_dev->vol_id, node_ids, &num_nodes);

        for (i = 0; i < num_nodes; i++) {
           node_ip = 0;
           node_state = -1;
           storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[i], &node_ip, &node_state);

           journEntry->dm_ack[num_nodes].ipAddr = node_ip;
 	   fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
           journEntry->dm_ack[num_nodes].ack_status = FDS_CLS_ACK;
	   journEntry->dm_ack[num_nodes].commit_status = FDS_CLS_ACK;
           storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
   
           // Call Update Catalog RPC call to DM
           sockaddr.sin_addr.s_addr = node_ip;
           inet_ntop(AF_INET, &(sockaddr.sin_addr), ip_address, INET_ADDRSTRLEN);
	   printf(" Catalog update to DM @ %s \n",ip_address);
           storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(ip_address, FDSP_DATA_MGR, endPoint);
           if (endPoint) {
               endPoint->fdspDPAPI->UpdateCatalogObject(fdsp_msg_hdr_dm, upd_obj_req);
           }
        }
        journEntry->num_dm_nodes = num_nodes;

	return 0;
}
END_C_DECLS


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
        msg_hdr->glob_volume_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_STOR_MGR;

        msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;


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
        msg_hdr->glob_volume_id = 0;

        msg_hdr->src_id = FDSP_STOR_HVISOR;
        msg_hdr->dst_id = FDSP_DATA_MGR;

   		msg_hdr->err_code=FDSP_ERR_SM_NO_SPACE;
        msg_hdr->result=FDSP_ERR_OK;

}


StorHvJournal::StorHvJournal(void)
{
	int i =0;

        for(i=0; i<= FDS_READ_WRITE_LOG_ENTRIES; i++)
        {
                rwlog_tbl[i].trans_state = FDS_TRANS_EMPTY;
                rwlog_tbl[i].replc_cnt = 0;
                rwlog_tbl[i].sm_ack_cnt = 0;
                rwlog_tbl[i].dm_ack_cnt = 0;
                rwlog_tbl[i].st_flag = 0;
                rwlog_tbl[i].lt_flag = 0;
        }
        next_trans_id = 1;

	return;
}

int StorHvJournal::get_trans_id(void)
{
        return next_trans_id++;

}
