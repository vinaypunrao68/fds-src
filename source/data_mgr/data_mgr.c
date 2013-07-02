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
  dmgr_log(LOG_NOTICE, "Received from Client: %s\n", req_msg);

 
  // Some basic sanity check here
  if(dm_req_msg_sanity_check(req_msg) < 0) {
    dmgr_log(LOG_WARNING, "Corrupt or ill formed message from client\n");
    return (0);
  }
  if (alloc_and_fill_dm_req_from_msg(req_msg, dm_req, &cliaddr, len, &dm_req) < 0) {
    dmgr_log(LOG_WARNING, "Ill formed message from client\n");
    return (0);
  }
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
  dmgr_log(LOG_NOTICE, "Data Mgr Daemon initialized txn cache %d\n", getpid());

  req_fd=socket(AF_INET,SOCK_DGRAM,0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=htonl(INADDR_ANY);
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
    dmgr_log(LOG_DEBUG, "Worker thread %d dequeueing work item\n", wt_info->id);
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
    handle_noop_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_LOAD_VOLUME:
    handle_load_volume_req(wt_info, req);

  case FDS_DMGR_CMD_OPEN_TXN:
    handle_open_txn_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_COMMIT_TXN:
    handle_commit_txn_req(wt_info, req);
    break;

  case FDS_DMGR_CMD_CANCEL_TXN:
    handle_cancel_txn_req(wt_info, req);
    break;

  default:
    dmgr_log(LOG_WARNING, "Unknown command received\n");
    break;
  }
  
  destroy_dm_req(req);
  
  return;

}

void handle_noop_req(dm_wthread_t *wt_info, dm_req_t *req) {

  return;

}

void handle_load_volume_req(dm_wthread_t *wt_info, dm_req_t *req) {
  
  char *succ_resp_msg = "Thank you. Volume loaded.";
  char *err_resp_msg = "Oops, load volume failed.";
  char *resp_msg;
  int to_send, n;
  dm_load_vol_req_t *lv_req = (dm_load_vol_req_t *)req;
  
  if (dmgr_txn_cache_vol_create(lv_req->vvc_vol_id) < 0) {
    resp_msg = err_resp_msg;
  } else {
    resp_msg = succ_resp_msg;
  }

  to_send = strlen(resp_msg) + 1;
  n = sendto(req->rsp_info.sockfd, resp_msg, to_send, 0,
	     req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request: %s\n",
           wt_info->id, resp_msg);
  return;


}

void handle_open_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction opened.";
  char *err_resp_msg = "Oops. Open Transaction failed.";
  int to_send, n;
  dm_open_txn_req_t *ot_req = (dm_open_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *resp_msg;

  txn = dmgr_txn_create(ot_req->vvc_vol_id, ot_req->txn_id);
  dmgr_fill_txn_info(txn, ot_req);
  if (dmgr_txn_open(txn) < 0) {
    resp_msg = err_resp_msg;
  } else {
    resp_msg = succ_resp_msg;
  }

  to_send = strlen(resp_msg) + 1;
  n = sendto(req->rsp_info.sockfd, resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request: %s\n",
	   wt_info->id, resp_msg);
  return;

}

void handle_commit_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction committed.";
  char *err_resp_msg = "Oops. Commit Transaction failed.";
  int to_send, n;
  dm_commit_txn_req_t *ct_req = (dm_commit_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *resp_msg;

  txn = dmgr_txn_get(ct_req->vvc_vol_id, ct_req->txn_id);
  if ((!txn) || (dmgr_txn_commit(txn) < 0)) {
    resp_msg = err_resp_msg;
  } else {
    resp_msg = succ_resp_msg;
  }
  // if this is a vvc modify req, we will need to send association table update to SM
  // until then, don't destroy the txn object but keep it around in commited state.
  dmgr_txn_destroy(txn);

  to_send = strlen(resp_msg) + 1;
  n = sendto(req->rsp_info.sockfd, resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request: %s\n",
	   wt_info->id, resp_msg);
  return;

}

void handle_cancel_txn_req(dm_wthread_t *wt_info, dm_req_t *req) {

  char *succ_resp_msg = "Thank you. Transaction canceled.";
  char *err_resp_msg = "Oops. Cancel Transaction failed.";
  int to_send, n;
  dm_cancel_txn_req_t *ct_req = (dm_cancel_txn_req_t *)req;
  dmgr_txn_t *txn;
  char *resp_msg;

  txn = dmgr_txn_get(ct_req->vvc_vol_id, ct_req->txn_id);
  if ((!txn) || (dmgr_txn_cancel(txn) < 0)) {
    resp_msg = err_resp_msg;
  } else {
    resp_msg = succ_resp_msg;
  }
  dmgr_txn_destroy(txn);

  to_send = strlen(resp_msg) + 1;
  n = sendto(req->rsp_info.sockfd, resp_msg, to_send, 0,
	 req->rsp_info.rsp_to_addr, req->rsp_info.addr_len);

  if (n < 0) {
    dmgr_log(LOG_ERROR, "DMgr send failed on socket: %s \n", strerror(errno));
    return;
  }
  if (n < to_send) {
    dmgr_log(LOG_ERROR, "DMgr socket error. Only %d bytes of %d sent. \n", n, to_send);
    return;
  }
  dmgr_log(LOG_DEBUG, "Worker thread %d responded to request: %s\n",
	   wt_info->id, resp_msg);
  return;

}

