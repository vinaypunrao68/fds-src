
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
END_C_DECLS
