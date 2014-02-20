#ifndef SOURCE_DATA_MGR_DM_H_
#define SOURCE_DATA_MGR_DM_H_

#include <string>

#include "include/data_mgr.h"
#include "data_mgr_transactions.h"

void init();
void handle_open_txn_req(dm_wthread_t *wt_info, dm_req_t *req);
void handle_commit_txn_req(dm_wthread_t *wt_info, dm_req_t *req);

#endif  // SOURCE_DATA_MGR_DM_H_
