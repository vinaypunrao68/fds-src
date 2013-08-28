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


#define SRC_IP  0x0a010a65

BEGIN_C_DECLS
int StorHvisorProcIoRd(void *dev_hdl, fbd_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2)
{
	struct fbd_device *fbd;
    FDS_RPC_EndPoint *endPoint = NULL; 
    unsigned int node_ip = 0;
	unsigned int doid_dlt_key=0;
    int num_nodes;
    int node_ids[256];
    int node_state = -1;
    fds::Error err(ERR_OK);
    ObjectID oid;

	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_GetObjTypePtr get_obj_req = new FDSP_GetObjType;
	unsigned int      trans_id = 0;
	fds_uint64_t data_offset  = req->sec * HVISOR_SECTOR_SIZE;
	fbd = fbd_dev;



   	storHvisor->InitSmMsgHdr(fdsp_msg_hdr);
	fdsp_msg_hdr->msg_code = FDSP_MSG_GET_OBJ_REQ;
	fdsp_msg_hdr->msg_id =  1;
    /* open transaction  */
    trans_id = storHvisor->journalTbl->get_trans_id();
    StorHvJournalEntry *journEntry = storHvisor->journalTbl->get_journal_entry(trans_id);

	journEntry->trans_state = FDS_TRANS_OPEN;
	journEntry->fbd_ptr = (void *)fbd;
	journEntry->write_ctx = (void *)req;
	journEntry->comp_req = comp_req;
	journEntry->comp_arg1 = arg1; // vbd
	journEntry->comp_arg2 = arg2; //vreq
	journEntry->sm_msg = fdsp_msg_hdr; 
	journEntry->dm_msg = NULL;
	journEntry->sm_ack_cnt = 0;
	journEntry->dm_ack_cnt = 0;
    journEntry->op = FDS_IO_READ;
    journEntry->data_obj_id.hash_high = 0;
    journEntry->data_obj_id.hash_low = 0;
    journEntry->data_obj_len = 0x1000;

	fdsp_msg_hdr->req_cookie = trans_id;

    err  = storHvisor->volCatalogCache->Query((fds_uint64_t)fbd->vol_id, data_offset, trans_id, &oid); 
    if (err.GetErrno() == ERR_PENDING_RESP) {
	printf("Vol catalog Cache Query pending : %d req:%p\n", err.GetErrno(),req);
            journEntry->trans_state = FDS_TRANS_VCAT_QUERY_PENDING;
            return 0;
    }

	if (err.GetErrno() == ERR_CAT_QUERY_FAILED)
	{
	    printf("Error reading the Vol catalog  Error code : %d req:%p\n", err.GetErrno(),req);
	    return err.GetErrno();
	}


        // We have a Cache HIT *$###
        //
        uint64_t doid_dlt = oid.GetHigh();
        doid_dlt_key = (doid_dlt >> 56);


        fdsp_msg_hdr->req_cookie = trans_id;

    	journEntry->op = FDS_IO_READ;
    	journEntry->data_obj_id.hash_high = oid.GetHigh();;
    	journEntry->data_obj_id.hash_low = oid.GetLow();;
    	journEntry->data_obj_len = req->len;

        // Lookup the Primary SM node-id/ip-address to send the GetObject to
        storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
        if(num_nodes == 0) {
          return -1;
        }
        storHvisor->dataPlacementTbl->getNodeInfo(node_ids[0], &node_ip, &node_state);

        // *****CAVEAT: Modification reqd
        // ******  Need to find out which is the primary SM and send this out to that SM. ********
        storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip, FDSP_STOR_MGR, &endPoint);

        // RPC Call GetObject to StorMgr
         journEntry->trans_state = FDS_TRANS_GET_OBJ;
         if (endPoint)
		 { 
       	   endPoint->fdspDPAPI->begin_GetObject(fdsp_msg_hdr, get_obj_req);
	   printf("Hvisor: Sent async GetObj req to SM\n");
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
 	ObjectID  objID;
	unsigned char doid_dlt_key;
	struct fbd_device *fbd;
    int num_nodes;
    FDS_RPC_EndPoint *endPoint = NULL;
    int node_ids[256];
    fds_uint32_t node_ip = 0;
    int node_state = -1;

	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_PutObjTypePtr put_obj_req = new FDSP_PutObjType;
	FDS_ProtocolInterface::FDSP_MsgHdrTypePtr fdsp_msg_hdr_dm = new FDSP_MsgHdrType;
	FDS_ProtocolInterface::FDSP_UpdateCatalogTypePtr upd_obj_req = new FDSP_UpdateCatalogType;

        fbd = (struct fbd_device *)dev_hdl;

        // Obtain MurmurHash on the data object
        MurmurHash3_x64_128(tmpbuf, data_size, 0, &objID );

	put_obj_req->data_obj = std::string((const char *)tmpbuf, (size_t )data_size);
	put_obj_req->data_obj_len = data_size;

        put_obj_req->data_obj_id.hash_high = upd_obj_req->data_obj_id.hash_high = objID.GetHigh();
        put_obj_req->data_obj_id.hash_low = upd_obj_req->data_obj_id.hash_low = objID.GetLow();
        fdsp_msg_hdr->glob_volume_id = fbd->vol_id;;
        fdsp_msg_hdr_dm->glob_volume_id = fbd->vol_id;;

	doid_dlt_key = objID.GetHigh() >> 56;

        //  *** Get a new Journal Entry in xaction-log journalTbl
	trans_id = storHvisor->journalTbl->get_trans_id();
        StorHvJournalEntry *journEntry = storHvisor->journalTbl->get_journal_entry(trans_id);

        // *** Initialize the journEntry with a open txn
	journEntry->fbd_ptr = (void *)fbd_dev;
        journEntry->trans_state = FDS_TRANS_OPEN;
        journEntry->write_ctx = (void *)req;
        journEntry->comp_req = comp_req;
        journEntry->comp_arg1 = arg1;
        journEntry->comp_arg2 = arg2;
        journEntry->sm_msg = fdsp_msg_hdr;
        journEntry->dm_msg = fdsp_msg_hdr_dm;
        journEntry->sm_ack_cnt = 0;
        journEntry->dm_ack_cnt = 0;
        journEntry->dm_commit_cnt = 0;
        journEntry->op = FDS_IO_WRITE;
        journEntry->data_obj_id.hash_high = objID.GetHigh();
        journEntry->data_obj_id.hash_low = objID.GetLow();
        journEntry->data_obj_len= data_size;;

        fdsp_msg_hdr->src_ip_lo_addr = SRC_IP;
    	fdsp_msg_hdr->req_cookie = trans_id;
   		storHvisor->InitSmMsgHdr(fdsp_msg_hdr);


        // DLT lookup from the dataplacement object
        num_nodes = 256;
        storHvisor->dataPlacementTbl->getDLTNodesForDoidKey(doid_dlt_key, node_ids, &num_nodes);
        for (i = 0; i < num_nodes; i++) {
           node_ip = 0;
           node_state = -1;
           // storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[i], &node_ip, &node_state);
           storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i], &node_ip, &node_state);
           journEntry->sm_ack[i].ipAddr = node_ip;
 	   fdsp_msg_hdr->dst_ip_lo_addr = node_ip;
           journEntry->sm_ack[i].ack_status = FDS_CLS_ACK;
           journEntry->num_sm_nodes = num_nodes;

           // Call Put object RPC to SM
            storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip, FDSP_STOR_MGR, &endPoint);
            if (endPoint) { 
                endPoint->fdspDPAPI->begin_PutObject(fdsp_msg_hdr, put_obj_req);
		printf("Hvisor: Sent async PutObj request to SM at %u\n", node_ip);
            }
	}

        // DMT lookup from the data placement object
        num_nodes = 256;
        storHvisor->InitDmMsgHdr(fdsp_msg_hdr_dm);
     	upd_obj_req->volume_offset = data_offset;
     	upd_obj_req->dm_transaction_id = 1;
     	upd_obj_req->dm_operation = FDS_DMGR_TXN_STATUS_OPEN;
     	fdsp_msg_hdr_dm->req_cookie = trans_id;
     	fdsp_msg_hdr_dm->src_ip_lo_addr = SRC_IP;

        storHvisor->dataPlacementTbl->getDMTNodesForVolume(fbd_dev->vol_id, node_ids, &num_nodes);

        for (i = 0; i < num_nodes; i++) {
           node_ip = 0;
           node_state = -1;
           //storHvisor->dataPlacementTbl->omClient->getNodeInfo(node_ids[i], &node_ip, &node_state);
           storHvisor->dataPlacementTbl->getNodeInfo(node_ids[i], &node_ip, &node_state);

           journEntry->dm_ack[i].ipAddr = node_ip;
 	   	   fdsp_msg_hdr_dm->dst_ip_lo_addr = node_ip;
           journEntry->dm_ack[i].ack_status = FDS_CLS_ACK;
	   	   journEntry->dm_ack[i].commit_status = FDS_CLS_ACK;
           journEntry->num_dm_nodes = num_nodes;
   
           // Call Update Catalog RPC call to DM
           storHvisor->rpcSwitchTbl->Get_RPC_EndPoint(node_ip, FDSP_DATA_MGR, &endPoint);
           if (endPoint){
               endPoint->fdspDPAPI->begin_UpdateCatalogObject(fdsp_msg_hdr_dm, upd_obj_req);
	       printf("Hvisor: Sent async UpdCatObj request to DM at %u\n", node_ip);
           }
        }

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
