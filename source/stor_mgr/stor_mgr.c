#include <stor_mgr.h>

fds_bool_t  stor_mgr_stopping = FALSE;
#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))
fds_stor_mgr_init() 
{
// Create all data structures 
   fds_disk_mgr_init();
    
}


void fds_stor_mgr_exit()
{
}

fds_sm_err_t stor_mgr_check_duplicate(fds_object_id_t *object_id, fds_uint32_t obj_len, fds_char_t *data_object)
{
  return FDS_SM_OK;
}

fds_sm_err_t stor_mgr_write_object(fds_object_id_t *object_id, fds_uint32_t obj_len, fds_char_t *data_object, fds_data_location_t *data_loc)
{
   //Hash the object_id to DiskNumber, FileName
fds_uint32_t disk_num = 1;
   

   // Now append the object to the end of this filename
   disk_mgr_write_object(object_id, obj_len, data_object, data_loc, disk_num);
}

fds_sm_err_t stor_mgr_write_obj_loc(fds_object_id_t *object_id, fds_uint32_t obj_len, fds_uint32_t volid, fds_data_location_t *data_loc)
{
fds_uint32_t disk_num = 1;
   // Enqueue the object location entry into the thread that maintains global index file
   //disk_mgr_write_obj_loc(object_id, obj_len, volid, data_loc);
}
/*------------------------------------------------------------------------- ------------
 * FDSP Protocol message processing 
 -------------------------------------------------------------------------------------*/
fds_sm_err_t stor_mgr_put_obj(fdsp_put_object_t *put_obj_req, fds_uint32_t volid, fds_uint32_t num_objs)
{
fds_char_t *put_obj_buf = (fds_char_t *) put_obj_req;
fds_data_location_t object_location_offset;
int obj_num=0;
fds_sm_err_t result = FDS_SM_OK;

   for(obj_num = 0; obj_num < num_objs; obj_num++) {
       // Find if this object is a duplicate
       result = stor_mgr_check_duplicate(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, (fds_char_t *)&put_obj_req->data_obj[0]);

       if ( result != FDS_SM_ERR_DUPLICATE) {
           // First write the object itself after hashing the objectId to Disknum/filename & obtain an offset entry
           result = stor_mgr_write_object(&put_obj_req->data_obj_id, put_obj_req->data_obj_len,
                             (fds_char_t *)&put_obj_req->data_obj[0], &object_location_offset);

           // Now write the object location entry in the global index file
           stor_mgr_write_obj_loc(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, volid, 
                                  &object_location_offset);

       } else {
           stor_mgr_write_obj_loc(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, volid, NULL);
       }
       // Move the buffer pointer to the next object
       put_obj_buf += (sizeof(fdsp_put_object_t) + put_obj_req->data_obj_len); 
       put_obj_req = (fdsp_put_object_t *)put_obj_buf;
   }
}

fds_sm_err_t stor_mgr_put_obj_req(fdsp_msg_t *fdsp_msg) {
   fdsp_put_object_t *put_obj_req = (fdsp_put_object_t *)&fdsp_msg->payload;

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);
    put_obj_req->data_obj_id.hash_high = ntohs(put_obj_req->data_obj_id.hash_high);
    put_obj_req->data_obj_id.hash_low = ntohs(put_obj_req->data_obj_id.hash_low);

    printf("StorageHVisor --> StorMgr : FDSP_MSG_PUT_OBJ_REQ ObjectId %016llx:%016llx\n",put_obj_req->data_obj_id.hash_high, put_obj_req->data_obj_id.hash_low);
    stor_mgr_put_obj(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

fds_sm_err_t stor_mgr_get_obj(fdsp_get_object_t *get_obj_req, fds_uint32_t volid, fds_uint32_t num_objs) {
     // stor_mgr_read_object(get_obj_req->data_obj_id, get_obj_req->data_obj_len, &get_obj_req->data_obj);
}

fds_sm_err_t stor_mgr_get_obj_req(fdsp_msg_t *fdsp_msg) {
   fdsp_get_object_t *get_obj_req = (fdsp_get_object_t *)&fdsp_msg->payload;

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);

    stor_mgr_get_obj(get_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}


fds_sm_err_t stor_mgr_proc_fdsp_msg(void *msg) 
{
 fdsp_msg_t *fdsp_msg = (fdsp_msg_t *)msg;
 fds_sm_err_t result;

 switch(fdsp_msg->msg_code) {
    case FDSP_MSG_PUT_OBJ_REQ:
        result = stor_mgr_put_obj_req(fdsp_msg);
        break;

    case FDSP_MSG_GET_OBJ_REQ:
        result = stor_mgr_get_obj_req(fdsp_msg);
        break;

    case FDSP_MSG_VERIFY_OBJ_REQ:
        break;

    case FDSP_MSG_UPDATE_CAT_OBJ_REQ:
        break;

    case FDSP_MSG_OFFSET_WRITE_OBJ_REQ:
        break;

    case FDSP_MSG_REDIR_WRITE_OBJ_REQ :
        break;

    default :
        break;
 }
}

/*------------------------------------------------------------------------- ------------
 * Storage Mgr Request processor : Picks up FDSP msgs from socket and schedules their processing
 -------------------------------------------------------------------------------------*/
void *stor_mgr_req_processor(void *sock_ptr) {
int n;
int sock = * ((int *)sock_ptr);
    char buffer[256];

    bzero(buffer,256);

    while(!stor_mgr_stopping) { 
        n = read(sock,buffer,255);
        if (n < 0)
        {
            perror("ERROR reading from socket");
            pthread_exit(NULL);
        }
       
        /* Process the FDSP msg from DM or SH */ 
        stor_mgr_proc_fdsp_msg((void *)buffer);

   }
   return NULL;
}




/*------------------------------------------------------------------------------------- 
 * Storage Mgr main  processor : Listen on the socket and spawn or assign thread from a pool
 -------------------------------------------------------------------------------------*/
void fds_stor_mgr_main(int xport_protocol)
{
    int sockfd, newsockfd, portno, clilen, err=0;
    pthread_t pthr;
    struct sockaddr_in serv_addr, cli_addr;
    int  n;
    char buffer[4096+256];

    if (xport_protocol == FDS_XPORT_PROTO_TCP) {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
        {
            perror("ERROR opening socket");
            exit(1);
        }
    
        bzero((char *) &serv_addr, sizeof(serv_addr));
        portno = FDS_STOR_MGR_LISTEN_PORT;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(portno);
     
        /* Now bind the host address using bind() call.*/
        if (bind(sockfd, (struct sockaddr *) &serv_addr,
                              sizeof(serv_addr)) < 0)
        {
             perror("ERROR on binding");
             exit(1);
        }
    
        /* Now start listening for the clients, here process will
     * go in sleep mode and will wait for the incoming connection
         */
        listen(sockfd, FDS_MAX_WAITING_CONNS);
        clilen = sizeof(cli_addr);
        
        /* Accept actual connection from the client */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, 
                                    &clilen);
        if (newsockfd < 0) 
        {
            perror("ERROR on accept");
            exit(1);
        }
        
        while(!stor_mgr_stopping)
        {
            newsockfd = accept(sockfd, 
                    (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
            {
                perror("ERROR on accept");
                exit(1);
                }
            /* Enqueue into a processing thread's queue */
            err = pthread_create(&pthr, NULL, &stor_mgr_req_processor, (void *)&newsockfd);
            if (err < 0)
            {
                    perror("ERROR on pthread_create");
	        exit(1);
            }
        } /* end of while */
   } else {
      /* UDP based processing */
      sockfd=socket(AF_INET,SOCK_DGRAM,0);

      bzero((char *) &serv_addr, sizeof(serv_addr));
      portno = FDS_STOR_MGR_DGRAM_PORT;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr= INADDR_ANY;  
      serv_addr.sin_port = htons(portno);
      bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

      for (;;)
      {
         clilen = sizeof(cli_addr);
         n = recvfrom(sockfd,buffer,FDSP_MAX_MSG_LEN,0,(struct sockaddr *)&cli_addr,&clilen);
         printf("Received the FDSP msg:\n");
         
         /* Process the FDSP msg from DM or SH */ 
         stor_mgr_proc_fdsp_msg((void *)buffer);
      }
   }
    
   return; 
}
    
int main(int argc, void **argv)
{
  fds_stor_mgr_init();
  fds_stor_mgr_main(FDS_XPORT_PROTO_UDP);
}


