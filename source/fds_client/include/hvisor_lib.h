void *hvisor_lib_init(void);
typedef void (*complete_req_cb_t)(void *arg1, void *arg2, td_request_t *treq, int res);
int hvisor_process_read_request(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *comp_arg1, void *comp_arg2);
int hvisor_process_write_request(void *dev_hdl, td_request_t *req, complete_req_cb_t comp_req, void *comp_arg1, void *comp_arg2);
