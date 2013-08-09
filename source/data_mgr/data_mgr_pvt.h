#include <syslog.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "fds_commons.h"
#include "include/data_mgr.h"
#include "include/fdsp.h"

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

  fds_uint32_t node_ip;
 
} fds_node_t;

typedef struct __rsp_info {

  int      sockfd;
  void     *rsp_to_addr; // sockaddr_t to respond to;
  int      addr_len;
  fds_uint32_t req_id; // need to include back in the response
  fds_uint32_t req_cookie; // need to include back in the response so request can correlate response with the request

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
  fds_uint16_t     vvc_vol_id;
  
} dm_load_vol_req_t;

typedef struct __dm_open_txn_req {

  dm_req_t         req_hdr;
  fds_uint32_t     txn_id;
  fds_uint16_t     vvc_vol_id;
  fds_uint64_t     vvc_blk_id;
  fds_doid_t       vvc_obj_id;
  fds_uint64_t     vvc_update_time;
  
} dm_open_txn_req_t;

typedef struct __dm_commit_txn_req {

  dm_req_t         req_hdr;
  fds_uint32_t     txn_id;
  fds_uint16_t     vvc_vol_id;
  
} dm_commit_txn_req_t;

typedef dm_commit_txn_req_t dm_cancel_txn_req_t;


static __inline__ int dm_req_msg_sanity_check(const char *mesg) {
  
  return (0);

}

#define DM_MSG_REQ_ID(fdsp_msg) fdsp_msg->msg_id
#define DM_MSG_VOLID(fdsp_msg) fdsp_msg->glob_volume_id
#define DM_MSG_REQ_COOKIE(fdsp_msg) fdsp_msg->req_cookie
#define DM_MSG_CMD_CODE(fdsp_msg) fdsp_msg->payload.update_catalog.dm_operation

#define DM_MSG_OT_TXNID(fdsp_msg) fdsp_msg->payload.update_catalog.dm_transaction_id
#define DM_MSG_OT_BLKID(fdsp_msg) fdsp_msg->payload.update_catalog.volume_offset
#define DM_MSG_OT_UPDTIME(dm_msg) 0xab01cd34

#define DM_MSG_CT_TXNID(fdsp_msg) fdsp_msg->payload.update_catalog.dm_transaction_id

#define DM_MSG_CaT_TXNID DM_MSG_CT_TXNID

#define DM_MSG_OT_OBJID(fdsp_msg) fdsp_msg->payload.update_catalog.data_obj_id
#define DM_MSG_OT_OBJID_PTR(fdsp_msg) &(fdsp_msg->payload.update_catalog.data_obj_id)

int alloc_and_fill_dm_req_from_msg(const char *mesg, 
                                   void *cli_addr,
                                   int cli_addr_len,
                                   dm_req_t **p_dm_req);

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
  fds_uint32_t num_items;

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
