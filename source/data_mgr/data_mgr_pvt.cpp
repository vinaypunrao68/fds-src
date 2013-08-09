#include "data_mgr_pvt.h"

int dmgr_req_struct_size[] = {

  sizeof(dm_req_t), //noop cmd
  sizeof(dm_load_vol_req_t), // load volume request
  sizeof(dm_open_txn_req_t), // open transaction cmd
  sizeof(dm_commit_txn_req_t), 
  sizeof(dm_cancel_txn_req_t)

}; 

int alloc_and_fill_dm_req_from_msg(const char *mesg, 
                                   void *cli_addr, int cli_addr_len, dm_req_t **p_dm_req) {

  int cmd;
  int alloc_sz;

  fdsp_msg_t *fdsp_msg = (fdsp_msg_t *) mesg;
  fdsp_update_catalog_t *dm_msg;
  dm_req_t *req;
  volid_t volid;

  // For now, the only cmd code we are interested in.
  if (fdsp_msg->msg_code !=  FDSP_MSG_UPDATE_CAT_OBJ_REQ) {
    return (-1);
  }
  dm_msg = (fdsp_update_catalog_t *)&(fdsp_msg->payload.update_catalog);

  cmd = DM_MSG_CMD_CODE(fdsp_msg);
  volid = DM_MSG_VOLID(fdsp_msg);
  dmgr_log(LOG_INFO, "Constructing request with command code %d for volume id %d\n", cmd, volid);

  alloc_sz = dmgr_req_struct_size[cmd];
  req = (dm_req_t *)malloc(alloc_sz);
  memset(req, 0, alloc_sz);

  req->rsp_info.rsp_to_addr = (void *) malloc(cli_addr_len);
  memcpy(req->rsp_info.rsp_to_addr, cli_addr, cli_addr_len);
  req->rsp_info.addr_len = cli_addr_len;
  req->rsp_info.req_id = DM_MSG_REQ_ID(fdsp_msg);
  req->rsp_info.req_cookie = DM_MSG_REQ_COOKIE(fdsp_msg);
  req->cmd = cmd;

  switch(cmd) {

  case FDS_DMGR_CMD_LOAD_VOLUME:
    {
      dm_load_vol_req_t *lv_req = (dm_load_vol_req_t *)req;

      lv_req->vvc_vol_id = DM_MSG_VOLID(fdsp_msg);
      break;

    };

  case FDS_DMGR_CMD_OPEN_TXN:
    {
      dm_open_txn_req_t *ot_req = (dm_open_txn_req_t *)req;
      int i;
      fds_object_id_t *p_obj_id;

      ot_req->txn_id = DM_MSG_OT_TXNID(fdsp_msg);
      ot_req->vvc_vol_id = DM_MSG_VOLID(fdsp_msg);
      ot_req->vvc_blk_id = DM_MSG_OT_BLKID(fdsp_msg);
      p_obj_id = &(DM_MSG_OT_OBJID(fdsp_msg));
      memcpy(&(ot_req->vvc_obj_id.obj_id), p_obj_id, sizeof(fds_object_id_t));
      dmgr_log(LOG_INFO, "Open transaction request with blk %d, obj_id - %llx%llx\n", 
	       DM_MSG_VOLID(fdsp_msg), p_obj_id->hash_high, p_obj_id->hash_low);
      ot_req->vvc_update_time = DM_MSG_OT_UPDTIME(dm_msg);
      break;

    };

  case FDS_DMGR_CMD_COMMIT_TXN:
    {
      dm_commit_txn_req_t *ct_req = (dm_commit_txn_req_t *)req;
      ct_req->txn_id = DM_MSG_CT_TXNID(fdsp_msg);
      ct_req->vvc_vol_id = DM_MSG_VOLID(fdsp_msg);
      break;
    }

  case FDS_DMGR_CMD_CANCEL_TXN:
    {
      dm_cancel_txn_req_t *ca_req = (dm_cancel_txn_req_t *)req;
      ca_req->txn_id = DM_MSG_CaT_TXNID(fdsp_msg);
      ca_req->vvc_vol_id = DM_MSG_VOLID(fdsp_msg);
      break;
    }

  default:
    break;
  }

  *p_dm_req = req;

  return (0);

}
