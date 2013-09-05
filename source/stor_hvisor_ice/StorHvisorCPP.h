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
void init_DPAPI();
void integration_stub( void *buf,  int len);
void *hvisor_lib_init(void);
int StorHvisorProcIoRd(void *dev_hdl, fbd_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2);
int StorHvisorProcIoWr(void *dev_hdl, fbd_request_t *req, complete_req_cb_t comp_req, void *arg1, void *arg2);
int unitTest(fds_uint32_t time_secs);
int unitTestFile(const char *inname, const char *outname);
void CreateStorHvisor(int argc, char *argv[]);
void CreateSHMode(int argc,
                  char *argv[],
                  fds_bool_t test_mode,
                  fds_uint32_t sm_port,
                  fds_uint32_t dm_port);
END_C_DECLS
