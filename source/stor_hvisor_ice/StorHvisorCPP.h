#include "StorHvisorNet.h"
#ifdef __cplusplus
#define BEGIN_C_DECLS extern "C" {
#define END_C_DECLS }
#else
#define BEGIN_C_DECLS
#define END_C_DECLS
#endif
BEGIN_C_DECLS
typedef void (*complete_req_cb_t)(void *arg1, void *arg2, fbd_request_t *treq, int res);
typedef int (*hv_create_blkdev)(uint64_t voluuid, uint64_t capacity);
typedef void  (*hv_delete_blkdev)(int minor);
void init_DPAPI();
void integration_stub( void *buf,  int len);
int StorHvisorProcIoRd(void *io);
int StorHvisorProcIoWr(void *io);
int unitTest(fds_uint32_t time_secs);
int unitTestFile(const char *inname, const char *outname, unsigned int base_vol, int num_vols);
void CreateStorHvisor(int argc, char *argv[], hv_create_blkdev cr_blkdev, hv_delete_blkdev del_blkdev);
void CreateSHMode(int argc,
                  char *argv[],
                  hv_create_blkdev cr_blkdev,
                  hv_delete_blkdev del_blkdev,
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port);
void DeleteStorHvisor(void);
void cppOut( char *format, ... );
void ctrlCCallbackHandler(int signal);
int  pushVolQueue(void *req);
END_C_DECLS
