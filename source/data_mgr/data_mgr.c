#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include <pthread.h>

#include "vvclib.h"
#include "tvclib.h"

#include "data_mgr.h"
#include "data_mgr_pvt.h"
#include "data_mgr_transactions.h"

dm_wthread_t *worker_thread[DMGR_NUM_WORKERS];

_LHASH *txn_cache_volume_table; 
pthread_mutex_t txn_cache_vol_table_lock;

static void daemonize(void)
{
  pid_t pid, sid;

  /* already a daemon */
  if ( getppid() == 1 ) return;

  /* Fork off the parent process */
  pid = fork();
  if (pid < 0) {
    printf("Fork failed\n");
    exit(1);
  }
  /* If we got a good PID, then we can exit the parent process. */
  if (pid > 0) {
    printf("Launched daemon with pid %d\n", pid);
    exit(0);
  }

  /* At this point we are executing as the child process */

  /* Change the file mode mask */
  umask(0);

  /* Create a new SID for the child process */
  sid = setsid();
  if (sid < 0) {
    exit(1);
  }

  /* Change the current working directory.  This prevents the current
     directory from being locked; hence not being able to remove it. */
  if ((chdir("/")) < 0) {
    exit(1);
  }

  /* Redirect standard files to /dev/null */
  freopen( "/dev/null", "r", stdin);
  freopen( "/dev/console", "w", stdout);
  freopen( "/dev/console", "w", stderr);
}

dm_req_t *get_next_request(int req_fd) {

  static char *req_msg;
  struct sockaddr_in cliaddr;
  socklen_t len;
  dm_req_t *dm_req;
  int n;

  if (!req_msg) {
    req_msg = (char *) malloc(REQ_BUF_SZ);
  }

  len = sizeof(cliaddr);

  n = recvfrom(req_fd, req_msg, REQ_BUF_SZ, 0, (struct sockaddr *)&cliaddr, &len);
  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr recv failed on socket\n");
    return (NULL);
  }
  if (n == 0) {
    return (NULL);
  }
  // Assume one req in one DGRAM for now, packable in less than REQ_BUF_SZ bytes
  dmgr_log(LOG_DEBUG, "Received %d bytes from client\n", n);

 
  // Some basic sanity check here
  if(dm_req_msg_sanity_check(req_msg) < 0) {
    dmgr_log(LOG_WARNING, "Corrupt or ill formed message from client\n");
    return (0);
  }
  if (alloc_and_fill_dm_req_from_msg(req_msg, &cliaddr, len, &dm_req) < 0) {
    dmgr_log(LOG_WARNING, "Ill formed message from client\n");
    return (0);
  }
  dmgr_log(LOG_NOTICE, "Received from Client command: %d\n", dm_req->cmd);
  dm_req->rsp_info.sockfd = req_fd;
  return (dm_req);

}

void *worker_thread_main(void *wt_arg);

static dm_wthread_t *create_worker_thread(int worker_id) {

  dm_wthread_t *worker;

  worker = (dm_wthread_t *)malloc(sizeof (dm_wthread_t));
  memset(worker, 0, sizeof(dm_wthread_t));
  worker->id = worker_id;
  worker->req_queue = queue_alloc(); // Queue alloc would alloc synchronization primitives necessary

  if (pthread_create(&worker->thread_id, NULL, worker_thread_main, (void *)worker) != 0) {
    dmgr_log(LOG_CRIT, "Unable to create worker thread\n");
    free(worker);
    return (0);
  }

  return (worker);

}

// Looks at the queue length of each worker and 
// selects the one with the least queue length.

// No locking, so we are only approximately correct about this.
// Worse problem is one thread being hogged by a long-running operation
// So queue length may not the right metric to load balance on.

dm_wthread_t  *get_next_worker() {

  int least_loaded_worker = 0;
  int least_queue_length = worker_thread[0]->req_queue->num_items;
  int i;

  for (i = 1; i < DMGR_NUM_WORKERS; i++) {
    if (worker_thread[i]->req_queue->num_items < least_queue_length) {
      least_queue_length = worker_thread[i]->req_queue->num_items;
      least_loaded_worker = i;
    }
  }

  return (worker_thread[least_loaded_worker]);

}


int main( int argc, char *argv[] ) {

  int req_fd;
  struct sockaddr_in servaddr,cliaddr;
  socklen_t len;
  int i;
  dm_wthread_t *next_worker;
 
#ifndef DMGR_LIVE_DEBUG

  printf("Trying to daemonize\n");
  daemonize();

#endif

  dmgr_log_init();
  dmgr_log(LOG_NOTICE, "Data Mgr Daemon alive with pid %d\n", getpid());

  dmgr_txn_cache_init();
  dmgr_log(LOG_NOTICE, "Data Mgr Daemon initialized txn cache\n");

  req_fd=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
  //servaddr.sin_addr.s_addr=htonl(0xc0a80105);
  servaddr.sin_port=htons(FDS_DMGR_SVR_PORT);
  bind(req_fd,(struct sockaddr *)&servaddr,sizeof(servaddr));

  // create worker threads
  for (i = 0; i < DMGR_NUM_WORKERS; i++) {
    worker_thread[i] = create_worker_thread(i);
  }

  while (1){
    dm_req_t *next_req;

    next_req = get_next_request(req_fd);
    if (next_req == NULL) {
      sleep(5);
      continue;
    }
    next_worker = get_next_worker();
    dmgr_log(LOG_INFO, "queueing work item to thread %d\n", next_worker->id);
    queue_enqueue(next_worker->req_queue, next_req);

  }

}

void process_request(dm_wthread_t *wt_info, dm_req_t *req);

void *worker_thread_main(void *wt_arg) {

  dm_wthread_t *wt_info = (dm_wthread_t *)wt_arg;

  while (1) {

    dm_req_t *next_req;

    next_req = queue_dequeue(wt_info->req_queue); //blocking
    dmgr_log(LOG_DEBUG, "Worker thread %d dequeueing work item with cmd %d\n", wt_info->id, next_req->cmd);
    process_request(wt_info, next_req);
    dmgr_log(LOG_DEBUG, "Worker thread %d completed work item\n", wt_info->id);

  }

}

void handle_noop_req(dm_wthread_t *wt_info, dm_req_t *req);
void handle_load_volume_req(dm_wthread_t *wt_info, dm_req_t *req);
void handle_open_txn_req(dm_wthread_t *wt_info, dm_req_t *req);
void handle_commit_txn_req(dm_wthread_t *wt_info, dm_req_t *req);
void handle_cancel_txn_req(dm_wthread_t *wt_info, dm_req_t *req);

void process_request(dm_wthread_t *wt_info, dm_req_t *req) {

  switch(req->cmd) {

  case FDS_DMGR_CMD_NOOP:
    dmgr_log(LOG_INFO, "No Op command received\n");
    handle_noop_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_LOAD_VOLUME:
    dmgr_log(LOG_INFO, "Load Vol command received\n");
    handle_load_volume_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_OPEN_TXN:
    dmgr_log(LOG_INFO, "Open transaction command received\n");
    handle_open_txn_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_COMMIT_TXN:
    dmgr_log(LOG_INFO, "Commit transaction command received\n");
    handle_commit_txn_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_CANCEL_TXN:
    dmgr_log(LOG_INFO, "Cancel transaction command received\n");
    handle_cancel_txn_req(wt_info, req);
    break;

  default:
    dmgr_log(LOG_WARNING, "Unknown command received\n");
    break;
  }
  
  destroy_dm_req(req);
  
  return;

}

#define FILL_RESP_FLDS(resp_msg, result, e_msg, cookie) \
  memset(&resp_msg, 0, sizeof(fdsp_msg_t)); \
  resp_msg.msg_code = FDSP_MSG_UPDATE_CAT_OBJ_RSP; \
  resp_msg.result = result; \
  if (e_msg) { \
    strncpy(resp_msg.err_msg.err_msg, e_msg, sizeof (fdsp_error_msg_t)); \
  } \
  resp_msg.req_cookie = cookie


void handle_noop_req(dm_wthread_t *wt_info, dm_req_t *req) {

  return;

}

void handle_load_volume_req(dm_wthread_t *wt_info, dm_req_t *req) {
  
  fdsp_msg_t resp_msg;
  char *err_msg = 0;
  int to_send, n;
  int result = FDSP_ERR_OK;
  int err_code = 0;
  dm_load_vol_req_t *lv_req = (dm_load_vol_req_t *)req;
  
  if (dmgr_txn_cache_vol_create(lv_req->vvc_vol_id) < 0) {
    result = FDSP_ERR_FAILED;
    err_msg = "Could not create volume\n"; 
  } else {
    result = FDSP_ERR_OK;
  }

  FILL_RESP_FLDS(resp_msg, result, err_msg, req->rsp_info.req_cookie);

  to_send = sizeof(resp_msg);
  n = sendto(req->rsp_info.sockfd, &resp_msg, to_send, 0,
	     req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request.\n",
           wt_info->id);
  return;


}

void handle_open_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction opened.";
  char *err_resp_msg1 = "Oops. Open Transaction failed.";
  char *err_resp_msg2 = "Oops. Commit Transaction failed.";
  int to_send, n;
  dm_open_txn_req_t *ot_req = (dm_open_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *err_msg = 0;
  fdsp_msg_t resp_msg;
  int result = FDSP_ERR_OK;
  int err_code = 0;

  if (!(dmgr_txn_cache_vol_created(ot_req->vvc_vol_id))) {
    dmgr_txn_cache_vol_create(ot_req->vvc_vol_id);
  }
  txn = dmgr_txn_create(ot_req->vvc_vol_id, ot_req->txn_id);
  dmgr_fill_txn_info(txn, ot_req);
  if (dmgr_txn_open(txn) < 0) {
    dmgr_log(LOG_WARNING, "Error opening transaction\n");
    result = FDSP_ERR_FAILED;
    err_msg = err_resp_msg1;
  } else {
    if (dmgr_txn_commit(txn) < 0) {
      dmgr_log(LOG_WARNING, "Error committing transaction\n");
      result = FDSP_ERR_FAILED;
      err_msg = err_resp_msg2;
    } else {
      result = FDSP_ERR_OK;
    }
  }

  FILL_RESP_FLDS(resp_msg, result, err_msg, req->rsp_info.req_cookie);

  to_send = sizeof(resp_msg);

  n = sendto(req->rsp_info.sockfd, &resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request\n",
	   wt_info->id);
  return;

}

void handle_commit_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction committed.";
  char *err_resp_msg = "Oops. Commit Transaction failed.";
  int to_send, n;
  dm_commit_txn_req_t *ct_req = (dm_commit_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *err_msg = 0;
  fdsp_msg_t resp_msg;
  int result = FDSP_ERR_OK;
  int err_code = 0;

  txn = dmgr_txn_get(ct_req->vvc_vol_id, ct_req->txn_id);
  if ((!txn) || (dmgr_txn_commit(txn) < 0)) {
    dmgr_log(LOG_WARNING, "Error committing transaction\n");
    result = FDSP_ERR_FAILED;
    err_msg = err_resp_msg;
  } else {
    result = FDSP_ERR_OK;
  }
  // if this is a vvc modify req, we will need to send association table update to SM
  // until then, don't destroy the txn object but keep it around in commited state.
  dmgr_txn_destroy(txn);

  FILL_RESP_FLDS(resp_msg, result, err_msg, req->rsp_info.req_cookie);

  to_send = sizeof(resp_msg);

  n = sendto(req->rsp_info.sockfd, &resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request.\n",
	   wt_info->id);
  return;

}

void handle_cancel_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction canceled.";
  char *err_resp_msg = "Oops. Cancel Transaction failed.";
  int to_send, n;
  dm_cancel_txn_req_t *ct_req = (dm_cancel_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *err_msg = 0;
  fdsp_msg_t resp_msg;
  int result = FDSP_ERR_OK;
  int err_code = 0;

  txn = dmgr_txn_get(ct_req->vvc_vol_id, ct_req->txn_id);
  if ((!txn) || (dmgr_txn_cancel(txn) < 0)) {
    result = FDSP_ERR_FAILED;
    err_msg = err_resp_msg;
  } else {
    result = FDSP_ERR_OK;
  }
  dmgr_txn_destroy(txn);

  FILL_RESP_FLDS(resp_msg, result, err_msg, req->rsp_info.req_cookie);

  to_send = sizeof(resp_msg);

  n = sendto(req->rsp_info.sockfd, &resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request.\n",
	   wt_info->id);
  return;

}

