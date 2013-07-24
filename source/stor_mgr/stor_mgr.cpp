#include "stor_mgr.h"
#include "disk_mgr.h"

fds_bool_t  stor_mgr_stopping = false;

#define FDS_XPORT_PROTO_TCP 1
#define FDS_XPORT_PROTO_UDP 2
#define FDSP_MAX_MSG_LEN (4*(4*1024 + 128))

stor_mgr_ctrl_blk_t fds_stor_mgr_blk;

fds_sm_err_t
stor_mgr_put_obj(fdsp_put_object_t *put_obj_req,
		 fds_uint32_t       volid,
		 fds_uint32_t       num_objs);

fds_sm_err_t
stor_mgr_get_obj(fdsp_get_object_t *get_obj_req,
		 fds_uint32_t       volid,
		 fds_uint32_t       num_objs);

// Use a single global levelDB for now
leveldb::DB* db;

void
fds_stor_mgr_init( char *db_locn) 
{
  // Create all data structures 
  fds_disk_mgr_init();
  
  // Create leveldb
  leveldb::Options options;
  options.create_if_missing = 1;
  leveldb::Status status = leveldb::DB::Open(options, db_locn, &db);
  assert(status.ok());

  std::cout << "LevelDB status is " << status.ToString() << std::endl;
}


void fds_stor_mgr_exit()
{
  delete db;
}

void fds_stor_mgr_unit_test()
{
  fds_sm_err_t err;

  err = FDS_SM_OK;

  std::cout << "Running unit test" << std::endl;
  
  /*
   * Create fake objects
   */
  std::string object_data("Hi, I'm object data.");
  fdsp_put_object_t *put_obj_req;
  put_obj_req = (fdsp_put_object_t *)malloc(sizeof(fdsp_put_object_t) +
					    object_data.length() + 1);
  memset(&(put_obj_req->data_obj_id),
	 0x00,
	 sizeof(put_obj_req->data_obj_id));
  put_obj_req->data_obj_id.hash_low = 0x101;
  put_obj_req->data_obj_len = object_data.length() + 1;
  memcpy((char *)put_obj_req->data_obj,
	 object_data.c_str(),
	 put_obj_req->data_obj_len);

  /*
   * Create fake volume ID
   */
  fds_uint32_t vol_id   = 0xbeef;
  fds_uint32_t num_objs = 1;

  /*
   * Write fake object.
   */
  err = stor_mgr_put_obj(put_obj_req, vol_id, num_objs);
  if (err != FDS_SM_OK) {
    std::cout << "Failed to put object " << std::endl;
    free(put_obj_req);
    return;
  }

  fdsp_get_object_t get_obj_req;
  memset(&(get_obj_req.data_obj_id),
	 0x00,
	 sizeof(get_obj_req.data_obj_id));
  get_obj_req.data_obj_id.hash_low = 0x101;
  err = stor_mgr_get_obj(&get_obj_req, vol_id, num_objs);
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

       if (result != FDS_SM_ERR_DUPLICATE) {
           // First write the object itself after hashing the objectId to Disknum/filename & obtain an offset entry
           result = stor_mgr_write_object(&put_obj_req->data_obj_id, put_obj_req->data_obj_len,
                             (fds_char_t *)&put_obj_req->data_obj[0], &object_location_offset);

           // Now write the object location entry in the global index file
           stor_mgr_write_obj_loc(&put_obj_req->data_obj_id, put_obj_req->data_obj_len, volid, 
                                  &object_location_offset);

	   /*
	    * This is the levelDB insertion. It's a totally
	    * separate DB from the one above.
	    */
	   leveldb::Slice key((char *)&(put_obj_req->data_obj_id),
			      sizeof(put_obj_req->data_obj_id));
	   leveldb::Slice value((char*)put_obj_req->data_obj,
				put_obj_req->data_obj_len);
	   leveldb::WriteOptions writeopts;
	   writeopts.sync = true;
	   leveldb::Status status = db->Put(writeopts,
					    key,
					    value);

	   if (! status.ok()) {
	     std::cout << "Failed to put object "
		       << status.ToString() << std::endl;
	   } else {
	     std::cout << "Successfully put key "
		       << key.ToString() << std::endl;
	   }

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
    //put_obj_req->data_obj_id.hash_high = ntohl(put_obj_req->data_obj_id.hash_high);
    //put_obj_req->data_obj_id.hash_low = ntohl(put_obj_req->data_obj_id.hash_low);

    printf("StorageHVisor --> StorMgr : FDSP_MSG_PUT_OBJ_REQ ObjectId %016llx:%016llx %x\n",put_obj_req->data_obj_id.hash_high, put_obj_req->data_obj_id.hash_low, put_obj_req->volume_offset);
    stor_mgr_put_obj(put_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

fds_sm_err_t stor_mgr_get_obj(fdsp_get_object_t *get_obj_req, fds_uint32_t volid, fds_uint32_t num_objs) {
     // stor_mgr_read_object(get_obj_req->data_obj_id, get_obj_req->data_obj_len, &get_obj_req->data_obj);

  leveldb::Slice key((char *)&(get_obj_req->data_obj_id),
		     sizeof(get_obj_req->data_obj_id));
  std::string value(&get_obj_req->data_obj[0]);
  //std::string value;
  leveldb::Status status = db->Get(leveldb::ReadOptions(), key, &value);

  if (! status.ok()) {
    std::cout << "Failed to get key " << key.ToString() << " with status "
	      << status.ToString() << std::endl;
  } else {
    memcpy(&get_obj_req->data_obj[0], value.c_str(), get_obj_req->data_obj_len);
    std::cout << "Successfully got value " << value << std::endl;
  }

}

fds_sm_err_t stor_mgr_get_obj_req(fdsp_msg_t *fdsp_msg) {
   fdsp_get_object_t *get_obj_req = (fdsp_get_object_t *)&fdsp_msg->payload;

    // Verify the integrity of the FDSP msg using chksums
    // 
    // stor_mgr_verify_msg(fdsp_msg);

    printf("StorageHVisor --> StorMgr : FDSP_MSG_GET_OBJ_REQ ObjectId %016llx:%016llx \n",get_obj_req->data_obj_id.hash_high, get_obj_req->data_obj_id.hash_low);
    stor_mgr_get_obj(get_obj_req, fdsp_msg->glob_volume_id, fdsp_msg->num_objects);
}

static inline void fdsp_swap_mgr_id(fdsp_msg_t *fdsp_msg) {
 fdsp_mgr_id_t temp_id;
 temp_id = fdsp_msg->dst_id;
 fdsp_msg->dst_id = fdsp_msg->src_id;
 fdsp_msg->src_id = temp_id;
}


void stor_mgr_send_fdsp_msg_response(fdsp_msg_t *fdsp_msg, struct sockaddr *cli_addr, socklen_t clilen) {
 fds_sm_err_t result;
 fds_uint32_t len=0;
 fdsp_get_object_t *get_obj_req;

 switch(fdsp_msg->msg_code) {
    case FDSP_MSG_PUT_OBJ_REQ:
        printf("SH-->SM : Rcvd Cookie %x \n",fdsp_msg->req_cookie);
        fdsp_msg->msg_code = FDSP_MSG_PUT_OBJ_RSP;
        len = sizeof(fdsp_msg_t);
        fdsp_swap_mgr_id(fdsp_msg);
        break;

    case FDSP_MSG_GET_OBJ_REQ:
        fdsp_msg->msg_code = FDSP_MSG_GET_OBJ_RSP;
        get_obj_req = (fdsp_get_object_t *)&fdsp_msg->payload;
        fdsp_swap_mgr_id(fdsp_msg);
        len= sizeof(fdsp_msg_t) + get_obj_req->data_obj_len;
        break;

    case FDSP_MSG_VERIFY_OBJ_REQ:
        break;

    case FDSP_MSG_OFFSET_WRITE_OBJ_REQ:
        break;

    case FDSP_MSG_REDIR_WRITE_OBJ_REQ :
        break;

    default :
        break;
 }
 sendto(fds_stor_mgr_blk.sockfd, fdsp_msg, len, 0,
               cli_addr, clilen);
}


fds_sm_err_t stor_mgr_proc_fdsp_msg(void *msg, struct sockaddr *cli_addr, socklen_t socklen) 
{
 fdsp_msg_t *fdsp_msg = (fdsp_msg_t *)msg;
 fdsp_msg_t *fdsp_rsp_msg;
 fds_sm_err_t result;
 fdsp_get_object_t *get_obj_req;
 fdsp_get_object_t *get_obj_rsp;

 switch(fdsp_msg->msg_code) {
    case FDSP_MSG_PUT_OBJ_REQ:
        fdsp_rsp_msg = fdsp_msg;
        result = stor_mgr_put_obj_req(fdsp_msg);
        break;

    case FDSP_MSG_GET_OBJ_REQ:
        fdsp_rsp_msg = (fdsp_msg_t *)malloc(sizeof(fdsp_msg_t) + ((fdsp_get_object_t *)(&fdsp_msg->payload))->data_obj_len);
        memcpy(fdsp_rsp_msg, fdsp_msg, sizeof(fdsp_msg_t));
        get_obj_req = (fdsp_get_object_t *)&fdsp_msg->payload;
        get_obj_rsp = (fdsp_get_object_t *)&fdsp_rsp_msg->payload;
        memcpy(get_obj_rsp, get_obj_req, sizeof(fdsp_get_object_t));
        result = stor_mgr_get_obj_req(fdsp_rsp_msg);
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

 stor_mgr_send_fdsp_msg_response(fdsp_rsp_msg, cli_addr, socklen);
 if (fdsp_msg != fdsp_rsp_msg) {
    free(fdsp_rsp_msg);
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
    stor_mgr_proc_fdsp_msg((void *)buffer, 0, 0);
    
  }
  return NULL;
}




/*------------------------------------------------------------------------------------- 
 * Storage Mgr main  processor : Listen on the socket and spawn or assign thread from a pool
 -------------------------------------------------------------------------------------*/
void fds_stor_mgr_main(int xport_protocol, int port_no)
{
    int sockfd, newsockfd, clilen, err=0;
    pthread_t pthr;
    struct sockaddr_in serv_addr, cli_addr;
    int  n;
    char buffer[4096+256];

    if (xport_protocol == FDS_XPORT_PROTO_TCP) {
        fds_stor_mgr_blk.sockfd = sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) 
        {
            perror("ERROR opening socket");
            exit(1);
        }
    
        bzero((char *) &serv_addr, sizeof(serv_addr));
        // portno = FDS_STOR_MGR_LISTEN_PORT;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(port_no);
     
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
        newsockfd = accept(sockfd,
			   (struct sockaddr *)&cli_addr, 
			   (socklen_t *)&clilen);
        if (newsockfd < 0) 
        {
            perror("ERROR on accept");
            exit(1);
        }
        
        while(!stor_mgr_stopping)
        {
            newsockfd = accept(sockfd, 
			       (struct sockaddr *) &cli_addr,
			       (socklen_t *)&clilen);
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
      fds_stor_mgr_blk.sockfd = sockfd=socket(AF_INET,SOCK_DGRAM,0);

      bzero((char *) &serv_addr, sizeof(serv_addr));
      // portno = FDS_STOR_MGR_DGRAM_PORT;
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_addr.s_addr= INADDR_ANY;  
      serv_addr.sin_port = htons(port_no);
      bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));

      for (;;)
      {
         clilen = sizeof(cli_addr);
         n = recvfrom(sockfd,
		      buffer,
		      FDSP_MAX_MSG_LEN,
		      0,
		      (struct sockaddr *)&cli_addr,
		      (socklen_t *)&clilen);
         printf("Received the FDSP msg:\n");
         
         /* Process the FDSP msg from DM or SH */ 
         stor_mgr_proc_fdsp_msg((void *)buffer,
				(struct sockaddr *)&cli_addr,
				clilen);
      }
   }
    
   return; 
}
    
int main(int argc, char *argv[])
{
  bool unit_test;
  int port_number = FDS_STOR_MGR_DGRAM_PORT;
  char *db_locn = "/tmp/testdb";

  unit_test = 0;

  if (argc > 1) {
    for (int i = 1; i < argc; i++) {
      std::string arg(argv[i]);
      if (arg == "--unit_test") {
	unit_test = true;
      } else if (arg == "-p") {
	port_number = atoi(argv[i+1]);
	i++;
      } else if (arg == "-d") {
	db_locn = argv[i+1];
	i++;
      } else {
	std::cerr << "Unknown option" << std::endl;
	return 0;
      }
    }
    
  }

  printf("Stor Mgr port_number : %d\n", port_number);

  fds_stor_mgr_init(db_locn);

  if (unit_test) {
    fds_stor_mgr_unit_test();
    fds_stor_mgr_exit();
    return 0;
  }

  fds_stor_mgr_main(FDS_XPORT_PROTO_UDP, port_number);
}


