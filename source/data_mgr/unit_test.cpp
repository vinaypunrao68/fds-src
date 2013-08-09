#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "fds_commons.h"
#include "data_mgr.h"
#include "fdsp.h"

char cmd_wd[128];
int sockfd;
struct sockaddr_in servaddr;

fds_doid_t test_obj_id;

int process_opentr_cmd(char *line_ptr);
int process_committr_cmd(char *line_ptr);
int process_canceltr_cmd(char *line_ptr);

int main(int argc, char *argv[]) {

  char *line_ptr = 0;
  size_t n_bytes;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=inet_addr(argv[1]);
  servaddr.sin_port=htons(FDS_DMGR_SVR_PORT);

  memset((char *)&test_obj_id, 0, sizeof(fds_doid_t)); 
  test_obj_id.obj_id.hash_high = 0xabcd1234;
  test_obj_id.obj_id.hash_low = 0xefef5678;

  while(1) {
    printf(">");
    getline(&line_ptr, &n_bytes, stdin);
    sscanf(line_ptr, "%s", cmd_wd);
    if (strcmp(cmd_wd, "quit") == 0) {
      break;
    } else if (strcmp(cmd_wd, "opentr") == 0) {
      process_opentr_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "committr") == 0) {
      process_committr_cmd(line_ptr);
    } else if (strcmp(cmd_wd, "canceltr") == 0) {
      process_canceltr_cmd(line_ptr);
    } else{
      printf("Unknown command %s. Please try again\n", cmd_wd);
    }
  }
  if (line_ptr) {
    free(line_ptr);
  }
}

#define RCV_BUF_SZ FDS_DMGR_MAX_REQ_SZ
char rcv_msg[RCV_BUF_SZ];

int process_opentr_cmd(char *line_ptr) {

  //char *req_msg = "Please open a transaction";
  fdsp_msg_t fdsp_msg;
  fdsp_msg_t *resp_msg;
  int n;

  fdsp_msg.msg_id = 21;
  fdsp_msg.req_cookie = 11131719;
  fdsp_msg.msg_code =  FDSP_MSG_UPDATE_CAT_OBJ_REQ;
  fdsp_msg.glob_volume_id = 3;
  fdsp_msg.payload.update_catalog.volume_offset = 4096;
  fdsp_msg.payload.update_catalog.dm_transaction_id = 76;
  fdsp_msg.payload.update_catalog.dm_operation = FDS_DMGR_CMD_OPEN_TXN;
  printf("Sending Dm msg with msg code %d and cmd code %d\n", fdsp_msg.msg_id, fdsp_msg.payload.update_catalog.dm_operation);
  memcpy(&fdsp_msg.payload.update_catalog.data_obj_id, &test_obj_id.obj_id, sizeof(fds_object_id_t));

  n = sendto(sockfd, &fdsp_msg, sizeof(fdsp_msg_t), 0, 
	 (const struct sockaddr *)&servaddr, sizeof(servaddr));
  if (n < sizeof(fdsp_msg_t)) {
    printf("Socket send error.\n");
    return (-1);
  } else {
    printf("Sent %d bytes\n", n);
  }
  n = recvfrom(sockfd, rcv_msg, RCV_BUF_SZ, 0, NULL, NULL);
  if (n < 0) {
    printf("Socket rcv error.\n");
    return (-1);
  }
  resp_msg =(fdsp_msg_t *)rcv_msg;
  // Assume whole response fits in one DGRAM for now, packable in less than REQ_BUF_SZ bytes
  printf("Received response for req %d :%d, %d, %s\n", resp_msg->req_cookie, resp_msg->result, resp_msg->err_code,
	 (resp_msg->result == FDSP_ERR_OK)? "NULL":resp_msg->err_msg.err_msg);
  return (0);

}

int process_committr_cmd(char *line_ptr) {

  fdsp_msg_t fdsp_msg;
  fdsp_msg_t *resp_msg;
  int n;

  fdsp_msg.msg_id = 21;
  fdsp_msg.req_cookie = 2329;
  fdsp_msg.msg_code =  FDSP_MSG_UPDATE_CAT_OBJ_REQ;
  fdsp_msg.glob_volume_id = 3;
  fdsp_msg.payload.update_catalog.volume_offset = 4096;
  fdsp_msg.payload.update_catalog.dm_transaction_id = 76;
  fdsp_msg.payload.update_catalog.dm_operation = FDS_DMGR_CMD_COMMIT_TXN;
  memcpy(&fdsp_msg.payload.update_catalog.data_obj_id, &test_obj_id.obj_id, sizeof(fds_object_id_t));

  n = sendto(sockfd, &fdsp_msg, sizeof(fdsp_msg_t), 0, 
	 (const struct sockaddr *)&servaddr, sizeof(servaddr));
  if (n < sizeof(fdsp_msg_t)) {
    printf("Socket send error.\n");
    return (-1);
  }
  n = recvfrom(sockfd, rcv_msg, RCV_BUF_SZ, 0, NULL, NULL);
  if (n < 0) {
    printf("Socket rcv error.\n");
    return (-1);
  }
  resp_msg =(fdsp_msg_t *)rcv_msg;
  // Assume whole response fits in one DGRAM for now, packable in less than REQ_BUF_SZ bytes
  printf("Received response for request %d:%d, %d, %s\n", resp_msg->req_cookie, resp_msg->result, resp_msg->err_code,
	 (resp_msg->result == FDSP_ERR_OK)? "NULL":resp_msg->err_msg.err_msg);

  return (0);

}

int process_canceltr_cmd(char *line_ptr) {
  return (0);
}
