/*
 This  include the shared  defines for fbd driver conffigurations 
*/


/* flags  */
#define FBD_READ_ONLY    (1 << 0)
#define FBD_SEND_FLUSH   (1 << 1)
#define FBD_SEND_TRIM    (1 << 2)


#define  FBD_FAILURE 		1

#define  FBD_OPEN_TARGET_CON_TCP	100
#define  FBD_OPEN_TARGET_CON_UDP	101
#define  FBD_CLOSE_TARGET_CON		102
#define  FBD_SET_TGT_SIZE		103
#define  FBD_SET_TGT_BLK_SIZE		104
#define  FBD_WRITE_DATA			105
#define  FBD_READ_DATA			106
#define  FBD_ADD_DEV			107
#define  FBD_DEL_DEV			108
#define  FBD_READ_VVC_CATALOG		109
#define  FBD_READ_DMT_TBL		110
#define  FBD_READ_DLT_TBL		111



/* cluster transport structure */

struct fbd_contbl {

	struct  socket *sock;
	struct sockaddr_storage dest_addr;
	struct sockaddr_storage loc_addr;

	/*   queue for send/recv packts */
	struct list_head out_queue;
	struct list_head out_sent;
	u64 out_seq;		 

};


struct fbd_device {
	int flags;
	int harderror;

	spinlock_t queue_lock;
	struct list_head queue_head;
	struct request *active_req;
	wait_queue_head_t active_wq;
	struct list_head waiting_queue;
	wait_queue_head_t waiting_wq;

	struct mutex tx_lock;
	struct gendisk *disk;
	int blocksize;
	u64 bytesize;
	int xmit_timeout;
	int	dev_id;		/*  fbd  block dev  id */
	int	dev_major;		/* fbd block major number */
	int 	io_cmd;
	/* sysfs  data structure support */
	struct device	dev;
	/* network connection details */
	struct socket *sock;
	long int   tcp_destAddr;
	long int   udp_destAddr;
	int 	   proto_type;
	/* message header */
	fdsp_msg_t	dm_msg;
	fdsp_msg_t	sm_msg;
	/*  vvc  defs */
	volid_t 	vol_id;
	vvc_vhdl_t	vhdl;
	char blk_name[100]
};

#define  FBD_CMD_READ			01
#define  FBD_CMD_WRITE			02
#define  FBD_CMD_FLUSH			03
#define  FBD_CMD_DISCARD		04

/* proto defines  */
#define FBD_PROTO_TCP			01	
#define FBD_PROTO_UDP			02
#define FBD_PROTO_MCAST			03

#define FBD_DEV_MAJOR_NUM		63
#define FBD_MINORS_PER_MAJOR   		16

#define FBD_CLUSTER_TCP_PORT_SM		6900
#define FBD_CLUSTER_TCP_PORT_DM		6901
#define FBD_CLUSTER_UDP_PORT_SM		9600
#define FBD_CLUSTER_UDP_PORT_DM		9601
