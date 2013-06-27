#include <stor_mgr.h>

fds_bool_t  stor_mgr_stopping = FALSE;


fds_stor_mgr_init() 
{
// Create all data structures 

}


fds_stor_mgr_exit()
{
}


/*------------------------------------------------------------------------- ------------
 * FDSP Protocol message processing 
 -------------------------------------------------------------------------------------*/
void stor_mgr_proc_fdsp_msg(void *msg) 
{
 exit(0);
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
void fds_stor_mgr_main(void)
{
    int sockfd, newsockfd, portno, clilen, err=0;
    pthread_t pthr;
    struct sockaddr_in serv_addr, cli_addr;
    int  n;

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

    return; 
}

int main(int argc, void *argv)
{
  fds_stor_mgr_main();
}


