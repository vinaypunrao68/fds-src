/*
 This  include the shared  defines for fbd driver conffigurations 
*/

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
