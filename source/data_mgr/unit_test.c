#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>


#include "fds_commons.h"
#include "data_mgr.h"
#include "fdsp.h"

char cmd_wd[128];
int sockfd;
struct sockaddr_in servaddr;



int main(int argc, char *argv[]) {

  char *line_ptr = 0;
  size_t n_bytes;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr=inet_addr(argv[1]);
  servaddr.sin_port=htons(FDS_DMGR_SVR_PORT);

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

doid_t test_obj_id = {'o','b','j','i','d','-','x','y','z', 0};

int process_opentr_cmd(char *line_ptr) {
  //char *req_msg = "Please open a transaction";
  fdsp_msg_t fdsp_msg;
  int n;

  fdsp_msg.msg_id = 21;
  fdsp_msg.msg_code =  FDSP_MSG_UPDATE_CAT_OBJ_REQ;
  fdsp_msg.glob_volume_id = 3;
  fdsp_msg.payload.update_catalog.volume_offset = 4096;
  fdsp_msg.payload.update_catalog.dm_transaction_id = 76;
  fdsp_msg.payload.update_catalog.dm_operation = FDS_DMGR_CMD_OPEN_TXN;
  printf("Sending Dm msg with msg code %d and cmd code %d\n", fdsp_msg.msg_id, fdsp_msg.payload.update_catalog.dm_operation);
  memcpy(&fdsp_msg.payload.update_catalog.data_obj_id, test_obj_id, sizeof(doid_t));

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
  // Assume whole response fits in one DGRAM for now, packable in less than REQ_BUF_SZ bytes
  printf("Received response :%s\n", rcv_msg);
  return (0);

}

int process_committr_cmd(char *line_ptr) {

  fdsp_msg_t fdsp_msg;
  int n;

  fdsp_msg.msg_id = 21;
  fdsp_msg.msg_code =  FDSP_MSG_UPDATE_CAT_OBJ_REQ;
  fdsp_msg.glob_volume_id = 3;
  fdsp_msg.payload.update_catalog.volume_offset = 4096;
  fdsp_msg.payload.update_catalog.dm_transaction_id = 76;
  fdsp_msg.payload.update_catalog.dm_operation = FDS_DMGR_CMD_COMMIT_TXN;
  memcpy(&fdsp_msg.payload.update_catalog.data_obj_id, test_obj_id, sizeof(doid_t));

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
  // Assume whole response fits in one DGRAM for now, packable in less than REQ_BUF_SZ bytes
  printf("Received response :%s\n", rcv_msg);
  return (0);

}

int process_canceltr_cmd(char *line_ptr) {
  return (0);
}
