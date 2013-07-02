#include <syslog.h>
#include <stdlib.h>
#include <string.h>

#include "fds_commons.h"
#include "data_mgr.h"

#include <openssl/lhash.h>

#define DMGR_NUM_WORKERS 8
#define REQ_BUF_SZ FDS_DMGR_MAX_REQ_SZ

#define DMGR_TVC_FILE_NAME_SZ 256
#define DMGR_MAX_TVC_FILE_SZ (0x1 << 20)   // 2^20 = 1MB TVC file log size

#ifdef DMGR_LIVE_DEBUG

#define dmgr_log_init()
#define dmgr_log(p, s, ...) printf(s, ##__VA_ARGS__)

#else 

#define dmgr_log_init() openlog("Data Mgr", LOG_CONS | LOG_PERROR | LOG_PID, LOG_DAEMON)
#define dmgr_log syslog

#endif

typedef struct __fds_node_t {

  uint32_t node_ip;
 
} fds_node_t;

typedef struct __rsp_info {

  int      sockfd;
  void     *rsp_to_addr; // sockaddr_t to respond to;
  int      addr_len;
  uint32_t req_id; // need to include back in the response

} rsp_info_t;

typedef struct __dm_req {

  struct __dm_req *next;
  fds_node_t    from_node;
  rsp_info_t    rsp_info;
  unsigned char cmd;
  char          pad[3];
  char cmd_data[0];
} dm_req_t;

typedef struct __dm_load_vol_req {

  dm_req_t     req_hdr;
  uint16_t     vvc_vol_id;
  
} dm_load_vol_req_t;

typedef struct __dm_open_txn_req {

  dm_req_t     req_hdr;
  uint32_t     txn_id;
  uint16_t     vvc_vol_id;
  uint64_t     vvc_blk_id;
  doid_t       vvc_obj_id;
  uint64_t     vvc_update_time;
  
} dm_open_txn_req_t;

typedef struct __dm_commit_txn_req {

  dm_req_t     req_hdr;
  uint32_t     txn_id;
  uint16_t     vvc_vol_id;
  
} dm_commit_txn_req_t;

typedef dm_commit_txn_req_t dm_cancel_txn_req_t;


static __inline__ int dm_req_msg_sanity_check(const char *mesg) {
  
  return (0);

}

int dmgr_req_struct_size[] = {

  sizeof(dm_req_t), //noop cmd
  sizeof(dm_load_vol_req_t), // load volume request
  sizeof(dm_open_txn_req_t), // open transaction cmd
  sizeof(dm_commit_txn_req_t), 
  sizeof(dm_cancel_txn_req_t)

}; 

#define DM_MSG_REQ_ID(msg) 786
#define DM_MSG_CMD_CODE(msg) FDS_DMGR_CMD_OPEN_TXN
#define DM_MSG_LV_VOLID(mesg) 1
#define DM_MSG_OT_TXNID(mesg) 818
#define DM_MSG_OT_VOLID(mesg) 1
#define DM_MSG_OT_BLKID(mesg) 10325
#define DM_MSG_OT_UPDTIME(mesg) 0xab01cd34

#define DM_MSG_CT_TXNID(mesg) 818
#define DM_MSG_CT_VOLID(mesg) 1

#define DM_MSG_CaT_TXNID DM_MSG_CT_TXNID
#define DM_MSG_CaT_VOLID DM_MSG_CT_VOLID

doid_t test_obj_id = {'o','b','j','i','d','-','x','y','z'};
#define DM_MSG_OT_OBJID(mesg) test_obj_id

static __inline__ int alloc_and_fill_dm_req_from_msg(const char *mesg, dm_req_t *req, 
						     void *cli_addr, int cli_addr_len, dm_req_t **p_dm_req) {

  int cmd = DM_MSG_CMD_CODE(mesg);
  int alloc_sz = dmgr_req_struct_size[cmd];

  req = (dm_req_t *)malloc(alloc_sz);
  memset(req, 0, alloc_sz);

  req->rsp_info.rsp_to_addr = (void *) malloc(cli_addr_len);
  memcpy(req->rsp_info.rsp_to_addr, cli_addr, cli_addr_len);
  req->rsp_info.addr_len = cli_addr_len;
  req->rsp_info.req_id = DM_MSG_REQ_ID(mesg);
  req->cmd = cmd;

  switch(cmd) {

  case FDS_DMGR_CMD_LOAD_VOLUME:
    {
      dm_load_vol_req_t *lv_req = (dm_load_vol_req_t *)req;

      lv_req->vvc_vol_id = DM_MSG_LV_VOLID(mesg);
      break;

    };

  case FDS_DMGR_CMD_OPEN_TXN:
    {
      dm_open_txn_req_t *ot_req = (dm_open_txn_req_t *)req;

      ot_req->txn_id = DM_MSG_OT_TXNID(mesg);
      ot_req->vvc_vol_id = DM_MSG_OT_VOLID(mesg);
      ot_req->vvc_blk_id = DM_MSG_OT_BLKID(mesg);
      memcpy(ot_req->vvc_obj_id, DM_MSG_OT_OBJID(mesg), sizeof(doid_t));
      ot_req->vvc_update_time = DM_MSG_OT_UPDTIME(mesg);
      break;

    };

  case FDS_DMGR_CMD_COMMIT_TXN:
    {
      dm_commit_txn_req_t *ct_req = (dm_commit_txn_req_t *)req;
      ct_req->txn_id = DM_MSG_CT_TXNID(mesg);
      ct_req->vvc_vol_id = DM_MSG_CT_VOLID(mesg);
      break;
    }

  case FDS_DMGR_CMD_CANCEL_TXN:
    {
      dm_cancel_txn_req_t *ca_req = (dm_cancel_txn_req_t *)req;
      ca_req->txn_id = DM_MSG_CaT_TXNID(mesg);
      ca_req->vvc_vol_id = DM_MSG_CaT_VOLID(mesg);
      break;
    }

  default:
    break;
  }

  *p_dm_req = req;

  return (0);

}

static __inline__ void destroy_dm_req(dm_req_t *req) {

  free(req->rsp_info.rsp_to_addr);
  free(req);
  return;

}

typedef struct __dm_req_queue {

  pthread_mutex_t lock;
  pthread_cond_t not_empty;
  dm_req_t  *head;
  dm_req_t  *tail;
  uint32_t num_items;

} dm_req_queue_t;

static __inline__ dm_req_queue_t *queue_alloc() {

  dm_req_queue_t *queue;

  queue = (dm_req_queue_t *)malloc(sizeof(dm_req_queue_t));
  queue->head = NULL;
  queue->tail = NULL;
  queue->num_items = 0;
  pthread_mutex_init(&queue->lock, NULL);
  pthread_cond_init(&queue->not_empty, NULL);
  return (queue);

}

static __inline__ int queue_enqueue(dm_req_queue_t *q, dm_req_t *req) {

  if (pthread_mutex_lock(&q->lock) != 0) {
    return (-1);
  }
  if (q->tail == NULL) {
    q->head = req;
    pthread_cond_signal(&q->not_empty);
  } else {
    q->tail->next = req;
  }
  q->tail = req;
  req->next = NULL;
  q->num_items ++;
  if (pthread_mutex_unlock(&q->lock) != 0) {
    return (-1);
  }
  return (0);

}

static __inline__ dm_req_t *queue_dequeue(dm_req_queue_t *q) {

  dm_req_t *req;

  if (pthread_mutex_lock(&q->lock) != 0) {
    return (0);
  }
  while(q->num_items <= 0) {
    pthread_cond_wait(&q->not_empty, &q->lock);
  }

  if (q->head == NULL) {
    dmgr_log(LOG_CRIT, "Unable to deque. Something wildly wrong\n");
  }
  req = q->head;
  q->head = q->head->next;
  if (q->head == NULL) {
    q->tail = NULL;
  }
  q->num_items --;
  req->next = NULL;
  pthread_mutex_unlock(&q->lock);
  return (req);

}

typedef struct __dm_worker_thread {

  int id;
  pthread_t thread_id;
  dm_req_queue_t *req_queue;

} dm_wthread_t;
